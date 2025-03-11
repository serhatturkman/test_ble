/*
 * Initial Zephyr BLE Project - Gateway & Node
 * Gateway (BLE Central) - Node (BLE Peripheral)
 * 
 * Structure:
 * - gateway/: BLE Central application
 * - node/: BLE Peripheral application
 * - boards/: Custom board configurations
 */

/* Gateway Application - BLE Central */

#include "zephyr.h"

#define CONFIG_UPDATE_INTERVAL K_MINUTES(5)
#define MAX_SENSOR_DATA_SIZE 2048
#define CONFIG_DATA_SIZE 100

static struct bt_conn *default_conn;
static uint8_t config_data[CONFIG_DATA_SIZE] = {0};

/* Function to get RTC timestamp */
static uint64_t get_rtc_timestamp_ms(void) {
    const struct device *rtc_dev = DEVICE_DT_GET(DT_NODELABEL(rtc0));
    if (!device_is_ready(rtc_dev)) {
        printk("RTC device not ready\n");
        return 0;
    }
    uint32_t ticks;
    counter_get_value(rtc_dev, &ticks);
    return (uint64_t)ticks;
}

/* Callback when data is received from a node */
static void node_data_received(struct bt_conn *conn, const uint8_t *data, uint16_t len) {
    uint64_t timestamp = get_rtc_timestamp_ms();
    printk("[%llu ms] [Gateway] Received data from Node (len: %d)\n", timestamp, len);
}

/* Callback when connection is established */
static void connected(struct bt_conn *conn, uint8_t err) {
    if (err) {
        printk("Connection failed (err %d)\n", err);
        return;
    }
    printk("Connected to Node!\n");
    default_conn = conn;
}

/* Callback when connection is disconnected */
static void disconnected(struct bt_conn *conn, uint8_t reason) {
    printk("Disconnected (reason %d)\n", reason);
    default_conn = NULL;
}

/* BLE Callbacks */
static struct bt_conn_cb conn_callbacks = {
    .connected = connected,
    .disconnected = disconnected,
};

/* Function to update and send configuration to Node */
static void send_configuration_update(void) {
    if (!default_conn) {
        printk("No active connection. Skipping config update.\n");
        return;
    }
    
    /* Modify some values in the config data */
    config_data[0]++;
    config_data[1]++;
    
    /* Simulate sending configuration update */
    uint64_t timestamp = get_rtc_timestamp_ms();
    printk("[%llu ms] [Gateway] Sending configuration update to Node\n", timestamp);
}

void main(void) {
    int err;
    printk("[Gateway] Starting BLE Central application\n");

    err = bt_enable(NULL);
    if (err) {
        printk("Bluetooth initialization failed (err %d)\n", err);
        return;
    }
    printk("Bluetooth initialized\n");

    bt_conn_cb_register(&conn_callbacks);
    printk("Scanning for nodes...\n");
    
    while (1) {
        /* Periodically send configuration update */
        send_configuration_update();
        k_sleep(CONFIG_UPDATE_INTERVAL);
    }
}