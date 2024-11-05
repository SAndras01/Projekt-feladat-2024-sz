/*
 * MAX31865.cpp
 *
 *  Created on: Sep 21, 2024
 *      Author: Sásdi András
 */

#include "MAX31865.hpp"

float MAX31865::tempFromRTD(uint16_t rtdValue_p)
{
    float resistance = (rtdValue_p >> 1) * R_REF / 32768.0; // 430.0 value of reference resistor
    float temperature = (resistance - r0) / 0.385; // 0.385 is the temperature coefficient
    return temperature;
}

uint16_t MAX31865::RTDFromTemp(float tempValue_p)
{
    float resistance = tempValue_p * 0.385 + r0;
    uint16_t rtdValue = static_cast<uint16_t>((resistance / R_REF) * 32768.0);

    //15 bit RTD value, LSB is do not care
    return rtdValue << 1;
}

MAX31865::MAX31865( SPI_HandleTypeDef *hspi_p, GPIO* csPin_p , GPIO* DRDYpin_p, RTD_type_t RTD_type_p )
{
	hspi = hspi_p;
	csPin = csPin_p;
	DRDYpin = DRDYpin_p;
	if(RTD_type_p == PT100)
	{
		r0 = 100;
	}
	else
	{
		r0 = 1000;
	}
}

HAL_StatusTypeDef MAX31865::init( MAX31865_FilterSetting_t filterSetting_p )
{
	HAL_StatusTypeDef stat;

	//First wake up the sensor by reading a register (it will return junk)
	uint8_t buff[1];
	readNFromAddres(MAX31865_CONFIG_REG_ADDRESS, buff, 1);

	//Turn the bias voltage on and set the filter
	uint8_t configValue = 0;

	configValue |= MAX31865_CONFIG_VBIAS_ON;

	//If 50Hz is set, set the bit, a bit value of 0 would set the 60Hz filter
	if ( filterSetting_p == MAX31865_FILTER_50HZ ) {configValue |= MAX31865_CONFIG_REG_FILTER_50Hz;}

	//Write the config register
	stat = writeNFromAddres(MAX31865_CONFIG_REG_ADDRESS, &configValue, 1);

	return stat;
}

HAL_StatusTypeDef MAX31865::readNFromAddres( uint8_t addr_p, uint8_t* rBuff_p, uint32_t dataSize_p )
{
	HAL_StatusTypeDef stat;

	csPin -> digitalWrite( GPIO_PIN_RESET );

	stat = HAL_SPI_Transmit( hspi, &addr_p, 1, HAL_MAX_DELAY) ; // Transmit and receive data
	stat = HAL_SPI_Receive( hspi, rBuff_p, dataSize_p, HAL_MAX_DELAY ); // Transmit and receive data

	csPin -> digitalWrite( GPIO_PIN_SET );

	return stat;
}

HAL_StatusTypeDef MAX31865::writeNFromAddres( uint8_t addr_p, uint8_t* wBuff_p, uint32_t dataSize_p )
{
	HAL_StatusTypeDef stat;

	//The device has a maximum of 4 consiquential writable registers
	uint8_t msgBuff[4] = {0, 0, 0, 0};

	//Construct the package
	msgBuff[0] = addr_p | MAX31865_WRITE_OFFSET_MASK;

	for(uint32_t i = 0; i < dataSize_p; i++)
	{
		msgBuff[1+i] = wBuff_p[i];
	}

	csPin -> digitalWrite( GPIO_PIN_RESET );

	stat = HAL_SPI_Transmit( hspi, msgBuff, dataSize_p+1, HAL_MAX_DELAY) ; // Transmit and receive data

	csPin -> digitalWrite( GPIO_PIN_SET );

	return stat;
}

void MAX31865::attachErrorHandler( MAX31865_ErroHandler handler_p )
{
	errorHandler = handler_p;
}

void MAX31865::setUpperThreshold(float thr_p)
{
	HAL_StatusTypeDef stat;
	uint16_t errors = 0;

	uint16_t thrRTDValue = RTDFromTemp(thr_p);

	uint8_t msgBuff[2];

	msgBuff[0] = thrRTDValue >> 8;	//MSB
	msgBuff[1] = thrRTDValue; 		//LSB

	stat = writeNFromAddres(MAX31865_HIGH_FAULT_MSB_REG_ADDRESS, (uint8_t*) msgBuff, sizeof(msgBuff));

	if( stat != HAL_OK && errorHandler != NULL)
	{
		errors += SPI_error;
		errorHandler(this, errors);
	}
}

void MAX31865::setLowerThreshold(float thr_p)
{
	HAL_StatusTypeDef stat;
	uint16_t errors = 0;

	uint16_t thrRTDValue = RTDFromTemp(thr_p);

	uint8_t msgBuff[2];

	msgBuff[0] = thrRTDValue >> 8;	//MSB
	msgBuff[1] = thrRTDValue; 		//LSB

	stat = writeNFromAddres(MAX31865_LOW_FAULT_MSB_REG_ADDRESS, (uint8_t*) msgBuff, sizeof(msgBuff));

	if( stat != HAL_OK && errorHandler != NULL)
	{
		errors += SPI_error;
		errorHandler(this, errors);
	}
}

float MAX31865::getUpperThreshold()
{
	HAL_StatusTypeDef stat;
	uint16_t errors = 0;
	uint8_t RTDbuff[2] = {0, 0};

	stat = readNFromAddres(MAX31865_HIGH_FAULT_MSB_REG_ADDRESS, RTDbuff, 2);

	uint16_t RTD = (RTDbuff[0] << 8) | RTDbuff[1]; // Combine

	if( stat != HAL_OK && errorHandler != NULL)
	{
		errors += SPI_error;
		errorHandler(this, errors);
	}

	return tempFromRTD(RTD);
}

float MAX31865::getLowerThreshold()
{
	HAL_StatusTypeDef stat;
	uint16_t errors = 0;
	uint8_t RTDbuff[2] = {0, 0};

	stat = readNFromAddres(MAX31865_LOW_FAULT_MSB_REG_ADDRESS, RTDbuff, 2);

	uint16_t RTD = (RTDbuff[0] << 8) | RTDbuff[1]; // Combine

	if( stat != HAL_OK && errorHandler != NULL)
	{
		errors += SPI_error;
		errorHandler(this, errors);
	}

	return tempFromRTD(RTD);
}

float MAX31865::singleMeas()
{
	HAL_StatusTypeDef stat;
	uint16_t errors = 0;
	uint8_t configValue;

	//read current config register
	stat = readNFromAddres(MAX31865_CONFIG_REG_ADDRESS, &configValue, 1);

	//new config value
	configValue |= MAX31865_CONFIG_ONE_SHOT;

	//set new config value
	stat = writeNFromAddres(MAX31865_CONFIG_REG_ADDRESS, &configValue, 1);


	//wait for the measuerement
	if(DRDYpin != NULL)
	{
		while( DRDYpin -> digitalRead() == GPIO_PIN_SET );
	}
	else
	{
		while( (configValue & MAX31865_CONFIG_ONE_SHOT) != 0 ) //The one-shot bit should self clear
		{
			stat = readNFromAddres(MAX31865_CONFIG_REG_ADDRESS, &configValue, 1);
		}
	}

	if( stat != HAL_OK && errorHandler != NULL)
	{
		errors += SPI_error;
		errorHandler(this, errors);
	}

	return getTemp();
}

void MAX31865::startContinousMeas()
{
	HAL_StatusTypeDef stat;
	uint16_t errors = 0;
	uint8_t configValue;

	//read current config register
	stat = readNFromAddres(MAX31865_CONFIG_REG_ADDRESS, &configValue, 1);

	//new config value
	configValue |= MAX31865_CONFIG_AUTO_CONV;

	//set new config value
	stat = writeNFromAddres(MAX31865_CONFIG_REG_ADDRESS, &configValue, 1);

	if( stat != HAL_OK && errorHandler != NULL)
	{
		errors += SPI_error;
		errorHandler(this, errors);
	}
}

void MAX31865::stopContinousMeas()
{
	HAL_StatusTypeDef stat;
	uint16_t errors = 0;
	uint8_t configValue;

	//read current config register
	stat = readNFromAddres(MAX31865_CONFIG_REG_ADDRESS, &configValue, 1);

	//new config value
	configValue &= ~MAX31865_CONFIG_AUTO_CONV;

	//set new config value
	stat = writeNFromAddres(MAX31865_CONFIG_REG_ADDRESS, &configValue, 1);

	if( stat != HAL_OK && errorHandler != NULL)
	{
		errors += SPI_error;
		errorHandler(this, errors);
	}
}

float MAX31865::getTemp()
{
	HAL_StatusTypeDef stat;
	uint16_t errors = 0;
	uint8_t RTDbuff[2] = {0, 0};

	stat = readNFromAddres(MAX31865_RTD_MSB_REG_ADDRESS, RTDbuff, 2);

	uint16_t RTD = (RTDbuff[0] << 8) | RTDbuff[1]; // Combine

	if( ( RTD & (uint16_t) 0x1 ) != 0 ) //RTD LSB D0 ( = fault bit)  is set
	{
		if( errorHandler != NULL )
		{
			errors += RTD_fault_general;
			errorHandler(this, errors);
		}
	}

	return tempFromRTD(RTD);

	if( stat != HAL_OK && errorHandler != NULL)
	{
		errors += SPI_error;
		errorHandler(this, errors);
	}
}


void MAX31865::runAutofaultDetection()
{
	HAL_StatusTypeDef stat;
	uint16_t errors = 0;
	uint8_t originalConfigValue;
	uint8_t faultDetectConfigValue;
	uint8_t currentConfigValue;

	//read current config register
	stat = readNFromAddres(MAX31865_CONFIG_REG_ADDRESS, &originalConfigValue, 1);

	//new config value
	faultDetectConfigValue = originalConfigValue | MAX31865_CONFIG_VBIAS_ON; //V bias ON

	faultDetectConfigValue &= ~MAX31865_CONFIG_AUTO_CONV; //turn auto OFF
	faultDetectConfigValue &= ~MAX31865_CONFIG_ONE_SHOT; //turn one-shot OFF
	faultDetectConfigValue &= ~MAX31865_CONFIG_REG_FAULT_STAT_CLEAR; //fault status clear OFF

	faultDetectConfigValue |= MAX31865_CONFIG_FAULT_DETECTION_AUTO_DELAY; //start auto fault detect cycle

	//set new config value
	stat = writeNFromAddres(MAX31865_CONFIG_REG_ADDRESS, &faultDetectConfigValue, 1);

	//Wait for the cycle to finnish
	stat = readNFromAddres(MAX31865_CONFIG_REG_ADDRESS, &currentConfigValue, 1);

	bool finnished = false;

	for(uint8_t i = 0; i < 100; i++)
	{
		stat = readNFromAddres(MAX31865_CONFIG_REG_ADDRESS, &currentConfigValue, 1);
		finnished = ( currentConfigValue & 0b00001100 ) == 0; //Configuration register D3 and D2 are cleared

		if(finnished)
		{
			//let the circuit leave the transient
			HAL_Delay(TIMECONSTANT_DELAY);
			break;
		}
		else
		{
			HAL_Delay(1);

		}
	}

	if( !finnished)
	{
		if(errorHandler != NULL)
		{
			errors += Fault_detect_stuck;
			errorHandler(this, errors);
		}
	}

	//Write back the original configuration value
	stat = writeNFromAddres(MAX31865_CONFIG_REG_ADDRESS, &originalConfigValue, 1);

	if( stat != HAL_OK && errorHandler != NULL)
	{
		errors += SPI_error;
		errorHandler(this, errors);
	}
}

void MAX31865::startManualfaultDetectionCycle()
{
	HAL_StatusTypeDef stat;
	uint16_t errors = 0;
	uint8_t faultDetectConfigValue;

	//read current config register
	stat = readNFromAddres(MAX31865_CONFIG_REG_ADDRESS, &manualFaultDetectOriginalConfigReg, 1);

	//new config value
	faultDetectConfigValue = manualFaultDetectOriginalConfigReg | MAX31865_CONFIG_VBIAS_ON; //V bias ON

	faultDetectConfigValue &= ~MAX31865_CONFIG_AUTO_CONV; //turn auto OFF
	faultDetectConfigValue &= ~MAX31865_CONFIG_ONE_SHOT; //turn one-shot OFF
	faultDetectConfigValue &= ~MAX31865_CONFIG_REG_FAULT_STAT_CLEAR; //fault status clear OFF

	faultDetectConfigValue |= MAX31865_CONFIG_FAULT_DETECTION_START_MANUAL; //start auto fault detect cycle

	//set new config value
	stat = writeNFromAddres(MAX31865_CONFIG_REG_ADDRESS, &faultDetectConfigValue, 1);

	if( stat != HAL_OK && errorHandler != NULL)
	{
		errors += SPI_error;
		errorHandler(this, errors);
	}
}

void MAX31865::endManualfaultDetectionCycle()
{
	HAL_StatusTypeDef stat;
	uint16_t errors = 0;
	uint8_t originalConfigValue;
	uint8_t faultDetectConfigValue;
	uint8_t currentConfigValue;

	//read current config register
	stat = readNFromAddres(MAX31865_CONFIG_REG_ADDRESS, &originalConfigValue, 1);

	//new config value
	faultDetectConfigValue = originalConfigValue | MAX31865_CONFIG_VBIAS_ON; //V bias ON

	faultDetectConfigValue &= ~MAX31865_CONFIG_AUTO_CONV; //turn auto OFF
	faultDetectConfigValue &= ~MAX31865_CONFIG_ONE_SHOT; //turn one-shot OFF
	faultDetectConfigValue &= ~MAX31865_CONFIG_REG_FAULT_STAT_CLEAR; //fault status clear OFF

	faultDetectConfigValue |= MAX31865_CONFIG_FAULT_DETECTION_STOP_MANUAL; //start auto fault detect cycle

	//set new config value
	stat = writeNFromAddres(MAX31865_CONFIG_REG_ADDRESS, &faultDetectConfigValue, 1);

	//Wait for the cycle to finnish
	stat = readNFromAddres(MAX31865_CONFIG_REG_ADDRESS, &currentConfigValue, 1);

	bool finnished = false;

	for(uint8_t i = 0; i < 100; i++)
	{
		stat = readNFromAddres(MAX31865_CONFIG_REG_ADDRESS, &currentConfigValue, 1);
		finnished = ( currentConfigValue & 0b00001100 ) == 0; //Configuration register D3 and D2 are cleared

		if(finnished)
		{
			//let the circuit leave the transient
			HAL_Delay(TIMECONSTANT_DELAY);
			break;
		}
		else
		{
			HAL_Delay(1);
		}
	}

	if( !finnished)
	{
		if(errorHandler != NULL)
		{
			errors += Fault_detect_stuck;
			errorHandler(this, errors);
		}
	}

	//Write back the original configuration value
	stat = writeNFromAddres(MAX31865_CONFIG_REG_ADDRESS, &manualFaultDetectOriginalConfigReg, 1);

	if( stat != HAL_OK && errorHandler != NULL)
	{
		errors += SPI_error;
		errorHandler(this, errors);
	}
}


bool MAX31865::faultReadout()
{
	HAL_StatusTypeDef stat;
	uint16_t errors = 0;
	uint8_t faultValue;
	uint8_t configValue;

	//test if fault detect cycle is finnished
	stat = readNFromAddres(MAX31865_CONFIG_REG_ADDRESS, &configValue, 1);
	bool finnished = ( configValue & 0b00001100 ) == 0; //Configuration register D3 and D2 are cleared

	if(! finnished)
	{
		return false;
	}

	//read fault register
	stat = readNFromAddres(MAX31865_FAULT_STATUS_REG_ADDRESS, &faultValue, 1);

	//Clear fault register
	//read current config register
	stat = readNFromAddres(MAX31865_CONFIG_REG_ADDRESS, &configValue, 1);
	//new config value
	configValue |= MAX31865_CONFIG_REG_FAULT_STAT_CLEAR;
	//set new config value
	stat = writeNFromAddres(MAX31865_CONFIG_REG_ADDRESS, &configValue, 1);

	if( stat != HAL_OK)
	{
		errors += SPI_error;
		errorHandler(this, errors);
		errors = 0; //so this error wont get handled twice
	}

	if(
		faultValue != 0
		&&
		( (faultValue & FAULT_STATUS_D1) == 0) && ( (faultValue & FAULT_STATUS_D1) == 0) //These bits are "do not care"
	)
	{

		if( ( faultValue & FAULT_STATUS_D7 ) != 0 ) { errors += HighThrehsold; }
		if( ( faultValue & FAULT_STATUS_D6 ) != 0 ) { errors += LowThrehsold; }
		if( ( faultValue & FAULT_STATUS_D5 ) != 0 ) { errors += VREFINneg_greater_0p85_x_VBIAS; }
		if( ( faultValue & FAULT_STATUS_D4 ) != 0 ) { errors += VREFINneg_smaller_0p85_x_VBIAS_Force_Open; }
		if( ( faultValue & FAULT_STATUS_D3 ) != 0 ) { errors += RTDINneg_smaller_0p85_x_VBIAS_Force_Open; }
		if( ( faultValue & FAULT_STATUS_D2 ) != 0 ) { errors += OV_or_UV; }

		errorHandler(this, errors);

		return false;
	}// If fault detected
	return true;
}


