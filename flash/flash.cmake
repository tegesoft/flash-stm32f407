get_filename_component(FLASH_SOURCE_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)

# add an option to enable or disable IHEX specific functions
option(FLASH_USE_IHEX "TRUE to enable the ihex functions, FALSE to exclude them from the build" FALSE)

set(FLASH_SRCS
    ${FLASH_SOURCE_DIR}/flash.h ${FLASH_SOURCE_DIR}/flash.c
    ${FLASH_SOURCE_DIR}/helper.h ${FLASH_SOURCE_DIR}/helper.c
)

if(FLASH_USE_IHEX)
    set(FLASH_SRCS ${FLASH_SRCS}
        ${FLASH_SOURCE_DIR}/ihex.h ${FLASH_SOURCE_DIR}/ihex.c
        ${FLASH_SOURCE_DIR}/ihexhelper.h ${FLASH_SOURCE_DIR}/ihexhelper.c
    )
endif()
