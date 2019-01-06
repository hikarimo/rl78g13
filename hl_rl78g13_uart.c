/*
	(C) 2016 hikarimo
*/

/*
	�V���A���E�A���C�E���j�b�g0�̃`���l��0�𑗐M�A�`���l��1����M�Ƃ��Ďg�p

	�g�p���j�b�g�A�`���l��
	+----------+----------+-------+-------+-------+----+
	| ���j�b�g | �`���l�� |  CSI  |��UART |  I2C  |    |
	+----------+----------+-------+-------+-------+----+
	| 0        | 0        | CSI00 | UART0 | IIC00 | �� |
	| 0        | 1        | CSI01 | UART0 | IIC01 | �� |
	| 0        | 2        | CSI10 | UART1 | IIC10 |    |
	| 0        | 3        | CSI11 | UART1 | IIC11 |    |
	+----------+----------+-------+-------+-------+----+
	| 1        | 0        | CSI20 | UART2 | IIC20 |    |
	| 1        | 1        | CSI21 | UART2 | IIC21 |    |
	+----------+----------+-------+-------+-------+----+

	�g�p�s��
	44Pin	P12/SO00/��TxD0/TOOLTxD/(INTP5)/(TI05)/(TO05)
	45Pin	P11/SI00/��RxD0/TOOLRxD/SDA00/(TI06)/(TO06)
*/

#pragma interrupt INTST0 uart_InterruptSend_IIC00
#pragma interrupt INTSR0 uart_InterruptReceive_IIC00

#include "hl_common.h"
#include <stdio.h>
#include <stdarg.h>
#include "hl_rl78g13_uart.h"

static int16_t	s_uart_SendBuffCnt_IIC00;
static int16_t	s_uart_SendBuffStartIdx_IIC00;
static int16_t	s_uart_SendBuffEndIdx_IIC00;
static uint8_t	*sp_uart_ReceiveBuff_IIC00;
static uint16_t	s_uart_ReceiveBuffLen_IIC00;
static int16_t	s_uart_ReceiveBuffCnt_IIC00;
static int16_t	s_uart_ReceiveBuffIdx_IIC00;
static int8_t	s_uart_SendBuff_IIC00[UART0_SEND_BUFF_SIZE];
static int8_t	s_uart_PrintBuff_IIC00[UART0_PRINT_BUFF];
static bool_t	s_uart_isSending_IIC00;
static bool_t	s_uart_isReceiving_IIC00;
static bool_t	s_is_Txd0_Free;

static bool_t uart_IsReceiveEnd_IIC00(void);
static int16_t uart_CalcFrequencyDivision(uint32_t brr, uint16_t fCLK, uint8_t *prs, uint16_t *sdr);
static void uart_ReceiveEnd_IIC00(void);
static void uart_ReceiveError_IIC00(uint8_t err_type);
static void uart_SendEnd_IIC00(void);

__interrupt void uart_InterruptSend_IIC00(void);
__interrupt static void uart_InterruptReceive_IIC00(void);

/*
	UART�ݒ�
*/
void Uart_Setup_IIC00(uint32_t clk)
{
	uint8_t prs;
	uint16_t sdr;

	/* �V���A���E�A���C�E���j�b�g0�̓��̓N���b�N�����̐��� */
	SAU0EN = 1u;		/* 0:�N���b�N������~�A1:���� */

	/* �N���b�N����҂� */
	NOP();
	NOP();
	NOP();
	NOP();

	/* PRS�ASDR���W�X�^�l�Z�o */
	if (uart_CalcFrequencyDivision(clk, HW_FCLK, &prs, &sdr) != 0)
	{
		prs = 0x4;
		sdr = 0x67;
	}

	/* �V���A���E�N���b�N�I�����W�X�^ */
	/*
	   15    14    13    12    11    10     9     8     7     6     5     4     3     2     1     0
	+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
	|   0 |   0 |   0 |   0 |   0 |   0 |   0 |   0 |PRS13|PRS12|PRS11|PRS10|PRS03|PRS02|PRS01|PRS00|
	+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
	PRS13,12,11,10
	  0000: CK01 = fCLK/2^0
	  0001: CK01 = fCLK/2^1
	  0010: CK01 = fCLK/2^2
	    :
	  1111: CK01 = fCLK/2^15
	PRS03,02,01,00
	  0000: CK00 = fCLK/2^0
	  0001: CK00 = fCLK/2^1
	  0010: CK00 = fCLK/2^2
	    :
	  1111: CK00 = fCLK/2^15
	*/
	/* �`�����l���ɋ�������N���b�N(CK00��CK01)��ݒ� */
	SPS0 = (prs | (prs << 4));	/* CK00�ACK01�N���b�N�ݒ� */

	/* �V���A���E�`���l����~���W�X�^ */
	ST0 |= 0x03;		/* �`���l��0��1�̓�����~ */

	/* ���荞�݋֎~ */
	STMK0 = 1u;			/* ���M�������荞�݋֎~(INTST0) */
	SRMK0 = 1u;			/* ��M�������荞�݋֎~(INTSR0) */
	SREMK0 = 1u;		/* ��M�G���[���荞�݋֎~(INTSRE0) */

	/* ���荞�ݗv���t���O������ */
	STIF0 = 0u;			/* ���M�������荞�� */
	SRIF0 = 0u;			/* ��M�������荞�� */
	SREIF0 = 0u;		/* ��M�G���[���荞�� */

	/* ���荞�ݗD�揇�ʎw��(INTST0) */
	/*
	+---------+---------+-----------+
	|  STPR10 |  STPR00 | �D�惌�x��|
	+---------+---------+-----------+
	|      0  |      0  |     0(��) |
	|      0  |      1  |     1     |
	|      1  |      0  |     2     |
	|      1  |      1  |     3(��) |
	+---------+---------+-----------+
	*/
	STPR10 = 1u;
	STPR00 = 1u;

	/* ���荞�ݗD�揇�ʎw��(INTSR0) */
	/*
	+---------+---------+-----------+
	|  SRPR10 |  SRPR00 | �D�惌�x��|
	+---------+---------+-----------+
	|      0  |      0  |     0(��) |
	|      0  |      1  |     1     |
	|      1  |      0  |     2     |
	|      1  |      1  |     3(��) |
	+---------+---------+-----------+
	*/
	SRPR10 = 1u;
	SRPR00 = 1u;

	/* �V���A���E���[�h�E���W�X�^ */
	/*
	   15    14    13    12    11    10     9     8     7     6     5     4     3     2     1     0
	+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
	| CKS | CCS |   0 |   0 |   0 |   0 |   0 | STS |   0 | SIS |   1 |   0 |   0 | MD2 | MD1 | MD0 |
	+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
	CKS
	  0: SPS���W�X�^�Őݒ肵������N���b�NCK00
	  1: SPS���W�X�^�Őݒ肵������N���b�NCK01
	CCS
	  0: CKS�r�b�g�Ŏw�肵������N���b�NfMCK�̕����N���b�N
	  1: SCKp�[�q����̓��̓N���b�NfSCK(CSI���[�h�̃X���[�u�]��)
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
	SMR00 = 0x0022;		/* fMCK = CK00�A���������肪�X�^�[�g�E�r�b�g�AUART���[�h�A�]���������荞�� */
	SMR01 = 0x0122;		/* fMCK = CK00�ARxD0�[�q�ŃX�^�[�g�E�g���K�A���������肪�X�^�[�g�E�r�b�g�AUART���[�h�A�]���������荞�� */

	/* �V���A���ʐM����ݒ背�W�X�^ */
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
	SCR00 = 0x8097;		/* ���M�̂݁A�p���e�B�Ȃ��ALSB�t�@�[�X�g�A�X�g�b�v�E�r�b�g��1�A8�r�b�g�E�f�[�^�� */
	SCR01 = 0x4097;		/* ��M�̂݁A�p���e�B�Ȃ��ALSB�t�@�[�X�g�A�X�g�b�v�E�r�b�g��1�A8�r�b�g�E�f�[�^�� */

	/* �V���A���E�f�[�^�E���W�X�^ */
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
	SDR00 = (sdr << 9);		/* �Z�o���ʂ����7�r�b�g�ɐݒ� */
	SDR01 = (sdr << 9);		/* �Z�o���ʂ����7�r�b�g�ɐݒ� */

	/* �m�C�Y�E�t�B���^�����W�X�^ */
	/*
	    7     6     5     4     3     2     1     0
	+-----+-----+-----+-----+-----+-----+-----+-----+
	|   0 |SNFEN|   0 |SNFEN|   0 |SNFEN|   0 |SNFEN|
	|     |   30|     |   20|     |   10|     |   00|
	+-----+-----+-----+-----+-----+-----+-----+-----+
	SNFEN30
	  0: RxD3�[�q�̃m�C�Y�E�t�B���^OFF
	  1: RxD3�[�q�̃m�C�Y�E�t�B���^ON
	SNFEN20
	  0: RxD2�[�q�̃m�C�Y�E�t�B���^OFF
	  1: RxD2�[�q�̃m�C�Y�E�t�B���^ON
	SNFEN10
	  0: RxD1�[�q�̃m�C�Y�E�t�B���^OFF
	  1: RxD1�[�q�̃m�C�Y�E�t�B���^ON
	SNFEN00
	  0: RxD0�[�q�̃m�C�Y�E�t�B���^OFF
	  1: RxD0�[�q�̃m�C�Y�E�t�B���^ON
	*/
	NFEN0 |= 1;		/* RxD0�[�q�̃m�C�Y�E�t�B���^ON */

	/* �V���A���E�t���O�E�N���A�E�g���K�E���W�X�^ */
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
	SIR01 = 0x7;	/* �S���N���A */

	/* �V���A���o�̓��W�X�^ */
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
	SO0 |= 0x01;		/* �`���l��0�̃V���A���E�f�[�^�o�͒l��HIGH */

	/* �V���A���o�̓��x���E���W�X�^ */
	/*
	   15    14    13    12    11    10     9     8     7     6     5     4     3     2     1     0
	+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
	|   0 |   0 |   0 |   0 |   0 |   0 |   0 |   0 |   0 |   0 |   0 |   0 |   0 | SOL2|   0 | SOL0|
	+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
	SOL2
	  0: �`���l��2�̒ʐM�f�[�^�́A���̂܂܏o��
	  1: �`���l��2�̒ʐM�f�[�^�́A���]���ďo��
	SOL0
	  0: �`���l��0�̒ʐM�f�[�^�́A���̂܂܏o��
	  1: �`���l��0�̒ʐM�f�[�^�́A���]���ďo��
	*/
	SOL0 |= 0x0;	/* �`���l��0�̏o�͂͂��̂܂� */

	/* �V���A���o�͋����W�X�^ */
	/*
	   15    14    13    12    11    10     9     8     7     6     5     4     3     2     1     0
	+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
	|   0 |   0 |   0 |   0 |   0 |   0 |   0 |   0 |   0 |   0 |   0 |   0 | SOE3| SOE2| SOE1| SOE0|
	+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
	SOE3
	  0: �`���l��3�̃V���A���ʐM����ɂ��o�͒�~
	  1: �`���l��3�V���A���ʐM����ɂ��o�͋���
	SOE2
	  0: �`���l��2�̃V���A���ʐM����ɂ��o�͒�~
	  1: �`���l��2�V���A���ʐM����ɂ��o�͋���
	SOE1
	  0: �`���l��1�̃V���A���ʐM����ɂ��o�͒�~
	  1: �`���l��1�V���A���ʐM����ɂ��o�͋���
	SOE0
	  0: �`���l��0�̃V���A���ʐM����ɂ��o�͒�~
	  1: �`���l��0�V���A���ʐM����ɂ��o�͋���
	*/
	SOE0 |= 0x01;		/* �`���l��0�̏o�͋��� */

	/* �|�[�g�E���[�h�E���W�X�^ */
	/* 0:�o�̓��[�h�A1:���̓��[�h*/
	PM1 |= 0x02;	/* P11����̓��[�h�ɐݒ� */

	/* �|�[�g�E���W�X�^ */
	P1 |= 0x04U;	/* P12��HIGH */
	PM1 &= ~(0x04);	/* P12���o�̓��[�h�ɐݒ� */
}
/*
	UART�J�n
*/
void Uart_Start_IIC00(void)
{
	/* �V���A���o�̓��W�X�^ */
	SO0 |= 0x0001;		/* �`���l��0�̃V���A���E�f�[�^�o�͒l��HIGH */

	/* �V���A���o�͋����W�X�^ */
	SOE0 |= 0x01;		/* �`���l��0�̃V���A���ʐM���� */

	/* �V���A���E�`���l���J�n���W�X�^ */
	SS0 |= 0x03;		/* �`���l��0�A1�̒ʐM�J�n */

	/* ���荞�ݗv���t���O������ */
	STIF0 = 0u;			/* ���M�������荞�ݗv���t���O������ */
	SRIF0 = 0u;			/* ��M�������荞�ݗv���t���O������ */

	/* ���荞�݋��� */
	STMK0 = 0u;			/* ���M�������荞�݋���(INTST0) */
	SRMK0 = 0u;			/* ��M�G���[���荞�݋���(INTSRE0) */
}
/*
	UART��~
*/
void Uart_Stop_IIC00(void)
{
	/* ���荞�ݐݒ� */
	STMK0 = 1u;			/* ���M�������荞�݋֎~(INTST0) */
	SRMK0 = 1u;			/* ��M�������荞�݋֎~(INTSR0) */

	/* �V���A���E�`���l����~���W�X�^ */
	ST0 |= 0x3;			/* ���j�b�g0�̃`���l��0�A1�̓�����~ */

	/* �V���A���o�͋����W�X�^ */
	SOE0 &= ~(0x01);	/* ���j�b�g0�̃`���l��0�̃V���A���ʐM�֎~ */

	/* ���荞�ݗv���t���O������ */
	STIF0 = 0u;			/* ���M�������荞�ݗv���t���O������ */
	SRIF0 = 0u;			/* ��M�������荞�ݗv���t���O������ */
}
/*
	1�o�C�g�̑��M������
*/
__interrupt static void uart_InterruptSend_IIC00(void)
{
	/* TSD0���J�����ꂽ */
	s_is_Txd0_Free = TRUE;

	/* �o�b�t�@�Ƀf�[�^���c���Ă���Α��M */
	if (s_uart_SendBuffCnt_IIC00 > 0)
	{
		/* TXD0�֊i�[ */
		TXD0 = s_uart_SendBuff_IIC00[s_uart_SendBuffStartIdx_IIC00];

		/* �o�b�t�@�̃J�E���^�����炷 */
		s_uart_SendBuffCnt_IIC00--;

		/* �C���f�b�N�X��i�߂� */
		s_uart_SendBuffStartIdx_IIC00++;
		if (s_uart_SendBuffStartIdx_IIC00 >= UART0_SEND_BUFF_SIZE) {
			s_uart_SendBuffStartIdx_IIC00 = 0;
		}
	}
	/* �o�b�t�@����Ȃ�S�Ă̑��M���I�� */
	else
	{
		uart_SendEnd_IIC00();
	}
}
/*
	��M����
*/
__interrupt static void uart_InterruptReceive_IIC00(void)
{
    uint8_t err_type;

	/* ��M�J�n�̐錾���Ȃ���΁A�Ȃɂ����Ȃ� */
	if (!s_uart_isReceiving_IIC00)
	{
		return;
	}

	err_type = (uint8_t)(SSR01 & 0x0007U);
	SIR01 = (uint16_t)err_type;

	/* �G���[�̗L���m�F */
	if (err_type != 0U)
	{
		uart_ReceiveError_IIC00(err_type);
		return;
	}

	/* �o�b�t�@�T�C�Y�m�F */
	if (s_uart_ReceiveBuffIdx_IIC00 >= s_uart_ReceiveBuffLen_IIC00)
	{
		uart_ReceiveEnd_IIC00();
		return;
	}

	/* ��M�����擾 */
	sp_uart_ReceiveBuff_IIC00[s_uart_ReceiveBuffIdx_IIC00] = RXD0;
	s_uart_ReceiveBuffCnt_IIC00++;

	/* ��M�̏I������ */
	if (uart_IsReceiveEnd_IIC00())
	{
		/* ��M�I�����̏��� */
		uart_ReceiveEnd_IIC00();
	}
	else
	{
		/* ��M�o�b�t�@��i�߂� */
		s_uart_ReceiveBuffIdx_IIC00++;
	}
}
/*
	1�������M
*/
int16_t Uart_Putc_IIC00(int8_t c)
{
	/* ���荞�݋֎~ */
	STMK0 = 1U;

	/* �o�b�t�@�ɓ��肫��Ȃ� */
	if ((s_uart_SendBuffCnt_IIC00 + 1) > UART0_SEND_BUFF_SIZE)
	{
		return (-1);
	}

	/* TXD0���󂫏�ԂȂ�i�[ */
	if (s_is_Txd0_Free)
	{
		/* TDX0�֊i�[�B�i�[���邱�ƂŁA���M�������J�n�����B */
		TXD0 = c;

		/* TXD0�i�[�� */
		s_is_Txd0_Free = FALSE;
	}
	/* ���M���Ȃ�A�o�b�t�@�ɗ��߂� */
	else
	{
		/* �o�b�t�@�ɑ�� */
		s_uart_SendBuff_IIC00[s_uart_SendBuffEndIdx_IIC00] = c;

		/* �o�b�t�@�J�E���^�𑝂₷ */
		s_uart_SendBuffCnt_IIC00++;

		/* �o�b�t�@�̃C���f�b�N�X��i�߂� */
		s_uart_SendBuffEndIdx_IIC00++;
		if (s_uart_SendBuffEndIdx_IIC00 >= UART0_SEND_BUFF_SIZE) {
			s_uart_SendBuffEndIdx_IIC00 = 0;
		}
	}

	/* ���荞�݋��� */
	STMK0 = 0u;

    return (0);
}
/*
	���M
*/
int16_t Uart_Send_IIC00(int8_t * const data, uint16_t len)
{
	int16_t i;
	int16_t ret = 0;

	/* ���M��ԃt���O�̍X�V */
	s_uart_isSending_IIC00 = TRUE;

	/* TXD0�̏�ԃt���O�̍X�V */
	s_is_Txd0_Free = TRUE;

	/* ���M�������1�����Â��M */
	for (i = 0; i < len; i++)
	{
		if (Uart_Putc_IIC00(data[i]) != 0)
		{
			ret = -1;
			break;
		}
	}
	return ret;
}
/*
	�����񑗐M
*/
int Uart_Printf_IIC00(const char *fmt, ...)
{
	int len;
	va_list	marker;

#if defined(va_starttop)
	va_starttop(marker, fmt);
#else
	va_start(marker, fmt);
#endif
	/* vsprintf�Ɋۓ��� */
	len = vsprintf(s_uart_PrintBuff_IIC00, fmt, marker);

	/* �o�b�t�@�T�C�Y�𒴂�����؂�̂� */
	if (len > UART0_PRINT_BUFF) {
		len = UART0_PRINT_BUFF;
	}

	/* ���M */
	return Uart_Send_IIC00(s_uart_PrintBuff_IIC00, len);
}
/*
	���M��Ԃ�Ԃ�
*/
bool_t Uart_IsSending_IIC00(void)
{
	return s_uart_isSending_IIC00;
}
/*
	��M�J�n��錾����
*/
int16_t Uart_Receive_IIC00(uint8_t *buff, int16_t len)
{
	/* ��M��ԃt���O���X�V */
	s_uart_isReceiving_IIC00 = TRUE;

	/* �o�b�t�@�p�C���f�b�N�X�������� */
	sp_uart_ReceiveBuff_IIC00 = buff;
	s_uart_ReceiveBuffLen_IIC00 = len;
	s_uart_ReceiveBuffIdx_IIC00 = 0;
	s_uart_ReceiveBuffCnt_IIC00 = 0;

	return 0;
}
/*
	���M�I��
*/
void uart_SendEnd_IIC00(void)
{
	/* ���M��ԃt���O���X�V */
	s_uart_isSending_IIC00 = FALSE;
}
/*
	��M�̏I�����ɍs������
*/
static void uart_ReceiveEnd_IIC00(void)
{
	/* ��M��ԃt���O���X�V */
	s_uart_isReceiving_IIC00 = FALSE;
}
/*
	��M�ς̃o�C�g��
*/
int16_t Uart_GetReceiveLen_IIC00(void)
{
	return s_uart_ReceiveBuffCnt_IIC00;
}
/*
	��M��Ԃ�Ԃ�
*/
bool_t Uart_IsReceiving_IIC00(void)
{
	return s_uart_isReceiving_IIC00;
}
/*
	�{�[���[�g�ݒ�p���W�X�^�l�v�Z
	brr: �{�[���[�g
	fCLK: ���C���N���b�N(kHz)
	PRS: PRS���W�X�^�̎Z�o����
	SDR: SDR���W�X�^�̎Z�o����
*/
static int16_t uart_CalcFrequencyDivision(uint32_t brr, uint16_t fCLK, uint8_t *prs, uint16_t *sdr)
{
	uint8_t		i;
	uint32_t	c_sdr;
	uint16_t	ret = -1;
	uint32_t	fCLKHz;

	fCLKHz = (uint32_t)fCLK * 1000;

	/* �u12.6.4 �{�[�E���[�g�̎Z�o�v����t�Z */
	for (i = 0; i < 16; i++)
	{
		c_sdr = ((fCLKHz >> i) / brr);
		if ((2 <= c_sdr) && (c_sdr <= 256))
		{
			*prs = i;
			*sdr = (uint16_t)((c_sdr / 2) - 1);
			ret = 0;
			break;
		}
	}
	return ret;
}
/*
	��M�̏I������
*/
static bool_t uart_IsReceiveEnd_IIC00(void)
{
	/* �o�b�t�@�������ς��A�܂��͉��s�R�[�h��M�Ŏ�M�I���Ƃ݂Ȃ� */
	if ((sp_uart_ReceiveBuff_IIC00[s_uart_ReceiveBuffIdx_IIC00] == '\n') || 
		((s_uart_ReceiveBuffIdx_IIC00 + 1) >= s_uart_ReceiveBuffLen_IIC00))
	{
		return TRUE;
	}
	return FALSE;
}
/*
	��M�ɃG���[���������ꍇ�̏���
*/
static void uart_ReceiveError_IIC00(uint8_t err_type)
{
	if (err_type & 0x0004)
	{
		/* �t���[�~���O�E�G���[ */
	}
	if (err_type & 0x0002)
	{
		/* �p���e�B�E�G���[ */
	}
	if (err_type & 0x0001)
	{
		/* �I�[�o�����E�G���[ */
	}
}
