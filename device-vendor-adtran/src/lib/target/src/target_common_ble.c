#include <stdbool.h>
#include <stdio.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/ioctl.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

#include "os.h"
#include "log.h"
#include "const.h"
#include "target.h"

/******************************************************************************
 *  BLE
 *****************************************************************************/

#define BLE_HCI_DEVICE_ID               0
#define BLE_INTERVAL_VALUE_DEFAULT      100
#define BLE_TXPOWER_VALUE_DEFAULT       0x5C    /* The Bluetooth beacon TxPower is not configurable. The TxPower contains an internal Plume value. */
#define BLE_ADV_DATA_UUID_ID            0xFE71
#define BLE_ADV_DATA_COMPANY_ID         0x0008
#define BLE_ADV_DATA_MAJOR_VER          0x00
#define BLE_ADV_DATA_MINOR_VER          0x04
#define BLE_ADV_DATA_TXPOWER            0x005C
#define BLE_ADV_PARAM_CHAN_MAP          0x07    // Channel 37, 38 and 39, all channels enabled (default)

#define HCI_TIMEOUT                     1000
/** Non connectable undirected advertising (BT CS 5.2 [Vol 4] Part E, Section 7.7.65.2). */
#define ADV_NONCONN_IND                 0x03
/** Connectable and scannable undirected advertising (BT CS 5.2 [Vol 4] Part E, Section 7.7.65.2). */
#define ADV_IND                         0x00

struct ble_adv_data {
    /** Length of the complete advertisement packet payload */
    uint8_t data_len; /*< = sizeof(data) */
    /**
     * Advertising data structure, exactly resembling 31 byte BLE advertisement packet payload
     *
     * https://confluence.plume.tech/display/PLAT/Bluetooth+Low+Energy+(BLE)#BluetoothLowEnergy(BLE)-Beacons(Broadcasterrole)
     */
    struct __attribute__ ((packed)) {
        /* Complete List of 16-bit Service Class UUIDs */
        uint8_t len_uuid;
        uint8_t ad_type_uuid;
        uint16_t service_uuid;

        /* Plume Design Inc SIG-member defined GATT service */
        uint8_t len_manufacturer;
        uint8_t ad_type_manufacturer;
        /* region 25 bytes of manufacturer specific data */
        uint16_t company_id;
        uint8_t major_ver;
        uint8_t minor_ver;
        uint16_t txpower;
        uint8_t serial_num[12];
        uint8_t msg_type;
        struct __attribute__ ((packed)) {
            uint8_t status;
            uint8_t _rfu[5];
            /* Random token is not present when not connectable */
        } msg;
        /* endregion 25 bytes of manufacturer specific data */
    } data;
} __attribute__ ((packed));

typedef enum
{
    BLE_CMD_ONBOARDING = 0x0,  // Cmd 0x00
    BLE_CMD_DIAGNOSTIC,        // Cmd 0x01
    BLE_CMD_LOCATE,            // Cmd 0x02
    BLE_CMD_MAX
} ble_command_t;

// BLE Commands
static c_item_t map_ble_command[] = {
    C_ITEM_STR(BLE_CMD_ONBOARDING,  "on_boarding"),
    C_ITEM_STR(BLE_CMD_DIAGNOSTIC,  "diagnostic"),
    C_ITEM_STR(BLE_CMD_LOCATE,      "locate"),
};

static advertising_enabled = false;

static int power_on(int hdev, int dd)
{
    LOGI("Powering on");

    if (ioctl(dd, HCIDEVUP, hdev) < 0) {
        if (errno != EALREADY) {
            LOGE("Can't up hci%d: %s %d",hdev, strerror(errno), errno);
            return -1;
        }
        LOGE("Already up hci%d: %s %d",hdev, strerror(errno), errno);
    }

    return 0;
}

static int power_off(int hdev, int dd)
{
    LOGI("Powering off");

    /* Advertising will be stopped for sure */
    advertising_enabled = false;
    /* Stop HCI device */
    if (ioctl(dd, HCIDEVDOWN, hdev) < 0) {
        LOGE("Can't stop hci%d: %s (%d)\n", hdev, strerror(errno), errno);
        return -1;
    }
    return 0;
}

static int push_adv_data(int hdev, int dd, struct ble_adv_data *ad, uint16_t interval)
{
    int rv;

    LOGI("Pushing advertisement data");

    /* The following commands will fail if the advertisement is currently enabled. */
    hci_le_set_advertise_enable(dd, 0, HCI_TIMEOUT); /* Ignore error if not enabled. */

    /* set advertisement data */
    rv = hci_send_cmd(dd, OGF_LE_CTL, OCF_LE_SET_ADVERTISING_DATA, sizeof(*ad), (void *)ad);
    if (rv != 0) {
        LOGE("Failed to set advertisement data, hci%d: %s (%d)\n", hdev, strerror(errno), errno);
        return -1;
    }

    /* set advertisement parameters */
    le_set_advertising_parameters_cp ad_param = {
        .min_interval = htobs(interval * 1000 / 625),
        .max_interval = htobs(interval * 1000 / 625),
        .advtype = ADV_NONCONN_IND,    /* Non-connectable undirected advertising */
        .own_bdaddr_type = 0x00,       /* Use public Bluetooth address */
        .chan_map = BLE_ADV_PARAM_CHAN_MAP
    };

    rv = hci_send_cmd(dd, OGF_LE_CTL, OCF_LE_SET_ADVERTISING_PARAMETERS, sizeof(ad_param), (void *)&ad_param);
    if (rv != 0) {
        LOGE("Failed to set advertisement params, hci%d: %s (%d)\n", hdev, strerror(errno), errno);
        return -1;
    }

    /* Again turn on advertising if it was enabled before */
    if (advertising_enabled) {
        hci_le_set_advertise_enable(dd, 1, HCI_TIMEOUT);
    }

    return 0;
}

static int start_adv(int hdev, int dd, struct ble_adv_data *ad)
{
    LOGI("Start advertising");

    if (advertising_enabled) {
        LOGD("BLE advertising already enabled, restarting...");
        /* If advertisement is currently enabled, disable it first to prevent error
         * when enabling it. Better than have controlled error here (occurs if the
         * advertisement is already disabled) than when enabling it. */
        hci_le_set_advertise_enable(dd, 0, HCI_TIMEOUT);
    }

    int rv = hci_le_set_advertise_enable(dd, 1, HCI_TIMEOUT);
    if (rv != 0) {
        LOGE("Could not start BLE advertising (rv = %d, errno = %s (%d))", rv, strerror(errno), errno);
    }

    advertising_enabled = true;

    return rv;
}

static int stop_adv(int hdev, int dd)
{
    LOGI("Stop advertising");

    int rv = hci_le_set_advertise_enable(dd, 0, HCI_TIMEOUT);
    if (rv != 0) {
        if (errno == EIO) {
            if (advertising_enabled) {
                /* Advertising was enabled - disabling it shall succeed */
                LOGW("Could not stop BLE advertising");
            }
            else {
                /* Advertising was probably already disabled (cCheck advertising_enabled doc for explanation) */
                LOGD("Advertising already disabled");
                rv = 0;
            }
        }
        else if (errno == ENETDOWN) {
            LOGD("HCI device already powered off");
            rv = 0;
        }
        else {
            LOGE("Could not stop BLE advertising (rv = %d, errno = %s (%d))", rv, strerror(errno), errno);
        }
    }
    advertising_enabled = false;

    return rv;
}

static int fill_adv_data(struct ble_adv_data *ad, uint16_t txpower, uint8_t serial_num[], uint8_t msg_type, uint8_t msg[])
{
    LOGI("Filling advertisement data");

    ad->data_len = sizeof(ad->data);
    ad->data.len_uuid = 3;
    ad->data.ad_type_uuid = 0x03;
    ad->data.service_uuid = htobs(BLE_ADV_DATA_UUID_ID);
    ad->data.len_manufacturer = 0x1A;
    ad->data.ad_type_manufacturer = 0xFF;
    ad->data.company_id = htobs(BLE_ADV_DATA_COMPANY_ID);
    ad->data.major_ver = BLE_ADV_DATA_MAJOR_VER;
    ad->data.minor_ver = BLE_ADV_DATA_MINOR_VER;
    ad->data.txpower = htobs(txpower);
    memcpy(&ad->data.serial_num, serial_num, sizeof(ad->data.serial_num));
    ad->data.msg_type = msg_type;
    memcpy(&ad->data.msg, msg, sizeof(ad->data.msg));

    return 0;
}


bool target_ble_broadcast_start(struct schema_AW_Bluetooth_Config *config)
{
    c_item_t *item;
    int hdev = BLE_HCI_DEVICE_ID;    // hci device id
    int dd;                          // hci driver descriptor
    int rv;
    struct ble_adv_data ad;
    uint16_t txpower;
    uint8_t serial_num[12] = {0};
    uint8_t msg_type;
    uint8_t msg[6] = {0};
    uint16_t interval;

    // Get appropriate command
    item = c_get_item_by_str(map_ble_command, config->command);
    if (item == NULL) {
        LOGE("Unknown command: %s", config->command);
        return false;
    }
    LOGN("Updating ble broadcast (interval=%d, txpwr=%d, cmd=%d, data=%s)",
         config->interval_millis, config->txpower, (int)item->key, config->payload);
    LOGN("yichia");

    dd = hci_open_dev(hdev);
    if (dd < 0) {
        LOGE("Could not open device, hci%d: %s (%d)", hdev, strerror(errno), errno);
        return false;
    }

    if (power_on(hdev, dd) != 0) {
        LOGE("Power on failed, hci%d: %s (%d)", hdev, strerror(errno), errno);
        goto failed;
    }

    //get serial number
    get_serial_number(serial_num, sizeof(serial_num)/sizeof(uint8_t));
    txpower = (config->txpower != 0) ? config->txpower : BLE_ADV_DATA_TXPOWER;
    msg_type = (uint8_t)item->key;
    rv = sscanf(config->payload, "%02x:%02x:%02x:%02x:%02x:%02x",
            &msg[0], &msg[1], &msg[2], &msg[3], &msg[4], &msg[5]);
    if (rv != 6) {
        LOGE("Invalid payload '%s'", config->payload);
        goto failed;
    } 
    fill_adv_data(&ad, txpower, serial_num, msg_type, msg);

    interval = (config->interval_millis != 0) ? config->interval_millis : BLE_INTERVAL_VALUE_DEFAULT;
    if (push_adv_data(hdev, dd, &ad, interval) != 0) {
        LOGE("Push advertisement data failed, hci%d: %s (%d)", hdev, strerror(errno), errno);
        goto failed;
    }

    if (start_adv(hdev, dd, &ad) != 0) {
        LOGE("Start advertisement failed, hci%d: %s (%d)", hdev, strerror(errno), errno);
        goto failed;
    }

failed:
    hci_close_dev(hdev);
    return 0;
}

bool target_ble_broadcast_stop(void)
{
    int hdev = BLE_HCI_DEVICE_ID;    // hci device id
    int dd;                          // hci driver descriptor

    LOGN("Stopping ble broadcast");

    dd = hci_open_dev(hdev);
    if (dd < 0) {
        LOGE("Could not open device hci%d: %s (%d)", hdev, strerror(errno), errno);
        return false;
    }

    if (stop_adv(hdev, dd) != 0) {
        LOGE("stop advertisement failed, hci%d: %s (%d)", hdev, strerror(errno), errno);
    }

    if (power_off(hdev, dd) != 0) {
        LOGE("power off failed, hci%d: %s (%d)", hdev, strerror(errno), errno);
    }

failed:
    hci_close_dev(hdev);
    return 0;
}
