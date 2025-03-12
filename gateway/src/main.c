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



/* Function to get RTC timestamp */
static int get_rtc_timestamp_ms(void) {
    return 0;
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

void bluetooth_connection(void) {
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

K_THREAD_DEFINE(bluetooth_connection_id, STACKSIZE, bluetooth_connection, NULL, NULL, NULL, PRIORITY, 0, 0);