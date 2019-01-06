/*
	(C) 2016 hikarimo
*/
#include "hl_common.h"

/*
	適当にウエイト

	msec : ウエイト時間(us)
*/
void Wait(int16_t usec)
{
	uint32_t cnt;

	/* 約1us x usec */
	cnt = (HW_FCLK  * (uint32_t)usec) / 1000 / 15;

	/* このループが15クロックくらいを見込む */
	while(cnt) {
		NOP();
		cnt--;
	}
}
