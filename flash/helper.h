
#ifndef HELPER_H
#define HELPER_H

#include <ff.h>
#include <stdint.h>

#define BOOTLOADER_SUCCESS          0
#define BOOTLOADER_ERROR_NOCARD     1
#define BOOTLOADER_ERROR_BADFS      2
#define BOOTLOADER_ERROR_NOFILE     3
#define BOOTLOADER_ERROR_BADHEX     4
#define BOOTLOADER_ERROR_BADFLASH   5

/**
 * @brief Flash the provided IHex @p file.
 * @param file IHex file describing the data to be flashed.
 * @return BOOTLOADER_SUCCESS on success
 * @return BOOTLOADER_ERROR_BADFLASH if an error occured when manipulating the flash memory
 * @return BOOTLOADER_ERROR_BADHEX if an error occured when reading the iHex file
 */
int flashIHexFile(FIL* file);

/**
 * @brief Jump to application located in flash.
 * @param address Address to jump to.
 *
 * @author Code stolen from "matis"
 * @link http://forum.chibios.org/phpbb/viewtopic.php?f=2&t=338
 */
void flashJumpApplication(uint32_t address);

#endif /* HELPER_H */
