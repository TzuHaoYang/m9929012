#include "cms_log.h"
#include "i2cled.h"

#include "log.h"
#include "osp_led.h"

typedef struct
{
    enum osp_led_state plume_state;
    i2c_led_state_t adtran_state;
} plume_state_data_t;

static plume_state_data_t _plume_state_map[] =
{
    { OSP_LED_ST_IDLE,           I2C_LED_STATE_OFF },
    { OSP_LED_ST_ERROR,          I2C_LED_STATE_PLUME_ERROR },
    { OSP_LED_ST_CONNECTED,      I2C_LED_STATE_PLUME_CONNECTED },
    { OSP_LED_ST_CONNECTING,     I2C_LED_STATE_PLUME_CONNECTING },
    { OSP_LED_ST_CONNECTFAIL,    I2C_LED_STATE_PLUME_CONNECTFAIL },
    { OSP_LED_ST_WPS,            I2C_LED_STATE_WPS },
    { OSP_LED_ST_OPTIMIZE,       I2C_LED_STATE_OPTIMIZE },
    { OSP_LED_ST_LOCATE,         I2C_LED_STATE_LOCATE },
    { OSP_LED_ST_HWERROR,        I2C_LED_STATE_HWERROR },
    { OSP_LED_ST_THERMAL,        I2C_LED_STATE_THERMAL },
//    { OSP_LED_ST_BTCONNECTABLE,  I2C_LED_STATE_BTCONNECTABLE },
    { OSP_LED_ST_BTCONNECTING,   I2C_LED_STATE_BTCONNECTING },
    { OSP_LED_ST_BTCONNECTED,    I2C_LED_STATE_BTCONNECTED },
    { OSP_LED_ST_BTCONNECTFAIL,  I2C_LED_STATE_BTCONNECTFAIL },
    { OSP_LED_ST_UPGRADING,      I2C_LED_STATE_UPGRADING },
    { OSP_LED_ST_UPGRADED,       I2C_LED_STATE_UPGRADED },
    { OSP_LED_ST_UPGRADEFAIL,    I2C_LED_STATE_UPGRADEFAIL },
    { OSP_LED_ST_HWTEST,         I2C_LED_STATE_HWTEST },
    { OSP_LED_ST_LAST,           I2C_LED_STATE_INVALID }
};

static i2c_led_state_t _osp_led_state_to_adtran(enum osp_led_state state)
{
    plume_state_data_t* map = _plume_state_map;
    for (; map->adtran_state != I2C_LED_STATE_INVALID && map->plume_state != state; map++);
    return map->adtran_state;
}

static enum osp_led_state _adtran_state_to_osp_led(i2c_led_state_t state)
{
    plume_state_data_t* map = _plume_state_map;
    for (; map->adtran_state != I2C_LED_STATE_INVALID && map->adtran_state != state; map++);
    return map->plume_state;
}

static uint32_t _adtran_priority_to_plume(i2c_led_priority_t priority)
{
    if (priority == I2C_LED_PRIORITY_DEFAULT) return OSP_LED_PRIORITY_DEFAULT;
    return 1;
}

int osp_led_init(int* led_cnt)
{
    return osp_led_set_state(OSP_LED_ST_CONNECTING, 2048);
}

int osp_led_set_state(enum osp_led_state state, uint32_t priority)
{
    int err = i2c_led_set(_osp_led_state_to_adtran(state)) == I2C_LED_RESULT_OK ? 0 : -1;
    LOGI("osp_led_set_state: %d, ls: %d: as: %d", err, state, _osp_led_state_to_adtran(state));
    return err;
}

int osp_led_clear_state(enum osp_led_state state)
{
    int err = i2c_led_clear(_osp_led_state_to_adtran(state)) == I2C_LED_RESULT_OK ? 0 : -1;
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
    i2c_led_state_t current_state = i2c_led_current_state();
    if(state != NULL)   { *state = _adtran_state_to_osp_led(current_state); }
    if(priority != NULL){ *priority = _adtran_priority_to_plume(i2c_led_state_priority(current_state)); }

    return 0;
}

/*
const char* osp_led_state_to_str(enum osp_led_state state)
{
    return i2c_led_state_to_str(_osp_led_state_to_adtran(state));
}

enum osp_led_state osp_led_str_to_state(const char* str)
{
    return _adtran_state_to_osp_led(i2c_led_str_to_state(str));
}
*/
