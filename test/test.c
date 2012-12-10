
#include "sdcard.h"
#include <flash/flash.h>
#include <flash/helper.h>
#include <ch.h>
#include <hal.h>
#include <chprintf.h>
#include <string.h>

#define TEST_FIRST_FREE_SECTOR 5

#define COLORIZE_GREEN(message) "\033[1;32m" message "\033[0m"
#define COLORIZE_RED(message) "\033[1;31m" message "\033[0m"

#define TEST_VERIFY(message, statement) \
do { \
    chprintf(chStdout, "%s - " message "\r\n", \
        statement ? COLORIZE_GREEN("PASS") : COLORIZE_RED("FAIL")); \
} while (0)

static BaseSequentialStream* chStdout = (BaseSequentialStream*)&SD6;

/**
 * @brief The flash_sector_def struct describes a flash sector
 */
struct flash_sector_def
{
    flashsector_t index; ///< Index of the sector in the flash memory
    size_t size; ///< Size of the sector in bytes
    flashaddr_t beginAddress; ///< First address of the sector in memory (inclusive)
    flashaddr_t endAddress; ///< Last address of the sector in memory (exclusive)
};

/**
 * @brief Definition of the stm32f407 sectors.
 */
const struct flash_sector_def f407_flash[FLASH_SECTOR_COUNT] =
{
    {  0,  16 * 1024, 0x08000000, 0x08004000},
    {  1,  16 * 1024, 0x08004000, 0x08008000},
    {  2,  16 * 1024, 0x08008000, 0x0800C000},
    {  3,  16 * 1024, 0x0800C000, 0x08010000},
    {  4,  64 * 1024, 0x08010000, 0x08020000},
    {  5, 128 * 1024, 0x08020000, 0x08040000},
    {  6, 128 * 1024, 0x08040000, 0x08060000},
    {  7, 128 * 1024, 0x08060000, 0x08080000},
    {  8, 128 * 1024, 0x08080000, 0x080A0000},
    {  9, 128 * 1024, 0x080A0000, 0x080C0000},
    { 10, 128 * 1024, 0x080C0000, 0x080E0000},
    { 11, 128 * 1024, 0x080E0000, 0x08100000}
};

#define PRINTABLE_CHARACTER_FIRST 0x20
#define PRINTABLE_CHARACTER_LAST 0x7e
#define PRINTABLE_CHARACTER_COUNT (PRINTABLE_CHARACTER_LAST - PRINTABLE_CHARACTER_FIRST + 1)

/**
 * @brief Fill @p buffer with a pattern
 * @param buffer Buffer to be filled
 * @param size Size of @p buffer
 */
static void fillWithPattern(char* buffer, size_t size)
{
    /* Generate a pattern with printable characters to ease debugging) */
    int c;
    for (c = 0; c < size; ++c)
        buffer[c] = (c % PRINTABLE_CHARACTER_COUNT) + PRINTABLE_CHARACTER_FIRST;
}

/**
 * @brief Tests related to the flash sectors
 */
static void testFlashSector()
{
    flashsector_t i;
    for (i = 0; i < FLASH_SECTOR_COUNT; ++i)
    {
        const struct flash_sector_def* sector = &f407_flash[i];

        chprintf(chStdout, "\r\n> test sector %u: 0x%x -> 0x%x (%u bytes)\r\n\r\n",
                 sector->index,
                 sector->beginAddress,
                 sector->endAddress,
                 sector->size);

        TEST_VERIFY("sector index", i == sector->index);
        TEST_VERIFY("sector size", flashSectorSize(i) == sector->size);
        TEST_VERIFY("sector begin address", flashSectorBegin(i) == sector->beginAddress);
        TEST_VERIFY("sector end address", flashSectorEnd(i) == sector->endAddress);
        TEST_VERIFY("sector lookup (begin address)", flashSectorAt(sector->beginAddress) == i);
        TEST_VERIFY("sector lookup (end address)", flashSectorAt(sector->endAddress - 1) == i);
        TEST_VERIFY("sector lookup (middle address)", flashSectorAt(sector->beginAddress + sector->size / 2) == i);
        if (i != 0)
            TEST_VERIFY("sector lookup (pre-begin address)", flashSectorAt(sector->beginAddress - 1) == i - 1);
        if (i != FLASH_SECTOR_COUNT - 1)
            TEST_VERIFY("sector lookup (post-end address)", flashSectorAt(sector->endAddress) == i + 1);
    }
}

/**
 * @brief The test_flashreadwrite_data struct defines a test case data for the read/write test.
 */
struct test_flashreadwrite_data
{
    char* testName; ///< Name of the test case
    flashaddr_t from; ///< First address to be read/write (inclusive)
    flashaddr_t to; ///< Last address to be read/write (exclusive)
};

#define TEST_BASE 0x08060000
#define TEST_DATASIZE sizeof(flashdata_t)
#define TEST_READWRITE_TESTCASE_COUNT 8

/**
 * @brief Test cases for the read/write test.
 *
 * "big data" tests read/write more than sizeof(flashdata_t) bytes while
 * "tiny data" tests read/write sizeof(flashdata_t) bytes at most.
 */
struct test_flashreadwrite_data flashreadwrite_testcases[TEST_READWRITE_TESTCASE_COUNT] =
{
    { "aligned start, aligned end, big data",          TEST_BASE +  1 * TEST_DATASIZE + 0, TEST_BASE +  4 * TEST_DATASIZE + 0 },
    { "aligned start, not aligned end, big data",      TEST_BASE +  4 * TEST_DATASIZE + 0, TEST_BASE +  6 * TEST_DATASIZE + 1 },
    { "not aligned start, not aligned end, big data",  TEST_BASE +  6 * TEST_DATASIZE + 1, TEST_BASE +  9 * TEST_DATASIZE - 1 },
    { "not aligned start, aligned end, big data",      TEST_BASE +  9 * TEST_DATASIZE - 1, TEST_BASE + 11 * TEST_DATASIZE + 0 },
    { "aligned start, aligned end, tiny data",         TEST_BASE + 11 * TEST_DATASIZE + 0, TEST_BASE + 12 * TEST_DATASIZE + 0 },
    { "aligned start, not aligned end, tiny data",     TEST_BASE + 12 * TEST_DATASIZE + 0, TEST_BASE + 12 * TEST_DATASIZE + 1 },
    { "not aligned start, not aligned end, tiny data", TEST_BASE + 12 * TEST_DATASIZE + 1, TEST_BASE + 13 * TEST_DATASIZE - 1 },
    { "not aligned start, aligned end, tiny data",     TEST_BASE + 13 * TEST_DATASIZE - 1, TEST_BASE + 13 * TEST_DATASIZE + 0 }
};

/**
 * @brief Execute the read/write test cases.
 */
static void testFlashReadWrite()
{
    /* Erase all sectors needed by the test cases */
    flashsector_t sector;
    for (sector = TEST_FIRST_FREE_SECTOR; sector < FLASH_SECTOR_COUNT; ++sector)
        flashSectorErase(sector);

    /* Execute the test cases */
    int i;
    for (i = 0; i < TEST_READWRITE_TESTCASE_COUNT; ++i)
    {
        const struct test_flashreadwrite_data* testcase = &flashreadwrite_testcases[i];

        chprintf(chStdout, "\r\n> test flash read/write: %s\r\n\r\n", testcase->testName);

        char writeBuffer[testcase->to - testcase->from];
        fillWithPattern(writeBuffer, testcase->to - testcase->from);
        char readBufferBefore[32];
        char readBufferAfter[32];
        TEST_VERIFY("read previous data (before target)", flashRead(testcase->from - sizeof(readBufferBefore), readBufferBefore, sizeof(readBufferBefore)) == FLASH_RETURN_SUCCESS);
        TEST_VERIFY("read previous data (after target)", flashRead(testcase->to, readBufferAfter, sizeof(readBufferAfter)) == FLASH_RETURN_SUCCESS);

        TEST_VERIFY("write data to flash", flashWrite(testcase->from, writeBuffer, testcase->to - testcase->from) == FLASH_RETURN_SUCCESS);
        TEST_VERIFY("previous data not overwritten (before target)", flashCompare(testcase->from - sizeof(readBufferBefore), readBufferBefore, sizeof(readBufferBefore)) == TRUE);
        TEST_VERIFY("previous data not overwritten (after target)", flashCompare(testcase->to, readBufferAfter, sizeof(readBufferAfter)) == TRUE);
        TEST_VERIFY("compare written data", flashCompare(testcase->from, writeBuffer, testcase->to - testcase->from) == TRUE);

        char readBuffer[testcase->to - testcase->from];
        memset(readBuffer, '#', testcase->to - testcase->from);
        TEST_VERIFY("read back written data", flashRead(testcase->from, readBuffer, testcase->to - testcase->from) == FLASH_RETURN_SUCCESS);
        TEST_VERIFY("compare read and write data", memcmp(readBuffer, writeBuffer, testcase->to - testcase->from) == 0);
    }
}

/**
 * @brief The test_pattern_def struct defines what patterns are expected
 * where in the iHex flash to be flashed for the iHex flashing test.
 *
 * To be noted that the pattern's data is used as a circular buffer.
 * Thus, if size < (to - from), data we will be reread from the beginning (that is, the iHex pattern can be data repeated several times).
 */
struct test_pattern_def
{
    flashaddr_t from; ///< First address of the pattern (inclusive)
    flashaddr_t to; ///< Last address of the pattern (exclusive)
    char* data; ///< Pattern's data
    size_t size; ///< Size of the pattern's data
};

/**
 * @brief Pattern definitions
 */
#define TEST_IHEX_FILE "test-f407.ihex"
#define TEST_IHEX_PATTERN_COUNT 3
#define TEST_IHEX_PATTERN_1 "unit test: chunk 1 pattern"
#define TEST_IHEX_PATTERN_2 "unit test: chunk 2 pattern"
#define TEST_IHEX_PATTERN_3 "unit test: chunk 3 pattern"

const struct test_pattern_def iHexPatterns[TEST_IHEX_PATTERN_COUNT] =
{
    { 0x08030460, 0x08030500, TEST_IHEX_PATTERN_1, sizeof(TEST_IHEX_PATTERN_1) - 1 },
    { 0x0803FF83, 0x08040011, TEST_IHEX_PATTERN_2, sizeof(TEST_IHEX_PATTERN_2) - 1 },
    { 0x08040500, 0x08040507, TEST_IHEX_PATTERN_3, sizeof(TEST_IHEX_PATTERN_3) - 1 }
};

/**
 * @brief Check that the sector is empty before the given address
 * @param address First address not checked
 * @return TRUE if erased, FALSE otherwise
 */
static bool_t checkSectorEmptyBefore(flashaddr_t address)
{
    flashaddr_t sectorBegin = flashSectorBegin(flashSectorAt(address));
    return flashIsErased(sectorBegin, address - sectorBegin);
}

/**
 * @brief Check the the sector is empty after the given address
 * @param address Last address not checked
 * @return TRUE if erased, FALSE otherwise
 */
static bool_t checkSectorEmptyAfter(flashaddr_t address)
{
    ++address;
    flashaddr_t sectorEnd = flashSectorEnd(flashSectorAt(address));
    return flashIsErased(address, sectorEnd - address);
}

/**
 * @brief Check that the pattern's data have be written correctly
 * @param pattern Pattern to be checked
 * @return TRUE if successfully written, FALSE otherwise
 */
static bool_t checkWrittenPattern(const struct test_pattern_def* pattern)
{
    flashaddr_t address = pattern->from;
    while (address < pattern->to)
    {
        size_t count = (size_t)(pattern->to - address);
        if (count > pattern->size)
            count = pattern->size;
        if (flashCompare(address, pattern->data, count) == FALSE)
            return FALSE;

        address += count;
    }

    return TRUE;
}

/**
 * @brief Check that the data between two patterns are erased
 * @param first First pattern (lower bound)
 * @param second Second pattern (upper bound)
 * @return TRUE if the data in between are erased, FALSE otherwise
 */
static bool_t checkEmptyBetweenPatterns(const struct test_pattern_def* first, const struct test_pattern_def* second)
{
    return flashIsErased(first->to, second->from - first->to);
}

/**
 * @brief Test case flashing a iHex file read from the SD card
 */
static void testIHex()
{
    chprintf(chStdout, "\r\n > test iHex flashing: " TEST_IHEX_FILE "\r\n\r\n");

    TEST_VERIFY("mount sd card", flashMountSDCard() == BOOTLOADER_SUCCESS);

    FIL firmwareFile;
    TEST_VERIFY("open iHex file", f_open(&firmwareFile, TEST_IHEX_FILE, FA_READ) == FR_OK);

    TEST_VERIFY("flash iHex file", flashIHexFile(&firmwareFile) == BOOTLOADER_SUCCESS);
    f_close(&firmwareFile);

    /* Written patterns check */
    int patternIndex;
    for (patternIndex = 0; patternIndex < TEST_IHEX_PATTERN_COUNT; ++patternIndex)
    {
        TEST_VERIFY("pattern correctly written",
                    checkWrittenPattern(&iHexPatterns[patternIndex]) == TRUE);
    }

    /* Erased data check */
    TEST_VERIFY("no data written before first pattern", checkSectorEmptyBefore(iHexPatterns[0].from) == TRUE);
    for (patternIndex = 0; patternIndex < TEST_IHEX_PATTERN_COUNT - 1; ++patternIndex)
    {
        TEST_VERIFY("no data written between two patterns",
                    checkEmptyBetweenPatterns(
                        &iHexPatterns[patternIndex],
                        &iHexPatterns[patternIndex + 1]) == TRUE);
    }
    TEST_VERIFY("no data written after last pattern", checkSectorEmptyAfter(iHexPatterns[TEST_IHEX_PATTERN_COUNT - 1].to - 1) == TRUE);
}

int main()
{
    /*
     * System initializations.
     * - HAL initialization, this also initializes the configured device drivers
     *   and performs the board-specific initializations.
     * - Kernel initialization, the main() function becomes a thread and the
     *   RTOS is active.
     */
    halInit();
    chSysInit();

    /* Activates the serial driver 3 using the driver default configuration */
    sdStart((SerialDriver*)chStdout, NULL);

    /* Start the tests */
    chprintf(chStdout, "\r\n=== test start ===\r\n\r\n");
    testFlashSector();
    testFlashReadWrite();
    testIHex();
    chprintf(chStdout, "\r\n=== test end ===\r\n\r\n");

    return 0;
}
