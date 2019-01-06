#include "hl_common.h"
#include <string.h>

/* フラッシュ・セルフ・プログラミング・ライブラリ */
#include "pfdl.h"                /* ライブラリ・ヘッダーファイル             */
#include "pfdl_types.h"          /* ライブラリ・ヘッダーファイル             */
#include "hl_dfw.h"

/* 処理切り替え用定義 */
#define R_PFDL_SAM_TARGET_ERASE TRUE    /* 事前消去設定(TRUEで実行)         */
#define R_PFDL_SAM_DIRECT_READ  TRUE    /* データ・フラッシュ直接読込設定   */
                                        /* ※データ・フラッシュがONの状態で */
                                        /*   バイト・アクセス実行時のみ可能 */
/* 基本データ */
#define R_PFDL_SAM_BLOCK_SIZE   0x400l  /* 標準ブロック・サイズ             */
#define R_PFDL_SAM_TARGET_BLOCK 0       /* 書き込み開始ブロック(0:F1000番地)*/
#define R_PFDL_SAM_WRITE_SIZE   10      /* 書き込みデータサイズ             */
#define R_PFDL_SAM_DREAD_OFSET  0x1000  /* 直接読み込みオフセット・アドレス */

/* PFDL初期設定値 */
#define R_PFDL_SAM_FDL_FRQ      32      /* 周波数設定(32Mhz)                */
#define R_PFDL_SAM_FDL_VOL      0x00    /* 電圧モード(フルスピードモード)   */

// No Len Num Data
typedef struct
{
	unsigned char no;
	unsigned char size;
	unsigned char num;
	unsigned char dummy;
} dfwHeader;
// no len num dummy data
//    4

#define	DFW_HEADER_SIZE	(4)

static pfdl_status_t dfw_searchWriteAddr(pfdl_u16 dataSize, pfdl_u16 *duhWriteAddress);
static pfdl_status_t dfw_start(void);
static void dfw_end(void);

// 削除
int dfw_Erase(void)
{
	/* 共通変数宣言 */
	pfdl_status_t	dtyFdlResult;		/* リターン値						*/
	pfdl_request_t	dtyRequester;		/* PFDL制御用変数(リクエスタ)		*/

	/* FDLの初期化処理 */
	dtyFdlResult = dfw_start();

	if (dtyFdlResult == PFDL_OK) {
		/* 対象ブロックの消去処理処理/消去コマンド設定 */
		dtyRequester.command_enu = PFDL_CMD_ERASE_BLOCK;

		/* 対象ブロックのブロック番号を設定 */
		dtyRequester.index_u16 = R_PFDL_SAM_TARGET_BLOCK;

		/* コマンド実行処理 */
		dtyFdlResult = PFDL_Execute( &dtyRequester );

		/* コマンド終了待ち */
		while (dtyFdlResult == PFDL_BUSY) {
			NOP();
			NOP();
			/* 終了確認処理 */
			dtyFdlResult = PFDL_Handler();
		}
	}

	dfw_end();

	return 0;
}
/*
	書き込める位置を検索
*/
pfdl_status_t dfw_searchWriteAddr(pfdl_u16 dataSize, pfdl_u16 *duhWriteAddress)
{
	pfdl_status_t	dtyFdlResult;		/* リターン値						*/
	pfdl_request_t	dtyRequester;		/* PFDL制御用変数(リクエスタ)		*/

	/* ブランク・チェックを行う */
	dtyRequester.index_u16	   = 0;
	dtyRequester.command_enu   = PFDL_CMD_BLANKCHECK_BYTES;
	dtyRequester.bytecount_u16 = dataSize;

	/* 書き込みチェックループ */
	for (*duhWriteAddress = 0; *duhWriteAddress < R_PFDL_SAM_BLOCK_SIZE; (*duhWriteAddress)++)
	{
		/* 開始アドレスと書き込みデータサイズを設定する */
		dtyRequester.index_u16 = *duhWriteAddress;

		/* コマンド実行処理 */
		dtyFdlResult = PFDL_Execute( &dtyRequester );

		/* コマンド終了待ち */
		while( dtyFdlResult == PFDL_BUSY ) {
			/* 終了確認処理 */
			dtyFdlResult = PFDL_Handler();
		}
		if (dtyFdlResult == PFDL_OK) {
			break;
		}
	}
	return dtyFdlResult;
}

int dfw_Write(unsigned char dataNo, unsigned char *data, unsigned char dataSize)
{
	/* 共通変数宣言 */
	pfdl_status_t	dtyFdlResult;		/* リターン値						*/
	pfdl_request_t	dtyRequester;		/* PFDL制御用変数(リクエスタ)		*/
	pfdl_u16		duhWriteAddress;	/* 書き込み用変数定義 */
	unsigned char	writeSize;
	int				i;
	unsigned char	writeBuff[256];
	unsigned char	*headerTop;
	unsigned char	dataNum;

    /* 割り込み許可 */
    EI();
    
	/* FDLの初期化処理		*/
	dtyFdlResult = dfw_start();
	if (dtyFdlResult != PFDL_OK)
	{
		dfw_end();
		return -1;
	}

	/* 書き込み位置検索 */
	writeSize = dataSize + DFW_HEADER_SIZE;
	dtyFdlResult = dfw_searchWriteAddr(writeSize, &duhWriteAddress);
	if (dtyFdlResult != PFDL_OK) {
		dfw_end();
		return -1;
	}

	/* データ管理番号検索 */
	headerTop = (unsigned char *)R_PFDL_SAM_DREAD_OFSET;
	dataNum = 0;
	while (headerTop < (unsigned char *)((R_PFDL_SAM_DREAD_OFSET + duhWriteAddress))) {
		if (((dfwHeader *)headerTop)->no == dataNo)
		{
			dataNum = ((dfwHeader *)headerTop)->num;
		}
		headerTop += ((dfwHeader *)headerTop)->size;
	}

	// 書き込みデータ生成
	writeBuff[0] = dataNo;		// no
	writeBuff[1] = dataSize;	// size
	writeBuff[2] = dataNum;		// num
	writeBuff[3] = 0;			// dummy
	memcpy(&(writeBuff[4]), data, writeSize);
	
	/* 書き込みコマンドを設定 */
	dtyRequester.index_u16	   = duhWriteAddress;
	dtyRequester.bytecount_u16 = writeSize;
	dtyRequester.data_pu08	   = writeBuff;
	dtyRequester.command_enu = PFDL_CMD_WRITE_BYTES;
	
	/* コマンド実行処理 */
	dtyFdlResult = PFDL_Execute( &dtyRequester );
	
	/* コマンド終了待ち */
	while( dtyFdlResult == PFDL_BUSY )
	{
		NOP();
		NOP();
		dtyFdlResult = PFDL_Handler();
	}
	
	/* 書き込みが正常終了したら内部ベリファイ処理を実行 */
	if( dtyFdlResult == PFDL_OK )
	{
		/* 内部ベリファイコマンドを設定 */
		dtyRequester.command_enu = PFDL_CMD_IVERIFY_BYTES;
		
		/* コマンド実行処理 */
		dtyFdlResult = PFDL_Execute( &dtyRequester );
		
		/* コマンド終了待ち */
		while( dtyFdlResult == PFDL_BUSY )
		{
			NOP();
			NOP();
			/* 終了確認処理 */
			dtyFdlResult = PFDL_Handler();
		}
	}
	/* FDL終了処理 */
	dfw_end();
	if (dtyFdlResult != PFDL_OK)
	{
		return -1;
	}
	else
	{
		return 0;
	}
}

int r_pfdl_read(unsigned char dataNo, unsigned char *data)
{

	return 0;
}

pfdl_status_t dfw_start(void)
{
    pfdl_status_t       dtyFdlResult;
    pfdl_descriptor_t   dtyDescriptor;
    
    /* 初期値入力 */
    dtyDescriptor.fx_MHz_u08            = R_PFDL_SAM_FDL_FRQ;  /* 周波数設定 */
    dtyDescriptor.wide_voltage_mode_u08 = R_PFDL_SAM_FDL_VOL;  /* 電圧モード */
    
    /* PFDL初期化関数実行 */
    dtyFdlResult = PFDL_Open( &dtyDescriptor );
    
    return dtyFdlResult;
}

void dfw_end(void)
{
    /* データ・フラッシュ・ライブラリ終了処理 */
    PFDL_Close();
}

