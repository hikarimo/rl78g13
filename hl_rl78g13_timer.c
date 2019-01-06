/*
	(C) 2016 hikarimo
*/

#pragma interrupt INTTM00 TIMER_TAU0_CH0_IntervalInterrupt

#include "hl_common.h"
#include "hl_rl78g13_timer.h"

/*
	�C���^�[�o���E�^�C�}�ݒ�
*/
void TIMER_Setup_TAU0(void)
{
	// ���ӃC�l�[�u���E���W�X�^
	TAU0EN = 1u;		// 0:�N���b�N������~�A1:����
	// �^�C�}�E�N���b�N�I�����W�X�^
	TPS0 = 0x0;			// 0:fCLK�A1:fCLK/2�A2:fCLK/4�A3:fCLK/8�A
						// 4:fCLK/16�A5:fCLK/32�A6:fCLK/64�A7:fCLK/128�A
						// 8:fCLK/256�A9:fCLK/512�A10:fCLK/1024�A11:fCLK/2048�A
						// 12:fCLK/4096�A13:fCLK/8192�A14:fCLK/16384�A15:fCLK/32768

	// �T�u�V�X�e���E�N���b�N�������[�h���W�X�^(OSMC)
	//   hl_rl78g13_hwsetup.c(clockSetup�֐�)�Őݒ�

	// �^�C�}�E�`�����l����~���W�X�^
	TT0 = 0xff;			// b15-12: 0�Œ�
						// b11: 0:�`���l��3�̏�ʑ�8�r�b�g�E�^�C�}�̃g���K���삵�Ȃ��A1:����
						// b10: 0�Œ�
						// b9:  0:�`���l��1�̏�ʑ�8�r�b�g�E�^�C�}�̃g���K���삵�Ȃ��A1:����
						// b8:  0�Œ�
						// b7-b0: 

	TMMK00 = 1u;		// 0:���荞�ݏ������A1:�֎~
	TMIF00 = 0u;		// ���荞�݃t���O������

	// 
	TMPR000 = 1u;		// 0,1 ���荞�ݗD�惌�x���̑I��
	TMPR100 = 1u;		//   00:���x��0(���D�揇��)
						//   01:���x��1
						//   10:���x��2
						//   11:���x��3(��D�揇��)

	// �^�C�}�E���[�h�E���W�X�^
	TMR00 = 0;			// b15-14: ����N���b�N�I��
						//   00: CK00��I��
						//   01: CK02��I��
						//   10: CK01��I��
						//   11: CK03��I��
						// b13: 0�Œ�
						// b12: 0:����N���b�N���g�p�A1:TI00�[�q����̓��͐M�����g�p
						// b11: �`���l��2,4,6�̏ꍇ
						//      0:�Ɨ��`���l������A1:�����`���l���A���Ń}�X�^�E�`�����l���Ƃ��ē���
						// b11: �`���l��1,3�̏ꍇ
						//      0:16�r�b�g�E�^�C�}�Ƃ��ē���(�P�ƁA�܂��͘A������̃X���[�u)�A1:8�r�b�g�E�^�C�}�Ƃ��ē���
						// b11: �`���l��0,5,7�̏ꍇ 0�Œ�
						// b10-b8: �X�^�[�g�E�g���K�A�L���v�`���E�g���K
						//   000: 

	TDR00 = 3200u;		// �^�C�}�E�f�[�^�E���W�X�^(�C���^�[�o������)
						//   32MHz��100us�ɂ���Ȃ�A3200�ɂȂ�B

	TO0 &= ~(1u);		// b15-b8: 0�Œ�
						//  b7-b0: 0:�`���l��7�`0�̃^�C�}�o�͒[�q�̏o�͒l��0�A1:1

	TOE0 &= ~(1u);		// b15-b8: 0�Œ�
						//  b7-b0: 0:TO7�`0�̑��싖�A1:�֎~
}

/*
	�C���^�[�o���E�^�C�}�J�n
*/
void TIMER_Start_TAU0_CH0(void)
{
	TMIF00 = 0u;		// 
	TMMK00 = 0u;		// 
	TS0 |= 1u;			// �^�C�}�E�`���l���J�n���W�X�^
}
/*
	�C���^�[�o���E�^�C�}��~
*/
void TIMER_Stop_TAU0_CH0(void)
{
	TT0 |= 1u;
	TMMK00 = 1u;
	TMIF00 = 0u;
}
/*
	�C���^�[�o���E�^�C�}�̊��荞�݃n���h��
*/
__interrupt static void TIMER_TAU0_CH0_IntervalInterrupt(void)
{
	static unsigned char c = 0;

	// �����ɏ���������
	c = !c;
	P0.0 = c;
}
