
#ifndef SDCARD_H
#define SDCARD_H

/**
 * @brief Init the correct SD card driver (MMC or SDC) and mount the filesystem
 * @return BOOTLOADER_SUCCESS on success
 * @return BOOTLOADER_ERROR_NOCARD if no SD card is inserted
 * @return BOOTLOADER_ERROR_BADFS if the filesystem could not be mounted
 */
int flashMountSDCard();

#endif /* SDCARD_H */
