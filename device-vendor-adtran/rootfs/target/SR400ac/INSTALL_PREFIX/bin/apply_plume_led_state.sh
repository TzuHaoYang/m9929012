#!/bin/sh 

#======================================================================================================================================
# apply_plume_led_state.sh
# This script is to control the Status LED specifically on SR400ac

#PULSE	Dim to Bright to Dim – cycle takes 2500msec
#BLINK	300msec ON, 300msec OFF, 700msec ON, 1500msec OFF (cycle 2800msec)

#   // States                     LED_STATE                    Color            Action           Priority              String representation
#   // --------------------------------------------------------------------------------------------------------------------------
#    { OSP_LED_ST_IDLE,           LED_STATE_OFF,               LED_COLOR_RED,   LED_OFF,         LED_PRIORITY_DEFAULT, "off" },
#    { OSP_LED_ST_ERROR,          LED_STATE_PLUME_ERROR,       LED_COLOR_RED,   LED_SHORT_PULSE, LED_PRIORITY_HIGH,    "plume_error" },
#    { OSP_LED_ST_CONNECTED,      LED_STATE_PLUME_CONNECTED,   LED_COLOR_RED,   LED_OFF,         LED_PRIORITY_IGNORE,  "plume_connected" },
#    { OSP_LED_ST_CONNECTING,     LED_STATE_PLUME_CONNECTING,  LED_COLOR_GREEN, LED_LONG_PULSE,  LED_PRIORITY_DEFAULT, "plume_connecting" },
#    { OSP_LED_ST_CONNECTFAIL,    LED_STATE_PLUME_CONNECTFAIL, LED_COLOR_WHITE, LED_LONG_PULSE,  LED_PRIORITY_DEFAULT, "plume_connectfail"},
#    { OSP_LED_ST_WPS,            LED_STATE_WPS,               LED_COLOR_BLUE,  LED_SHORT_PULSE, LED_PRIORITY_IGNORE,  "wps"},
#    { OSP_LED_ST_OPTIMIZE,       LED_STATE_OPTIMIZE,          LED_COLOR_GREEN, LED_BLINK,       LED_PRIORITY_DEFAULT, "optimize"},
#    { OSP_LED_ST_LOCATE,         LED_STATE_LOCATE,            LED_COLOR_GREEN, LED_BLINK,       LED_PRIORITY_DEFAULT, "locate" },
#    { OSP_LED_ST_HWERROR,        LED_STATE_HWERROR,           LED_COLOR_RED,   LED_SHORT_PULSE, LED_PRIORITY_HIGH,    "hwerror" },
#    { OSP_LED_ST_THERMAL,        LED_STATE_THERMAL,           LED_COLOR_RED,   LED_SHORT_PULSE, LED_PRIORITY_HIGH,    "thermal" }
#    { OSP_LED_ST_BTCONNECTABLE,  LED_STATE_BTCONNECTABLE,     LED_COLOR_BLUE,  LED_SHORT_PULSE, LED_PRIORITY_IGNORE,  "btconnectable" },
#    { OSP_LED_ST_BTCONNECTING,   LED_STATE_BTCONNECTING,      LED_COLOR_BLUE,  LED_SHORT_PULSE, LED_PRIORITY_IGNORE,  "btconnecting" },
#    { OSP_LED_ST_BTCONNECTED,    LED_STATE_BTCONNECTED,       LED_COLOR_BLUE,  LED_SHORT_PULSE, LED_PRIORITY_IGNORE,  "btconnected" },
#    { OSP_LED_ST_BTCONNECTFAIL,  LED_STATE_BTCONNECTFAIL,     LED_COLOR_BLUE,  LED_SHORT_PULSE, LED_PRIORITY_IGNORE,  "btconnectfail" },
#    { OSP_LED_ST_UPGRADING,      LED_STATE_UPGRADING,         LED_COLOR_BLUE,  LED_SHORT_PULSE, LED_PRIORITY_IGNORE,  "upgrading" },
#    { OSP_LED_ST_UPGRADED,       LED_STATE_UPGRADED,          LED_COLOR_BLUE,  LED_SHORT_PULSE, LED_PRIORITY_IGNORE,  "upgraded" },
#    { OSP_LED_ST_UPGRADEFAIL,    LED_STATE_UPGRADEFAIL,       LED_COLOR_BLUE,  LED_SHORT_PULSE, LED_PRIORITY_IGNORE,  "upgradefail" },
#    { OSP_LED_ST_HWTEST,         LED_STATE_HWTEST,            LED_COLOR_ALL,   LED_STEADY_ON,   LED_PRIORITY_DEFAULT, "hwtest" },
#    { OSP_LED_ST_LAST,           LED_STATE_OFF,               LED_COLOR_RED,   LED_OFF,         LED_PRIORITY_DEFAULT, "unknown"}

#RED_STATUS="/sys/class/leds/bcm53xx:red:status/brightness"
#GREEN_STATUS="/sys/class/leds/bcm53xx:green:status/brightness"
#BLUE_STATUS="/sys/class/leds/bcm53xx:blue:status/brightness"
#=======================================================================================================================================

LED_STATE_STR="$1"
MAX_BRIGHT=255
MIN_BRIGHT=0
RED="red"
GREEN="green"
BLUE="blue"
WHITE="white"
LED_CONFIG_STATE_FILE="/tmp/.ledCurrState"
SHORT_PULSE=250000
LONG_PULSE=2500000
#***************************************************************
# toggle(): This function is to set the required brightness to a 
# specific colour passed to this function
# toggle <colour> <brightness>
#***************************************************************
toggle() 
{
    echo ${2} > /sys/class/leds/bcm53xx\:${1}\:status/brightness
}

#***************************************************************
# led_off(): This function set the brightness of all colours to 
# zero
#***************************************************************
led_off()
{
  toggle $RED $MIN_BRIGHT
  toggle $GREEN $MIN_BRIGHT
  toggle $BLUE $MIN_BRIGHT
}

#***************************************************************
# led_off_unused(): This function is to clear the brightness of 
# unwanted leds. Pass the required colour to be visible as an 
# argument and other colour will be cleared.
# led_clear_unused <required colour>
#***************************************************************
led_clear_unused()
{
    colour="${1}"
    if [ "$colour" == "$RED" ]; then
       toggle $GREEN $MIN_BRIGHT 
       toggle $BLUE $MIN_BRIGHT
    fi

    if [ "$colour" == "$GREEN" ]; then
       toggle $RED $MIN_BRIGHT
       toggle $BLUE $MIN_BRIGHT
    fi

    if [ "$colour" == "$BLUE" ]; then
       toggle $RED $MIN_BRIGHT
       toggle $GREEN $MIN_BRIGHT
    fi
}

#***************************************************************
# led_dim_to_bright():This function is to set the brightness from
# dim to bright.
# led_dim_to_bright <colour> <delay to sleep>
#***************************************************************
led_dim_to_bright()
{
    colour=${1}
    delay=${2}
    i=$MIN_BRIGHT

    while [ $i -le $MAX_BRIGHT ]
    do
        if  [ "$colour" == "$WHITE" ]; then
            toggle $RED $i
            toggle $GREEN $i
            toggle $BLUE $i
        else
            toggle $colour $i
        fi

        usleep $delay
        i=`expr $i + 1`
    done

}

#***************************************************************
# led_bright_to_dim(): This function is to set the brightness from
# bright to dim.
# led_bright_to_dim <colour> <delay to sleep>>
#***************************************************************
led_bright_to_dim()
{
    colour=${1}
    delay=${2}
    i=$MAX_BRIGHT

    while [ $i -ge $MIN_BRIGHT ]
    do
        if  [ "$colour" == "$WHITE" ]; then
            toggle $RED $i
            toggle $GREEN $i
            toggle $BLUE $i
        else
            toggle $colour $i
        fi

        usleep $delay
        i=`expr $i - 1`
    done
}
#***************************************************************
# led_steady_on(): This function is to keep a led ON continuously.
# led_steady_on <colour>
#***************************************************************
led_steady_on()
{
    colour=${1}
    led_clear_unused $colour
    toggle $colour $MAX_BRIGHT 
}

#***************************************************************
# led_on_all(): This function is to keep RGB with full brightness
# This simulates colour white.
#***************************************************************
led_on_all() 
{
    toggle $RED $MAX_BRIGHT
    toggle $GREEN $MAX_BRIGHT
    toggle $BLUE $MAX_BRIGHT
}

#***************************************************************
# led_pulse(): This function is to simulate dim-to-bright-to-dim
# led_pulse <colour> <pulse in ms>
#***************************************************************
led_pulse()
{
   colour=${1}
   pulseTms=${2}
   unitpulse=$(awk "BEGIN {x=$pulseTms/2; y=x/$MAX_BRIGHT; print y}")
   delay=$(echo $unitpulse | awk '{print ($0-int($0)<0.499)?int($0):int($0)+1}')
   led_clear_unused $colour
   while [ 1 ]; do
       led_dim_to_bright $colour $delay
       led_bright_to_dim $colour $delay
   done
}

#***************************************************************
# led_blink(): This function is to blink a led as per given 
# cycle time.
#BLINK   300msec ON, 300msec OFF, 700msec ON, 1500msec OFF (cycle 2800msec)
# led_blink <colour>
#***************************************************************
led_blink()
{
    colour=${1}
    led_clear_unused $colour
    while [ 1 ]; do
        toggle $colour $MAX_BRIGHT
        usleep 300000
        toggle $colour $MIN_BRIGHT
        usleep 300000
        toggle $colour $MAX_BRIGHT
        usleep 700000
        toggle $colour $MIN_BRIGHT
        usleep 1500000
    done 
}

#***************************************************************
# apply_led_config_state(): This functions to set the led state 
# based on led_config_state passed to it.
# apply_led_config_state <led config state>
#***************************************************************
apply_led_config_state()
{
    led_config_state_str=${1}
    case $led_config_state_str in
    "plume_error" | "hwerror" | "thermal")
        led_pulse $RED $SHORT_PULSE
        ;;
    "plume_connecting")
        led_pulse $GREEN $LONG_PULSE
        ;;
    "plume_connectfail")
        led_pulse $WHITE $LONG_PULSE
        ;;
    "optimize" | "locate")
        led_blink $GREEN
        ;;
    "wps" | "btconnectable" | "btconnecting" | "btconnected" | "btconnectfail")
        led_pulse $BLUE $SHORT_PULSE
        ;;
    "upgrading" | "upgraded" | "upgradefail")
        led_pulse $BLUE $SHORT_PULSE
        ;;
    "hwtest")
        led_on_all
        ;;
    "off" | "plume_connected" | "unknown")
        led_off
        ;;
    *)
        led_pulse $GREEN $LONG_PULSE
        ;;
    esac
}

#***************************************************************
# main(): This is a main function. Call apply_led_config_state
# to set the led state. 
#***************************************************************
main()
{
     apply_led_config_state "$LED_STATE_STR"
}

main


