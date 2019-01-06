#ifndef _DFW_H_
#define _DFW_H_

/******************************************************************************
 Include <System Includes>, "Project Includes"
******************************************************************************/
/* �W�����C�u����(�����^�C���E���C�u�����g�p) */
#include <string.h>             /*  */

/* �t���b�V���E�Z���t�E�v���O���~���O�E���C�u���� */
#include "pfdl.h"                /* ���C�u�����E�w�b�_�[�t�@�C��             */
#include "pfdl_types.h"          /* ���C�u�����E�w�b�_�[�t�@�C��             */

/******************************************************************************
 Prototype declarations
******************************************************************************/
extern pfdl_status_t dfw_start (void);
extern void dfw_end (void);
extern int dfw_Erase(void);
extern int dfw_Write(unsigned char dataNo, unsigned char *data, unsigned char dataSize);
extern int r_pfdl_read(unsigned char dataNo, unsigned char *data);

#endif // !_DFW_H_
