/*
*Function:
*Programed by:
*Complete date:
*Modified by:
*Modified date:
*Version:
*Remarks:
*/
#include "App_InfShow.h"
#include "App_Language.h"

/*
*Function:
*Input Paragramed:
*Output Paragramed:
*Remarks:
*/
void App_StartInf(void)
{
	
	RCC_ClocksTypeDef get_rcc_clock; 
	RCC_GetClocksFreq(&get_rcc_clock);

	USR_UsrLog("****************************************");
	USR_UsrLog("****************************************");
	USR_UsrLog("****************************************");
	USR_UsrLog("****************************************");
	USR_UsrLog("****************************************");
	USR_UsrLog("****************************************");
	USR_UsrLog("****************************************");
	USR_UsrLog("****************************************");
	USR_UsrLog("             %s--%s   ", __TIME__, __DATE__);
	USR_UsrLog("SYSCLK_Frequency=%d", get_rcc_clock.SYSCLK_Frequency);
	USR_UsrLog("HCLK_Frequency=%d", get_rcc_clock.HCLK_Frequency);
	USR_UsrLog("PCLK1_Frequency=%d", get_rcc_clock.PCLK1_Frequency);
	USR_UsrLog("PCLK2_Frequency=%d", get_rcc_clock.PCLK2_Frequency);
	USR_UsrLog("****************************************");
	USR_UsrLog("****************************************");
	USR_UsrLog("****************************************");
	USR_UsrLog("****************************************");
	USR_UsrLog("****************************************");
	USR_UsrLog("****************************************");
	USR_UsrLog("****************************************");
	USR_UsrLog("****************************************");
	USR_UsrLog("****************************************");
	
}







/*
End of files
*/
