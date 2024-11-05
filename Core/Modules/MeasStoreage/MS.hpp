/**
 * @file MS.hpp
 * @brief Interface for handling measurement storage in EEPROM.
 *
 * This file contains the implementation for storing, reading, and managing
 * measurement entries in an EEPROM device using I2C communication.
 *
 * @details The MeasurementStorage class provides methods to handle the storage of
 * measurement entries, including adding new entries and reading stored entries.
 * It also includes error handling for EEPROM-related issues such as overflows and communication errors.
 *
 * @author Sásdi András
 * @date September 21, 2024
 */


#ifndef MODULES_MEASSTOREAGE_MS_HPP_
#define MODULES_MEASSTOREAGE_MS_HPP_

#include <string.h>
#include "stm32f4xx_hal.h"

/// @brief EEPROM address for storing timestamp.
#define TIMESTAMP_ADDRESS    0
/// @brief EEPROM address for storing counter value (timestamp is uint64, hence 8 bytes).
#define COUNTER_ADDRESS     8
/// @brief EEPROM address for storing maximum size (counter is uint16, hence 2 bytes).
#define MAX_SIZE_ADDRESS    10

// Macros for handling error codes
/**
 * @brief Check if an overflow write error has occurred.
 * @param err Error code to be checked.
 * @return True if overflow write error is present, false otherwise.
 */
#define check_Overflow_write_error( err )    ( (err & Overflow_write_error) != 0 )

/**
 * @brief Check if an overflow read error has occurred.
 * @param err Error code to be checked.
 * @return True if overflow read error is present, false otherwise.
 */
#define check_Overflow_read_error( err )     ( (err & Overflow_read_error) != 0 )

/**
 * @brief Check if the measurement storage is empty.
 * @param err Error code to be checked.
 * @return True if the storage is empty, false otherwise.
 */
#define check_Empty_MS_error( err )          ( (err & Empty_MS_error) != 0 )

/**
 * @brief Check if an I2C communication error has occurred.
 * @param err Error code to be checked.
 * @return True if an I2C error is present, false otherwise.
 */
#define check_I2C_error( err )               ( (err & I2C_error) != 0 )

/**
 * @struct MeasEntry
 * @brief A structure to store a single measurement entry.
 */
struct MeasEntry
{
    uint8_t  measID;  ///< Measurement ID.
    uint16_t deltaT;  ///< Time delta of the measurement.
    uint32_t measData; ///< The actual measurement data.
    static const uint8_t len = sizeof(uint8_t) + sizeof(uint16_t) + sizeof(uint32_t); ///< Length of the measurement entry.
};

// Forward declaration of MeasurementStorage class
class MeasurementStorage;


/**
 * @enum MS_ErrorCode_t
 * @brief Enumeration for possible error codes in the measurement storage system.
 */
typedef enum{
    Overflow_write_error = 0b0000000000000001,  /*!< Counter reached maximum size. */
    Overflow_read_error  = 0b0000000000000010,  /*!< Read address exceeds maximum size. */
    Empty_MS_error       = 0b0000000000000100,  /*!< Storage is empty, read operation not possible. */
    I2C_error            = 0b0000000000001000,  /*!< I2C communication error. */
    Maxsize_error        = 0b0000000000010000,  /*!< I2C communication error. */
} MS_ErrorCode_t;


/**
 * @typedef MS_ErroHandler
 * @brief A function pointer type for handling errors in the measurement storage system.
 */
typedef void(*MS_ErroHandler)( MeasurementStorage* caller, uint16_t ErrorCode_p );

/**
 * @class MeasurementStorage
 * @brief A class for managing measurement storage in EEPROM.
 */
class MeasurementStorage
{
private:
    MS_ErroHandler errorHandler = NULL; ///< Error handler function pointer.

    /**
     * @brief I2C address of the EEPROM.
     */
    uint8_t EEPROMAddress;

    /**
     * @brief HAL I2C handle used to communicate with the EEPROM.
     */
    I2C_HandleTypeDef* I2Ccontroller = NULL;

    /**
     * @brief Length of an EEPROM page (128 bytes for 24LC512).
     */
    uint8_t pageLen;

    uint16_t freePages; ///< Number of free pages in EEPROM.


    void incrementCounter();
    void resetCounter();
    void setTimestamp(uint64_t timestamp_p);
    void setMaxSize(uint16_t maxSize_p);
public:
    /**
     * @brief Constructor to initialize the MeasurementStorage class.
     * @param I2Ccontroller_p I2C handle for communication.
     * @param EEPROMAddress_p EEPROM I2C address.
     * @param pageLen_p Length of a page in EEPROM (default is 128).
     * @param freePages_p Number of free pages (default is 499).
     */
    MeasurementStorage(I2C_HandleTypeDef* I2Ccontroller_p, uint8_t EEPROMAddress_p, uint8_t pageLen_p = 128, uint16_t freePages_p = 499);

    /**
     * @brief Initializes the measurement storage with a timestamp.
     * @param Timestamp_p The initial timestamp to set.
     */
    void init(uint64_t Timestamp_p);

    /**
     * @brief Reads the current counter value.
     * @return The number of entries stored in the EEPROM.
     */
    uint16_t readCounter();

    /**
     * @brief Reads the stored timestamp from EEPROM.
     * @return The stored timestamp.
     */
    uint64_t readTimestamp();

    /**
     * @brief Gets the maximum size of the storage.
     * @return The maximum number of entries that can fit into the storage.
     */
    uint16_t getMaxSize();

    /**
     * @brief Adds a measurement entry to the storage.
     * @param MeasEntry_p The measurement entry to add.
     */
    void addEntry(MeasEntry MeasEntry_p);

    /**
     * @brief Retrieves a measurement entry at a specific location.
     * @param location_p The location of the entry to retrieve.
     * @param entryBuffer_p Buffer to store the retrieved entry.
     * @return True if the entry was retrieved successfully, false otherwise.
     */
    bool getEntryAt(uint16_t location_p, MeasEntry* entryBuffer_p);
};

/**
 * @brief Writes data to EEPROM with multiple retry attempts.
 * @param hi2c I2C handle for communication.
 * @param DevAddress EEPROM I2C address.
 * @param MemAddress Starting memory address in EEPROM.
 * @param MemAddSize Size of the memory address (usually 16-bit).
 * @param pData Pointer to the data to be written.
 * @param Size Size of the data to be written.
 * @param Timeout Timeout for the I2C operation.
 * @param maxTries Maximum number of retry attempts (default is 100).
 * @param delayLen Delay length between retries (default is 5 ms).
 * @return HAL status indicating success or failure of the operation.
 */
HAL_StatusTypeDef write2EEPROM(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint16_t MemAddress, uint16_t MemAddSize, uint8_t *pData, uint16_t Size, uint32_t Timeout, uint8_t maxTries = 100, uint8_t delayLen = 5);

/**
 * @brief Reads data from EEPROM with multiple retry attempts.
 * @param hi2c I2C handle for communication.
 * @param DevAddress EEPROM I2C address.
 * @param MemAddress Starting memory address in EEPROM.
 * @param MemAddSize Size of the memory address (usually 16-bit).
 * @param pData Pointer to the buffer to store the read data.
 * @param Size Size of the data to be read.
 * @param Timeout Timeout for the I2C operation.
 * @param maxTries Maximum number of retry attempts (default is 100).
 * @param delayLen Delay length between retries (default is 5 ms).
 * @return HAL status indicating success or failure of the operation.
 */
HAL_StatusTypeDef readFromEEPROM(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint16_t MemAddress, uint16_t MemAddSize, uint8_t *pData, uint16_t Size, uint32_t Timeout, uint8_t maxTries = 100, uint8_t delayLen = 5);

/**
 * @brief Writes data to EEPROM across multiple pages.
 * @param I2Ccontroller I2C handle for communication.
 * @param EEPROMAddress EEPROM I2C address.
 * @param start Starting address in EEPROM.
 * @param data_p Pointer to the data to be written.
 * @param len Length of the data to be written.
 * @param pageLen Length of a single EEPROM page.
 * @return HAL status indicating success or failure of the operation.
 */
HAL_StatusTypeDef writeMultiPage( I2C_HandleTypeDef* I2Ccontroller, uint8_t EEPROMAddress, uint16_t start, uint8_t *data_p, uint16_t len, uint8_t pageLen );

/**
 * @brief Deletes a region of memory in EEPROM by writing 0xFF.
 * @param I2Ccontroller I2C handle for communication.
 * @param EEPROMAddress EEPROM I2C address.
 * @param start Starting address in EEPROM.
 * @param len Length of the region to delete.
 * @param pageLen Length of a single EEPROM page.
 * @return HAL status indicating success or failure of the operation.
 */
HAL_StatusTypeDef deleteRegion( I2C_HandleTypeDef* I2Ccontroller, uint8_t EEPROMAddress, uint16_t start, uint16_t len, uint8_t pageLen );

#endif /* MODULES_MEASSTOREAGE_MS_HPP_ */
