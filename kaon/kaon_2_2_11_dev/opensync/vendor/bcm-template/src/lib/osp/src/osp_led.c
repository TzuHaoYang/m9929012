#include "log.h"
#include "osp_led.h"


#define LED_PATH "/sys/devices/virtual/leds/"
#define POWER_LED LED_PATH "BlPowerOn_11"
#define WPS_GREEN_LED LED_PATH "WPSWireless_5"
#define WPS_BLUE_LED  LED_PATH "Reserved_9"

#define LED_ON          "default-on"
#define LED_OFF         "none"
#define LED_BREATHE     "breathe"
#define LED_BLINK       "timer"
#define LED_PATTERN     "pattern"


enum osp_led_state current_state;


static const char* led_state_str[OSP_LED_ST_LAST] =
{
    [OSP_LED_ST_IDLE]           = "idle",
    [OSP_LED_ST_ERROR]          = "error",
    [OSP_LED_ST_CONNECTED]      = "connected",
    [OSP_LED_ST_CONNECTING]     = "connecting",
    [OSP_LED_ST_CONNECTFAIL]    = "connectfail",
    [OSP_LED_ST_WPS]            = "wps",
    [OSP_LED_ST_OPTIMIZE]       = "optimize",
    [OSP_LED_ST_LOCATE]         = "locate",
    [OSP_LED_ST_HWERROR]        = "hwerror",
    [OSP_LED_ST_THERMAL]        = "thermal",
    [OSP_LED_ST_BTCONNECTABLE]  = "btconnectable",
    [OSP_LED_ST_BTCONNECTING]   = "btconnecting",
    [OSP_LED_ST_BTCONNECTED]    = "btconnected",
    [OSP_LED_ST_BTCONNECTFAIL]  = "btconnectfail",
    [OSP_LED_ST_UPGRADING]      = "upgrading",
    [OSP_LED_ST_UPGRADED]       = "upgraded",
    [OSP_LED_ST_UPGRADEFAIL]    = "upgradefail",
    [OSP_LED_ST_HWTEST]         = "hwtest",
};


const char* osp_led_state_to_str(enum osp_led_state state)
{
    if ((state < 0) || (state >= OSP_LED_ST_LAST))
        return "";

    return led_state_str[state];
}

enum osp_led_state osp_led_str_to_state(const char *str)
{
    int i;

    for (i = 0; i < OSP_LED_ST_LAST; i++)
    {
        if (!strcmp(str, led_state_str[i]))
        {
            return (enum osp_led_state)i;
        }
    }

    return OSP_LED_ST_LAST;
}

int osp_led_init(int *led_cnt)
{
    LOGW("osp_led: Dummy implementation of %s", __func__);
    current_state = OSP_LED_ST_IDLE;
    return 0;
}

int osp_led_set_state(enum osp_led_state state, uint32_t priority)
{
    LOGI("osp_led: set state implementation of %s state %d", __func__, state);
    char led_cmd[256] = "";
    switch (state)
    {
        case OSP_LED_ST_CONNECTED:                           /**< Connected */
            snprintf(led_cmd, sizeof(led_cmd), "echo %s > %s/trigger", LED_ON, POWER_LED);
            current_state = OSP_LED_ST_CONNECTED;
            break;
        case OSP_LED_ST_CONNECTING:                         /**< Connecting */
            snprintf(led_cmd, sizeof(led_cmd), "echo %s > %s/trigger", LED_BREATHE, POWER_LED);
            current_state = OSP_LED_ST_CONNECTING;
            break;
        case OSP_LED_ST_CONNECTFAIL:                        /**< Failed to connect */
            snprintf(led_cmd, sizeof(led_cmd), "echo %s > %s/trigger", LED_BREATHE, POWER_LED);
            current_state = OSP_LED_ST_CONNECTFAIL;
            break;
        case OSP_LED_ST_WPS:                                       /**< WPS active */
            snprintf(led_cmd, sizeof(led_cmd), "echo %s > %s/trigger", LED_BLINK, WPS_GREEN_LED);
            current_state = OSP_LED_ST_WPS;
            break;
        case OSP_LED_ST_OPTIMIZE:                               /**< Optimization in progress */
            snprintf(led_cmd, sizeof(led_cmd), "echo %s > %s/trigger", LED_BREATHE, WPS_BLUE_LED);
            current_state = OSP_LED_ST_OPTIMIZE;
            break;
		case OSP_LED_ST_HWERROR:                               /**< Optimization in progress */
            snprintf(led_cmd, sizeof(led_cmd), "echo %s > %s/trigger", LED_BLINK, WPS_BLUE_LED);
            current_state = OSP_LED_ST_HWERROR;
            break;
		case OSP_LED_ST_LOCATE:          /**< Locating */
            snprintf(led_cmd, sizeof(led_cmd), "echo %s > %s/trigger", LED_PATTERN, WPS_BLUE_LED);
            current_state = OSP_LED_ST_LOCATE;
            break;
#if 0     /* not used as per document */
        case OSP_LED_ST_HWERROR:                              /**< Hardware fault */
        case OSP_LED_ST_THERMAL:                                /**< Thermal panic*/
        case OSP_LED_ST_UPGRADING:                            /**< Upgrade in progress */
        case OSP_LED_ST_UPGRADED:                             /**< Upgrade finished */
        case OSP_LED_ST_UPGRADEFAIL:                         /**< Upgrade failed */
#endif
        default:
            return 0;
        break;
    }

    if (strlen(led_cmd) > 0 && system(led_cmd))
    {
        LOGW("osp_led: led set failed");
        return -1;
    }
    return 0;
}

int osp_led_clear_state(enum osp_led_state state)
{
    char led_cmd[256] = "";
    LOGI("osp_led: clear state implementation of %s", __func__);
    switch(state)
    {
        case OSP_LED_ST_OPTIMIZE:
        case OSP_LED_ST_HWERROR:
        case OSP_LED_ST_LOCATE:
            snprintf(led_cmd, sizeof(led_cmd), "echo %s > %s/trigger", LED_OFF, WPS_BLUE_LED);
			break;
		case OSP_LED_ST_WPS:                                       /**< WPS done */
            snprintf(led_cmd, sizeof(led_cmd), "echo %s > %s/trigger", LED_OFF, WPS_GREEN_LED);
            break;
        default:
        break;
    }
	if (strlen(led_cmd) > 0 && system(led_cmd))
	{
		LOGW("osp_led: led clear failed ");
		return -1;
	}
	return 0;
}

int osp_led_reset(void)
{
    LOGI("osp_led: Dummy implementation of %s", __func__);
    return 0;
}

int osp_led_get_state(enum osp_led_state *state, uint32_t *priority)
{
    LOGI("osp_led: get the led state %s", __func__);
    *state = current_state;
    return 0;
}
