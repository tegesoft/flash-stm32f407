project(flash-stm32f407-test)

include(${CHIBIOS_ROOT}/tools/cmake/chibios.cmake)
include(${CHIBIOS_ROOT}/tools/cmake/ext/chprintf.cmake)
include(${CHIBIOS_ROOT}/tools/cmake/ext/fatfs.cmake)

include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

include("../flash/flash.cmake")

set(TEST_SRCS
    ${FLASH_SRCS}
    chconf.h
    halconf.h
    mcuconf.h
    ffconf.h
    test.c
    sdcard.h
    sdcard.c
)

add_chibios_executable(flash-stm32f407-test ${TEST_SRCS})
add_flash_target(flash-stm32f407-test)
