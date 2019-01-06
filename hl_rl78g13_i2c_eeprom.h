/*
	(C) 2016 hikarimo
*/

#ifndef _HL_RL78G13_I2C_EEPROM_H_
#define _HL_RL78G13_I2C_EEPROM_H_

#include "hl_common.h"

extern void I2C_EEPROM_Read(uint8_t slaveAddr ,uint16_t eepromAddr, uint8_t *pData, uint16_t len);
extern void I2C_EEPROM_Write(uint8_t slaveAddr ,uint16_t eepromAddr, uint8_t *pData, uint16_t len);

#endif	/* _HL_RL78G13_I2C_EEPROM_H_ */
