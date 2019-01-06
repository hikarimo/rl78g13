/*
	(C) 2016 hikarimo
*/

#pragma interrupt INTIT INTERVAL_TIMER_IntervalInterrupt

#include "hl_common.h"
#include "hl_rl78g13_interval_timer.h"

/*
	�C���^�[�o���E�^�C�}�ݒ�
*/
void INTERVAL_TIMER_Setup(void)
{
	// ���ӃC�l�[�u���E���W�X�^
	RTCEN = 1u;			// 0:�N���b�N������~�A1:����
	// �C���^�[�o���E�^�C�}�E�R���g���[���E���W�X�^
	ITMC = 0u;			// b7 0:�J�E���^�����~(�J�E���g�E�N���A)�A1:�J�E���^����J�n
						// b6-b4: 0�Œ�
						// b3-b0: 12�r�b�g�E�C���^�[�o���E�^�C�}�̃R���y�A�l
	ITMK = 1U;			// 0:���荞�݋��A1:���荞�݋֎~
	ITIF = 0U;			// ���荞�݃t���O������
	ITPR1 = 1U;			// ���荞�ݗD��x
	ITPR0 = 1U;			// 
	ITMC = 0x0EA5;		// 
}

/*
	�C���^�[�o���E�^�C�}�J�n
*/
void INTERVAL_TIMER_Start(void)
{
	ITIF = 0U;
	ITMK = 0U;
	ITMC |= 0x8000;
}
/*
	�C���^�[�o���E�^�C�}��~
*/
void INTERVALTIMER_Stop(void)
{
	ITMK = 1U;
	ITIF = 0U;
	ITMC &= (unsigned short)~(0x8000);
}
/*
	�C���^�[�o���E�^�C�}�̊��荞�݃n���h��
*/
__interrupt static void INTERVAL_TIMER_IntervalInterrupt(void)
{
	static unsigned char c = 0;

	// �����ɏ���������
	c = !c;
	P0.0 = c;
}
