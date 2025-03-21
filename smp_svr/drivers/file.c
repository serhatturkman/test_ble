#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/fs/fs.h>
#include "file.h"

LOG_MODULE_REGISTER(file, CONFIG_LOG_DEFAULT_LEVEL);

int create_file(const char *dir_path, const char *file_name, const void *data, size_t size)
{
    char full_path[MAX_PATH_LEN];
    struct fs_file_t file;
    int rc;

    if (!dir_path || !file_name || !data || !size) {
        LOG_ERR("Invalid parameters");
        return -EINVAL;
    }

    if (strlen(dir_path) + strlen(file_name) + 2 > MAX_PATH_LEN) {
        LOG_ERR("Path too long");
        return -ENAMETOOLONG;
    }

    // Create directory if it doesn't exist
    rc = fs_mkdir(dir_path);
    if (rc < 0 && rc != -EEXIST) {
        LOG_ERR("Failed to create directory %s: %d", dir_path, rc);
        return rc;
    }

    // Construct full path safely
    rc = snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, file_name);
    if (rc < 0 || rc >= sizeof(full_path)) {
        LOG_ERR("Path construction failed");
        return -ENAMETOOLONG;
    }

    // Initialize and open file
    fs_file_t_init(&file);
    rc = fs_open(&file, full_path, FS_O_CREATE | FS_O_RDWR | FS_O_TRUNC);
    if (rc < 0) {
        LOG_ERR("Failed to open %s: %d", full_path, rc);
        return rc;
    }

    // Write data
    rc = fs_write(&file, data, size);
    if (rc < 0) {
        LOG_ERR("Failed to write to %s: %d", full_path, rc);
        fs_close(&file);
        return rc;
    }

    if (rc != size) {
        LOG_ERR("Incomplete write to %s: %d/%zu", full_path, rc, size);
        fs_close(&file);
        return -EIO;
    }

    rc = fs_close(&file);
    if (rc < 0) {
        LOG_ERR("Failed to close %s: %d", full_path, rc);
        return rc;
    }

    LOG_INF("Successfully created file: %s", full_path);
    return 0;
}

int test_create_text_file(void)
{
    static const char test_string[] = "caoi bella aq";
    static const char *dir_path = "/lfs1/data/text";
    
    int ret = create_file(dir_path, "test_string.txt", test_string, sizeof(test_string) - 1);
    if (ret < 0) {
        LOG_ERR("Failed to create text file in %s [%d]", dir_path, ret);
        return ret;
    }
    
    LOG_INF("Text file created successfully in %s", dir_path);
    return 0;
}

int test_create_binary_file(void)
{
    static const uint16_t measurements[] = {0x1234, 0x5678, 0x9ABC, 0xDEF0};
    static const char *dir_path = "/lfs1/data/binary";
    
    int ret = create_file(dir_path, "measurements.bin", measurements, sizeof(measurements));
    if (ret < 0) {
        LOG_ERR("Failed to create binary file in %s [%d]", dir_path, ret);
        return ret;
    }
    
    LOG_INF("Binary file created successfully in %s", dir_path);
    return 0;
}