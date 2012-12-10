cmake_minimum_required(VERSION 2.8)

project(flash-stm32f407)

set(CHIBIOS_ROOT "../ChibiOS")
get_filename_component(CHIBIOS_ROOT "${CHIBIOS_ROOT}" ABSOLUTE)

set(CHIBIOS_BOARD_CMAKE_FILE "${CHIBIOS_ROOT}/tools/cmake/boards/olimex-stm32-p407-sdio.cmake")

include(${CHIBIOS_ROOT}/tools/cmake/openocd.cmake)

add_subdirectory(test)
