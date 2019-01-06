/*
	(C) 2016 hikarimo
*/

/*
	シリアル・アレイ・ユニット0のチャネル2を使用

	使用ユニット、チャネル
	+----------+----------+-------+-------+-------+----+
	| ユニット | チャネル |  CSI  |  UART |↓I2C  |    |
	+----------+----------+-------+-------+-------+----+
	| 0        | 0        | CSI00 | UART0 | IIC00 |    |
	| 0        | 1        | CSI01 | UART0 | IIC01 |    |
	| 0        | 2        | CSI10 | UART1 | IIC10 | ← |
	| 0        | 3        | CSI11 | UART1 | IIC11 |    |
	+----------+----------+-------+-------+-------+----+
	| 1        | 0        | CSI20 | UART2 | IIC20 |    |
	| 1        | 1        | CSI21 | UART2 | IIC21 |    |
	+----------+----------+-------+-------+-------+----+

	使用クロック
	CK00、→CK01、CK10、CK11

	fCLK－分周(SPS0:PRS0x)－CK00－選択(SMR02)－fMCK－CKS分周(SDR02)－fTCLK
        ＼分周(SPS0:PRS1x)－CK01／

	使用ピン
	P04/SCK10/→SCL10
	P03/ANI16/SI10/RxD1/→SDA10
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

static uint8_t	*sp_i2c_TxData;								/* 送信データ */
static uint16_t s_i2c_TxCount_IIC01;						/* 送信データの残り */
static uint8_t	*sp_i2c10_RxData;							/* 受信データ */
static uint16_t	s_i2c_RxCount;								/* 受信済データ数 */
static uint16_t	s_i2c_RxLength;								/* 受信データ数 */
static uint8_t	s_i2c_MasterStatus;							/* 送受信状態 */
static uint16_t	s_i2c_Clock;								/* I2C動作クロック */
static bool_t	s_I2C_IsReadingWriteing_IIC01;				/* 読み書き処理の状態 */
static void (*sp_i2c_CallbackMasterSend_IIC01)(void);		/* 送信完了時コールバック */
static void (*sp_i2c_CallbackMasterReceive_IIC01)(void);	/* 受信完了時コールバック */

static void i2c_CallbackMasterError_IIC01(MD_STATUS flag);
static int16_t i2c_CalcFrequencyDivision(uint16_t bps, uint16_t fCLK, uint8_t *prs, uint16_t *sdr);
static void i2c_Wait(void);
static void i2c_SendEnd_IIC01(void);
static void i2c_ReceiveEnd_IIC01(void);

/*
	I2C設定

	clk : I2C動作クロック(kHz)
*/
void I2C_Setup_IIC01(uint16_t clk)
{
	uint8_t		prs;
	uint16_t	sdr;
	uint16_t	sps;

	s_i2c_Clock = clk;
	s_I2C_IsReadingWriteing_IIC01 = FALSE;

	/* 周辺イネーブル・レジスタ(PER0) */
	/*
	    7       6       5       4       3       2       1       0
	+-------+-------+-------+-------+-------+-------+-------+-------+
	| RTCEN |IICA1EN| ADCEN |IICA0EN| SAU1EN| SAU0EN| TAU1EN| TAU0EN|
	+-------+-------+-------+-------+-------+-------+-------+-------+
	*/
	/* シリアル・アレイ・ユニット0の入力クロック供給の制御 */
	SAU0EN = 1u;		/* クロック供給 */

	/* クロック安定待ち */
	NOP();
	NOP();
	NOP();
	NOP();

	/* PRS、SDRレジスタ値算出 */
	if (i2c_CalcFrequencyDivision(clk, HW_FCLK, &prs, &sdr) != 0)
	{
		/* 設定できなかった */
		while(1){
			;
		}
	}

	/* シリアル・クロック選択レジスタ(SPSm) */
	/*
	   15    14    13    12    11    10     9     8     7     6     5     4     3     2     1     0
	+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
	|   0 |   0 |   0 |   0 |   0 |   0 |   0 |   0 |PRS13|PRS12|PRS11|PRS10|PRS03|PRS02|PRS01|PRS00|
	+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
	PRS13,12,11,10(ユニット1)
	  0000: CK01 = fCLK/2^0
	  0001: CK01 = fCLK/2^1
	  0010: CK01 = fCLK/2^2
	    :
	  1111: CK01 = fCLK/2^15
	PRS03,02,01,00(ユニット0)
	  0000: CK00 = fCLK/2^0
	  0001: CK00 = fCLK/2^1
	  0010: CK00 = fCLK/2^2
	    :
	  1111: CKm0 = fCLK/2^15
	*/
	/* ユニット0のCK01の分周を設定 */
	sps = ((SPS0 & 0xff0f) | (prs << 4));
	SPS0 = sps;

	/* シリアル・チャネル停止レジスタ(STm) */
	/*
	   15    14    13    12    11    10     9     8     7     6     5     4     3     2     1     0
	+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
	|   0 |   0 |   0 |   0 |   0 |   0 |   0 |   0 |   0 |   0 |   0 |   0 | ST03| ST02| ST01| ST00|
	+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
	ST03
	  0: トリガせず、SE03を0クリア、通信動作を停止(チャネル0)
	ST02
	  0: トリガせず、SE02を0クリア、通信動作を停止(チャネル1)
	ST01
	  0: トリガせず、SE01を0クリア、通信動作を停止(チャネル2)
	ST00
	  0: トリガせず、SE00を0クリア、通信動作を停止(チャネル3)
	*/
	ST0 |= 0x0004;		/* チャネル2の動作を停止 */

	/* 割り込み禁止 */
	IICMK10 = 1u;	 	/* チャネル2の割り込み禁止(INTIIC10) */

	/* 割り込み要求フラグ初期化 */
	IICIF10 = 0u;		/* チャネル2の割り込みフラグ(INTIIC10) */

	/* 割り込み優先順位指定(INTIIC10) */
	/*
	+---------+---------+-----------+
	| IICPR110| IICPR010| 優先レベル|
	+---------+---------+-----------+
	|      0  |      0  |     0(高) |
	|      0  |      1  |     1     |
	|      1  |      0  |     2     |
	|      1  |      1  |     3(低) |
	+---------+---------+-----------+
	*/
	/* 割り込み優先レベル3 */
	IICPR110 = 1u;
	IICPR010 = 1u;

	/* シリアル・フラグ・クリア・トリガ・レジスタ(SIRmn) */
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
	/* ユニット0のチャネル2 */
	SIR02 = 0x0003;	/* パリティ・エラー・フラグ、オーバーラン・エラー・フラグをクリア */

	/* シリアル・モード・レジスタ(SMRmn) */
	/*
	   15    14    13    12    11    10     9     8     7     6     5     4     3     2     1     0
	+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
	| CKS | CCS |   0 |   0 |   0 |   0 |   0 | STS |   0 | SIS |   1 |   0 |   0 | MD2 | MD1 | MD0 |
	+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
	CKS
	  0: fMCKにCK00を使用
	  1: fMCKにCK01を使用
	CCS
	  0: fTCLKにCKSビットで指定した動作クロックfMCKの分周クロックを使用
	  1: fTCLKにSCKp端子からの入力クロックfSCKを使用(CSIモードのスレーブ転送)
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
	/* ユニット0のチャネル2 */
	SMR02 = 0x8024;		/* fTMCKにCK01、簡易I2Cモード、転送完了割り込み */

	/* シリアル通信動作設定レジスタ(SCRmn) */
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
	/* ユニット0のチャネル2 */
	SCR02 = 0x0017;		/* 通信禁止 */

	/* シリアル・データ・レジスタ(SDRmn) */
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
	/* ユニット0のチャネル2 */
	SDR02 = (sdr << 9);

	/* シリアル出力レジスタ(SOm) */
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
	SO0 |= 0x0404;		/* チャネル1のシリアル・データ出力値、シリアル・クロック出力値をHIGH */

	/* Set SCL10, SDA10 pin */

	/* ポート・モード・コントロール・レジスタ0 */
	/*
	    7     6     5     4     3     2     1     0
	+-----+-----+-----+-----+-----+-----+-----+-----+
	|   1 |   1 |   1 |   1 |PMC03|PMC02|PMC01|PMC00|
	+-----+-----+-----+-----+-----+-----+-----+-----+
	PMC03,PMC02,PMC01,PMC00
	  0: デジタル入出力、1:アラログ入力
	*/
	PMC0 &= 0xF7u;		/* P03をデジタル入出力 */

	/* ポート・レジスタ */
	P0 |= 0x18u;		/* P04、P03をHIGH */

	/* ポート・モード・レジスタ */
	/* 0: 出力モード、1:入力モード */
	PM0 &= 0xE7u;		/* P04を出力、P03を出力 */

	/* ポート出力モード・レジスタ */
	POM0 |= 0x18;		/* b0-b7 0:通常出力モード、1:N-chオープン・ドレイン出力 */
}
/*
	I2C停止
*/
void I2C_Stop_IIC01(void)
{
	IICMK10 = 1u;		/* チャネル2の割り込み禁止(INTIIC10) */
	ST0 |= 0x0004;		/* チャネル2の動作を停止 */
	/* 割り込み要求フラグ初期化 */
	IICIF10 = 0u;		/* チャネル2の割り込み(INTIIC10) */
}
/*
	送信

	adr : スレーブアドレス
	data: 送信データ
	len : 送信データバイト数
	cbFunc: 送信済時のコールバック関数
*/
void I2C_MasterSend_IIC01(uint8_t adr, uint8_t * const data, uint16_t len, void (*cbFunc)(void))
{
	s_i2c_MasterStatus = I2C_SEND_FLAG;
	sp_i2c_CallbackMasterSend_IIC01 = cbFunc;

	/* Set paramater */
	s_i2c_TxCount_IIC01 = len;
	sp_i2c_TxData = data;

	/* 割り込み要求フラグ初期化 */
	IICIF10 = 0u;		/* チャネル2の割り込み(INTIIC10) */
	IICMK10 = 0u;	 	/* チャネル2の割り込み許可(INTIIC10) */

	SIO10 = W_MODE(adr);

	if (sp_i2c_CallbackMasterSend_IIC01 == NULL) {
		while(s_i2c_MasterStatus != I2C_MASTER_FLAG_CLEAR)
		{
			;
		}
	}
}
/*
	受信

	adr  : スレーブアドレス
	rxBuf: 受信データバッファ
	rxNum: 受信データバイト数
	cbFunc: 受信済時のコールバック関数
*/
void I2C_MasterReceive_IIC01(uint8_t adr, uint8_t * const rxBuf, uint16_t rxNum, void (*cbFunc)(void))
{
	s_i2c_MasterStatus = I2C_RECEIVE_FLAG;
	sp_i2c_CallbackMasterReceive_IIC01 = cbFunc;

	s_i2c_RxLength = rxNum;
	s_i2c_RxCount = 0u;
	sp_i2c10_RxData = rxBuf;

	IICIF10 = 0u;		/* チャネル2の割り込み(INTIIC10) */
	IICMK10 = 0u;	 	/* チャネル2の割り込み許可(INTIIC10) */

	SIO10 = R_MODE(adr);

	if (sp_i2c_CallbackMasterReceive_IIC01 == NULL) {
		while(s_i2c_MasterStatus != I2C_MASTER_FLAG_CLEAR)
		{
			;
		}
	}
}
/*
	スタート・コンディション
*/
void I2C_StartCondition_IIC01(void)
{
	s_i2c_MasterStatus = I2C_MASTER_FLAG_CLEAR;
	s_I2C_IsReadingWriteing_IIC01 = TRUE;

	SCR02 &= ~(0xc000);	/* 通信禁止 */
	SCR02 |= 0x8000;	/* 送信のみ */

	SO0 &= ~(0x0004);	/* SDAをLowにする */

	/* 待ち */
	i2c_Wait();

	SO0 &= ~(0x0400);	/* SCLをLowにする */

	/* シリアル出力許可レジスタ */
	SOE0 |= (0x0004);	/* チャネル2の出力を許可 */
	/* シリアル・チャネル開始レジスタ */
	SS0 |= 0x0004u;		/* チャネル2の通信を許可 */
}
/*
	リスタート・コンディション
*/
void I2C_ReStartCondition_IIC01(void)
{
	s_i2c_MasterStatus = I2C_MASTER_FLAG_CLEAR;

	/* シリアル・チャネル停止レジスタ */
	ST0 |= (0x0004);	/* チャネル2の動作を停止 */
	/* シリアル出力許可レジスタ */
	SOE0 &= ~(0x0004);	/* チャネル2の出力を禁止 */

	SO0 |= (0x0404);	/* SDA,SCLをHighにする */

	/* 待ち */
	i2c_Wait();

	SO0 &= ~(0x0004);	/* SDAをLowにする */

	/* 待ち */
	i2c_Wait();

	SO0 &= ~(0x0400); 	/* SCLをLowにする */

	/* シリアル出力許可レジスタ */
	SOE0 |= (0x0004);	/* チャネル2の出力を許可 */
	/* シリアル・チャネル開始レジスタ */
	SS0 |= 0x0004u;		/* チャネル2の通信を許可 */
}
/*
	ストップ・コンディション
*/
void I2C_StopCondition_IIC01(void)
{
	int i;
	/* シリアル・チャネル停止レジスタ */
	ST0 |= (0x0004);	/* チャネル2の動作を停止 */
	/* シリアル出力許可レジスタ */
	SOE0 &= ~(0x0004);	/* チャネル2の出力を禁止 */

	SO0 &= ~(0x0004);	/* SDAをLowにする */
	SO0 |= (0x0400); 	/* SCLをHighにする */

	/* 待ち */
	i2c_Wait();

	SO0 |= (0x0004); 	/* SDAをHighにする */

	/* 待ち */
	i2c_Wait();

	s_i2c_MasterStatus = I2C_MASTER_FLAG_CLEAR;
	s_I2C_IsReadingWriteing_IIC01 = FALSE;
}
/*
	書き込み

	adr: スレーブアドレス
	data: 書き込みデータ
	len: 書き込みデータバイト数
*/
void I2C_Write_IIC01(uint8_t adr, uint8_t *data, uint16_t len)
{
	/* スタート・コンディション */
	I2C_StartCondition_IIC01();
	/* 送信 */
	I2C_MasterSend_IIC01(adr, data, len, I2C_StopCondition_IIC01);
}
/*
	読み込み

	adr: スレーブアドレス
	data: 書き込みデータ
	len: 書き込みデータバイト数
*/
void I2C_Read_IIC01(uint8_t adr ,uint8_t *data, uint16_t len)
{
	/* スタート・コンディション */
	I2C_StartCondition_IIC01();
	/* 受信 */
	I2C_MasterReceive_IIC01(adr ,data, len, I2C_StopCondition_IIC01);
}
/*
	送受信状態
*/
bool_t I2C_IsReadingWriteing_IIC01(void)
{
	return s_I2C_IsReadingWriteing_IIC01;
}
/*
	送信完了割り込みハンドラ
	「CS+ for CA,CX V3.01.00 [19 Aug 2015] コード生成／端子図プラグイン V1.04.04.02 [31 Jul 2015]より」
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
	送信終了時の処理
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
	受信終了時の処理
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
	送受信エラー用コールバック関数

	flag: エラー状態
*/
static void i2c_CallbackMasterError_IIC01(MD_STATUS flag)
{
	s_i2c_MasterStatus = I2C_MASTER_FLAG_CLEAR;
}
/*
	ボーレート設定用レジスタ値計算

	bps:  転送レート(kHz)
	fCLK: メインクロック(kHz)
	prs:  PRSレジスタの算出結果
	sdr:  SDRレジスタの算出結果
*/
static int16_t i2c_CalcFrequencyDivision(uint16_t bps, uint16_t fCLK, uint8_t *prs, uint16_t *sdr)
{
	uint8_t		i;
	uint32_t	c_sdr;
	uint16_t	ret = -1;

	/* 「12.8.5 転送レートの算出」から逆算 */
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
	待ち
*/
void i2c_Wait(void)
{
	Wait(1000 / s_i2c_Clock);
}
