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
static uint8_t config_data[100] = {0};              // Received config data

static struct bt_conn *current_conn = NULL;
static struct bt_gatt_notify_params notify_params;

static void adv_sent_cb(struct bt_le_ext_adv *adv,
			struct bt_le_ext_adv_sent_info *info);

static struct bt_le_ext_adv_cb adv_callbacks = {
	.sent = adv_sent_cb,
};

static const struct bt_data ad[] = {
    BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

static struct bt_le_adv_param param =
    BT_LE_ADV_PARAM_INIT(BT_LE_ADV_OPT_EXT_ADV |
                             BT_LE_ADV_OPT_USE_IDENTITY,
                         BT_GAP_ADV_FAST_INT_MIN_2,
                         BT_GAP_ADV_FAST_INT_MAX_2,
                         NULL);

static struct bt_le_per_adv_param per_adv_param = {
    .interval_min = BT_GAP_PER_ADV_SLOW_INT_MIN,
    .interval_max = BT_GAP_PER_ADV_SLOW_INT_MAX,
    .options = BT_LE_ADV_OPT_USE_TX_POWER,
};

static struct bt_le_ext_adv *adv_set;

static void adv_sent_cb(struct bt_le_ext_adv *adv,
			struct bt_le_ext_adv_sent_info *info)
{
	printk("Advertiser[%d] %p sent %d\n", bt_le_ext_adv_get_index(adv),
	       adv, info->num_sent);
}

/* Callback when device is connected */
static void connected(struct bt_conn *conn, uint8_t err)
{
    if (err)
    {
        printk("[Node] Connection failed (err %d)\n", err);
        return;
    }
    printk("[Node] Connected to Gateway!\n");
    current_conn = conn;
}

/* Callback when device is disconnected */
static void disconnected(struct bt_conn *conn, uint8_t reason)
{
    printk("[Node] Disconnected (reason %d)\n", reason);
    current_conn = NULL;

    // Restart advertising
    bt_le_adv_start(&param, ad, ARRAY_SIZE(ad), NULL, 0);
}

static struct bt_le_ext_adv_start_param ext_adv_start_param = {
	.timeout = 0,
	.num_events = 0,
};

/* BLE Callbacks */
static struct bt_conn_cb conn_callbacks = {
    .connected = connected,
    .disconnected = disconnected,
};

/* Callback for Configuration Write */
static ssize_t write_config(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                            const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
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
                                              NULL, write_config, config_data));

/* Sensor Data Notification */
static void send_sensor_data(void)
{
    if (!current_conn)
    {
        printk("[Node] No active connection. Skipping data send.\n");
        return;
    }

    // Fill sensor data with mock values
    for (int i = 0; i < SENSOR_DATA_SIZE; i++)
    {
        sensor_data[i] = i;
    }

    notify_params.attr = &node_service.attrs[1]; // Point to Notify Characteristic
    notify_params.data = sensor_data;
    notify_params.len = SENSOR_DATA_SIZE;
    notify_params.func = NULL;
    notify_params.user_data = NULL;

    int err = bt_gatt_notify_cb(current_conn, &notify_params);
    if (err)
    {
        printk("[Node] Failed to send sensor data (err %d)\n", err);
    }
    else
    {
        printk("[Node] Sensor data sent!\n");
    }
}

/* Function to initialize BLE */
static void init_ble(void)
{
    char addr_s[BT_ADDR_LE_STR_LEN];
    int err;

    printk("[Node] Initializing BLE...\n");

    err = bt_enable(NULL);
    if (err)
    {
        printk("[Node] Bluetooth initialization failed (err %d)\n", err);
        return;
    }
    printk("[Node] Bluetooth initialized\n");

    printk("Advertising set create...");
	err = bt_le_ext_adv_create(&param, &adv_callbacks, &adv_set);
	if (err) {
		printk("failed (err %d)\n", err);
		return;
	}
	printk("success\n");

	err = bt_le_ext_adv_set_data(adv_set, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err) {
		printk("failed (err %d)\n", err);
		return;
	}

	printk("Periodic advertising params set...");
	err = bt_le_per_adv_set_param(adv_set, &per_adv_param);
	if (err) {
		printk("failed (err %d)\n", err);
		return;
	}
	printk("success\n");

    printk("Extended advertising enable...");
	err = bt_le_ext_adv_start(adv_set, &ext_adv_start_param);
	if (err) {
		printk("failed (err %d)\n", err);
		return;
	}
	printk("success\n");
}

/* Node BLE Thread */
void node_ble_thread(void)
{
    init_ble();

    while (1)
    {
        send_sensor_data();
        k_sleep(K_SECONDS(10));
    }
}

K_THREAD_DEFINE(node_ble_thread_id, STACKSIZE, node_ble_thread, NULL, NULL, NULL, PRIORITY, 0, 0);
