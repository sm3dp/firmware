#ifndef __USART_H
#define __USART_H
#include "stdio.h"	
#include "sys.h" 





#define EN_USART1_RX 			1		

#define RX_BUFFER_SIZE 128
typedef struct ring_buffer
{
  unsigned char buffer[RX_BUFFER_SIZE];
  int head;
  int tail;
}ring_buffer;
extern  ring_buffer rx_buffer;	
extern  ring_buffer rx_buffer2;	


void checkDTR(void);
void uart1_init(u32 bound);
void checkRx(void);
unsigned int MYSERIAL_available(void);
unsigned int MYSERIAL_available2(void);

u8 MYSERIAL_read(void);
u8 MYSERIAL_read2(void);

void MYSERIAL_flush(void);

void usart1_send(u8* str);
void usart2_send(char* str);

void uart2_init(u32 bound);

#endif


