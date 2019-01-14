/*
	(C) 2017 hikarimo

	�f�o�b�O�p�֐�
*/
#pragma sfr	/* for RL78 */

#include <stdio.h>

/* �o�̓|�[�g(�f�o�b�O�p�Ɏg����|�[�g��ݒ�) */
#define	setPort(x)		(P0.0 = x)

/* �o�b�t�@�E�T�C�Y(UART�p) */
#define	MAX_SEND_BUFF	(100)
/* �o�b�t�@�E�T�C�Y(sprintf�p) */
#define	MAX_PRINT_BUFF	(50)
/* �|�[�g�o�� */
#define	PORT_LOW		(0)
#define	PORT_HIGH		(1)

/* UART�o�͏�� */
typedef enum {
	SEND_STS_START,	/* �J�n		*/
	SEND_STS_DATA,	/* ���M��	*/
	SEND_STS_STOP	/* ��~		*/
} _SEND_STS;
/* �o�b�t�@(UART�p) */
static char	s_SendBuffer[MAX_SEND_BUFF];
/* �o�b�t�@(sprintf�p) */
static char	s_PrintBuffer[MAX_PRINT_BUFF];
/* �g�p���̃o�b�t�@�T�C�Y */
static volatile int s_UseBuffSize;
/* �o�b�t�@�i�[ */
static void setSendBuffer(char *str, int len);

/*
	�f�o�b�O�p������o��
 */
void PrintDebugLog(const char *fmt, ...)
{
	int		len;
	va_list	marker;

#if defined(va_starttop)	/* for RL78 */
	va_starttop(marker, fmt);
#else
	va_start(marker, fmt);
#endif
	len = vsprintf(s_PrintBuffer, fmt, marker);
	setSendBuffer(s_PrintBuffer, len);
}
/*
	�o�b�t�@�i�[
*/
static void setSendBuffer(char *str, int len)
{
	static int buffIdx = 0;
	int i;

	/* ��������o�b�t�@�Ɋi�[ */
	for (i = 0; i < len; i++) {
		/* �T�C�Y�I�[�o�[�Ȃ���߂� */
		if (s_UseBuffSize >= MAX_SEND_BUFF) {
			break;
		}
		/* 1�����Âi�[ */
		s_SendBuffer[buffIdx++] = str[i];
		if (buffIdx >= MAX_SEND_BUFF) {
			buffIdx = 0;
		}
		s_UseBuffSize++;
	}
}
/*
	�o�b�t�@���M

	�|�[�g�𒼐ڑ��삵��UART�o�͂��s��
	�f�[�^8bit
	�p���e�B�Ȃ�
	�X�g�b�v�r�b�g1bit
	�t���[����Ȃ�

	���̊֐����{�[���[�g�ɍ��킹�ČĂԁB
	�C���^�[�o���^�C�}�Ƃ��B
      9600bps -> 104.17us
     14400bps ->  69.44us
     19200bps ->  52.08us
     38400bps ->  26.04us
     57600bps ->  17.36us
    115200bps ->   8.68us
    230400bps ->   4.34us
    460800bps ->   2.17us
    921600bps ->   1.09us
*/
void SendDebugPort(void)
{
	static _SEND_STS sendSts = SEND_STS_START;
	static int sendBitNum = 0;
	static int sndIdx = 0;

	switch (sendSts) {
	case SEND_STS_START:
		/* �X�^�[�g�r�b�g���M */
		if (s_UseBuffSize != 0) {
			setPort(PORT_LOW);
			sendSts = SEND_STS_DATA;
		}
		break;
	case SEND_STS_DATA:
		/* �f�[�^���M */
		setPort((s_SendBuffer[sndIdx] >> sendBitNum) & 0x01);
		sendBitNum++;
		if (sendBitNum >= 8) {
			sendBitNum = 0;
			sendSts = SEND_STS_STOP;
		}
		break;
	case SEND_STS_STOP:
		/* �X�g�b�v�r�b�g���M */
		setPort(PORT_HIGH);
		sendSts = SEND_STS_START;
		sndIdx++;
		if (sndIdx >= MAX_SEND_BUFF) {
			sndIdx = 0;
		}
		s_UseBuffSize--;
		break;
	}
}
