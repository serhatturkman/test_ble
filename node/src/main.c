#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys/printk.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/gap.h>

/* Thread Configuration */
#define STACKSIZE 1024
#define PRIORITY 7

/* Max buffer size for sensor data */
#define SENSOR_DATA_SIZE 20

static uint8_t sensor_data[SENSOR_DATA_SIZE] = {0}; // Mock sensor data
static uint8_t config_data[100] = {0}; // Received config data

static struct bt_conn *current_conn = NULL;
static struct bt_gatt_notify_params notify_params;

/* BLE Advertising Data */
static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),  
    BT_DATA_BYTES(BT_DATA_UUID16_ALL, BT_UUID_16_ENCODE(0x180F))  // Ensure this is included
};


/* BLE Advertising Parameters */
static struct bt_le_adv_param adv_params = {
    .id = BT_ID_DEFAULT,
    .sid = 0,
    .secondary_max_skip = 0,
    .options = BT_LE_ADV_OPT_USE_NAME,
    .interval_min = BT_GAP_ADV_FAST_INT_MIN_2,
    .interval_max = BT_GAP_ADV_FAST_INT_MAX_2,
    .peer = NULL
};

/* Callback when device is connected */
static void connected(struct bt_conn *conn, uint8_t err) {
    if (err) {
        printk("[Node] Connection failed (err %d)\n", err);
        return;
    }
    printk("[Node] Connected to Gateway!\n");
    current_conn = conn;
}

/* Callback when device is disconnected */
static void disconnected(struct bt_conn *conn, uint8_t reason) {
    printk("[Node] Disconnected (reason %d)\n", reason);
    current_conn = NULL;
    
    // Restart advertising
    bt_le_adv_start(&adv_params, ad, ARRAY_SIZE(ad), NULL, 0);
}

/* BLE Callbacks */
static struct bt_conn_cb conn_callbacks = {
    .connected = connected,
    .disconnected = disconnected,
};



/* Callback for Configuration Write */
static ssize_t write_config(struct bt_conn *conn, const struct bt_gatt_attr *attr, 
                            const void *buf, uint16_t len, uint16_t offset, uint8_t flags) {
    memcpy(config_data, buf, len);
    printk("[Node] Configuration Updated (Len: %d)\n", len);
    return len;
}

/* BLE GATT Service and Characteristics */
BT_GATT_SERVICE_DEFINE(node_service,
    BT_GATT_PRIMARY_SERVICE(BT_UUID_DECLARE_16(0x180F)), // Battery Service UUID (Example)

    /* Notify Characteristic (Sensor Data) */
    BT_GATT_CHARACTERISTIC(BT_UUID_DECLARE_16(0x2A19),
        BT_GATT_CHRC_NOTIFY,
        BT_GATT_PERM_NONE,
        NULL, NULL, NULL),

    /* Write Characteristic (Configuration Updates) */
    BT_GATT_CHARACTERISTIC(BT_UUID_DECLARE_16(0x2A1A),
        BT_GATT_CHRC_WRITE,
        BT_GATT_PERM_WRITE,
        NULL, write_config, config_data)
);

/* Sensor Data Notification */
static void send_sensor_data(void) {
    if (!current_conn) {
        printk("[Node] No active connection. Skipping data send.\n");
        return;
    }

    // Fill sensor data with mock values
    for (int i = 0; i < SENSOR_DATA_SIZE; i++) {
        sensor_data[i] = i;
    }

    notify_params.attr = &node_service.attrs[1]; // Point to Notify Characteristic
    notify_params.data = sensor_data;
    notify_params.len = SENSOR_DATA_SIZE;
    notify_params.func = NULL;
    notify_params.user_data = NULL;

    int err = bt_gatt_notify_cb(current_conn, &notify_params);
    if (err) {
        printk("[Node] Failed to send sensor data (err %d)\n", err);
    } else {
        printk("[Node] Sensor data sent!\n");
    }
}

/* Function to initialize BLE */
static void init_ble(void) {
    int err;

    printk("[Node] Initializing BLE...\n");

    err = bt_enable(NULL);
    if (err) {
        printk("[Node] Bluetooth initialization failed (err %d)\n", err);
        return;
    }
    printk("[Node] Bluetooth initialized\n");

    bt_conn_cb_register(&conn_callbacks);

    /* Start advertising */
    err = bt_le_adv_start(&adv_params, ad, ARRAY_SIZE(ad), NULL, 0);
    if (err) {
        printk("[Node] Advertising failed to start (err %d)\n", err);
        return;
    }
    printk("[Node] Advertising started\n");
}

/* Node BLE Thread */
void node_ble_thread(void) {
    init_ble();

    while (1) {
        send_sensor_data();
        k_sleep(K_SECONDS(10));
    }
}

K_THREAD_DEFINE(node_ble_thread_id, STACKSIZE, node_ble_thread, NULL, NULL, NULL, PRIORITY, 0, 0);
