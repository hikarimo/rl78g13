/*
	(C) 2016 hikarimo
*/

/*
	シリアル・アレイ・ユニット0のチャネル0を送信、チャネル1を受信として使用

	使用ユニット、チャネル
	+----------+----------+-------+-------+-------+----+
	| ユニット | チャネル |  CSI  |↓UART |  I2C  |    |
	+----------+----------+-------+-------+-------+----+
	| 0        | 0        | CSI00 | UART0 | IIC00 | ← |
	| 0        | 1        | CSI01 | UART0 | IIC01 | ← |
	| 0        | 2        | CSI10 | UART1 | IIC10 |    |
	| 0        | 3        | CSI11 | UART1 | IIC11 |    |
	+----------+----------+-------+-------+-------+----+
	| 1        | 0        | CSI20 | UART2 | IIC20 |    |
	| 1        | 1        | CSI21 | UART2 | IIC21 |    |
	+----------+----------+-------+-------+-------+----+

	使用ピン
	44Pin	P12/SO00/→TxD0/TOOLTxD/(INTP5)/(TI05)/(TO05)
	45Pin	P11/SI00/→RxD0/TOOLRxD/SDA00/(TI06)/(TO06)
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
	UART設定
*/
void Uart_Setup_IIC00(uint32_t clk)
{
	uint8_t prs;
	uint16_t sdr;

	/* シリアル・アレイ・ユニット0の入力クロック供給の制御 */
	SAU0EN = 1u;		/* 0:クロック供給停止、1:供給 */

	/* クロック安定待ち */
	NOP();
	NOP();
	NOP();
	NOP();

	/* PRS、SDRレジスタ値算出 */
	if (uart_CalcFrequencyDivision(clk, HW_FCLK, &prs, &sdr) != 0)
	{
		prs = 0x4;
		sdr = 0x67;
	}

	/* シリアル・クロック選択レジスタ */
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
	/* チャンネルに供給するクロック(CK00とCK01)を設定 */
	SPS0 = (prs | (prs << 4));	/* CK00、CK01クロック設定 */

	/* シリアル・チャネル停止レジスタ */
	ST0 |= 0x03;		/* チャネル0と1の動作を停止 */

	/* 割り込み禁止 */
	STMK0 = 1u;			/* 送信完了割り込み禁止(INTST0) */
	SRMK0 = 1u;			/* 受信完了割り込み禁止(INTSR0) */
	SREMK0 = 1u;		/* 受信エラー割り込み禁止(INTSRE0) */

	/* 割り込み要求フラグ初期化 */
	STIF0 = 0u;			/* 送信完了割り込み */
	SRIF0 = 0u;			/* 受信完了割り込み */
	SREIF0 = 0u;		/* 受信エラー割り込み */

	/* 割り込み優先順位指定(INTST0) */
	/*
	+---------+---------+-----------+
	|  STPR10 |  STPR00 | 優先レベル|
	+---------+---------+-----------+
	|      0  |      0  |     0(高) |
	|      0  |      1  |     1     |
	|      1  |      0  |     2     |
	|      1  |      1  |     3(低) |
	+---------+---------+-----------+
	*/
	STPR10 = 1u;
	STPR00 = 1u;

	/* 割り込み優先順位指定(INTSR0) */
	/*
	+---------+---------+-----------+
	|  SRPR10 |  SRPR00 | 優先レベル|
	+---------+---------+-----------+
	|      0  |      0  |     0(高) |
	|      0  |      1  |     1     |
	|      1  |      0  |     2     |
	|      1  |      1  |     3(低) |
	+---------+---------+-----------+
	*/
	SRPR10 = 1u;
	SRPR00 = 1u;

	/* シリアル・モード・レジスタ */
	/*
	   15    14    13    12    11    10     9     8     7     6     5     4     3     2     1     0
	+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
	| CKS | CCS |   0 |   0 |   0 |   0 |   0 | STS |   0 | SIS |   1 |   0 |   0 | MD2 | MD1 | MD0 |
	+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
	CKS
	  0: SPSレジスタで設定した動作クロックCK00
	  1: SPSレジスタで設定した動作クロックCK01
	CCS
	  0: CKSビットで指定した動作クロックfMCKの分周クロック
	  1: SCKp端子からの入力クロックfSCK(CSIモードのスレーブ転送)
	STS
	  0: スタート・トリガ要因としてソフトウェア・トリガのみ有効(CSI、UART送信、簡易I2C時に選択)
	  1: スタート・トリガ要因としてRxD0端子の有効エッジ(UART受信時に選択)
	SIS
	  0: 立ち下がりエッジをスタート・ビットとして検出
	  1: 立ち上がりエッジをスタート・ビットとして検出(入力される通信データは、反転)
	MD2,MD1
	  00: CSIモード
	  01: UARTモード
	  10: 簡易I2Cモード
	  11: 設定禁止
	MD0
	  0: 割り込み要因、転送完了割り込み
	  1: 割り込み要因、バッファ空き割り込み
	*/
	SMR00 = 0x0022;		/* fMCK = CK00、立ち下がりがスタート・ビット、UARTモード、転送完了割り込み */
	SMR01 = 0x0122;		/* fMCK = CK00、RxD0端子でスタート・トリガ、立ち下がりがスタート・ビット、UARTモード、転送完了割り込み */

	/* シリアル通信動作設定レジスタ */
	/*
	   15    14    13    12    11    10     9     8     7     6     5     4     3     2     1     0
	+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
	| TXE | RXE | DAP | CKP |   0 | EOC | PTC1| PTC0| DIR |   0 | SLC1| SLC0|   0 |   1 | DLS1| DLS0|
	+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
	TXE,RXE
	  00: 通信禁止
	  01: 受信のみ
	  10: 送信のみ
	  11: 送受信
	DAP,CKP
	  00: UARTモード(簡易I2Cモード時は、00指定)
	EOC
	  0: CSIモード(簡易I2Cモード、UART送信時は、0指定)
	PTC1,PTC0
	  00: パリティ・ビットなし(CSI、簡易I2Cは00を設定)
	  01: 0パリティ
	  10: 偶数パリティ
	  11: 奇数パリティ
	DIR
	  0: MSBファーストで入出力(管理I2Cは、0指定)
	  1: LSBファーストで入出力
	SLC1,SLC0
	  00: ストップ・ビットなし(CSIは、00指定)
	  01: ストップ・ビット長、1ビット(UART、簡易I2Cは、01指定)
	  10: ストップ・ビット長、2ビット(ユニット0のチャネル0、2とユニット1のチャネル0、2のみ)
	  11: 設定禁止
	DLS1,DLS0
	  00: 設定禁止
	  01: 9ビット・データ長(UARTモード時のみ)
	  10: 7ビット・データ長
	  11: 8ビット・データ長(簡易I2Cは、11指定)
	*/
	SCR00 = 0x8097;		/* 送信のみ、パリティなし、LSBファースト、ストップ・ビット長1、8ビット・データ長 */
	SCR01 = 0x4097;		/* 受信のみ、パリティなし、LSBファースト、ストップ・ビット長1、8ビット・データ長 */

	/* シリアル・データ・レジスタ */
	/*
	   15    14    13    12    11    10     9     8     7     6     5     4     3     2     1     0
	+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
	|      動作クロックの分周設定(7ビット)    |            送受信バッファ(9ビット)                  |
	+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
	動作クロックの分周設定
	  0000000: fMCK/2
	  0000001: fMCK/4
	  0000010: fMCK/6
	  0000011: fMCK/8
	   :
	  1111110: fMCK/254
	  1111111: fMCK/256
	*/
	SDR00 = (sdr << 9);		/* 算出結果を上位7ビットに設定 */
	SDR01 = (sdr << 9);		/* 算出結果を上位7ビットに設定 */

	/* ノイズ・フィルタ許可レジスタ */
	/*
	    7     6     5     4     3     2     1     0
	+-----+-----+-----+-----+-----+-----+-----+-----+
	|   0 |SNFEN|   0 |SNFEN|   0 |SNFEN|   0 |SNFEN|
	|     |   30|     |   20|     |   10|     |   00|
	+-----+-----+-----+-----+-----+-----+-----+-----+
	SNFEN30
	  0: RxD3端子のノイズ・フィルタOFF
	  1: RxD3端子のノイズ・フィルタON
	SNFEN20
	  0: RxD2端子のノイズ・フィルタOFF
	  1: RxD2端子のノイズ・フィルタON
	SNFEN10
	  0: RxD1端子のノイズ・フィルタOFF
	  1: RxD1端子のノイズ・フィルタON
	SNFEN00
	  0: RxD0端子のノイズ・フィルタOFF
	  1: RxD0端子のノイズ・フィルタON
	*/
	NFEN0 |= 1;		/* RxD0端子のノイズ・フィルタON */

	/* シリアル・フラグ・クリア・トリガ・レジスタ */
	/*
	   15    14    13    12    11    10     9     8     7     6     5     4     3     2     1     0
	+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
	|   0 |   0 |   0 |   0 |   0 |   0 |   0 |   0 |   0 |   0 |   0 |   0 |   0 | FECT| PECT| OVCT|
	+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
	FECT
	  0: クリアしない
	  1: フレーミング・エラー・フラグを0クリア
	PECT
	  0: クリアしない
	  1: パリティ・エラー・フラグを0クリア
	OVCT
	  0: クリアしない
	  1: オーバーラン・エラー・フラグを0クリア
	*/
	SIR01 = 0x7;	/* 全部クリア */

	/* シリアル出力レジスタ */
	/*
	   15    14    13    12    11    10     9     8     7     6     5     4     3     2     1     0
	+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
	|   0 |   0 |   0 |   0 |CKO03|CKO02|CKO01|CKO00|   0 |   0 |   0 |   0 | SO03| SO02| SO01| SO00|
	+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
	CKO03,CKO02,CKO01,CKO00
	  0: シリアル・クロック出力値が0、1:1
	SO03,SO02,SO01,SO00
	  0: シリアル・データ出力値が0、1:1
	*/
	SO0 |= 0x01;		/* チャネル0のシリアル・データ出力値をHIGH */

	/* シリアル出力レベル・レジスタ */
	/*
	   15    14    13    12    11    10     9     8     7     6     5     4     3     2     1     0
	+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
	|   0 |   0 |   0 |   0 |   0 |   0 |   0 |   0 |   0 |   0 |   0 |   0 |   0 | SOL2|   0 | SOL0|
	+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
	SOL2
	  0: チャネル2の通信データは、そのまま出力
	  1: チャネル2の通信データは、反転して出力
	SOL0
	  0: チャネル0の通信データは、そのまま出力
	  1: チャネル0の通信データは、反転して出力
	*/
	SOL0 |= 0x0;	/* チャネル0の出力はそのまま */

	/* シリアル出力許可レジスタ */
	/*
	   15    14    13    12    11    10     9     8     7     6     5     4     3     2     1     0
	+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
	|   0 |   0 |   0 |   0 |   0 |   0 |   0 |   0 |   0 |   0 |   0 |   0 | SOE3| SOE2| SOE1| SOE0|
	+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
	SOE3
	  0: チャネル3のシリアル通信動作による出力停止
	  1: チャネル3シリアル通信動作による出力許可
	SOE2
	  0: チャネル2のシリアル通信動作による出力停止
	  1: チャネル2シリアル通信動作による出力許可
	SOE1
	  0: チャネル1のシリアル通信動作による出力停止
	  1: チャネル1シリアル通信動作による出力許可
	SOE0
	  0: チャネル0のシリアル通信動作による出力停止
	  1: チャネル0シリアル通信動作による出力許可
	*/
	SOE0 |= 0x01;		/* チャネル0の出力許可 */

	/* ポート・モード・レジスタ */
	/* 0:出力モード、1:入力モード*/
	PM1 |= 0x02;	/* P11を入力モードに設定 */

	/* ポート・レジスタ */
	P1 |= 0x04U;	/* P12をHIGH */
	PM1 &= ~(0x04);	/* P12を出力モードに設定 */
}
/*
	UART開始
*/
void Uart_Start_IIC00(void)
{
	/* シリアル出力レジスタ */
	SO0 |= 0x0001;		/* チャネル0のシリアル・データ出力値をHIGH */

	/* シリアル出力許可レジスタ */
	SOE0 |= 0x01;		/* チャネル0のシリアル通信許可 */

	/* シリアル・チャネル開始レジスタ */
	SS0 |= 0x03;		/* チャネル0、1の通信開始 */

	/* 割り込み要求フラグ初期化 */
	STIF0 = 0u;			/* 送信完了割り込み要求フラグ初期化 */
	SRIF0 = 0u;			/* 受信完了割り込み要求フラグ初期化 */

	/* 割り込み許可 */
	STMK0 = 0u;			/* 送信完了割り込み許可(INTST0) */
	SRMK0 = 0u;			/* 受信エラー割り込み許可(INTSRE0) */
}
/*
	UART停止
*/
void Uart_Stop_IIC00(void)
{
	/* 割り込み設定 */
	STMK0 = 1u;			/* 送信完了割り込み禁止(INTST0) */
	SRMK0 = 1u;			/* 受信完了割り込み禁止(INTSR0) */

	/* シリアル・チャネル停止レジスタ */
	ST0 |= 0x3;			/* ユニット0のチャネル0、1の動作を停止 */

	/* シリアル出力許可レジスタ */
	SOE0 &= ~(0x01);	/* ユニット0のチャネル0のシリアル通信禁止 */

	/* 割り込み要求フラグ初期化 */
	STIF0 = 0u;			/* 送信完了割り込み要求フラグ初期化 */
	SRIF0 = 0u;			/* 受信完了割り込み要求フラグ初期化 */
}
/*
	1バイトの送信が完了
*/
__interrupt static void uart_InterruptSend_IIC00(void)
{
	/* TSD0が開放された */
	s_is_Txd0_Free = TRUE;

	/* バッファにデータが残っていれば送信 */
	if (s_uart_SendBuffCnt_IIC00 > 0)
	{
		/* TXD0へ格納 */
		TXD0 = s_uart_SendBuff_IIC00[s_uart_SendBuffStartIdx_IIC00];

		/* バッファのカウンタを減らす */
		s_uart_SendBuffCnt_IIC00--;

		/* インデックスを進める */
		s_uart_SendBuffStartIdx_IIC00++;
		if (s_uart_SendBuffStartIdx_IIC00 >= UART0_SEND_BUFF_SIZE) {
			s_uart_SendBuffStartIdx_IIC00 = 0;
		}
	}
	/* バッファが空なら全ての送信が終了 */
	else
	{
		uart_SendEnd_IIC00();
	}
}
/*
	受信あり
*/
__interrupt static void uart_InterruptReceive_IIC00(void)
{
    uint8_t err_type;

	/* 受信開始の宣言がなければ、なにもしない */
	if (!s_uart_isReceiving_IIC00)
	{
		return;
	}

	err_type = (uint8_t)(SSR01 & 0x0007U);
	SIR01 = (uint16_t)err_type;

	/* エラーの有無確認 */
	if (err_type != 0U)
	{
		uart_ReceiveError_IIC00(err_type);
		return;
	}

	/* バッファサイズ確認 */
	if (s_uart_ReceiveBuffIdx_IIC00 >= s_uart_ReceiveBuffLen_IIC00)
	{
		uart_ReceiveEnd_IIC00();
		return;
	}

	/* 受信文字取得 */
	sp_uart_ReceiveBuff_IIC00[s_uart_ReceiveBuffIdx_IIC00] = RXD0;
	s_uart_ReceiveBuffCnt_IIC00++;

	/* 受信の終了判定 */
	if (uart_IsReceiveEnd_IIC00())
	{
		/* 受信終了時の処理 */
		uart_ReceiveEnd_IIC00();
	}
	else
	{
		/* 受信バッファを進める */
		s_uart_ReceiveBuffIdx_IIC00++;
	}
}
/*
	1文字送信
*/
int16_t Uart_Putc_IIC00(int8_t c)
{
	/* 割り込み禁止 */
	STMK0 = 1U;

	/* バッファに入りきらない */
	if ((s_uart_SendBuffCnt_IIC00 + 1) > UART0_SEND_BUFF_SIZE)
	{
		return (-1);
	}

	/* TXD0が空き状態なら格納 */
	if (s_is_Txd0_Free)
	{
		/* TDX0へ格納。格納することで、送信処理が開始される。 */
		TXD0 = c;

		/* TXD0格納中 */
		s_is_Txd0_Free = FALSE;
	}
	/* 送信中なら、バッファに溜める */
	else
	{
		/* バッファに代入 */
		s_uart_SendBuff_IIC00[s_uart_SendBuffEndIdx_IIC00] = c;

		/* バッファカウンタを増やす */
		s_uart_SendBuffCnt_IIC00++;

		/* バッファのインデックスを進める */
		s_uart_SendBuffEndIdx_IIC00++;
		if (s_uart_SendBuffEndIdx_IIC00 >= UART0_SEND_BUFF_SIZE) {
			s_uart_SendBuffEndIdx_IIC00 = 0;
		}
	}

	/* 割り込み許可 */
	STMK0 = 0u;

    return (0);
}
/*
	送信
*/
int16_t Uart_Send_IIC00(int8_t * const data, uint16_t len)
{
	int16_t i;
	int16_t ret = 0;

	/* 送信状態フラグの更新 */
	s_uart_isSending_IIC00 = TRUE;

	/* TXD0の状態フラグの更新 */
	s_is_Txd0_Free = TRUE;

	/* 送信文字列を1文字づつ送信 */
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
	文字列送信
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
	/* vsprintfに丸投げ */
	len = vsprintf(s_uart_PrintBuff_IIC00, fmt, marker);

	/* バッファサイズを超えたら切り捨て */
	if (len > UART0_PRINT_BUFF) {
		len = UART0_PRINT_BUFF;
	}

	/* 送信 */
	return Uart_Send_IIC00(s_uart_PrintBuff_IIC00, len);
}
/*
	送信状態を返す
*/
bool_t Uart_IsSending_IIC00(void)
{
	return s_uart_isSending_IIC00;
}
/*
	受信開始を宣言する
*/
int16_t Uart_Receive_IIC00(uint8_t *buff, int16_t len)
{
	/* 受信状態フラグを更新 */
	s_uart_isReceiving_IIC00 = TRUE;

	/* バッファ用インデックスを初期化 */
	sp_uart_ReceiveBuff_IIC00 = buff;
	s_uart_ReceiveBuffLen_IIC00 = len;
	s_uart_ReceiveBuffIdx_IIC00 = 0;
	s_uart_ReceiveBuffCnt_IIC00 = 0;

	return 0;
}
/*
	送信終了
*/
void uart_SendEnd_IIC00(void)
{
	/* 送信状態フラグを更新 */
	s_uart_isSending_IIC00 = FALSE;
}
/*
	受信の終了時に行う処理
*/
static void uart_ReceiveEnd_IIC00(void)
{
	/* 受信状態フラグを更新 */
	s_uart_isReceiving_IIC00 = FALSE;
}
/*
	受信済のバイト数
*/
int16_t Uart_GetReceiveLen_IIC00(void)
{
	return s_uart_ReceiveBuffCnt_IIC00;
}
/*
	受信状態を返す
*/
bool_t Uart_IsReceiving_IIC00(void)
{
	return s_uart_isReceiving_IIC00;
}
/*
	ボーレート設定用レジスタ値計算
	brr: ボーレート
	fCLK: メインクロック(kHz)
	PRS: PRSレジスタの算出結果
	SDR: SDRレジスタの算出結果
*/
static int16_t uart_CalcFrequencyDivision(uint32_t brr, uint16_t fCLK, uint8_t *prs, uint16_t *sdr)
{
	uint8_t		i;
	uint32_t	c_sdr;
	uint16_t	ret = -1;
	uint32_t	fCLKHz;

	fCLKHz = (uint32_t)fCLK * 1000;

	/* 「12.6.4 ボー・レートの算出」から逆算 */
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
	受信の終了条件
*/
static bool_t uart_IsReceiveEnd_IIC00(void)
{
	/* バッファがいっぱい、または改行コード受信で受信終了とみなす */
	if ((sp_uart_ReceiveBuff_IIC00[s_uart_ReceiveBuffIdx_IIC00] == '\n') || 
		((s_uart_ReceiveBuffIdx_IIC00 + 1) >= s_uart_ReceiveBuffLen_IIC00))
	{
		return TRUE;
	}
	return FALSE;
}
/*
	受信にエラーがあった場合の処理
*/
static void uart_ReceiveError_IIC00(uint8_t err_type)
{
	if (err_type & 0x0004)
	{
		/* フレーミング・エラー */
	}
	if (err_type & 0x0002)
	{
		/* パリティ・エラー */
	}
	if (err_type & 0x0001)
	{
		/* オーバラン・エラー */
	}
}
