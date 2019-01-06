/*
	(C) 2016 hikarimo
*/

#pragma interrupt INTIT INTERVAL_TIMER_IntervalInterrupt

#include "hl_common.h"
#include "hl_rl78g13_interval_timer.h"

/*
	インターバル・タイマ設定
*/
void INTERVAL_TIMER_Setup(void)
{
	// 周辺イネーブル・レジスタ
	RTCEN = 1u;			// 0:クロック供給停止、1:供給
	// インターバル・タイマ・コントロール・レジスタ
	ITMC = 0u;			// b7 0:カウンタ動作停止(カウント・クリア)、1:カウンタ動作開始
						// b6-b4: 0固定
						// b3-b0: 12ビット・インターバル・タイマのコンペア値
	ITMK = 1U;			// 0:割り込み許可、1:割り込み禁止
	ITIF = 0U;			// 割り込みフラグ初期化
	ITPR1 = 1U;			// 割り込み優先度
	ITPR0 = 1U;			// 
	ITMC = 0x0EA5;		// 
}

/*
	インターバル・タイマ開始
*/
void INTERVAL_TIMER_Start(void)
{
	ITIF = 0U;
	ITMK = 0U;
	ITMC |= 0x8000;
}
/*
	インターバル・タイマ停止
*/
void INTERVALTIMER_Stop(void)
{
	ITMK = 1U;
	ITIF = 0U;
	ITMC &= (unsigned short)~(0x8000);
}
/*
	インターバル・タイマの割り込みハンドラ
*/
__interrupt static void INTERVAL_TIMER_IntervalInterrupt(void)
{
	static unsigned char c = 0;

	// ここに処理を書く
	c = !c;
	P0.0 = c;
}
