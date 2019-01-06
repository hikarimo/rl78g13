/*
	(C) 2016 hikarimo
*/

/*
	MICROCHIP MCP23017用
	16-Bit I/O Expander with Serial Interface
*/

#include "hl_common.h"
#include <stdio.h>
#include "hl_rl78g13_i2c.h"
#include "hl_rl78g13_i2c_gpio.h"

#define GPIO_REG_IODIRA	((uint8_t)(0x00))
#define GPIO_REG_IODIRB	((uint8_t)(0x01))
#define GPIO_REG_GPIOA	((uint8_t)(0x12))
#define GPIO_REG_GPIOB	((uint8_t)(0x13))

/*
	GPIOレジスタ設定
*/
void I2C_GPIO_SetReg(uint8_t slaveAddr, uint8_t reg, uint8_t data)
{
	uint8_t sendData[2];

	/* レジスタアドレス送信 */
	sendData[0] = reg;
	/* スタート・コンディション */
	I2C_StartCondition_IIC01();
	/* 送信 */
	I2C_MasterSend_IIC01(slaveAddr, sendData, 1, NULL);
	/* リスタート・コンディション */
	I2C_ReStartCondition_IIC01();

	/* データ送信 */
	sendData[0] = data;
	/* 送信 */
	I2C_MasterSend_IIC01(slaveAddr ,sendData, 1, NULL);
	/* ストップ・コンディション */
	I2C_StopCondition_IIC01();
}
/*
	初期化
*/
void I2C_GPIO_Setup(uint8_t slaveAddr)
{
	I2C_GPIO_SetReg(slaveAddr, GPIO_REG_IODIRA, 0x00);
}
/*
	出力
*/
void I2C_GPIO_Output(uint8_t slaveAddr, uint8_t data)
{
	I2C_GPIO_SetReg(slaveAddr, GPIO_REG_GPIOA, data);
}
