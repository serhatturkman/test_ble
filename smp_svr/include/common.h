#pragma once

#include <zephyr/kernel.h>
#include <zephyr/device.h>


#define LOG_LEVEL LOG_LEVEL_DBG
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(smp_bt_sample);

void start_smp_bluetooth_adverts(void);

//End of common.h
