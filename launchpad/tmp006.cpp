#include "tmp006.h"
#define USE_USCI_B1 


	
 void tmp006::begin(uint16_t totalSamples)
{	
   // Initialize Wire
    Wire.begin();
    // Reset TMP006 
    writeRegister(TMP006_CONFIG_REG, TMP006_RESET);
    // Configure TMP006 for initialization 
    writeRegister(TMP006_CONFIG_REG, TMP006_POWER_UP + totalSamples + CONVERSION_BIT_ENABLE);
    	
}


/*!
 * SLEEP FUNCTION
 * Wakes up TMP006 from power down mode without reconfiguration.
 *
 * @param None
 *
 * @return None
 *
 */

	
void tmp006::wakeUp(void)
{
    uint16_t settings;

    // Read and save current settings 
    settings = readRegister(TMP006_CONFIG_REG);
    // Power-up TMP006 with current settings*/
    settings |= TMP006_POWER_UP;
    // Writes to Config register*/
    writeRegister(TMP006_CONFIG_REG, settings);
}
	

 /*!
 * WAKEUP FUNCTION
 * Disables TMP006 by putting it into power down mode.
 *
 * @param None
 *
 * @return None
 *
 */

	
void tmp006::sleep(void)
{
    uint16_t settings;

    /* Read current settings */
    settings = readRegister(TMP006_CONFIG_REG);
    /* Mask out and power down TMP006 */
    settings &= ~(TMP006_POWER_UP);
    /* Power down TMP006 */
    writeRegister(TMP006_CONFIG_REG, settings);
}


 /*******************************************************************************
 *
 * LOWER LEVEL LIBRARY CALLS
 * Allows for more customization, most new users will have no need to  directly 
 * use or configure any of the following code
 * These functions are called on by user level library calls
 *
 ******************************************************************************/   
 
    
/*!
 * READY FUNCTION
 * Function to poll and check if object voltage and ambient temperature results are ready to read.
 *
 * @param None
 *
 * @return \b 0 \b - Data not ready \n
 *         \b 1 \b - Data is ready
 *
 */
	
uint8_t tmp006::ready(void)
{
    /* Read DRDY status from Config register */
     if (CONVERSION_DONE & readRegister(TMP006_CONFIG_REG))
        return 1;
     else
        return 0;
}


/*!
 * READ FROM TMP006 FUNCTION
 * Reads data out of the TMP006 in I2C Master Mode.
 * Uses functions in the Wire library
 *
 * @param registerName is the name of the register you are reading from
 *
 *
 * @return uint16_t value of Register
 *
 */




uint16_t tmp006::readRegister(uint8_t registerName)
{
    uint16_t value;

    /* Begin Tranmission at address of device on bus */
	Wire.beginTransmission(ADR1_0_ADR0_1);
	/* Send Pointer Register Byte */
    Wire.write(registerName);
    /* Sends Stop */
	Wire.endTransmission();
     /* Requests 2 bytes fron Slave */
	Wire.requestFrom(ADR1_0_ADR0_1, 2);
     /* Wait Until 2 Bytes are Ready*/
	while(Wire.available() < 2)	{}

    /* Read*/
    value = Wire.read();
	value = value << 8;
	value|= Wire.read();

    return value;
}


/*!
 * WRITE TO TMP006 FUNCTION
 * Writes data to TMP006 via I2C
 * Uses functions in the Wire library
 *
 * @param registerName is the base address of the I2C Master module.
 * @param data is the bytes you want to be written.
 *
 * This function transmit data to TMP006 via the pointer and data to be
 * transmitted.
 *
 * @return None.
 *
 */


void tmp006::writeRegister(uint8_t registerName, uint16_t data)
{
    /* Begin Tranmission at address of device on bus */
	Wire.beginTransmission(ADR1_0_ADR0_1);
	/* Send Pointer Register Byte */
	Wire.write(registerName);
    /* Read*/
	Wire.write((unsigned char)(data>>8));
	Wire.write((unsigned char)(data&0x00FF));
    /* Sends Stop */
	Wire.endTransmission();

}


/*!
 * TEMPERATURE DATA FUNCTION
 * Function to calculate and return the temperature and raw values from TMP006
 * sensor.
 *
 * @param tempStrucPtr is the pointer to the TMP006_TempStruct to hold result
 *
 * @return None
 *
 */
 

	
void tmp006::getTempStruct(TMP006_TempStruct* tempStrucPtr)
 {

    /* When the data is ready. */
 	while(!ready());
	/*Read the object voltage */
	tempStrucPtr->vObj = readRegister(Volt_REG);
	/* Read the ambient temperature */
	tempStrucPtr->tDie = readRegister(Temp_REG);
	// !TODO: Still debugging
	//tempStrucPtr->tempFixedPoint = TMP006_Calculate(tempStrucPtr->vObj,tempStrucPtr->tDie );


	// For now, let's use the float function

	/*Calculate the temperature in Kelvin */
	tempStrucPtr->tDie_K = ((float)(tempStrucPtr->tDie>>2) *.03125) + 273.15;
	/*Calculate the Voltage in volts*/
	tempStrucPtr->vObj_V = (float)(tempStrucPtr->vObj);
	/*Calculate the Temperature with corrections in Celsius*/
	tempStrucPtr->temp = Calculate_Temp(&(tempStrucPtr->tDie_K), &(tempStrucPtr->vObj_V));

    
}

/*!
 * TEMPERATURE  FUNCTION
 * Function to calculate and return the temperature and raw values from TMP006
 * sensor.
 *
 * @param tempStrucPtr is the pointer to the TMP006_TempStruct to hold result
 *
 * @return temperature in Celsius (type float)
 *
 */

float tmp006::getTemp()
{
	TMP006_TempStruct tempStruct;
	getTempStruct(&tempStruct);
	return tempStruct.temp;
}


/*!
 * TEMPERATURE CALCULATION FUNCTION
 * Function to calculate temperature based on tdie and vobj
 *
 * @param tDie temperature of the die in Kelvin
 * @param vObj object in Volts
 *
 * @return temperature value in Celsius
 *
 */
float Calculate_Temp(float *tDie_K, float *vObj_V)

{
    /* Calculate TMP006. This needs to be reviewed and calibrated by TMP group */
	float SS0 = 6.4e-14;       /* Default S0 cal value */
	float a1 = 1.75e-3;
	float a2 = -1.678e-5;
	float b0 = -2.94e-5;
	float b1 = -5.7e-7;
	float b2 = 4.63e-9;
	float c2 = 13.4;
	float Tref = 298.15;
	if (*vObj_V >32127){
	*vObj_V=*vObj_V-65535;
	};
	
//Serial.print(*tDie_K);
//Serial.print(" ");
//Serial.print(*vObj_V);
//Serial.print(" "); 
    //float correctedtemp = *tDie_K - Tref;
    float S = SS0*(1+a1*(*tDie_K - Tref)+a2*(*tDie_K - Tref)*(*tDie_K - Tref));
    float Vos = b0 + b1*(*tDie_K - Tref) + b2*(*tDie_K - Tref)*(*tDie_K - Tref);

    //float correctedvolt = *vObj_V - Vos;
    float fObj = *vObj_V *.00000015625 - Vos + c2*(*vObj_V * .00000015625 - Vos)*(*vObj_V * .00000015625 - Vos);
	float Tobj = sqrt(sqrt(((*tDie_K)*(*tDie_K)*(*tDie_K)*(*tDie_K)) + (fObj/S)));



    return (Tobj - 273.15);
}


