/*
	(C) 2016 hikarimo
*/

#ifndef _HL_RL78G13_I2C_H_
#define _HL_RL78G13_I2C_H_

#include "hl_common.h"

extern void I2C_Setup_IIC01(uint16_t clk);
extern void I2C_Stop_IIC01(void);
extern void I2C_StartCondition_IIC01(void);
extern void I2C_ReStartCondition_IIC01(void);
extern void I2C_StopCondition_IIC01(void);
extern void I2C_MasterSend_IIC01(uint8_t adr, uint8_t * const data, uint16_t len, void (*cbFunc)(void));
extern void I2C_MasterReceive_IIC01(uint8_t adr, uint8_t * const rxBuf, uint16_t rxNum, void (*cbFunc)(void));
extern void I2C_Write_IIC01(uint8_t adr, uint8_t *data, uint16_t len);
extern void I2C_Read_IIC01(uint8_t adr ,uint8_t *data, uint16_t len);
extern bool_t I2C_IsReadingWriteing_IIC01(void);

#endif	/* _HL_RL78G13_I2C_H_*/
