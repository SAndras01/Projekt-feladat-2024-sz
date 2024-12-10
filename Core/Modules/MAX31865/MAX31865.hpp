/**
 * @file MAX31865.hpp
 * @brief Interface for MAX31865 RTD-to-Digital converter.
 * This file provides functions and macros to interact with the MAX31865 sensor for temperature measurement.
 *
 * @details The MAX31865 is a resistance-to-digital converter optimized for PT100 and PT1000.
 * It can operate in both 2-wire, 3-wire, and 4-wire configurations, with all of which are supported.
 *
 * @author Sásdi András
 * @date September 21, 2024
 */
#ifndef MODULES_MAX31865_MAX31865_HPP_
#define MODULES_MAX31865_MAX31865_HPP_

#include "GPIO.hpp"
#include "MAX31865_Regmap.hpp"
#include "stm32f4xx_hal.h"
#include <stdint.h>

#define R_REF 423

/// Delay after fault check to let the circuit stabilize
#define TIMECONSTANT_DELAY 100

/// @brief Checks if the measured resistance is greater than the high fault threshold.
/// @param err Error code to be checked.
/// @return True if the high threshold error is present, false otherwise.
#define check_HighThrehsold( err ) ((err & HighThrehsold) != 0)

/// @brief Checks if the measured resistance is less than the low fault threshold.
/// @param err Error code to be checked.
/// @return True if the low threshold error is present, false otherwise.
#define check_LowThrehsold( err ) ((err & LowThrehsold) != 0)

/// @brief Checks if VREFIN- is greater than 0.85 times VBIAS.
/// @param err Error code to be checked.
/// @return True if VREFIN- is greater than 0.85 * VBIAS, false otherwise.
#define check_VREFINneg_greater_0p85_x_VBIAS( err ) ((err & VREFINneg_greater_0p85_x_VBIAS) != 0)

/// @brief Checks if VREFIN- is smaller than 0.85 times VBIAS while force is open.
/// @param err Error code to be checked.
/// @return True if VREFIN- is smaller than 0.85 * VBIAS (force open), false otherwise.
#define check_VREFINneg_smaller_0p85_x_VBIAS_Force_Open( err ) \
    ((err & VREFINneg_smaller_0p85_x_VBIAS_Force_Open) != 0)

/// @brief Checks if RTDIN- is smaller than 0.85 times VBIAS while force is open.
/// @param err Error code to be checked.
/// @return True if RTDIN- is smaller than 0.85 * VBIAS (force open), false otherwise.
#define check_RTDINneg_smaller_0p85_x_VBIAS_Force_Open( err ) \
    ((err & RTDINneg_smaller_0p85_x_VBIAS_Force_Open) != 0)

/// @brief Checks if an overvoltage (OV) or undervoltage (UV) condition is present.
/// @param err Error code to be checked.
/// @return True if an OV or UV condition is present, false otherwise.
#define check_OV_or_UV( err ) ((err & OV_or_UV) != 0)

/// @brief Checks if an SPI communication error occurred.
/// @param err Error code to be checked.
/// @return True if an SPI error is present, false otherwise.
#define check_SPI_error( err ) ((err & SPI_error) != 0)

/// @brief Checks if a fault detection stuck error occurred.
/// @param err Error code to be checked.
/// @return True if a fault detection stuck condition is present, false otherwise.
#define check_Fault_detect_stuck( err ) ((err & Fault_detect_stuck) != 0)

/// @brief Checks if a general RTD fault is detected.
/// @param err Error code to be checked.
/// @return True if a general RTD fault is present, false otherwise.
#define check_RTD_fault_general( err ) ((err & RTD_fault_general) != 0)


/**
 * @enum MAX31865_FilterSetting_t
 * @brief Settings for the noise filter in the MAX31865.
 */
typedef enum{
	MAX31865_FILTER_60HZ = 0,	/*!< Use this if the dominant bacground noise frequency is 60 Hz */
	MAX31865_FILTER_50HZ = 1,	/*!< Use this if the dominant bacground noise frequency is 50 Hz */
} MAX31865_FilterSetting_t;


/**
 * @enum MAX31865_ErrorCode_t
 * @brief Error flags to be used with MAX31865_ErroHandler type function.
 */
typedef enum{
	HighThrehsold 									= 0b0000000000000001,	/*!< Measured resistance greater than High Fault Threshold value  */
	LowThrehsold 									= 0b0000000000000010,	/*!< Measured resistance less than Low Fault Threshold value  */
	VREFINneg_greater_0p85_x_VBIAS 					= 0b0000000000000100,	/*!< V<SUB>REFIN-</SUB> > 0.85 x V<SUB>BIAS</SUB> */
	VREFINneg_smaller_0p85_x_VBIAS_Force_Open 		= 0b0000000000001000,	/*!< V<SUB>REFIN-</SUB> < 0.85 x V<SUB>BIAS</SUB>  (FORCE- open)  */
	RTDINneg_smaller_0p85_x_VBIAS_Force_Open 		= 0b0000000000010000,	/*!< V<SUB>RTDIN-</SUB>  < 0.85 x V<SUB>BIAS</SUB>  (FORCE- open)  */
	OV_or_UV										= 0b0000000000100000,	/*!< Any protected input voltage >VDD or <GND1  */
	SPI_error										= 0b0000000001000000,   /*!< The HAL layer threw an SPI error */
	Fault_detect_stuck								= 0b0000000010000000,   /*!< The auto clearing bit indicating the finish of a cycle didn't clear */
	RTD_fault_general								= 0b0000000100000000,	/*!< General RTD value detected while reading the temperature register */
} MAX31865_ErrorCode_t;

/**
 * @enum RTD_type_t
 * @brief Used to set the used RTD type.
 */
typedef enum{
	PT100,	/*!< Use this if the connected RTD is a PT100 */
	PT1000	/*!< Use this if the connected RTD is a PT1000 */
} RTD_type_t;

class MAX31865; // Forward declaration

typedef void(*MAX31865_ErroHandler)( MAX31865* caller, uint16_t ErrorCode_p );



/**
 * @brief Class for the MAX31865 temperature sensor amplifier, with attached TP100
 *
 * With this driver the basic functionality of the IC can be accessed. Temperature reading can be done as well as setting the upper and lower thresholds.
 * This class supports both  continuous and single-shot measurement modes. The built-in error detection of the amplifier is also supported.
 *
 *
 * Example:
	* \code
	#include "MAX31865.hpp"
	#include "GPIO_Class.hpp"
	⋮
	GPIO TEMP_SENS_CS(TEMP_SENS_CS_GPIO_Port, TEMP_SENS_CS_Pin);
	GPIO TEMP_RDY(TEMP_RDY_GPIO_Port, TEMP_RDY_Pin);

	MAX31865 myPT100(&hspi1, &TEMP_SENS_CS, &TEMP_RDY);
    ⋮

    int main(void)
	{
	⋮

	myPT100.init();

	float singleMeas = myPT100.singleMeas();
 * \endcode
 * */
class MAX31865{
private:
	/**
	 * @brief The type of the RTD connected to the sensor determines the resistance at 0C. The type of RTD can be set with the \link MAX31865 constructor \endlink
	 */
	uint8_t r0;

	/**
	 * @brief The HAL SPI handle that will be used to communicate with the device
	 */
	SPI_HandleTypeDef *hspi;

	/**
	 * @brief The \link GPIO \endlink that is used as the chip select pin.
	 */
	GPIO* csPin;

	/**
	 * @brief The \link GPIO \endlink that is connected to the DRDY pin of the device. If it is not set, the device can still be used, but it is highly recommended
	 */
	GPIO* DRDYpin;

	/**
	 * @brief The \link MAX31865_FilterSetting_t filter setting\endlink that will be used to configure the device
	 */
	MAX31865_FilterSetting_t filterSetting;

	/**
	 * @brief If attached an error Handler function will be called when encountering a problem
	 * @note must abide to the following argument list: ( MAX31865* caller, MAX31865_ErrorCode_t ErrorCode_p )
	 *
	 */
	MAX31865_ErroHandler errorHandler = NULL;

    /**
	* @brief Reads N bytes starting from the given address
	*
	* @param addr_p memory address where the data starts
	* @param rBuff_p pointer where the read data will be stored
	* @param dataSize_p the size of the data to be read
	*
	* @returns an HAL_StatusTypeDef that represents the succes of the communication
	*/
	HAL_StatusTypeDef readNFromAddres( uint8_t addr_p, uint8_t* rBuff_p, uint32_t dataSize_p );

    /**
	* @brief Writes N bytes starting from the given address
	*
	* @param addr_p memory address where the data starts
	* @param rBuff_p pointer where to the new data is stored
	* @param dataSize_p the size of the data to be written
	*
	* @returns an HAL_StatusTypeDef that represents the succes of the communication
	*/
	HAL_StatusTypeDef writeNFromAddres( uint8_t addr_p, uint8_t* wBuff_p, uint32_t dataSize_p );

    /**
	* @brief Convert raw RTD reading into Celsius
	*
	* @param rtdValue_p raw RTD reading
	*/
	float tempFromRTD(uint16_t rtdValue_p);

    /**
	* @brief Converts temperature given in Celsius into corresponding RTD reading
	*
	* @param tempValue_p temprature in Celsius to be converted
	*/
	uint16_t RTDFromTemp(float tempValue_p);

    /**
	* @brief Used to store the value of the config reg between the start and the end of a manual fault detection cycle
	*/
	uint8_t manualFaultDetectOriginalConfigReg;

public:

    /**
	* @brief Constructor for the MAX31865 class
	*
	* @param hspi_p the HAL spi handler that will be used to communicate with the device
	* @param csPin_p the chip select pin
	* @param DRDYpin_p the DRDY pin
	* @param RTD_type_p the type of RTD used (PT100 or PT1000) defaults to PT100
	*
	* @note before use the \link MAX31865::init \endlink function also have to be called
	*/
	MAX31865( SPI_HandleTypeDef *hspi_p, GPIO* csPin_p , GPIO* DRDYpin_p = NULL, RTD_type_t RTD_type_p = PT100 );

    /**
	* @brief Initialize the device
	*
	* Set the configuration register based on the given filter setting, and get the device ready for use
	*
	* @param filterSetting_p The requred notch frequencies for the noise rejection filter
	*
	*/
	HAL_StatusTypeDef init( MAX31865_FilterSetting_t filterSetting_p = MAX31865_FILTER_50HZ);

	/**
	* @brief Attaches an Error handler function handler to the instance.
	* @param handler_p The function to be used (must abide to the following argument list: ( MAX31865* caller, MAX31865_ErrorCode_t ErrorCode_p )
	* Example:
		* \code
		void exampleCallback( MAX31865* caller, uint16_t ErrorCode_p )
		{
			if(check_HighThrehsold( ErrorCode_p ))
			{
				mySerial.println("High threshold crossed");
			}

			if(check_LowThrehsold( ErrorCode_p ))
			{
				mySerial.println("Low threshold crossed");
			}

			if(check_VREFINneg_greater_0p85_x_VBIAS( ErrorCode_p ))
			{
				mySerial.println("RTDIN or FORCE Short detected");
			}

			if(check_VREFINneg_smaller_0p85_x_VBIAS_Force_Open( ErrorCode_p ))
			{
				mySerial.println("RTDIN or FORCE Short detected");
			}

			if(check_RTDINneg_smaller_0p85_x_VBIAS_Force_Open( ErrorCode_p ))
			{
				mySerial.println("RTDIN or FORCE Short detected");
			}

			if(check_OV_or_UV( ErrorCode_p ))
			{
				mySerial.println("Under voltage or over voltage detected");
			}

			if(check_SPI_error( ErrorCode_p ))
			{
				mySerial.println("SPI error");
			}

			if(check_Fault_detect_stuck( ErrorCode_p ))
			{
				mySerial.println("Fault detect cycle stuck");
			}

			if(check_RTD_fault_general( ErrorCode_p ))
			{
				mySerial.println("General RTD fault detected");
			}
		}
		⋮
		int main(void)
		{
		⋮
		  myPT100.init();
		  myPT100.attachErrorHandler(exampleCallback);

		  myPT100.startContinousMeas();

		  myPT100.setUpperThreshold(36);
		  myPT100.setLowerThreshold(34);

		  float upper = myPT100.getUpperThreshold();
		  float lower = myPT100.getLowerThreshold();

		  float meas;
		  while (1)
		  {
			  meas = myPT100.getTemp();
			  mySerial.println(meas);

			  myPT100.runAutofaultDetection();

			  myPT100.faultReadout();
		  }
		  ⋮
	 * \endcode
	 *
	 * @note Macros, like \link check_HighThrehsold \endlink are defined in order to ease the handling of the error code
	*/
	void attachErrorHandler( MAX31865_ErroHandler handler_p );

	/**
	* @brief Sets the upper threshold value
	*
	* The High Fault Threshold and Low Fault Threshold registers select the trip thresholds for RTD fault detection.
	* The results of RTD conversions are compared with the values in these registers to generate the “Fault”
	*
	* @param thr_p The required high threshold in celsius
	*
	* @note As the value is given in float and gets converted to an RTD reading value, the quantization error will cause a difference between the set and the actual value. (resolution is about 0.034 °C/LSB)
	*/
	void setUpperThreshold(float thr_p);

	/**
	* @brief Sets the lower threshold value
	*
	* The High Fault Threshold and Low Fault Threshold registers select the trip thresholds for RTD fault detection.
	* The results of RTD conversions are compared with the values in these registers to generate the “Fault”
	*
	* @param thr_p The required low threshold in celsius
	*
	* @note As the value is given in float and gets converted to an RTD reading value, the quantization error will cause a difference between the set and the actual value. (resolution is about 0.034 °C/LSB)
	*/
	void setLowerThreshold(float thr_p);

	/**
	* @brief Gets the set upper threshold value
	*
	* Reads the threshold value form the device's register and converts it into celsius
	*
	* @returns The high threshold value in celsius
	*
	*/
	float getUpperThreshold();

	/**
	* @brief Gets the set lower threshold value
	*
	* Reads the threshold value form the device's register and converts it into celsius
	*
	* @returns The low threshold value in celsius
	*
	*/
	float getLowerThreshold();

	/**
	* @brief Get a single measurement
	*
	* Triggers a single-shot measurement and return with the value.
	*
	* @returns The measured temperature in celsius
	*
	*/
	float singleMeas();

	/**
	* @brief Start continuous measurement
	*
	* The device will continuously update it's temperature register. Conversions occur continuously at a 50/60Hz rate
	*/
	void startContinousMeas();

	/**
	* @brief Stops the continous measurement
	*/
	void stopContinousMeas();

	/**
	* @brief Get the value of the device's temperature register converted into celsius
	*
	* The value will only be up-to-date if the device is in continuous measurement mode
	*
	* @returns The value of the device's temperature register converted into celsius
	*
	* @warning Only use this function when the continous measurement has been started by \link MAX31865::startContinousMeas startContinousMeas \endlink. To get a single measurement, use the \link MAX31865::singleMeas singleMeas \endlink function!
	*
	*/
	float getTemp();


	/**
	* @brief Run an automatic fault check
	*
	* If the external RTD interface circuitry has a time constant not greater than 100us, the automatic fault detection is enough.
	*
	* @warning Don't use if the external RTD interface circuitry includes an input filter with a time constant greater than 100us!
	*
	*/
	void runAutofaultDetection();

	/**
	* @brief Start the fault checking sequence
	*
	* This function starts the first step of the two step fault detection cycle.
	* First The MAX31865 checks for faults while the FORCE- input switch is closed, and when the check completes, the FORCE-input switch opens.
	*
	* @note Ensure that VBIAS has been on for at least 5 time constants before starting the cycle!
	*
	*/
	void startManualfaultDetectionCycle();

	/**
	* @brief Finishes the fault checking sequence
	*
	* This function starts the second step of the two step fault detection cycle thus finishing that.
	* Again, wait at least 5 time constants, before calling this function. Now the MAX31865 now checks for faults while the FORCE- inputs switch is open.
	*
	* @note Wait at least 5 time constants, before calling this function!
	*
	*/
	void endManualfaultDetectionCycle();

	/**
	* @brief Read the fault register
	*
	* The value of the fault register is read. The handling of the result is passed to the attached error handler function, if one was given.
	* A boolean value is returned to allow for basic fault detection even if no error handler is attached.
	*
	* @returns True if no fault is detected
	*
	* @note In order for the readout to be up-tp-date, a fault detection cycle should be run first.
	*
	* Example with automatic cycle:
	* \code
	  ⋮
	  myPT100.startContinousMeas();
	  float meas;
	  bool state;
	  while (1)
	  {
		  meas = myPT100.getTemp();
		  mySerial.println(meas);

		  myPT100.runAutofaultDetection();

		  state = myPT100.faultReadout();
		  if(state){ mySerial.println("O"); }	else { mySerial.println("X"); }
	  }
	  ⋮
	* \endcode
	*/
	bool faultReadout();
};

#endif /* MODULES_MAX31865_MAX31865_HPP_ */
