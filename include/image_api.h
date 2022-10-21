#ifndef IMAGE_API_H 
#define IMAGE_API_H 

/**
 * @brief The image handler to be userd by the manager.
 */
typedef struct ImageHandler *ImageHandlerPtr;

/**
 * @brief Enum with possible return from interface functions.
 * Possible return values are:
 * - IMAGE_OPERATION_OK:                    Operation was successful.
 * - IMAGE_OPERATION_ERROR:                 Generic error.
 */
typedef enum
{
    IMAGE_OPERATION_OK = 0,
    IMAGE_OPERATION_ERROR
} ImageOperationResult;

/**
 * Create and initialize a new image handler.
 *
 * @param[out] handler a handler for the image manager.
 * @return COMMUNICATION_OPERATION_OK if success.
 * @return COMMUNICATION_OPERATION_ERROR otherwise.
 */
ImageOperationResult create_handler(
    ImageHandlerPtr *handler);

/**
 * Destroy a image handler.
 *
 * @param[in] handler a handler for the image manager.
 * @return COMMUNICATION_OPERATION_OK if success.
 * @return COMMUNICATION_OPERATION_ERROR otherwise.
 */
ImageOperationResult destroy_handler(
    ImageHandlerPtr *handler);

/**
 * Import image to local directory.
 *
 * @param[in] handler a handler for the image manager.
 * @param[in] path image path to be imported.
 * @param[out] part_number part number of the imported image.
 * @return IMAGE_OPERATION_OK if success.
 * @return IMAGE_OPERATION_ERROR otherwise.
 */
ImageOperationResult import_image (
    ImageHandlerPtr handler,
    const char* path,
    char** part_number
    );

/**
 * Remove image from local directory.
 * 
 * @param[in] handler a handler for the image manager.
 * @param[in] part_number part number of the image to be removed.
 * @return IMAGE_OPERATION_OK if success.
 * @return IMAGE_OPERATION_ERROR otherwise.
 */
ImageOperationResult remove_image (
    ImageHandlerPtr handler,
    const char* part_number
    );

/**
 * Get part number of all imported images.
 * 
 * @param[in] handler a handler for the image manager.
 * @param[out] part_numbers list of imported part numbers. Memory will be
 * allocated and must be freed by the caller.
 * @param[out] list_size number of entries in the list.
 * @return IMAGE_OPERATION_OK if success.
 * @return IMAGE_OPERATION_ERROR otherwise.
 */
ImageOperationResult get_images (
    ImageHandlerPtr handler,
    char** part_numbers[],
    int *list_size
    );

/**
 * Get path of image with given part number.
 * 
 * @param[in] handler a handler for the image manager.
 * @param[in] part_number part number of the image.
 * @param[out] path path of the image.
 * @return IMAGE_OPERATION_OK if success.
 * @return IMAGE_OPERATION_ERROR otherwise.
 */
ImageOperationResult get_image_path (
    ImageHandlerPtr handler,
    const char* part_number,
    char** path
    );

/**
 * Get compatibility file path.
 * 
 * @param[in] handler a handler for the image manager.
 * @param[in] part_numbers list of part numbers to get compatibility file.
 * @param[in] list_size size of the list of part numbers.
 * @param[out] path path to the compatibility file.
 * @return IMAGE_OPERATION_OK if success.
 * @return IMAGE_OPERATION_ERROR otherwise.
 */
ImageOperationResult get_compatibility_path (
    ImageHandlerPtr handler,
    const char** part_numbers,
    int list_size,
    char** path
    );

#endif // IMAGE_API_H 