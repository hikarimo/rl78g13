#ifndef _DFW_H_
#define _DFW_H_

/******************************************************************************
 Include <System Includes>, "Project Includes"
******************************************************************************/
/* 標準ライブラリ(ランタイム・ライブラリ使用) */
#include <string.h>             /*  */

/* フラッシュ・セルフ・プログラミング・ライブラリ */
#include "pfdl.h"                /* ライブラリ・ヘッダーファイル             */
#include "pfdl_types.h"          /* ライブラリ・ヘッダーファイル             */

/******************************************************************************
 Prototype declarations
******************************************************************************/
extern pfdl_status_t dfw_start (void);
extern void dfw_end (void);
extern int dfw_Erase(void);
extern int dfw_Write(unsigned char dataNo, unsigned char *data, unsigned char dataSize);
extern int r_pfdl_read(unsigned char dataNo, unsigned char *data);

#endif // !_DFW_H_
