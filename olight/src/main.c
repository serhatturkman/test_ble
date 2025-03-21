/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 * Copyright (c) 2020 Prevas A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/stats/stats.h>
#include <zephyr/usb/usb_device.h>

#include "pdm.h"
#include "pwm.h"
#include "file.h"

#ifdef CONFIG_MCUMGR_GRP_FS
#include <zephyr/device.h>
#include <zephyr/fs/fs.h>
#include <zephyr/fs/littlefs.h>
#endif
#ifdef CONFIG_MCUMGR_GRP_STAT
#include <zephyr/mgmt/mcumgr/grp/stat_mgmt/stat_mgmt.h>
#endif

#define LOG_LEVEL LOG_LEVEL_DBG
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(smp_sample);

#include <string.h>
#include "common.h"

#define MAINSTACKSIZE 2048

/* size of stack area used by each thread */
#define STACKSIZE 1024

/* scheduling priority used by main thread */
#define MAIN_THREAD_PRIORITY 1

/* scheduling priority used by other threads */
#define OTHER_THREAD_PRIORITY 1

K_FIFO_DEFINE(printk_fifo);
K_SEM_DEFINE(init_sem, 0, 1);  // Initial count 0, max count 1

#define STORAGE_PARTITION_LABEL storage_partition
#define STORAGE_PARTITION_ID FIXED_PARTITION_ID(STORAGE_PARTITION_LABEL)

/* Define an example stats group; approximates seconds since boot. */
STATS_SECT_START(smp_svr_stats)
STATS_SECT_ENTRY(ticks)
STATS_SECT_END;

/* Assign a name to the `ticks` stat. */
STATS_NAME_START(smp_svr_stats)
STATS_NAME(smp_svr_stats, ticks)
STATS_NAME_END(smp_svr_stats);

/* Define an instance of the stats group. */
STATS_SECT_DECL(smp_svr_stats)
smp_svr_stats;

#ifdef CONFIG_MCUMGR_GRP_FS
FS_LITTLEFS_DECLARE_DEFAULT_CONFIG(cstorage);
static struct fs_mount_t littlefs_mnt = {
	.type = FS_LITTLEFS,
	.fs_data = &cstorage,
	.storage_dev = (void *)STORAGE_PARTITION_ID,
	.mnt_point = "/lfs1"};
#endif


// Define states for the BLE cycle
enum ble_state
{
	BLE_ACTIVE,
	BLE_SLEEPING
};

// Shared state variable
static enum ble_state current_state = BLE_ACTIVE;

// Thread function for BLE cycle
static void ble_cycle_thread(void *arg1, void *arg2, void *arg3)
{
	// Wait for main_init to complete
	k_sem_take(&init_sem, K_FOREVER);

	while (1)
	{
		switch (current_state)
		{
		case BLE_ACTIVE:
			start_smp_bluetooth_adverts();
			k_sleep(K_MSEC(5000));
			current_state = BLE_SLEEPING;
			break;

		case BLE_SLEEPING:
			stop_smp_bluetooth_adverts();
			k_sleep(K_MSEC(50000));
			current_state = BLE_ACTIVE;
			break;
		}
		STATS_INC(smp_svr_stats, ticks);
	}
}

static int main_init(void)
{
	int rc = STATS_INIT_AND_REG(smp_svr_stats, STATS_SIZE_32,
								"smp_svr_stats");

	if (rc < 0)
	{
		LOG_ERR("Error initializing stats system [%d]", rc);
	}

	/* Register the built-in mcumgr command handlers. */
#ifdef CONFIG_MCUMGR_GRP_FS
	rc = fs_mount(&littlefs_mnt);
	if (rc < 0)
	{
		LOG_ERR("Error mounting littlefs [%d]", rc);
	}
#endif

#ifdef CONFIG_MCUMGR_TRANSPORT_BT
	start_smp_bluetooth_adverts();
#endif

	if (IS_ENABLED(CONFIG_USB_DEVICE_STACK))
	{
		rc = usb_enable(NULL);

		/* Ignore EALREADY error as USB CDC is likely already initialised */
		if (rc != 0 && rc != -EALREADY)
		{
			LOG_ERR("Failed to enable USB");
			return 0;
		}
	}
	/* using __TIME__ ensure that a new binary will be built on every
	 * compile which is convenient when testing firmware upgrade.
	 */
	LOG_INF("build time: " __DATE__ " " __TIME__);

	if (test_create_text_file() != 0)
	{
		return 0;
	}
	if (test_create_binary_file() != 0)
	{
		return 0;
	}

	return 1;
}

int main(void)
{
    if (main_init() != 0)  // Note: changed condition to != 0 for success
    {
        // Signal the BLE thread to start
        k_sem_give(&init_sem);
        
        /* The system work queue handles all incoming mcumgr requests.  Let the
         * main thread idle while the mcumgr server runs.
         */
        while (1) {
            k_sleep(K_FOREVER);
        }
    }
    return 0;
}
// Thread definition
K_THREAD_DEFINE(ble_cycle_thread_id,
				MAINSTACKSIZE,		// Stack size
				ble_cycle_thread,	// Thread function
				NULL, NULL, NULL,	// Args
				K_PRIO_PREEMPT(1),	// Priority
				0,					// Options
				0);

K_THREAD_DEFINE(pdm_thread_id, 
				STACKSIZE, 			
				pdm_test, 	
				NULL, NULL, NULL, 	
				OTHER_THREAD_PRIORITY, 	
				0, 	
				0);
