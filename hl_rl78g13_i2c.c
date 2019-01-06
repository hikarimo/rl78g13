/*
	(C) 2016 hikarimo
*/

/*
	�V���A���E�A���C�E���j�b�g0�̃`���l��2���g�p

	�g�p���j�b�g�A�`���l��
	+----------+----------+-------+-------+-------+----+
	| ���j�b�g | �`���l�� |  CSI  |  UART |��I2C  |    |
	+----------+----------+-------+-------+-------+----+
	| 0        | 0        | CSI00 | UART0 | IIC00 |    |
	| 0        | 1        | CSI01 | UART0 | IIC01 |    |
	| 0        | 2        | CSI10 | UART1 | IIC10 | �� |
	| 0        | 3        | CSI11 | UART1 | IIC11 |    |
	+----------+----------+-------+-------+-------+----+
	| 1        | 0        | CSI20 | UART2 | IIC20 |    |
	| 1        | 1        | CSI21 | UART2 | IIC21 |    |
	+----------+----------+-------+-------+-------+----+

	�g�p�N���b�N
	CK00�A��CK01�ACK10�ACK11

	fCLK�|����(SPS0:PRS0x)�|CK00�|�I��(SMR02)�|fMCK�|CKS����(SDR02)�|fTCLK
        �_����(SPS0:PRS1x)�|CK01�^

	�g�p�s��
	P04/SCK10/��SCL10
	P03/ANI16/SI10/RxD1/��SDA10
*/
#pragma interrupt INTIIC10 i2c_Interrupt_IIC01

#include "hl_common.h"
#include <stdio.h>
#include <stdarg.h>
#include "hl_rl78g13_i2c.h"

#define W_MODE(ADDRESS)	((ADDRESS << 1)&0xFE)				/* Write */
#define R_MODE(ADDRESS)	((ADDRESS << 1)|0x01)				/* Read */
#define I2C_MASTER_FLAG_CLEAR    (0x00U)
#define I2C_SEND_FLAG            (0x01U)
#define I2C_RECEIVE_FLAG         (0x02U)
#define I2C_SENDED_ADDRESS_FLAG  (0x04U)

static uint8_t	*sp_i2c_TxData;								/* ���M�f�[�^ */
static uint16_t s_i2c_TxCount_IIC01;						/* ���M�f�[�^�̎c�� */
static uint8_t	*sp_i2c10_RxData;							/* ��M�f�[�^ */
static uint16_t	s_i2c_RxCount;								/* ��M�σf�[�^�� */
static uint16_t	s_i2c_RxLength;								/* ��M�f�[�^�� */
static uint8_t	s_i2c_MasterStatus;							/* ����M��� */
static uint16_t	s_i2c_Clock;								/* I2C����N���b�N */
static bool_t	s_I2C_IsReadingWriteing_IIC01;				/* �ǂݏ��������̏�� */
static void (*sp_i2c_CallbackMasterSend_IIC01)(void);		/* ���M�������R�[���o�b�N */
static void (*sp_i2c_CallbackMasterReceive_IIC01)(void);	/* ��M�������R�[���o�b�N */

static void i2c_CallbackMasterError_IIC01(MD_STATUS flag);
static int16_t i2c_CalcFrequencyDivision(uint16_t bps, uint16_t fCLK, uint8_t *prs, uint16_t *sdr);
static void i2c_Wait(void);
static void i2c_SendEnd_IIC01(void);
static void i2c_ReceiveEnd_IIC01(void);

/*
	I2C�ݒ�

	clk : I2C����N���b�N(kHz)
*/
void I2C_Setup_IIC01(uint16_t clk)
{
	uint8_t		prs;
	uint16_t	sdr;
	uint16_t	sps;

	s_i2c_Clock = clk;
	s_I2C_IsReadingWriteing_IIC01 = FALSE;

	/* ���ӃC�l�[�u���E���W�X�^(PER0) */
	/*
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
	| RTCEN |IICA1EN| ADCEN |IICA0EN| SAU1EN| SAU0EN| TAU1EN| TAU0EN|
	+-------+-------+-------+-------+-------+-------+-------+-------+
	*/
	/* �V���A���E�A���C�E���j�b�g0�̓��̓N���b�N�����̐��� */
	SAU0EN = 1u;		/* �N���b�N���� */

	/* �N���b�N����҂� */
	NOP();
	NOP();
	NOP();
	NOP();

	/* PRS�ASDR���W�X�^�l�Z�o */
	if (i2c_CalcFrequencyDivision(clk, HW_FCLK, &prs, &sdr) != 0)
	{
		/* �ݒ�ł��Ȃ����� */
		while(1){
			;
		}
	}

	/* �V���A���E�N���b�N�I�����W�X�^(SPSm) */
	/*
	   15    14    13    12    11    10     9     8     7     6     5     4     3     2     1     0
	+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
	|   0 |   0 |   0 |   0 |   0 |   0 |   0 |   0 |PRS13|PRS12|PRS11|PRS10|PRS03|PRS02|PRS01|PRS00|
	+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
	PRS13,12,11,10(���j�b�g1)
	  0000: CK01 = fCLK/2^0
	  0001: CK01 = fCLK/2^1
	  0010: CK01 = fCLK/2^2
	    :
	  1111: CK01 = fCLK/2^15
	PRS03,02,01,00(���j�b�g0)
	  0000: CK00 = fCLK/2^0
	  0001: CK00 = fCLK/2^1
	  0010: CK00 = fCLK/2^2
	    :
	  1111: CKm0 = fCLK/2^15
	*/
	/* ���j�b�g0��CK01�̕�����ݒ� */
	sps = ((SPS0 & 0xff0f) | (prs << 4));
	SPS0 = sps;

	/* �V���A���E�`���l����~���W�X�^(STm) */
	/*
	   15    14    13    12    11    10     9     8     7     6     5     4     3     2     1     0
	+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
	|   0 |   0 |   0 |   0 |   0 |   0 |   0 |   0 |   0 |   0 |   0 |   0 | ST03| ST02| ST01| ST00|
	+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
	ST03
	  0: �g���K�����ASE03��0�N���A�A�ʐM������~(�`���l��0)
	ST02
	  0: �g���K�����ASE02��0�N���A�A�ʐM������~(�`���l��1)
	ST01
	  0: �g���K�����ASE01��0�N���A�A�ʐM������~(�`���l��2)
	ST00
	  0: �g���K�����ASE00��0�N���A�A�ʐM������~(�`���l��3)
	*/
	ST0 |= 0x0004;		/* �`���l��2�̓�����~ */

	/* ���荞�݋֎~ */
	IICMK10 = 1u;	 	/* �`���l��2�̊��荞�݋֎~(INTIIC10) */

	/* ���荞�ݗv���t���O������ */
	IICIF10 = 0u;		/* �`���l��2�̊��荞�݃t���O(INTIIC10) */

	/* ���荞�ݗD�揇�ʎw��(INTIIC10) */
	/*
	+---------+---------+-----------+
	| IICPR110| IICPR010| �D�惌�x��|
	+---------+---------+-----------+
	|      0  |      0  |     0(��) |
	|      0  |      1  |     1     |
	|      1  |      0  |     2     |
	|      1  |      1  |     3(��) |
	+---------+---------+-----------+
	*/
	/* ���荞�ݗD�惌�x��3 */
	IICPR110 = 1u;
	IICPR010 = 1u;

	/* �V���A���E�t���O�E�N���A�E�g���K�E���W�X�^(SIRmn) */
	/*
	   15    14    13    12    11    10     9     8     7     6     5     4     3     2     1     0
	+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
	|   0 |   0 |   0 |   0 |   0 |   0 |   0 |   0 |   0 |   0 |   0 |   0 |   0 | FECT| PECT| OVCT|
	+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
	FECT
	  0: �N���A���Ȃ�
	  1: �t���[�~���O�E�G���[�E�t���O��0�N���A
	PECT
	  0: �N���A���Ȃ�
	  1: �p���e�B�E�G���[�E�t���O��0�N���A
	OVCT
	  0: �N���A���Ȃ�
	  1: �I�[�o�[�����E�G���[�E�t���O��0�N���A
	*/
	/* ���j�b�g0�̃`���l��2 */
	SIR02 = 0x0003;	/* �p���e�B�E�G���[�E�t���O�A�I�[�o�[�����E�G���[�E�t���O���N���A */

	/* �V���A���E���[�h�E���W�X�^(SMRmn) */
	/*
	   15    14    13    12    11    10     9     8     7     6     5     4     3     2     1     0
	+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
	| CKS | CCS |   0 |   0 |   0 |   0 |   0 | STS |   0 | SIS |   1 |   0 |   0 | MD2 | MD1 | MD0 |
	+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
	CKS
	  0: fMCK��CK00���g�p
	  1: fMCK��CK01���g�p
	CCS
	  0: fTCLK��CKS�r�b�g�Ŏw�肵������N���b�NfMCK�̕����N���b�N���g�p
	  1: fTCLK��SCKp�[�q����̓��̓N���b�NfSCK���g�p(CSI���[�h�̃X���[�u�]��)
	STS
	  0: �X�^�[�g�E�g���K�v���Ƃ��ă\�t�g�E�F�A�E�g���K�̂ݗL��(CSI�AUART���M�A�Ȉ�I2C���ɑI��)
	  1: �X�^�[�g�E�g���K�v���Ƃ���RxD0�[�q�̗L���G�b�W(UART��M���ɑI��)
	SIS
	  0: ����������G�b�W���X�^�[�g�E�r�b�g�Ƃ��Č��o
	  1: �����オ��G�b�W���X�^�[�g�E�r�b�g�Ƃ��Č��o(���͂����ʐM�f�[�^�́A���])
	MD2,MD1
	  00: CSI���[�h
	  01: UART���[�h
	  10: �Ȉ�I2C���[�h
	  11: �ݒ�֎~
	MD0
	  0: ���荞�ݗv���A�]���������荞��
	  1: ���荞�ݗv���A�o�b�t�@�󂫊��荞��
	*/
	/* ���j�b�g0�̃`���l��2 */
	SMR02 = 0x8024;		/* fTMCK��CK01�A�Ȉ�I2C���[�h�A�]���������荞�� */

	/* �V���A���ʐM����ݒ背�W�X�^(SCRmn) */
	/*
	   15    14    13    12    11    10     9     8     7     6     5     4     3     2     1     0
	+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
	| TXE | RXE | DAP | CKP |   0 | EOC | PTC1| PTC0| DIR |   0 | SLC1| SLC0|   0 |   1 | DLS1| DLS0|
	+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
	TXE,RXE
	  00: �ʐM�֎~
	  01: ��M�̂�
	  10: ���M�̂�
	  11: ����M
	DAP,CKP
	  00: UART���[�h(�Ȉ�I2C���[�h���́A00�w��)
	EOC
	  0: CSI���[�h(�Ȉ�I2C���[�h�AUART���M���́A0�w��)
	PTC1,PTC0
	  00: �p���e�B�E�r�b�g�Ȃ�(CSI�A�Ȉ�I2C��00��ݒ�)
	  01: 0�p���e�B
	  10: �����p���e�B
	  11: ��p���e�B
	DIR
	  0: MSB�t�@�[�X�g�œ��o��(�Ǘ�I2C�́A0�w��)
	  1: LSB�t�@�[�X�g�œ��o��
	SLC1,SLC0
	  00: �X�g�b�v�E�r�b�g�Ȃ�(CSI�́A00�w��)
	  01: �X�g�b�v�E�r�b�g���A1�r�b�g(UART�A�Ȉ�I2C�́A01�w��)
	  10: �X�g�b�v�E�r�b�g���A2�r�b�g(���j�b�g0�̃`���l��0�A2�ƃ��j�b�g1�̃`���l��0�A2�̂�)
	  11: �ݒ�֎~
	DLS1,DLS0
	  00: �ݒ�֎~
	  01: 9�r�b�g�E�f�[�^��(UART���[�h���̂�)
	  10: 7�r�b�g�E�f�[�^��
	  11: 8�r�b�g�E�f�[�^��(�Ȉ�I2C�́A11�w��)
	*/
	/* ���j�b�g0�̃`���l��2 */
	SCR02 = 0x0017;		/* �ʐM�֎~ */

	/* �V���A���E�f�[�^�E���W�X�^(SDRmn) */
	/*
	   15    14    13    12    11    10     9     8     7     6     5     4     3     2     1     0
	+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
	|      ����N���b�N�̕����ݒ�(7�r�b�g)    |            ����M�o�b�t�@(9�r�b�g)                  |
	+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
	����N���b�N�̕����ݒ�
	  0000000: fMCK/2
	  0000001: fMCK/4
	  0000010: fMCK/6
	  0000011: fMCK/8
	   :
	  1111110: fMCK/254
	  1111111: fMCK/256
	*/
	/* ���j�b�g0�̃`���l��2 */
	SDR02 = (sdr << 9);

	/* �V���A���o�̓��W�X�^(SOm) */
	/*
	   15    14    13    12    11    10     9     8     7     6     5     4     3     2     1     0
	+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
	|   0 |   0 |   0 |   0 |CKO03|CKO02|CKO01|CKO00|   0 |   0 |   0 |   0 | SO03| SO02| SO01| SO00|
	+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
	CKO03,CKO02,CKO01,CKO00
	  0: �V���A���E�N���b�N�o�͒l��0�A1:1
	SO03,SO02,SO01,SO00
	  0: �V���A���E�f�[�^�o�͒l��0�A1:1
	*/
	SO0 |= 0x0404;		/* �`���l��1�̃V���A���E�f�[�^�o�͒l�A�V���A���E�N���b�N�o�͒l��HIGH */

	/* Set SCL10, SDA10 pin */

	/* �|�[�g�E���[�h�E�R���g���[���E���W�X�^0 */
	/*
	    7     6     5     4     3     2     1     0
	+-----+-----+-----+-----+-----+-----+-----+-----+
	|   1 |   1 |   1 |   1 |PMC03|PMC02|PMC01|PMC00|
	+-----+-----+-----+-----+-----+-----+-----+-----+
	PMC03,PMC02,PMC01,PMC00
	  0: �f�W�^�����o�́A1:�A�����O����
	*/
	PMC0 &= 0xF7u;		/* P03���f�W�^�����o�� */

	/* �|�[�g�E���W�X�^ */
	P0 |= 0x18u;		/* P04�AP03��HIGH */

	/* �|�[�g�E���[�h�E���W�X�^ */
	/* 0: �o�̓��[�h�A1:���̓��[�h */
	PM0 &= 0xE7u;		/* P04���o�́AP03���o�� */

	/* �|�[�g�o�̓��[�h�E���W�X�^ */
	POM0 |= 0x18;		/* b0-b7 0:�ʏ�o�̓��[�h�A1:N-ch�I�[�v���E�h���C���o�� */
}
/*
	I2C��~
*/
void I2C_Stop_IIC01(void)
{
	IICMK10 = 1u;		/* �`���l��2�̊��荞�݋֎~(INTIIC10) */
	ST0 |= 0x0004;		/* �`���l��2�̓�����~ */
	/* ���荞�ݗv���t���O������ */
	IICIF10 = 0u;		/* �`���l��2�̊��荞��(INTIIC10) */
}
/*
	���M

	adr : �X���[�u�A�h���X
	data: ���M�f�[�^
	len : ���M�f�[�^�o�C�g��
	cbFunc: ���M�ώ��̃R�[���o�b�N�֐�
*/
void I2C_MasterSend_IIC01(uint8_t adr, uint8_t * const data, uint16_t len, void (*cbFunc)(void))
{
	s_i2c_MasterStatus = I2C_SEND_FLAG;
	sp_i2c_CallbackMasterSend_IIC01 = cbFunc;

	/* Set paramater */
	s_i2c_TxCount_IIC01 = len;
	sp_i2c_TxData = data;

	/* ���荞�ݗv���t���O������ */
	IICIF10 = 0u;		/* �`���l��2�̊��荞��(INTIIC10) */
	IICMK10 = 0u;	 	/* �`���l��2�̊��荞�݋���(INTIIC10) */

	SIO10 = W_MODE(adr);

	if (sp_i2c_CallbackMasterSend_IIC01 == NULL) {
		while(s_i2c_MasterStatus != I2C_MASTER_FLAG_CLEAR)
		{
			;
		}
	}
}
/*
	��M

	adr  : �X���[�u�A�h���X
	rxBuf: ��M�f�[�^�o�b�t�@
	rxNum: ��M�f�[�^�o�C�g��
	cbFunc: ��M�ώ��̃R�[���o�b�N�֐�
*/
void I2C_MasterReceive_IIC01(uint8_t adr, uint8_t * const rxBuf, uint16_t rxNum, void (*cbFunc)(void))
{
	s_i2c_MasterStatus = I2C_RECEIVE_FLAG;
	sp_i2c_CallbackMasterReceive_IIC01 = cbFunc;

	s_i2c_RxLength = rxNum;
	s_i2c_RxCount = 0u;
	sp_i2c10_RxData = rxBuf;

	IICIF10 = 0u;		/* �`���l��2�̊��荞��(INTIIC10) */
	IICMK10 = 0u;	 	/* �`���l��2�̊��荞�݋���(INTIIC10) */

	SIO10 = R_MODE(adr);

	if (sp_i2c_CallbackMasterReceive_IIC01 == NULL) {
		while(s_i2c_MasterStatus != I2C_MASTER_FLAG_CLEAR)
		{
			;
		}
	}
}
/*
	�X�^�[�g�E�R���f�B�V����
*/
void I2C_StartCondition_IIC01(void)
{
	s_i2c_MasterStatus = I2C_MASTER_FLAG_CLEAR;
	s_I2C_IsReadingWriteing_IIC01 = TRUE;

	SCR02 &= ~(0xc000);	/* �ʐM�֎~ */
	SCR02 |= 0x8000;	/* ���M�̂� */

	SO0 &= ~(0x0004);	/* SDA��Low�ɂ��� */

	/* �҂� */
	i2c_Wait();

	SO0 &= ~(0x0400);	/* SCL��Low�ɂ��� */

	/* �V���A���o�͋����W�X�^ */
	SOE0 |= (0x0004);	/* �`���l��2�̏o�͂����� */
	/* �V���A���E�`���l���J�n���W�X�^ */
	SS0 |= 0x0004u;		/* �`���l��2�̒ʐM������ */
}
/*
	���X�^�[�g�E�R���f�B�V����
*/
void I2C_ReStartCondition_IIC01(void)
{
	s_i2c_MasterStatus = I2C_MASTER_FLAG_CLEAR;

	/* �V���A���E�`���l����~���W�X�^ */
	ST0 |= (0x0004);	/* �`���l��2�̓�����~ */
	/* �V���A���o�͋����W�X�^ */
	SOE0 &= ~(0x0004);	/* �`���l��2�̏o�͂��֎~ */

	SO0 |= (0x0404);	/* SDA,SCL��High�ɂ��� */

	/* �҂� */
	i2c_Wait();

	SO0 &= ~(0x0004);	/* SDA��Low�ɂ��� */

	/* �҂� */
	i2c_Wait();

	SO0 &= ~(0x0400); 	/* SCL��Low�ɂ��� */

	/* �V���A���o�͋����W�X�^ */
	SOE0 |= (0x0004);	/* �`���l��2�̏o�͂����� */
	/* �V���A���E�`���l���J�n���W�X�^ */
	SS0 |= 0x0004u;		/* �`���l��2�̒ʐM������ */
}
/*
	�X�g�b�v�E�R���f�B�V����
*/
void I2C_StopCondition_IIC01(void)
{
	int i;
	/* �V���A���E�`���l����~���W�X�^ */
	ST0 |= (0x0004);	/* �`���l��2�̓�����~ */
	/* �V���A���o�͋����W�X�^ */
	SOE0 &= ~(0x0004);	/* �`���l��2�̏o�͂��֎~ */

	SO0 &= ~(0x0004);	/* SDA��Low�ɂ��� */
	SO0 |= (0x0400); 	/* SCL��High�ɂ��� */

	/* �҂� */
	i2c_Wait();

	SO0 |= (0x0004); 	/* SDA��High�ɂ��� */

	/* �҂� */
	i2c_Wait();

	s_i2c_MasterStatus = I2C_MASTER_FLAG_CLEAR;
	s_I2C_IsReadingWriteing_IIC01 = FALSE;
}
/*
	��������

	adr: �X���[�u�A�h���X
	data: �������݃f�[�^
	len: �������݃f�[�^�o�C�g��
*/
void I2C_Write_IIC01(uint8_t adr, uint8_t *data, uint16_t len)
{
	/* �X�^�[�g�E�R���f�B�V���� */
	I2C_StartCondition_IIC01();
	/* ���M */
	I2C_MasterSend_IIC01(adr, data, len, I2C_StopCondition_IIC01);
}
/*
	�ǂݍ���

	adr: �X���[�u�A�h���X
	data: �������݃f�[�^
	len: �������݃f�[�^�o�C�g��
*/
void I2C_Read_IIC01(uint8_t adr ,uint8_t *data, uint16_t len)
{
	/* �X�^�[�g�E�R���f�B�V���� */
	I2C_StartCondition_IIC01();
	/* ��M */
	I2C_MasterReceive_IIC01(adr ,data, len, I2C_StopCondition_IIC01);
}
/*
	����M���
*/
bool_t I2C_IsReadingWriteing_IIC01(void)
{
	return s_I2C_IsReadingWriteing_IIC01;
}
/*
	���M�������荞�݃n���h��
	�uCS+ for CA,CX V3.01.00 [19 Aug 2015] �R�[�h�����^�[�q�}�v���O�C�� V1.04.04.02 [31 Jul 2015]���v
*/
__interrupt static void i2c_Interrupt_IIC01(void)
{
	if ((SSR02L & 0x02) && (s_i2c_TxCount_IIC01 != 0u)) {
		i2c_CallbackMasterError_IIC01(MD_NACK);
	}
	else if((SSR02L & 0x01) && (s_i2c_TxCount_IIC01 != 0u)) {
		i2c_CallbackMasterError_IIC01(MD_OVERRUN);
	}
	else {
		/* Control for master send */
		if ((s_i2c_MasterStatus & I2C_SEND_FLAG) == 1u) {
			if (s_i2c_TxCount_IIC01 > 0u) {
				SIO10 = *sp_i2c_TxData;
				sp_i2c_TxData++;
				s_i2c_TxCount_IIC01--;
			}
			else {
				i2c_SendEnd_IIC01();
			}
		}
		/* Control for master receive */
		else {
			if ((s_i2c_MasterStatus & I2C_SENDED_ADDRESS_FLAG) == 0u) {
				ST0 |= 0x0004;
				SCR02 &= ~(0xc000);
				SCR02 |= (0x4000);
				SS0 |= (0x0004);
				s_i2c_MasterStatus |= I2C_SENDED_ADDRESS_FLAG;
				
				if (s_i2c_RxLength == 1u) {
					SOE0 &= ~(0x0004);	 /* disable IIC10 out */
				}
				SIO10 = 0xff;
			}
			else {
				if (s_i2c_RxCount < s_i2c_RxLength) {
					*sp_i2c10_RxData = SIO10;
					sp_i2c10_RxData++;
					s_i2c_RxCount++;
					
					if (s_i2c_RxCount == (s_i2c_RxLength - 1u)) {
						SOE0 &= ~(0x0004);	 /* disable IIC10 out */
						SIO10 = 0xff;
					}
					else if (s_i2c_RxCount == s_i2c_RxLength) {
						i2c_ReceiveEnd_IIC01();
					}
					else {
						SIO10 = 0xff;
					}
				}
			}
		}
	}
}
/*
	���M�I�����̏���
*/
void i2c_SendEnd_IIC01(void)
{
	s_i2c_MasterStatus = I2C_MASTER_FLAG_CLEAR;
	if (sp_i2c_CallbackMasterSend_IIC01) {
		(*sp_i2c_CallbackMasterSend_IIC01)();
		sp_i2c_CallbackMasterSend_IIC01 = NULL;
	}
}
/*
	��M�I�����̏���
*/
void i2c_ReceiveEnd_IIC01(void)
{
	s_i2c_MasterStatus = I2C_MASTER_FLAG_CLEAR;

	if (sp_i2c_CallbackMasterReceive_IIC01) {
		(*sp_i2c_CallbackMasterReceive_IIC01)();
		sp_i2c_CallbackMasterReceive_IIC01 = NULL;
	}
}
/*
	����M�G���[�p�R�[���o�b�N�֐�

	flag: �G���[���
*/
static void i2c_CallbackMasterError_IIC01(MD_STATUS flag)
{
	s_i2c_MasterStatus = I2C_MASTER_FLAG_CLEAR;
}
/*
	�{�[���[�g�ݒ�p���W�X�^�l�v�Z

	bps:  �]�����[�g(kHz)
	fCLK: ���C���N���b�N(kHz)
	prs:  PRS���W�X�^�̎Z�o����
	sdr:  SDR���W�X�^�̎Z�o����
*/
static int16_t i2c_CalcFrequencyDivision(uint16_t bps, uint16_t fCLK, uint8_t *prs, uint16_t *sdr)
{
	uint8_t		i;
	uint32_t	c_sdr;
	uint16_t	ret = -1;

	/* �u12.8.5 �]�����[�g�̎Z�o�v����t�Z */
	for (i = 1; i <= 16; i++)
	{
		c_sdr = ((fCLK >> i) / bps);
		if ((1 <= c_sdr) && (c_sdr <= 128))
		{
			*prs = (i - 1);
			*sdr = (uint16_t)(c_sdr - 1);
			ret = 0;
			break;
		}
	}
	return ret;
}
/*
	�҂�
*/
void i2c_Wait(void)
{
	Wait(1000 / s_i2c_Clock);
}
