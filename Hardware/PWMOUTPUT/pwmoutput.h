#ifndef __PWMOUTPUT_H
#define __PWMOUTPUT_H
#include "sys.h"

void TIM3_Int_Init(u16 arr,u16 psc);
void TIM3_PWM_Init(u16 arr,u16 psc);

#endif
