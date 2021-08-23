#include <time.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <sys/mman.h>
#include <unistd.h>
#include <semaphore.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "log.h"
#include "osp_ledlib.h"
#include "led_private.h"
#include "osp_led.h"

#define LED_SHMEM_FILE "/ledsharedmem"
#define LED_SEM_NAME "/ledsem" 
#define LED_SHMEM_SIZE sizeof(shmem_overlay_t)
#define LED_MAX_STATE_STACK_LINKS 10
#define LED_CALLOC_FAILED 0
#define APPLY_LED_STATE_SCRIPT "/usr/opensync/bin/apply_plume_led_state.sh"

typedef struct
{
    led_state_t state;
    uint32_t next_index;
} state_stack_link_t;

typedef struct
{
    uint32_t root_link_index;
    state_stack_link_t links[LED_MAX_STATE_STACK_LINKS];
} shmem_overlay_t;

typedef struct
{
    bool is_valid;
    int fd;
    shmem_overlay_t* overlay;
    sem_t* semptr;
} shmem_t;

typedef enum
{
    LED_OFF,
    LED_STEADY_ON,
    LED_SHORT_PULSE, // cycle period 250 ms
    LED_LONG_PULSE,  // cycle period 2500 ms
    LED_BLINK        
} led_action_t;

typedef struct
{
    led_state_t state;
    led_colors_t color;
    led_action_t action;
    led_priority_t priority;
    char* str;
} led_state_data_t;

/* NOTE: States that have an ignored priority are valid, we just don't
 * need to track them on the stack. They are currently not associated with 
 * any LED behavior.
 * The commented LED states are currently not supported by opensync core.
 * Can be enabled in future.
 */
static led_state_data_t _state_data[] =
{
   // LED_STATE                    Color            Action           Priority              String representation
   // --------------------------------------------------------------------------------------------------------------------------
    { LED_STATE_OFF,               LED_COLOR_RED,   LED_OFF,         LED_PRIORITY_DEFAULT, "off" },
    { LED_STATE_PLUME_ERROR,       LED_COLOR_RED,   LED_SHORT_PULSE, LED_PRIORITY_HIGH,    "plume_error" },
    { LED_STATE_PLUME_CONNECTED,   LED_COLOR_RED,   LED_OFF,         LED_PRIORITY_DEFAULT, "plume_connected" },
    { LED_STATE_PLUME_CONNECTING,  LED_COLOR_GREEN, LED_LONG_PULSE,  LED_PRIORITY_DEFAULT, "plume_connecting" },
    { LED_STATE_PLUME_CONNECTFAIL, LED_COLOR_WHITE, LED_LONG_PULSE,  LED_PRIORITY_DEFAULT, "plume_connectfail"},
    { LED_STATE_WPS,               LED_COLOR_BLUE,  LED_SHORT_PULSE, LED_PRIORITY_IGNORE,  "wps"},
    { LED_STATE_OPTIMIZE,          LED_COLOR_GREEN, LED_BLINK,       LED_PRIORITY_DEFAULT, "optimize"},
    { LED_STATE_LOCATE,            LED_COLOR_GREEN, LED_BLINK,       LED_PRIORITY_DEFAULT, "locate" },
    { LED_STATE_HWERROR,           LED_COLOR_RED,   LED_SHORT_PULSE, LED_PRIORITY_HIGH,    "hwerror" },
    { LED_STATE_THERMAL,           LED_COLOR_RED,   LED_SHORT_PULSE, LED_PRIORITY_HIGH,    "thermal" },
//    { LED_STATE_BTCONNECTABLE,     LED_COLOR_BLUE,  LED_SHORT_PULSE, LED_PRIORITY_IGNORE,  "btconnectable" },
    { LED_STATE_BTCONNECTING,      LED_COLOR_BLUE,  LED_SHORT_PULSE, LED_PRIORITY_IGNORE,  "btconnecting" },
    { LED_STATE_BTCONNECTED,       LED_COLOR_BLUE,  LED_SHORT_PULSE, LED_PRIORITY_IGNORE,  "btconnected" },
    { LED_STATE_BTCONNECTFAIL,     LED_COLOR_BLUE,  LED_SHORT_PULSE, LED_PRIORITY_IGNORE,  "btconnectfail" },
    { LED_STATE_UPGRADING,         LED_COLOR_BLUE,  LED_SHORT_PULSE, LED_PRIORITY_IGNORE,  "upgrading" },
    { LED_STATE_UPGRADED,          LED_COLOR_BLUE,  LED_SHORT_PULSE, LED_PRIORITY_IGNORE,  "upgraded" },
    { LED_STATE_UPGRADEFAIL,       LED_COLOR_BLUE,  LED_SHORT_PULSE, LED_PRIORITY_IGNORE,  "upgradefail" },
//    { LED_STATE_HWTEST,            LED_COLOR_ALL,   LED_STEADY_ON,   LED_PRIORITY_DEFAULT, "hwtest" },
    { LED_STATE_INVALID,           LED_COLOR_RED,   LED_OFF,         LED_PRIORITY_DEFAULT, "unknown"}
};

typedef struct
{
    enum osp_led_state plume_state;
    led_state_t adtran_state;
} plume_state_data_t;

static plume_state_data_t _plume_state_map[] =
{
   //PLUME STATE                 ADTRAN STATE
   //-----------------------------------------
    { OSP_LED_ST_IDLE,           LED_STATE_OFF },
    { OSP_LED_ST_ERROR,          LED_STATE_PLUME_ERROR },
    { OSP_LED_ST_CONNECTED,      LED_STATE_PLUME_CONNECTED },
    { OSP_LED_ST_CONNECTING,     LED_STATE_PLUME_CONNECTING },
    { OSP_LED_ST_CONNECTFAIL,    LED_STATE_PLUME_CONNECTFAIL },
    { OSP_LED_ST_WPS,            LED_STATE_WPS },
    { OSP_LED_ST_OPTIMIZE,       LED_STATE_OPTIMIZE },
    { OSP_LED_ST_LOCATE,         LED_STATE_LOCATE },
    { OSP_LED_ST_HWERROR,        LED_STATE_HWERROR },
    { OSP_LED_ST_THERMAL,        LED_STATE_THERMAL },
//    { OSP_LED_ST_BTCONNECTABLE,  LED_STATE_BTCONNECTABLE },
    { OSP_LED_ST_BTCONNECTING,   LED_STATE_BTCONNECTING },
    { OSP_LED_ST_BTCONNECTED,    LED_STATE_BTCONNECTED },
    { OSP_LED_ST_BTCONNECTFAIL,  LED_STATE_BTCONNECTFAIL },
    { OSP_LED_ST_UPGRADING,      LED_STATE_UPGRADING },
    { OSP_LED_ST_UPGRADED,       LED_STATE_UPGRADED },
    { OSP_LED_ST_UPGRADEFAIL,    LED_STATE_UPGRADEFAIL },
//    { OSP_LED_ST_HWTEST,         LED_STATE_HWTEST },
    { OSP_LED_ST_LAST,           LED_STATE_INVALID }
};

static led_state_data_t* _state_data_from_state(led_state_t state)
{
    int i = 0;
    for (; _state_data[i].state != LED_STATE_INVALID && _state_data[i].state != state; i++);
    return &_state_data[i];
}

const char* led_state_to_str(led_state_t state)
{
    led_state_data_t* data = _state_data_from_state(state);
    return data->str ? data->str : "(none)";
}

led_state_t led_str_to_state(const char* str)
{
    int i = 0;
    for (; _state_data[i].state != LED_STATE_INVALID && strcmp(str, _state_data[i].str); i++);
    return _state_data[i].state;
}

led_priority_t led_state_priority(led_state_t state)
{
    led_state_data_t* data = _state_data_from_state(state);
    return data->priority;
}

// Refer to the comment above the _calloc_state_stack_link function for more information
static state_stack_link_t* _get_link_ptr(shmem_t* shmem_info, uint32_t link_index)
{
    return &(shmem_info->overlay->links[link_index - 1]);
}

static void _free_state_stack_link(shmem_t* shmem_info, uint32_t link_index)
{
    memset(_get_link_ptr(shmem_info, link_index), 0, sizeof(state_stack_link_t));
}

static void _unlink_and_free_oldest_state_stack_link(shmem_t* shmem_info)
{
    state_stack_link_t* prev_link = NULL;
    uint32_t current_link_index = shmem_info->overlay->root_link_index;

    while (true)
    {
        state_stack_link_t* current_link = _get_link_ptr(shmem_info, current_link_index);
        
        if (! current_link->next_index)
        {
            if (prev_link)
                prev_link->next_index = 0;
            else
                shmem_info->overlay->root_link_index = 0;

            _free_state_stack_link(shmem_info, current_link_index);
            return;
        }

        prev_link = current_link;
        current_link_index = current_link->next_index;
    }
}

/* This function finds the first empty state_stack_link_t slot in shared
 * memory that is free and returns its index +1. One is added so that 0
 * (LED_CALLOC_FAILED) can be used to indicate a failed calloc. This +1
 * to the index is removed in the _get_link_ptr function before resolving
 * it to the corresponding memory region.
 */
static uint32_t _calloc_state_stack_link(shmem_t* shmem_info)
{
    state_stack_link_t* current_link = shmem_info->overlay->links;
    int i = 0;
    // .state can never be zero, so this is used to find a memory region
    // that is not occupied.
    for (; current_link[i].state && i < LED_MAX_STATE_STACK_LINKS; i++);
    return i == LED_MAX_STATE_STACK_LINKS ? LED_CALLOC_FAILED : i + 1;
}

static uint32_t _guaranteed_calloc_state_stack_link(shmem_t* shmem_info)
{
    uint32_t new_index = _calloc_state_stack_link(shmem_info);
    if (new_index == LED_CALLOC_FAILED)
    {
        _unlink_and_free_oldest_state_stack_link(shmem_info);
        return _calloc_state_stack_link(shmem_info);
    }

    return new_index;
}

static void _state_stack_add(shmem_t* shmem_info, led_state_t state)
{
    uint32_t new_link_index = _guaranteed_calloc_state_stack_link(shmem_info);
    state_stack_link_t* new_link = _get_link_ptr(shmem_info, new_link_index);
    
    new_link->state = state;

    uint32_t root_link_index = shmem_info->overlay->root_link_index;

    if (root_link_index)
        new_link->next_index = root_link_index;

    shmem_info->overlay->root_link_index = new_link_index;
}

/* This function finds the most recent link that contains the given
 * state and removes it.
 */
static led_result_t _state_stack_del(shmem_t* shmem_info, led_state_t state)
{
    state_stack_link_t* prev_link = NULL;
    state_stack_link_t* current_link = NULL;
    uint32_t current_link_index = shmem_info->overlay->root_link_index;

    while (current_link_index)
    {
        current_link = _get_link_ptr(shmem_info, current_link_index);
        
        if (current_link->state == state)
            break;

        prev_link = current_link;
        current_link_index = current_link->next_index;
    }

    if (! current_link_index)
    {
        LOGE("Requested to del state \"%s\" from the stack, but it was not found.", led_state_to_str(state));
        return LED_RESULT_ERROR;
    }

    if (! prev_link)
        shmem_info->overlay->root_link_index = current_link->next_index;
    else
        prev_link->next_index = current_link->next_index;

    _free_state_stack_link(shmem_info, current_link_index);
    
    return LED_RESULT_OK;
}

static led_state_t _highest_priority_state(shmem_t* shmem_info)
{
    state_stack_link_t* highest_priority_link = NULL;
    uint32_t current_link_index = shmem_info->overlay->root_link_index;

    while (current_link_index)
    {
        state_stack_link_t* current_link = _get_link_ptr(shmem_info, current_link_index);
        
        if (! highest_priority_link || led_state_priority(current_link->state) > led_state_priority(highest_priority_link->state))
            highest_priority_link = current_link;

        current_link_index = current_link->next_index;
    }

    return highest_priority_link ? highest_priority_link->state : LED_STATE_OFF;
}

static void dump_state_stack(shmem_t* shmem_info)
{
    uint32_t current_link_index = shmem_info->overlay->root_link_index;
    while (current_link_index)
    {
       state_stack_link_t* current_link = _get_link_ptr(shmem_info, current_link_index);
       LOGD("STACK_States: %s",led_state_to_str(current_link->state));
       current_link_index = current_link->next_index;
    }
}

static void run_apply_led_state_script(led_state_t state)
{
    char cmd[100];
    if (access(APPLY_LED_STATE_SCRIPT,F_OK) == 0)
    {
        memset(cmd, 0, sizeof(cmd));
        snprintf(cmd, sizeof(cmd), "pkill -f %s", APPLY_LED_STATE_SCRIPT);
        system(cmd);
        system("sleep 1");
        memset(cmd, 0, sizeof(cmd));     
        snprintf(cmd, sizeof(cmd), "/bin/sh %s %s > /dev/null 2>&1 &", APPLY_LED_STATE_SCRIPT, led_state_to_str(state));
        system(cmd);
        LOGI("Apply_plume_led_State to : %s", led_state_to_str(state));
    }
    else
    {
        LOGE("run_apply_led_state_script: %s Not found",APPLY_LED_STATE_SCRIPT);
    }
}

static led_result_t _update_physical_led(shmem_t* shmem_info, led_state_t prev_state)
{
    led_state_t current_state = _highest_priority_state(shmem_info);

    if (current_state == prev_state)
    {
        return LED_RESULT_OK;
    }
    run_apply_led_state_script(current_state); 

    dump_state_stack(shmem_info);

    return LED_RESULT_OK;
}

static bool _is_shmem_info_invalid(shmem_t* shmem_info)
{
    return ! shmem_info->is_valid;
}

void _close_shmem(shmem_t* shmem_info)
{
    if (shmem_info->semptr)
    {
        if (sem_post(shmem_info->semptr) < 0)
            LOGE("Can't post to semaphore!");

        sem_close(shmem_info->semptr);
    }
    
    munmap(shmem_info->overlay, LED_SHMEM_SIZE);
    close(shmem_info->fd);
    memset(shmem_info, 0, sizeof(shmem_t));
}

shmem_t _init_shmem()
{
    shmem_t shmem_info = { .is_valid = false };
    
    shmem_info.semptr = sem_open(LED_SEM_NAME, O_CREAT, 0644, 1);
    if (shmem_info.semptr == SEM_FAILED)
    {
        LOGE("Can't open semaphore to lock shared mem operations");
        return shmem_info;
    }

    if (sem_wait(shmem_info.semptr) < 0)
    {
        LOGE("Error trying to lock the semaphore");
        _close_shmem(&shmem_info);
        return shmem_info;
    }

    shmem_info.fd = shm_open(LED_SHMEM_FILE, O_RDWR | O_CREAT, 0644);
    if (shmem_info.fd < 0)
    {
        LOGE("Can't open shared mem segment");
        _close_shmem(&shmem_info);
        return shmem_info;
    }

    if (ftruncate(shmem_info.fd, LED_SHMEM_SIZE) < 0)
    {
        LOGD("Shared memory backing file already truncated");  // This isn't necessarily an error
    }

    shmem_info.overlay = mmap(NULL, LED_SHMEM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shmem_info.fd, 0);
    if (shmem_info.overlay == MAP_FAILED)
    {
        LOGE("Can't mmap opened shared memory segment");
        _close_shmem(&shmem_info);
        return shmem_info;
    }

    shmem_info.is_valid = true;
    return shmem_info;
}

void led_state_stack_iter(state_stack_iter_func_t func, void* param)
{
    shmem_t shmem_info = _init_shmem();
    if (_is_shmem_info_invalid(&shmem_info)) return;

    uint32_t current_link_index = shmem_info.overlay->root_link_index;

    while (current_link_index)
    {
        state_stack_link_t* current_link = _get_link_ptr(&shmem_info, current_link_index);
        func(current_link->state, param);
        current_link_index = current_link->next_index;
    }

    _close_shmem(&shmem_info);
}

led_result_t led_set(led_state_t desired_state)
{
    if (desired_state < LED_STATE_FIRST || desired_state >= LED_STATE_LAST) return LED_RESULT_ERROR;
    if (led_state_priority(desired_state) == LED_PRIORITY_IGNORE) return LED_RESULT_OK;

    shmem_t shmem_info = _init_shmem();
    if (_is_shmem_info_invalid(&shmem_info)) return LED_RESULT_ERROR;

    led_state_t prev_state = _highest_priority_state(&shmem_info);
    _state_stack_add(&shmem_info, desired_state);
    led_result_t result = _update_physical_led(&shmem_info, prev_state);

    _close_shmem(&shmem_info);
    return result;
}

led_result_t led_clear(led_state_t desired_state)
{
    if (desired_state < LED_STATE_FIRST || desired_state >= LED_STATE_LAST) return LED_RESULT_ERROR;
    if (led_state_priority(desired_state) == LED_PRIORITY_IGNORE) return LED_RESULT_OK;

    shmem_t shmem_info = _init_shmem();
    if (_is_shmem_info_invalid(&shmem_info)) return LED_RESULT_ERROR;
    
    led_state_t prev_state = _highest_priority_state(&shmem_info);
    led_result_t result = _state_stack_del(&shmem_info, desired_state);
    result |= _update_physical_led(&shmem_info, prev_state);

    _close_shmem(&shmem_info);
    return result;
}

led_state_t led_current_state()
{
    shmem_t shmem_info = _init_shmem();
    if (_is_shmem_info_invalid(&shmem_info)) return LED_STATE_INVALID;

    led_state_t state = _highest_priority_state(&shmem_info);

    _close_shmem(&shmem_info);
    return state;
}

void _state_stack_reset()
{
    shmem_t shmem_info = _init_shmem();
    if (_is_shmem_info_invalid(&shmem_info)) return;

    uint32_t current_link_index = shmem_info.overlay->root_link_index;
    while (current_link_index)
    {
        uint32_t next_link_index = _get_link_ptr(&shmem_info, current_link_index)->next_index;
        _free_state_stack_link(&shmem_info, current_link_index);
        current_link_index = next_link_index;
    }

    shmem_info.overlay->root_link_index = 0;
    
    _close_shmem(&shmem_info);
}

void dump_state_stack_bin()
{
    shmem_t shmem_info = _init_shmem();
    uint32_t current_link_index = shmem_info.overlay->root_link_index;
    while (current_link_index)
    {
       state_stack_link_t* current_link = _get_link_ptr(&shmem_info, current_link_index);
       printf("STACK_States: %s \n",led_state_to_str(current_link->state));
       current_link_index = current_link->next_index;
    }

    _close_shmem(&shmem_info);
}

/****************************************************************************
	OSP LED API's
*****************************************************************************/
static led_state_t _osp_led_state_to_adtran(enum osp_led_state state)
{
    plume_state_data_t* map = _plume_state_map;
    for (; map->adtran_state != LED_STATE_INVALID && map->plume_state != state; map++);
    return map->adtran_state;
}

static enum osp_led_state _adtran_state_to_osp_led(led_state_t state)
{
    plume_state_data_t* map = _plume_state_map;
    for (; map->adtran_state != LED_STATE_INVALID && map->adtran_state != state; map++);
    return map->plume_state;
}

static uint32_t _adtran_priority_to_plume(led_priority_t priority)
{
    if (priority == LED_PRIORITY_DEFAULT) return OSP_LED_PRIORITY_DEFAULT;
    return 1;
}

int osp_led_init(int* led_cnt)
{
    run_apply_led_state_script(OSP_LED_ST_CONNECTING);
    return osp_led_set_state(OSP_LED_ST_CONNECTING, 2048);
}

int osp_led_set_state(enum osp_led_state state, uint32_t priority)
{
    int err = led_set(_osp_led_state_to_adtran(state)) == LED_RESULT_OK ? 0 : -1;
    LOGI("osp_led_set_state: %d, ls: %d: as: %d", err, state, _osp_led_state_to_adtran(state));
    return err;
}


int osp_led_clear_state(enum osp_led_state state)
{
    int err = led_clear(_osp_led_state_to_adtran(state)) == LED_RESULT_OK ? 0 : -1;
    LOGI("osp_led_clear_state: %d, ls: %d: as: %d", err, state, _osp_led_state_to_adtran(state));
    return err;
}

int osp_led_reset(void)
{
    _state_stack_reset();
    return 0;
}

int osp_led_get_state(enum osp_led_state* state, uint32_t* priority)
{
    led_state_t current_state = led_current_state();
    if(state != NULL)   { *state = _adtran_state_to_osp_led(current_state); }
    if(priority != NULL){ *priority = _adtran_priority_to_plume(led_state_priority(current_state)); }

    return 0;
}

