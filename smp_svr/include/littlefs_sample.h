#pragma once

#include "common.h"


#include <zephyr/fs/fs.h>
#include <zephyr/fs/littlefs.h>
#include <zephyr/logging/log.h>
#include <zephyr/storage/flash_map.h>



int create_measurement(char *file_name, uint16_t *measurement_value, uint32_t size);

//End of littlefs_sample.h