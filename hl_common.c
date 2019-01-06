/*
	(C) 2016 hikarimo
*/
#include "hl_common.h"

/*
	�K���ɃE�G�C�g

	msec : �E�G�C�g����(us)
*/
void Wait(int16_t usec)
{
	uint32_t cnt;

	/* ��1us x usec */
	cnt = (HW_FCLK  * (uint32_t)usec) / 1000 / 15;

	/* ���̃��[�v��15�N���b�N���炢�������� */
	while(cnt) {
		NOP();
		cnt--;
	}
}
