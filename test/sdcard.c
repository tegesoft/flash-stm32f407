
#include "sdcard.h"
#include <flash/helper.h>
#include <ch.h>
#include <hal.h>

/* SD card related */

/** @brief FS object */
static FATFS fatfs;

#if HAL_USE_MMC_SPI

static MMCDriver MMCD1;

/* SPI_CR1_BR:
 * 000 = STM32_PCLK1 / 2    100 = STM32_PCLK1 / 32
 * 001 = STM32_PCLK1 / 4    101 = STM32_PCLK1 / 64
 * 010 = STM32_PCLK1 / 8    110 = STM32_PCLK1 / 128
 * 011 = STM32_PCLK1 / 16   111 = STM32_PCLK1 / 256
 */

/* Maximum speed SPI configuration (42MHz/2=21MHz, CPHA=0, CPOL=0, MSb first).*/
static SPIConfig hs_spicfg = {NULL, GPIOD, GPIOD_SPI3_CS, 0};

/* Low speed SPI configuration (42MHz/128=328kHz, CPHA=0, CPOL=0, MSb first).*/
static SPIConfig ls_spicfg = {NULL, GPIOD, GPIOD_SPI3_CS,
                              SPI_CR1_BR_2 | SPI_CR1_BR_1};

/* MMC/SD over SPI driver configuration.*/
static MMCConfig mmccfg = {&SPID3, &ls_spicfg, &hs_spicfg};

#endif

int flashMountSDCard()
{
    /* Initializes the SDC or MMC driver */
#if HAL_USE_SDC
    sdcStart(&SDCD1, NULL);
#elif HAL_USE_MMC_SPI
    mmcObjectInit(&MMCD1);
    mmcStart(&MMCD1, &mmccfg);
#endif

    /* Wait for card */
#if HAL_USE_SDC
    if (!blkIsInserted(&SDCD1) || sdcConnect(&SDCD1) == CH_FAILED)
        return BOOTLOADER_ERROR_NOCARD;
    if (f_mount(0, &fatfs) != FR_OK)
    {
        sdcDisconnect(&SDCD1);
        return BOOTLOADER_ERROR_BADFS;
    }
#elif HAL_USE_MMC_SPI
    if (!blkIsInserted(&MMCD1) || mmcConnect(&MMCD1) == CH_FAILED)
        return BOOTLOADER_ERROR_NOCARD;
    if (f_mount(0, &fatfs) != FR_OK)
    {
        mmcDisconnect(&MMCD1);
        return BOOTLOADER_ERROR_BADFS;
    }
#endif

    return BOOTLOADER_SUCCESS;
}
