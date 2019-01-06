/*
	(C) 2016 hikarimo
*/

#ifndef _HL_RL78G13_UART_H_
#define _HL_RL78G13_UART_H_

#define UART0_SEND_BUFF_SIZE	(128)
#define UART0_PRINT_BUFF		(128)

extern void Uart_Setup_IIC00(uint32_t clk);
extern void Uart_Start_IIC00(void);
extern void Uart_Stop_IIC00(void);
extern int16_t Uart_Send_IIC00(int8_t * const data, uint16_t len);
extern int16_t Uart_Receive_IIC00(uint8_t *buff, int16_t len);
extern int16_t Uart_Putc_IIC00(int8_t c);
extern bool_t Uart_IsSending_IIC00(void);
extern bool_t Uart_IsReceiving_IIC00(void);
extern int16_t Uart_GetReceiveLen_IIC00(void);
extern int Uart_Printf_IIC00(const char *fmt, ...);

#endif	/* _HL_RL78G13_UART_H_ */
