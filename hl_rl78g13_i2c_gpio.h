/* 
	(C) 2016 hikarimo
*/

#ifndef _HL_RL78G13_I2C_GPIO_H_
#define _HL_RL78G13_I2C_GPIO_H_

#include "hl_common.h"

extern void I2C_GPIO_SetReg(uint8_t slaveAddr, uint8_t reg, uint8_t data);
extern void I2C_GPIO_Setup(uint8_t slaveAddr);
extern void I2C_GPIO_Output(uint8_t slaveAddr, uint8_t data);

#endif /* _HL_RL78G13_I2C_GPIO_H_ */
