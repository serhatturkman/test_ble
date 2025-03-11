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

static uint8_t config_data[100] = {0};

static void bt_ready(int err) {
    if (err) {
        printk("Bluetooth init failed (err %d)\n", err);
        return;
    }
    printk("Bluetooth initialized\n");
}

int main(void) {
    int err;
    printk("[GATEWAY] Starting BLE Central application\n");

    err = bt_enable(bt_ready);
    if (err) {
        printk("Bluetooth initialization failed (err %d)\n", err);
        return 0;
    }
    
    while (1) {
        printk("[GATEWAY] Waiting for Node data...\n");
        k_sleep(K_SECONDS(30));
    }
    return 0;
}
