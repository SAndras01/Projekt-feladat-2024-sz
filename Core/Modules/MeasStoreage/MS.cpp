/*
 * MS.cpp
 *
 *  Created on: Sep 21, 2024
 *      Author: Sásdi András
 */

#include "MS.hpp"

HAL_StatusTypeDef write2EEPROM(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint16_t MemAddress, uint16_t MemAddSize, uint8_t *pData, uint16_t Size, uint32_t Timeout, uint8_t maxTries, uint8_t delayLen)
{
	HAL_StatusTypeDef stat;
	HAL_Delay(delayLen);
	stat = HAL_I2C_Mem_Write(hi2c, DevAddress, MemAddress, MemAddSize, pData, Size, Timeout);

	uint8_t trycounter = 0;
	while(stat != HAL_OK && trycounter <= maxTries)
	{
		stat = HAL_I2C_IsDeviceReady(hi2c, DevAddress, 2, 100);
		trycounter++;
	}

	return stat;
}

HAL_StatusTypeDef readFromEEPROM(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint16_t MemAddress, uint16_t MemAddSize, uint8_t *pData, uint16_t Size, uint32_t Timeout, uint8_t maxTries, uint8_t delayLen)
{
	HAL_StatusTypeDef stat;
	HAL_Delay(delayLen);
	stat = HAL_I2C_Mem_Read(hi2c, DevAddress, MemAddress, MemAddSize, pData, Size, Timeout);

	uint8_t trycounter = 0;
	while(stat != HAL_OK && trycounter <= maxTries)
	{
		stat = HAL_I2C_Mem_Read(hi2c, DevAddress, MemAddress, MemAddSize, pData, Size, Timeout);
		trycounter++;
	}

	return stat;
}

HAL_StatusTypeDef writeMultiPage( I2C_HandleTypeDef* I2Ccontroller, uint8_t EEPROMAddress, uint16_t start, uint8_t *data_p, uint16_t len, uint8_t pageLen )
{
	HAL_StatusTypeDef stat;

	uint16_t remainder; //this much is left until the end of the current page
	while(len != 0)
	{
		remainder = (pageLen-((start)%pageLen));

		if( remainder <= len )
		{
			HAL_Delay(5);
			stat = HAL_I2C_Mem_Write(I2Ccontroller, EEPROMAddress<<1, start, sizeof(start), data_p, remainder, HAL_MAX_DELAY);

			while(stat != HAL_OK)
			{
				stat = HAL_I2C_IsDeviceReady(I2Ccontroller, EEPROMAddress<<1, 2, 100);
			}

			start += remainder; //shift the "writer"
			len -= remainder; //decrease the remaining length
			data_p += remainder*sizeof(uint8_t); //shift the "reader"
		}
		else//write the rest of the data
		{
			HAL_Delay(5);
			stat = HAL_I2C_Mem_Write(I2Ccontroller, EEPROMAddress<<1, start, sizeof(start), data_p, len, HAL_MAX_DELAY);

			while(stat != HAL_OK)
			{
				stat = HAL_I2C_IsDeviceReady(I2Ccontroller, EEPROMAddress<<1, 2, 100);
			}

			len = 0;

		}
	}
	return stat;
}

//Basically the same as writeMultiPage, with the exception that here an array filled with 128 0xFF is used as the data
HAL_StatusTypeDef deleteRegion( I2C_HandleTypeDef* I2Ccontroller, uint8_t EEPROMAddress, uint16_t start, uint16_t len, uint8_t pageLen )
{
	uint8_t ereaserBuffer[pageLen];
	for(uint8_t i = 0; i < pageLen; i++){ ereaserBuffer[i]=0xFF; }

	HAL_StatusTypeDef stat;

	uint16_t remainder; //this much is left until the end of the current page
	while(len != 0)
	{
		remainder = (pageLen-((start)%pageLen));

		if( remainder <= len )
		{
			//255 from start until EOP then len = len-remainder, start = start + remainder
			HAL_Delay(5);
			stat = HAL_I2C_Mem_Write(I2Ccontroller, EEPROMAddress<<1, start, sizeof(start), ereaserBuffer, remainder, HAL_MAX_DELAY);

			while(stat != HAL_OK)
			{
				stat = HAL_I2C_IsDeviceReady(I2Ccontroller, EEPROMAddress<<1, 2, 100);
			}

			//Here the "reader" is not increased
			start += remainder;
			len -= remainder;
		}
		else
		{
			//255 from start until len then len = 0
			HAL_Delay(5);
			stat = HAL_I2C_Mem_Write(I2Ccontroller, EEPROMAddress<<1, start, sizeof(start), ereaserBuffer, len, HAL_MAX_DELAY);

			while(stat != HAL_OK)
			{
				stat = HAL_I2C_IsDeviceReady(I2Ccontroller, EEPROMAddress<<1, 2, 100);
			}

			len = 0;

		}
	}
	return stat;
}

MeasurementStorage::MeasurementStorage( I2C_HandleTypeDef* I2Ccontroller_p, uint8_t EEPROMAddress_p, uint8_t pageLen_p, uint16_t freePages_p )
{
	this -> I2Ccontroller = I2Ccontroller_p;
	this -> EEPROMAddress = EEPROMAddress_p;
	this -> pageLen = pageLen_p;
	this -> freePages = freePages_p;
}

void MeasurementStorage::init(uint64_t Timestamp_p)
{
	setTimestamp(Timestamp_p);
	resetCounter();

	uint16_t maxSize = (pageLen * freePages) / MeasEntry::len;

	setMaxSize(maxSize);
}

void MeasurementStorage::incrementCounter()
{
	uint16_t errors = 0;
	HAL_StatusTypeDef stat;
	uint16_t counter = 0;
	uint16_t maxSize = 0;

	stat = readFromEEPROM(I2Ccontroller, EEPROMAddress<<1, COUNTER_ADDRESS, sizeof(uint16_t), (uint8_t*)&counter, sizeof(uint16_t), HAL_MAX_DELAY);
	stat = readFromEEPROM(I2Ccontroller, EEPROMAddress<<1, COUNTER_ADDRESS, sizeof(uint16_t), (uint8_t*)&maxSize, sizeof(uint16_t), HAL_MAX_DELAY);

	counter++;

	stat = write2EEPROM(I2Ccontroller, EEPROMAddress<<1, COUNTER_ADDRESS, sizeof(uint16_t), (uint8_t*)&counter, sizeof(uint16_t), HAL_MAX_DELAY);

	if( stat != HAL_OK && errorHandler != NULL)
	{
		errors += I2C_error;
		errorHandler(this, errors);
	}

	if( counter == maxSize && errorHandler != NULL)
	{
		errors += Overflow_write_error;
		errorHandler(this, errors);
	}
}

void MeasurementStorage::resetCounter()
{
	uint16_t errors = 0;
	HAL_StatusTypeDef stat;
	uint16_t counter = 0;

	stat = write2EEPROM(I2Ccontroller, EEPROMAddress<<1, COUNTER_ADDRESS, sizeof(uint16_t), (uint8_t*)&counter, sizeof(uint16_t), HAL_MAX_DELAY);

	if( stat != HAL_OK && errorHandler != NULL)
	{
		errors += I2C_error;
		errorHandler(this, errors);
	}
}

void MeasurementStorage::setTimestamp(uint64_t timestamp_p)
{
	uint16_t errors = 0;
	HAL_StatusTypeDef stat;

	stat = write2EEPROM(I2Ccontroller, EEPROMAddress<<1, TIMESTAMP_ADDRESS, sizeof(uint16_t), (uint8_t*)&timestamp_p, sizeof(uint64_t), HAL_MAX_DELAY);

	if( stat != HAL_OK && errorHandler != NULL)
	{
		errors += I2C_error;
		errorHandler(this, errors);
	}
}

void MeasurementStorage::setMaxSize(uint16_t maxSize_p)
{
	uint16_t errors = 0;
	HAL_StatusTypeDef stat;

	stat = write2EEPROM(I2Ccontroller, EEPROMAddress<<1, MAX_SIZE_ADDRESS, sizeof(uint16_t), (uint8_t*)&maxSize_p, sizeof(uint16_t), HAL_MAX_DELAY);

	if( stat != HAL_OK && errorHandler != NULL)
	{
		errors += I2C_error;
		errorHandler(this, errors);
	}
}

uint16_t MeasurementStorage::readCounter()
{
	uint16_t errors = 0;
	HAL_StatusTypeDef stat;
	uint16_t counter = 0;

	stat = readFromEEPROM(I2Ccontroller, EEPROMAddress<<1, COUNTER_ADDRESS, sizeof(uint16_t), (uint8_t*)&counter, sizeof(uint16_t), HAL_MAX_DELAY);

	if( stat != HAL_OK && errorHandler != NULL)
	{
		errors += I2C_error;
		errorHandler(this, errors);
	}

	return counter;
}

uint64_t MeasurementStorage::readTimestamp()
{
	uint16_t errors = 0;
	HAL_StatusTypeDef stat;
	uint64_t timestamp = 0;

	stat = readFromEEPROM(I2Ccontroller, EEPROMAddress<<1, TIMESTAMP_ADDRESS, sizeof(uint16_t), (uint8_t*)&timestamp, sizeof(uint64_t), HAL_MAX_DELAY);

	if( stat != HAL_OK && errorHandler != NULL)
	{
		errors += I2C_error;
		errorHandler(this, errors);
	}

	return timestamp;
}

uint16_t MeasurementStorage::getMaxSize()
{
	uint16_t errors = 0;
	HAL_StatusTypeDef stat;
	uint16_t maxSize = 0;

	stat = readFromEEPROM(I2Ccontroller, EEPROMAddress<<1, MAX_SIZE_ADDRESS, sizeof(uint16_t), (uint8_t*)&maxSize, sizeof(uint16_t), HAL_MAX_DELAY);

	if( stat != HAL_OK && errorHandler != NULL)
	{
		errors += I2C_error;
		errorHandler(this, errors);
	}

	return maxSize;
}

void MeasurementStorage::addEntry(MeasEntry MeasEntry_p)
{
	uint8_t EntryBuffer[MeasEntry::len];
	for(uint8_t i = 0; i<MeasEntry::len; i++) { EntryBuffer[i] = 0; }

	memcpy((uint8_t*)(EntryBuffer), 									&(MeasEntry_p.measID), 		sizeof(uint8_t));
	memcpy((uint8_t*)(EntryBuffer+sizeof(uint8_t)), 					&(MeasEntry_p.deltaT), 		sizeof(uint16_t));
	memcpy((uint8_t*)(EntryBuffer+sizeof(uint8_t)+sizeof(uint16_t)), 	&(MeasEntry_p.measData), 	sizeof(uint32_t));

	uint16_t counter = readCounter();
	uint16_t EntryAddr = pageLen + (counter * MeasEntry::len);

	writeMultiPage(I2Ccontroller, EEPROMAddress, EntryAddr, EntryBuffer, MeasEntry::len, pageLen);

	incrementCounter();
}

bool MeasurementStorage::getEntryAt(uint16_t location_p, MeasEntry* entryBuffer_p)
{
	uint16_t errors = 0;
	uint16_t count = readCounter();
	uint16_t maxSize = getMaxSize();

	//Want to read outside of boundaries. -1, as counter of 0 means 0 stored, the "writer head" is set to 0, where as location starts from 0
	if(count == 0)
	{
		if( errorHandler != NULL)
		{
			errors += Empty_MS_error;
			errorHandler(this, errors);
		}
		return false;
	}

	else if( (location_p > maxSize-1) or (count > maxSize))
	{
		if( errorHandler != NULL)
		{
			errors += Maxsize_error;
			errorHandler(this, errors);
		}
		return false;
	}

	else if(location_p > count-1)
	{
		if( errorHandler != NULL)
		{
			errors += Overflow_read_error;
			errorHandler(this, errors);
		}
		return false;
	}

	uint16_t EntryAddr = pageLen + (location_p * MeasEntry::len);

	uint8_t readBuffer[MeasEntry::len];

	readFromEEPROM(I2Ccontroller, EEPROMAddress<<1, EntryAddr, sizeof(uint16_t), readBuffer, MeasEntry::len, HAL_MAX_DELAY);

	memcpy(&entryBuffer_p->measID, 		(uint8_t*)(readBuffer), 									sizeof(uint8_t));
	memcpy(&entryBuffer_p->deltaT, 		(uint8_t*)(readBuffer+sizeof(uint8_t)),						sizeof(uint16_t));
	memcpy(&entryBuffer_p->measData,	(uint8_t*)(readBuffer+sizeof(uint8_t)+sizeof(uint16_t)),	sizeof(uint32_t));

	return true;
}
