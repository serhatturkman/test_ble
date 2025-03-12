#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys/printk.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gap.h>

/* Thread Configuration */
#define STACKSIZE 1024
#define PRIORITY 7

#define CONFIG_UPDATE_INTERVAL K_MINUTES(5)
#define SCAN_TIMEOUT K_SECONDS(30)
#define SCAN_RETRY_INTERVAL K_SECONDS(10)

#define SENSOR_DATA_SIZE 20
#define CONFIG_DATA_SIZE 100

static struct bt_conn *default_conn = NULL;
static struct bt_gatt_subscribe_params subscribe_params;
static uint8_t config_data[CONFIG_DATA_SIZE] = {0};
static uint16_t config_handle = 0;  // Will store the characteristic handle for config writes

static uint8_t discover_func(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                             struct bt_gatt_discover_params *params);

/* Scan parameters */
static const struct bt_le_scan_param scan_params = {
    .type       = BT_HCI_LE_SCAN_PASSIVE,
    .options    = BT_LE_SCAN_OPT_NONE,
    .interval   = 0x0010,  
    .window     = 0x0010,  
};

static struct bt_gatt_discover_params discover_params = {
    .uuid = NULL,  
    .func = discover_func,  // ✅ Correct function assignment
    .start_handle = 0x0001,
    .end_handle = 0xFFFF,
    .type = BT_GATT_DISCOVER_ATTRIBUTE
};

/* Callback function for receiving sensor data */
static uint8_t sensor_data_received(struct bt_conn *conn, struct bt_gatt_subscribe_params *params,
                                    const void *data, uint16_t len) {
    printk("[Gateway] Received Sensor Data (%d bytes)\n", len);
    
    const uint8_t *received_data = (const uint8_t *)data;
    printk("[Gateway] Data: %02X %02X %02X ...\n", received_data[0], received_data[1], received_data[2]);

    return BT_GATT_ITER_CONTINUE;
}

/* BLE Connection Callback */
static void connected(struct bt_conn *conn, uint8_t err) {
    if (err) {
        printk("[Gateway] Connection failed (err %d)\n", err);
        return;
    }
    
    printk("[Gateway] Connected to Node!\n");
    default_conn = conn;

    int ret = bt_gatt_discover(conn, &discover_params);
    if (ret) {
        printk("[Gateway] Failed to start discovery (err %d)\n", ret);
    } else {
        printk("[Gateway] Started GATT service discovery...\n");
    }
}

/* BLE Disconnection Callback */
static void disconnected(struct bt_conn *conn, uint8_t reason) {
    printk("[Gateway] Disconnected (reason %d)\n", reason);
    default_conn = NULL;

    /* Wait before restarting scan */
    k_sleep(K_SECONDS(5));

    int err = bt_le_scan_start(&scan_params, NULL);
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

/* GATT Discovery Callback */
static uint8_t discover_func(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                             struct bt_gatt_discover_params *params) {
    if (!attr) {
        printk("[Gateway] Discovery complete. No matching attribute found.\n");
        return BT_GATT_ITER_STOP;
    }

    uint16_t handle = attr->handle;
    printk("[Gateway] Discovered Attribute Handle: 0x%04X\n", handle);

    if (!subscribe_params.ccc_handle) {
        /* Found the Notify Characteristic for Sensor Data */
        subscribe_params.ccc_handle = handle;
        subscribe_params.notify = sensor_data_received;
        
        int err = bt_gatt_subscribe(conn, &subscribe_params);
        if (err) {
            printk("[Gateway] Subscription failed (err %d)\n", err);
        } else {
            printk("[Gateway] Subscribed to sensor data!\n");
        }
    } else if (!config_handle) {
        /* Found the Config Write Characteristic */
        config_handle = handle;
        printk("[Gateway] Found Config Write Handle: 0x%04X\n", config_handle);
    }

    return BT_GATT_ITER_STOP;
}



/* Send Configuration Update */
static void send_configuration_update(void) {
    if (!default_conn) {
        printk("[Gateway] No active connection. Skipping config update.\n");
        return;
    }

    if (config_handle == 0) {
        printk("[Gateway] Config handle not found. Skipping config update.\n");
        return;
    }

    config_data[0]++;
    config_data[1]++;

    int err = bt_gatt_write_without_response(default_conn, config_handle, config_data, CONFIG_DATA_SIZE, false);
    if (err) {
        printk("[Gateway] Config update failed (err %d)\n", err);
    } else {
        printk("[Gateway] Config update sent to Node\n");
    }
}

/* Bluetooth Connection Handler */
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

    while (1) {
        if (!default_conn) {
            printk("[Gateway] Scanning for nodes (Timeout: 30s)...\n");
            err = bt_le_scan_start(&scan_params, NULL);
            if (err) {
                printk("[Gateway] Scanning failed to start (err %d)\n", err);
                k_sleep(SCAN_RETRY_INTERVAL);
                continue;
            }
            k_sleep(SCAN_TIMEOUT);
            bt_le_scan_stop();
            printk("[Gateway] Scan timeout, retrying in 10s...\n");
            k_sleep(SCAN_RETRY_INTERVAL);
        } else {
            send_configuration_update();
        }
        k_sleep(CONFIG_UPDATE_INTERVAL);
    }
}

K_THREAD_DEFINE(bluetooth_connection_id, STACKSIZE, bluetooth_connection, NULL, NULL, NULL, PRIORITY, 0, 0);
