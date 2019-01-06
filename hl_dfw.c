#include "hl_common.h"
#include <string.h>

/* �t���b�V���E�Z���t�E�v���O���~���O�E���C�u���� */
#include "pfdl.h"                /* ���C�u�����E�w�b�_�[�t�@�C��             */
#include "pfdl_types.h"          /* ���C�u�����E�w�b�_�[�t�@�C��             */
#include "hl_dfw.h"

/* �����؂�ւ��p��` */
#define R_PFDL_SAM_TARGET_ERASE TRUE    /* ���O�����ݒ�(TRUE�Ŏ��s)         */
#define R_PFDL_SAM_DIRECT_READ  TRUE    /* �f�[�^�E�t���b�V�����ړǍ��ݒ�   */
                                        /* ���f�[�^�E�t���b�V����ON�̏�Ԃ� */
                                        /*   �o�C�g�E�A�N�Z�X���s���̂݉\ */
/* ��{�f�[�^ */
#define R_PFDL_SAM_BLOCK_SIZE   0x400l  /* �W���u���b�N�E�T�C�Y             */
#define R_PFDL_SAM_TARGET_BLOCK 0       /* �������݊J�n�u���b�N(0:F1000�Ԓn)*/
#define R_PFDL_SAM_WRITE_SIZE   10      /* �������݃f�[�^�T�C�Y             */
#define R_PFDL_SAM_DREAD_OFSET  0x1000  /* ���ړǂݍ��݃I�t�Z�b�g�E�A�h���X */

/* PFDL�����ݒ�l */
#define R_PFDL_SAM_FDL_FRQ      32      /* ���g���ݒ�(32Mhz)                */
#define R_PFDL_SAM_FDL_VOL      0x00    /* �d�����[�h(�t���X�s�[�h���[�h)   */

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

// �폜
int dfw_Erase(void)
{
	/* ���ʕϐ��錾 */
	pfdl_status_t	dtyFdlResult;		/* ���^�[���l						*/
	pfdl_request_t	dtyRequester;		/* PFDL����p�ϐ�(���N�G�X�^)		*/

	/* FDL�̏��������� */
	dtyFdlResult = dfw_start();

	if (dtyFdlResult == PFDL_OK) {
		/* �Ώۃu���b�N�̏�����������/�����R�}���h�ݒ� */
		dtyRequester.command_enu = PFDL_CMD_ERASE_BLOCK;

		/* �Ώۃu���b�N�̃u���b�N�ԍ���ݒ� */
		dtyRequester.index_u16 = R_PFDL_SAM_TARGET_BLOCK;

		/* �R�}���h���s���� */
		dtyFdlResult = PFDL_Execute( &dtyRequester );

		/* �R�}���h�I���҂� */
		while (dtyFdlResult == PFDL_BUSY) {
			NOP();
			NOP();
			/* �I���m�F���� */
			dtyFdlResult = PFDL_Handler();
		}
	}

	dfw_end();

	return 0;
}
/*
	�������߂�ʒu������
*/
pfdl_status_t dfw_searchWriteAddr(pfdl_u16 dataSize, pfdl_u16 *duhWriteAddress)
{
	pfdl_status_t	dtyFdlResult;		/* ���^�[���l						*/
	pfdl_request_t	dtyRequester;		/* PFDL����p�ϐ�(���N�G�X�^)		*/

	/* �u�����N�E�`�F�b�N���s�� */
	dtyRequester.index_u16	   = 0;
	dtyRequester.command_enu   = PFDL_CMD_BLANKCHECK_BYTES;
	dtyRequester.bytecount_u16 = dataSize;

	/* �������݃`�F�b�N���[�v */
	for (*duhWriteAddress = 0; *duhWriteAddress < R_PFDL_SAM_BLOCK_SIZE; (*duhWriteAddress)++)
	{
		/* �J�n�A�h���X�Ə������݃f�[�^�T�C�Y��ݒ肷�� */
		dtyRequester.index_u16 = *duhWriteAddress;

		/* �R�}���h���s���� */
		dtyFdlResult = PFDL_Execute( &dtyRequester );

		/* �R�}���h�I���҂� */
		while( dtyFdlResult == PFDL_BUSY ) {
			/* �I���m�F���� */
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
	/* ���ʕϐ��錾 */
	pfdl_status_t	dtyFdlResult;		/* ���^�[���l						*/
	pfdl_request_t	dtyRequester;		/* PFDL����p�ϐ�(���N�G�X�^)		*/
	pfdl_u16		duhWriteAddress;	/* �������ݗp�ϐ���` */
	unsigned char	writeSize;
	int				i;
	unsigned char	writeBuff[256];
	unsigned char	*headerTop;
	unsigned char	dataNum;

    /* ���荞�݋��� */
    EI();
    
	/* FDL�̏���������		*/
	dtyFdlResult = dfw_start();
	if (dtyFdlResult != PFDL_OK)
	{
		dfw_end();
		return -1;
	}

	/* �������݈ʒu���� */
	writeSize = dataSize + DFW_HEADER_SIZE;
	dtyFdlResult = dfw_searchWriteAddr(writeSize, &duhWriteAddress);
	if (dtyFdlResult != PFDL_OK) {
		dfw_end();
		return -1;
	}

	/* �f�[�^�Ǘ��ԍ����� */
	headerTop = (unsigned char *)R_PFDL_SAM_DREAD_OFSET;
	dataNum = 0;
	while (headerTop < (unsigned char *)((R_PFDL_SAM_DREAD_OFSET + duhWriteAddress))) {
		if (((dfwHeader *)headerTop)->no == dataNo)
		{
			dataNum = ((dfwHeader *)headerTop)->num;
		}
		headerTop += ((dfwHeader *)headerTop)->size;
	}

	// �������݃f�[�^����
	writeBuff[0] = dataNo;		// no
	writeBuff[1] = dataSize;	// size
	writeBuff[2] = dataNum;		// num
	writeBuff[3] = 0;			// dummy
	memcpy(&(writeBuff[4]), data, writeSize);
	
	/* �������݃R�}���h��ݒ� */
	dtyRequester.index_u16	   = duhWriteAddress;
	dtyRequester.bytecount_u16 = writeSize;
	dtyRequester.data_pu08	   = writeBuff;
	dtyRequester.command_enu = PFDL_CMD_WRITE_BYTES;
	
	/* �R�}���h���s���� */
	dtyFdlResult = PFDL_Execute( &dtyRequester );
	
	/* �R�}���h�I���҂� */
	while( dtyFdlResult == PFDL_BUSY )
	{
		NOP();
		NOP();
		dtyFdlResult = PFDL_Handler();
	}
	
	/* �������݂�����I������������x���t�@�C���������s */
	if( dtyFdlResult == PFDL_OK )
	{
		/* �����x���t�@�C�R�}���h��ݒ� */
		dtyRequester.command_enu = PFDL_CMD_IVERIFY_BYTES;
		
		/* �R�}���h���s���� */
		dtyFdlResult = PFDL_Execute( &dtyRequester );
		
		/* �R�}���h�I���҂� */
		while( dtyFdlResult == PFDL_BUSY )
		{
			NOP();
			NOP();
			/* �I���m�F���� */
			dtyFdlResult = PFDL_Handler();
		}
	}
	/* FDL�I������ */
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
    
    /* �����l���� */
    dtyDescriptor.fx_MHz_u08            = R_PFDL_SAM_FDL_FRQ;  /* ���g���ݒ� */
    dtyDescriptor.wide_voltage_mode_u08 = R_PFDL_SAM_FDL_VOL;  /* �d�����[�h */
    
    /* PFDL�������֐����s */
    dtyFdlResult = PFDL_Open( &dtyDescriptor );
    
    return dtyFdlResult;
}

void dfw_end(void)
{
    /* �f�[�^�E�t���b�V���E���C�u�����I������ */
    PFDL_Close();
}

