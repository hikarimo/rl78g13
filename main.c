#include "hl_common.h"
#if 0
#include "hl_rl78g13_wdt.h"
#include "hl_rl78g13_interval_timer.h"
#include "hl_rl78g13_uart.h"
#endif
#include "hl_rl78g13_i2c.h"
#include "hl_rl78g13_uart.h"
#include "hl_dfw.h"
#if 0
#include "hl_rl78g13_i2c_eeprom.h"
#include "hl_rl78g13_i2c_gpio.h"
#endif

uint8_t uartRcvData[100];

unsigned char wData[5] = "AB\n";
unsigned char rData[5];
byte_t dfData[5] = "1234";

void Reply(void)
{
	int16_t idx;

	if (Uart_GetReceiveLen_IIC00() <=0)
	{
		return;
	}

	idx = Uart_GetReceiveLen_IIC00() - 1;

	/* ���s�R�[�h����菜�� */
	uartRcvData[idx] = '\0';
	if ((idx > 0) && (uartRcvData[idx - 1] == '\r'))
	{
		uartRcvData[idx - 1] = '\0';
	}

	/* ��M������������J�b�R�t���đ��M */
	Uart_Printf_IIC00("(%s)\n", uartRcvData);
}
int main(void)
{
	int32_t i;
	int d;

	EI();	// ���荞�݋���

//	dfw_Erase();
	dfw_Write(0, dfData, 5);

#if 1	/* UART����M�e�X�g */
	Uart_Start_IIC00();
	Uart_Receive_IIC00(uartRcvData, sizeof(uartRcvData));
	while(1)
	{
		if (!Uart_IsReceiving_IIC00())
		{
			Reply();
			Uart_Receive_IIC00(uartRcvData, sizeof(uartRcvData));
		}
	}
#endif

#if 0	/* I2C�}�X�^�������݃e�X�g */
	while(1) {
		if (!I2C_IsReadingWriteing_IIC01()) {
			I2C_Write_IIC01(8, wData, 3);
		}
		Wait(1000);
	}
#endif
#if 0	/* I2C�}�X�^�ǂݍ��݃e�X�g */
	while(1) {
		if (!I2C_IsReadingWriteing_IIC01()) {
			I2C_Read_IIC01(8, rData, 3);
		}
		Wait(1000);
	}
#endif
	return 0;
}
