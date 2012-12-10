get_filename_component(FLASH_SOURCE_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)

set(FLASH_SRCS
    ${FLASH_SOURCE_DIR}/flash.h ${FLASH_SOURCE_DIR}/flash.c
    ${FLASH_SOURCE_DIR}/ihex.h ${FLASH_SOURCE_DIR}/ihex.c
    ${FLASH_SOURCE_DIR}/helper.h ${FLASH_SOURCE_DIR}/helper.c
)

