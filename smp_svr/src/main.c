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

/* size of stack area used by each thread */
#define STACKSIZE 1024

/* scheduling priority used by main thread */
#define MAIN_THREAD_PRIORITY 1

/* scheduling priority used by other threads */
#define OTHER_THREAD_PRIORITY 1

K_FIFO_DEFINE(printk_fifo);

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

static int create_file(char *file_name, const void *measurement_value, int size)
{
	struct fs_file_t file;
	int rc, ret;
	fs_file_t_init(&file);

	rc = fs_open(&file, file_name, FS_O_CREATE | FS_O_RDWR | FS_O_TRUNC);
	if (rc < 0)
	{
		LOG_ERR("FAIL: open %s: %d", file_name, rc);
		return rc;
	}
	rc = fs_write(&file, measurement_value, size);
	if (rc < 0)
	{
		LOG_ERR("FAIL: write %s: %d", file_name, rc);
		return rc;
	}
	ret = fs_close(&file);
	if (ret < 0)
	{
		LOG_ERR("FAIL: close %s: %d", file_name, ret);
		return ret;
	}
	return 0;
}

int test_create_text_file(void)
{
	char test_string[] = "caoi bella aq";
	int ret = create_file("/lfs1/test_string.txt", test_string, sizeof(test_string));
	if (ret < 0)
	{
		LOG_ERR("Failed to create test string file [%d]", ret);
	}
	else
	{
		LOG_INF("Test String file created successfully");
	}
	return ret;
}

int test_create_binary_file(void)
{
	uint16_t test_16_bit_measurement[] = {0x1234, 0x5678, 0x9ABC, 0xDEF0};
	int ret = create_file("/lfs1/test_16_bit.bin", test_16_bit_measurement, sizeof(test_16_bit_measurement));
	if (ret < 0)
	{
		LOG_ERR("Failed to create 16-bit measurement file [%d]", ret);
	}
	else
	{
		LOG_INF("Test 16-bit Measurement file created successfully");
	}
	return ret;
}

int main(void)
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
	/* The system work queue handles all incoming mcumgr requests.  Let the
	 * main thread idle while the mcumgr server runs.
	 */
	while (1)
	{
		k_sleep(K_MSEC(50000));
		STATS_INC(smp_svr_stats, ticks);
	}
	return 0;
}

K_THREAD_DEFINE(pdm_thread_id, STACKSIZE, pdm_test, NULL, NULL, NULL, OTHER_THREAD_PRIORITY, 0, 0);
