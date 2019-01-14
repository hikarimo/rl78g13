/*
	(C) 2017 hikarimo

	デバッグ用関数
*/
#pragma sfr	/* for RL78 */

#include <stdio.h>

/* 出力ポート(デバッグ用に使えるポートを設定) */
#define	setPort(x)		(P0.0 = x)

/* バッファ・サイズ(UART用) */
#define	MAX_SEND_BUFF	(100)
/* バッファ・サイズ(sprintf用) */
#define	MAX_PRINT_BUFF	(50)
/* ポート出力 */
#define	PORT_LOW		(0)
#define	PORT_HIGH		(1)

/* UART出力状態 */
typedef enum {
	SEND_STS_START,	/* 開始		*/
	SEND_STS_DATA,	/* 送信中	*/
	SEND_STS_STOP	/* 停止		*/
} _SEND_STS;
/* バッファ(UART用) */
static char	s_SendBuffer[MAX_SEND_BUFF];
/* バッファ(sprintf用) */
static char	s_PrintBuffer[MAX_PRINT_BUFF];
/* 使用中のバッファサイズ */
static volatile int s_UseBuffSize;
/* バッファ格納 */
static void setSendBuffer(char *str, int len);

/*
	デバッグ用文字列出力
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
	バッファ格納
*/
static void setSendBuffer(char *str, int len)
{
	static int buffIdx = 0;
	int i;

	/* 文字列をバッファに格納 */
	for (i = 0; i < len; i++) {
		/* サイズオーバーなら諦める */
		if (s_UseBuffSize >= MAX_SEND_BUFF) {
			break;
		}
		/* 1文字つづつ格納 */
		s_SendBuffer[buffIdx++] = str[i];
		if (buffIdx >= MAX_SEND_BUFF) {
			buffIdx = 0;
		}
		s_UseBuffSize++;
	}
}
/*
	バッファ送信

	ポートを直接操作してUART出力を行う
	データ8bit
	パリティなし
	ストップビット1bit
	フロー制御なし

	この関数をボーレートに合わせて呼ぶ。
	インターバルタイマとか。
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
		/* スタートビット送信 */
		if (s_UseBuffSize != 0) {
			setPort(PORT_LOW);
			sendSts = SEND_STS_DATA;
		}
		break;
	case SEND_STS_DATA:
		/* データ送信 */
		setPort((s_SendBuffer[sndIdx] >> sendBitNum) & 0x01);
		sendBitNum++;
		if (sendBitNum >= 8) {
			sendBitNum = 0;
			sendSts = SEND_STS_STOP;
		}
		break;
	case SEND_STS_STOP:
		/* ストップビット送信 */
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
