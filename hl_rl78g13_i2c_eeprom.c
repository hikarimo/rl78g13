/*
	(C) 2016 hikarimo
*/
/*
	MICROCHIP 24LCXX�p(32KBIT-512KBIT)
	I2C �V���A��EEPROM
*/

#include "hl_common.h"
#include "hl_rl78g13_i2c.h"
#include "hl_rl78g13_i2c_eeprom.h"

#define EEPROM_PAGE_SIZE	(64u)
#define EEPROM_ADDR_SIZE	(2u)

static void i2c_Eeprom_wait(void);
static void i2c_Eeprom_CallbackReadSendAddrEnd(void);
static void i2c_Eeprom_CallbackReadWriteEnd(void);

static uint8_t i2c_Eeprom_ReadSlaveAddr;
static uint8_t *i2c_Eeprom_ReadAddr;
static uint16_t i2c_Eeprom_ReadLength;

/*
	EEPROM�ǂݍ���
	�y�[�W���ׂ����ꍇ�̃G���[�`�F�b�N�͂��ĂȂ�
*/
void I2C_EEPROM_Read(uint8_t slaveAddr ,uint16_t eepromAddr, uint8_t *pData, uint16_t len)
{
	uint8_t eepromAddrAry[EEPROM_ADDR_SIZE];

	i2c_Eeprom_ReadSlaveAddr = slaveAddr;
	i2c_Eeprom_ReadAddr = pData;
	i2c_Eeprom_ReadLength = len;

	/* EEPROM�A�h���X�ݒ� */
	eepromAddrAry[0] = (uint8_t)((eepromAddr >> sizeof(uint8_t)) & 0xff);
	eepromAddrAry[1] = (uint8_t)(eepromAddr & 0xff);
	/* �X�^�[�g�E�R���f�B�V���� */
	I2C_StartCondition_IIC01();
	/* ���M */
	I2C_MasterSend_IIC01(slaveAddr, eepromAddrAry, EEPROM_ADDR_SIZE, i2c_Eeprom_CallbackReadSendAddrEnd);
}
/*
	EEPROM��������
	�y�[�W���ׂ����ꍇ�̃G���[�`�F�b�N�͂��ĂȂ�
*/
void I2C_EEPROM_Write(uint8_t slaveAddr ,uint16_t eepromAddr, uint8_t *pData, uint16_t len)
{
	int16_t i;
	uint8_t sendData[EEPROM_PAGE_SIZE + EEPROM_ADDR_SIZE];

	/* ���M�f�[�^�쐬 */
	sendData[0] = (uint8_t)((eepromAddr >> sizeof(uint8_t)) & 0xff);
	sendData[1] = (uint8_t)(eepromAddr & 0xff);
	for (i = 0; i < len; i++) {
		sendData[i + EEPROM_ADDR_SIZE] = pData[i];
	}
	/* �X�^�[�g�E�R���f�B�V���� */
	I2C_StartCondition_IIC01();
	/* ���M */
	I2C_MasterSend_IIC01(slaveAddr, sendData, EEPROM_ADDR_SIZE + len, i2c_Eeprom_CallbackReadWriteEnd);
}
static void i2c_Eeprom_CallbackReadSendAddrEnd(void)
{
	/* ���X�^�[�g�E�R���f�B�V���� */
	I2C_ReStartCondition_IIC01();
	/* ��M */
	I2C_MasterReceive_IIC01(i2c_Eeprom_ReadSlaveAddr, i2c_Eeprom_ReadAddr, i2c_Eeprom_ReadLength, i2c_Eeprom_CallbackReadWriteEnd);
}
static void i2c_Eeprom_CallbackReadWriteEnd(void)
{
	/* �X�g�b�v�E�R���f�B�V���� */
	I2C_StopCondition_IIC01();
	/* �҂� */
	i2c_Eeprom_wait();
}
/*
	�҂�
*/
void i2c_Eeprom_wait(void)
{
	uint16_t	i;

	for (i = 0; i <= 20000; i++)
	{
		NOP();
	}
}
