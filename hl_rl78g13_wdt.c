/*
	(C) 2016 hikarimo
*/

// �E�H�b�`�h�b�O�E�^�C�}�̃C���^�[�o�����荞�ݓo�^
#pragma interrupt INTWDTI WDT_IntervalInterrupt

#include "hl_common.h"

/*
	�E�H�b�`�h�b�O�E�^�C�}�ݒ�
*/
void WDT_Setup(void)
{
	// �E�H�b�`�h�b�O�E�^�C�}�ݒ�
	//   -gb�I�v�V����(CA78K0R(�r���h�E�c�[��)->�����N�E�I�v�V����->���[�U�E�I�v�V�����E�o�C�g�l)
	//   �܂���hl_rl78g13_obtbyte.asm�ŃE�H�b�`�h�b�O�̐ݒ���s���B

	// �E�H�b�`�h�b�O�E�^�C�}�̊��荞�ݐݒ�
	WDTIMK = 1u;		// 0:���荞�݋��A1:�֎~
	WDTIIF = 0u;		// ���荞�݃t���O������(0:���荞�݂���A1:�Ȃ�)
	WDTIPR0 = 1u;		// 0,1 ���荞�ݗD�惌�x���̑I��
	WDTIPR1 = 1u;		//   00:���x��0(���D�揇��)
						//   01:���x��1
						//   10:���x��2
						//   11:���x��3(��D�揇��)
	WDTIMK = 0u;		// 0:���荞�݋��A1:�֎~
}
/*
	�E�H�b�`�h�b�O�E�^�C�}�̃J�E���^�N���A
*/
void WDT_Restart(void)
{
	WDTE = 0xAC;	// �E�H�b�`�h�b�O�E�^�C�}�̃J�E���^�N���A
					//   ACh�ȊO���������ނƓ������Z�b�g�ƂȂ�
}
/*
	�E�H�b�`�h�b�O�E�^�C�}�̃C���^�[�o�����荞�݃n���h��
	  ���荞�݋���(WDTIMK=0)�ł���΁A�I�[�o�[�t���[���Ԃ�75%+1/2fIL���B���ɌĂ΂��
*/
__interrupt static void WDT_IntervalInterrupt(void)
{
	// �I�[�o�[�t���[�O�ɂ�邱�Ƃ�����΁A�����ɏ����B
}
