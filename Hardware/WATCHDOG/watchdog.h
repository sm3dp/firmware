#ifndef WATCHDOG_H
#define WATCHDOG_H
#include "sys.h"

void IWDG_Init(u8 prer,u16 rlr);
void IWDG_Feed(void);

#endif
