#ifndef _WWDG_H
#define _WWDG_H
#include "sys.h"
 
void WWDG_Init(u8 tr,u8 wr,u32 fprer);
void WWDG_IRQHandler(void);
#define FUN_1 'a'
#define FUN_2 'b'
#define FUN_3 'c'
#define FUN_4 'd'
#define FUN_5 'e'
#define FUN_6 'f'
#define FUN_7 'g'
#define FUN_8 'h'
#define FUN_9 'i'
#define FUN_10 'j'
#define FUN_11 'k'
#define FUN_12 'l'
#define FUN_13 'm'
#define FUN_14 'n'
#define FUN_15 'o'

#define IRQ_1 '0'
#define IRQ_2 '1'
#define IRQ_3 '2'
#define IRQ_4 '3'
#define IRQ_5 '4'
#define IRQ_6 '5'
#define IRQ_7 '6'
#define IRQ_8 '7'
#define IRQ_9 '8'
#define IRQ_10 '9'
extern u8 FunctionCode;
extern u8 WWDG_CNT;




#endif


