#include <stdio.h>
#include <stdarg.h>

#if 1
#define	INT_DEC_MAX_ORDER	(1000000000u)
#define	INT_HEX_MAX_ORDER		(0x10000000)
#define	INT_DEC_MAX_ORDER_NUM	(10)
#define	INT_HEX_MAX_ORDER_NUM	(8)
#else
#define	INT_DEC_MAX_ORDER		(10000u)
#define	INT_HEX_MAX_ORDER		(0x1000)
#define	INT_DEC_MAX_ORDER_NUM	(5)
#define	INT_HEX_MAX_ORDER_NUM	(4)
#endif

typedef enum {FMT_NORM, FMT_PERCENT} FMT_STS;

/* ƒoƒbƒtƒ@Ši”[ */
void PrintDebugLog(const char *fmt, ...);

int main(void)
{
	printf("--1\n");
	PrintDebugLog("%d %d\n", 10, 23456);
	PrintDebugLog("%x %x\n", 10, 23456);
	PrintDebugLog("%d %d\n", -10, -23456);
	PrintDebugLog("%x %x\n", -10, -23456);
	PrintDebugLog("%d %u %x\n", 20, 20, 20);
	PrintDebugLog("%d %u %x\n", 0, 0, 0);
	PrintDebugLog("%s %x\n", "abc", 10);
	printf("--2\n");
	PrintDebugLog("%4d\n", 10);
	PrintDebugLog("%3d\n", 10);
	PrintDebugLog("%2d\n", 10);
	printf("--3\n");
	PrintDebugLog("%04d\n", 10);
	PrintDebugLog("%03d\n", 10);
	PrintDebugLog("%02d\n", 10);
	PrintDebugLog("%6d %8d\n", -10, -23456);
	PrintDebugLog("%6x %8x\n", -10, -23456);
	PrintDebugLog("%06d %08d\n", -10, -23456);
	PrintDebugLog("%06x %08x\n", -10, -23456);
	printf("--4\n");
	PrintDebugLog("0x%04X\n", 0x1A);
	PrintDebugLog("0x%04X\n", 0x12A);
	PrintDebugLog("0x%04X\n", 0x123A);

	PrintDebugLog("%02X\n", 0xA);

	return 0;
}
/*
	
*/
void PrintDebugLog(const char *fmt, ...)
{
	va_list			marker;
	FMT_STS			fmtSts;
	int				argI;
	char			*argS;
	int				fmtIdx;
	int				strIdx;
	unsigned int	uiNum;
	unsigned int	order;
	unsigned int	oneOrder;
	unsigned int	orderNum;
	char			outChar;
	unsigned char	printFlg;
	unsigned char	minusFlg;
	int 			width;
	char			flagStr;
	unsigned char	hexLetterStr;

	va_start(marker, fmt);

	vprintf(fmt, marker);

	fmtSts = FMT_NORM;
	fmtIdx = 0;
	while((outChar = fmt[fmtIdx]) != '\0') {
		if (outChar == '%') {
			flagStr = ' ';
			width = 0;
			minusFlg = 0;
			printFlg = 0;
			fmtSts = FMT_PERCENT;
			hexLetterStr = 'a';
		} else if (fmtSts == FMT_PERCENT) {
			if ('0' == outChar) {
				flagStr = '0';
			} else if (('1' <= outChar) && (outChar <= '9')) {
				width = outChar - '0';
			} else if (outChar == 's') {
				argS = va_arg(marker, char*);
				strIdx = 0;
				while(argS[strIdx] != '\0') {
					putchar(argS[strIdx]);
					strIdx++;
				}
				fmtSts = FMT_NORM;
			} else {
				argI = va_arg(marker, int);
				switch(outChar) {
				case 'd':
					order = INT_DEC_MAX_ORDER;
					orderNum = INT_DEC_MAX_ORDER_NUM;
					oneOrder = 10;
					if (argI < 0) {
						uiNum = -argI;
						minusFlg = 1;
					} else {
						uiNum = argI;
					}
					break;
				case 'u':
					order = INT_DEC_MAX_ORDER;
					orderNum = INT_DEC_MAX_ORDER_NUM;
					oneOrder = 10;
					uiNum = (unsigned int)argI;
					break;
				case 'X':
					hexLetterStr = 'A';
				case 'x':
					order = INT_HEX_MAX_ORDER;
					orderNum = INT_HEX_MAX_ORDER_NUM;
					oneOrder = 0x10;
					uiNum = (unsigned int)argI;
					break;
				}
				while(order) {
					outChar = uiNum / order;
					uiNum %= order;
					order /= oneOrder;
					if (outChar > 0) {
						printFlg = 1;
					}
					if (printFlg == 1) {
						if (minusFlg == 1) {
							minusFlg = 0;
							putchar('-');
						}
						if (outChar > 9) {
							putchar(outChar + hexLetterStr - 10);
						} else {
							putchar(outChar + '0');
						}
					} else {
						if (orderNum == 1) {
							putchar('0');
						} else if (orderNum <= width) {
							if (minusFlg == 1) {
								if ((flagStr == '0') ||
									(((uiNum / order) > 0) && (minusFlg == 1))) {
									putchar('-');
									minusFlg = 0;
								} else {
									putchar(flagStr);
								}
							} else {
								putchar(flagStr);
							}
						}
					}
					orderNum--;
				}
				fmtSts = FMT_NORM;
			}
		} else {
			putchar(fmt[fmtIdx]);
		}
		fmtIdx++;
	}
}
