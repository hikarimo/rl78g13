/*
	(C) 2016 hikarimo
*/

// ウォッチドッグ・タイマのインターバル割り込み登録
#pragma interrupt INTWDTI WDT_IntervalInterrupt

#include "hl_common.h"

/*
	ウォッチドッグ・タイマ設定
*/
void WDT_Setup(void)
{
	// ウォッチドッグ・タイマ設定
	//   -gbオプション(CA78K0R(ビルド・ツール)->リンク・オプション->ユーザ・オプション・バイト値)
	//   またはhl_rl78g13_obtbyte.asmでウォッチドッグの設定を行う。

	// ウォッチドッグ・タイマの割り込み設定
	WDTIMK = 1u;		// 0:割り込み許可、1:禁止
	WDTIIF = 0u;		// 割り込みフラグ初期化(0:割り込みあり、1:なし)
	WDTIPR0 = 1u;		// 0,1 割り込み優先レベルの選択
	WDTIPR1 = 1u;		//   00:レベル0(高優先順位)
						//   01:レベル1
						//   10:レベル2
						//   11:レベル3(低優先順位)
	WDTIMK = 0u;		// 0:割り込み許可、1:禁止
}
/*
	ウォッチドッグ・タイマのカウンタクリア
*/
void WDT_Restart(void)
{
	WDTE = 0xAC;	// ウォッチドッグ・タイマのカウンタクリア
					//   ACh以外を書き込むと内部リセットとなる
}
/*
	ウォッチドッグ・タイマのインターバル割り込みハンドラ
	  割り込み許可(WDTIMK=0)であれば、オーバーフロー時間の75%+1/2fIL到達時に呼ばれる
*/
__interrupt static void WDT_IntervalInterrupt(void)
{
	// オーバーフロー前にやることがあれば、ここに書く。
}
