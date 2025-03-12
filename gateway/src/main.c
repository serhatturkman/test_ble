#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/logging/log.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gap.h>

/* size of stack area used by each thread */
#define STACKSIZE 1024

/* scheduling priority used by each thread */
#define PRIORITY 7

#define CONFIG_UPDATE_INTERVAL K_MINUTES(5)
#define MAX_SENSOR_DATA_SIZE 2048
#define CONFIG_DATA_SIZE 100

static struct bt_conn *default_conn;
static uint8_t config_data[CONFIG_DATA_SIZE] = {0};

/* Scan parameters */
static const struct bt_le_scan_param scan_params = {
    .type       = BT_HCI_LE_SCAN_PASSIVE,
    .options    = BT_LE_SCAN_OPT_NONE,
    .interval   = 0x0010,  // 10 ms
    .window     = 0x0010,  // 10 ms
};

/* Forward declaration of device_found function */
static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
                         struct net_buf_simple *ad);


/* Function to get RTC timestamp (stub) */
static int get_rtc_timestamp_ms(void) {
    return 0;
}

/* Callback when connection is established */
static void connected(struct bt_conn *conn, uint8_t err) {
    if (err) {
        printk("[Gateway] Connection failed (err %d)\n", err);
        return;
    }
    printk("[Gateway] Connected to Node!\n");
    default_conn = conn;
}

/* Callback when connection is disconnected */
static void disconnected(struct bt_conn *conn, uint8_t reason) {
    printk("[Gateway] Disconnected (reason %d)\n", reason);
    default_conn = NULL;

    /* Restart scanning if disconnected */
    int err = bt_le_scan_start(&scan_params, device_found);
    if (err) {
        printk("[Gateway] Scanning failed to restart (err %d)\n", err);
    } else {
        printk("[Gateway] Restarting scan for nodes...\n");
    }
}

/* BLE Callbacks */
static struct bt_conn_cb conn_callbacks = {
    .connected = connected,
    .disconnected = disconnected,
};

/* Function to update and send configuration to Node */
static void send_configuration_update(void) {
    if (!default_conn) {
        printk("[Gateway] No active connection. Skipping config update.\n");
        return;
    }

    /* Modify some values in the config data */
    config_data[0]++;
    config_data[1]++;

    /* Simulate sending configuration update */
    uint64_t timestamp = get_rtc_timestamp_ms();
    printk("[%llu ms] [Gateway] Sending configuration update to Node\n", timestamp);
}

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
                         struct net_buf_simple *ad) {
    char addr_str[BT_ADDR_LE_STR_LEN];

    /* Corrected macros */
    if (type == BT_GAP_ADV_TYPE_ADV_IND || type == BT_GAP_ADV_TYPE_ADV_DIRECT_IND) {
        bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));
        printk("[Gateway] Device found: %s (RSSI %d)\n", addr_str, rssi);
    }
}


void bluetooth_connection(void) {
    int err;
    printk("[Gateway] Starting BLE Central application\n");

    err = bt_enable(NULL);
    if (err) {
        printk("[Gateway] Bluetooth initialization failed (err %d)\n", err);
        return;
    }
    printk("[Gateway] Bluetooth initialized\n");

    bt_conn_cb_register(&conn_callbacks);

    /* Start scanning for nodes */
    err = bt_le_scan_start(&scan_params, device_found);
    if (err) {
        printk("[Gateway] Scanning failed to start (err %d)\n", err);
        return;
    }
    printk("[Gateway] Scanning started\n");

    while (1) {
        if (default_conn) {
            send_configuration_update();
        }
        k_sleep(CONFIG_UPDATE_INTERVAL);
    }
}

K_THREAD_DEFINE(bluetooth_connection_id, STACKSIZE, bluetooth_connection, NULL, NULL, NULL, PRIORITY, 0, 0);
