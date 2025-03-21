#ifndef FILE_H
#define FILE_H

#include <zephyr/kernel.h>
#include <zephyr/fs/fs.h>

#define MAX_PATH_LEN 255
#define MAX_FILENAME_LEN 64

/**
 * @brief Create a file in the specified directory with given content
 *
 * @param dir_path Directory path where the file will be created
 * @param file_name Name of the file to create
 * @param data Pointer to the data to write
 * @param size Size of the data in bytes
 * @return int 0 on success, negative errno on failure
 */
int create_file(const char *dir_path, const char *file_name, const void *data, size_t size);

/**
 * @brief Create a test text file in the specified directory
 *
 * @return int 0 on success, negative errno on failure
 */
int test_create_text_file(void);

/**
 * @brief Create a test binary file with measurements
 *
 * @return int 0 on success, negative errno on failure
 */
int test_create_binary_file(void);

#endif /* FILE_H */