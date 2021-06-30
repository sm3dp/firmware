/*Includes ------------------------------------------------------------------*/
#include "main.h"
#include "usart.h"
#include "exfuns.h"
#include "sys.h"
#include "delay.h"
#include "24cxx.h"
#include "sdio_sdcard.h"
#include "malloc.h"
#include "ff.h"
#include "exfuns.h"
#include "sm_firmware.h"


#include "lcd.h"
#include "piclib.h"
#include "lcd_menu.h"
#include "text.h"
#include "guix.h"

#include <stdio.h>
#include <stdlib.h>
#include "string.h"


#include "bsp_spi_flash.h"
#include "sm_plus.h"
#include "mmc_sd.h"
#include "sd_spi.h"
#include "bsp.h"

#include "App_ADC.h"
#include "App_Timer.h"
#include "App_InfShow.h"
#include "App_SystemPortConfig.h"
#include "App_Language.h"
#include "watchdog.h"

#include "laser.h"

static __IO uint32_t uwTimingDelay;
RCC_ClocksTypeDef RCC_Clocks;





/*
*Function:
*Input Paragramed:
*Output Paragramed:
*Remarks:
*/
void App_FORMAT_SPI_FLASH(void)
{
	int res;
	FRESULT res_sd;
	printf("System Start Format SPI FLASH...");
	res_sd = f_mount(NULL, "1:", 1);			 //unmount spi_flash
	res = f_mount(fs[1], "1:", 1);					//mount spi flash

	//=========format spi flash
	res_sd = f_mkfs("1:", 0, 0);							//format spi flash
	res_sd = f_mount(NULL, "1:", 1);			 //unmount spi_flash
	res_sd = f_mount(fs[1], "1:", 1); 			//remount spi_flash
	//============================
	res = f_mount(fs[1], "1:", 1);					//
	if(res == FR_NO_FILESYSTEM)
	{
		printf(">>SPI FLASH还没有文件系统，即将进行格式化...\r\n");
		/* 格式化 */
		res_sd = f_mkfs("1:", 0, 0);

		if(res_sd == FR_OK)
		{
			printf(">>SPI FLASH已成功格式化文件系统。\r\n");
			/* 格式化后，先取消挂载 */
			res_sd = f_mount(NULL, "1:", 1);
			/* 重新挂载	*/
			res_sd = f_mount(fs[1], "1:", 1);
		}
		else
		{
			printf("<<格式化失败.>>\r\n");
			while(1);
		}
	}
	printf("System SPI FLASH Format Completed...");
}

void reset_all_gpio(void)
{
	/*Reset All GPIO Port Config*/
	GPIO_DeInit(GPIOA);
	GPIO_DeInit(GPIOB);
	GPIO_DeInit(GPIOC);
	GPIO_DeInit(GPIOD);
	GPIO_DeInit(GPIOE);
	GPIO_DeInit(GPIOF);

}

void init_fatfs(void)
{
	int res;
	u32 total, free;
	//init sd card fatfs
	res = f_mount(fs[0], "0:", 1);					//mount TF Card
	if(res)
	{
		USR_UsrLog("f_mount filed error=0x%02x", res);
	}
	else
	{
		USR_UsrLog("f_mount successed");
		card.cardOK = TRUE;
	}
	if(exf_getfree("0", &total, &free))	//getc sd card free size
	{
		USR_UsrLog("SD Card Fatfs Error!");
	}
	else
	{
		USR_UsrLog("SD Total Size:%d", total >> 10);
		USR_UsrLog("SD Free Size:%d", free >> 10);
	}


	//Init spiflash file system
	res = f_mount(fs[1], "1:", 1);					//
	if(res == FR_NO_FILESYSTEM)
	{
		App_FORMAT_SPI_FLASH();
	}
	else if(res == FR_OK)
	{
		USR_UsrLog("f_mount spiflash successed");
	}
	else
	{
		USR_UsrLog("f_mount spiflash filed (%d)", res);
	}
	if(exf_getfree("1", &total, &free))	//getc spi flash free size
	{
		USR_UsrLog("SPI Flash Fatfs Error!");
	}
	else
	{

		USR_UsrLog("SPI Flash Total Size:%d", total >> 10);
		USR_UsrLog("SPI Flash Free Size:%d", free >> 10);
	}

}
void check_update_resource(void)
{
	DIR src_dir;
	int res;
	res = f_opendir(&src_dir, "0:/SYSTEM"); //
	if(res == FR_OK)//
	{
		App_FORMAT_SPI_FLASH();
		LCD_Clear(WHITE);
		gui_phy.back_color = WHITE;
		res = strlen("Font is being upgraded");
		gui_show_string("Font is being upgraded", 0, 0, res * 8, 16, 16, BLACK);
		UpdateFlg = res;
		f_closedir(&src_dir);//close files
		copy_font();
		f_rename("0:/SYSTEM", "0:/OLDSYSTEM");
		UpdateFlg = 0;
	}
	else
	{
		USR_ErrLog("font is not exit");
	}
	res = f_opendir(&src_dir, "0:/diy_icon");
	if(res == FR_OK)
	{
		LCD_Clear(WHITE);
		gui_phy.back_color = WHITE;
		res = strlen("Picture is being upgraded");
		gui_show_string("Picture is being upgraded", 0, 0, res * 8, 16, 16, BLACK);
		UpdateFlg = res;
		f_closedir(&src_dir);//close files
		copy_diy_icons();
		ls_spi_diy_icon();
		f_rename("0:/diy_icon", "0:/OLDdiy_icon");
		UpdateFlg = 0;
	}
	else
	{
		USR_ErrLog("picture resource not exit");
	}
}
void app_timer_init(void)
{
	TIM6_Init(9, 8399);
	App_TIM5_Init();
	App_TIM7_Init();
	App_TIM2_Config();
}
/*
*Function:
*Input Paragramed:
*Output Paragramed:
*Remarks:
*/
int main(void)
{
	int res;
	u32 total, free;
	CPUID[0] = *(u32 *)0x1FFF7A10;	//get cpu id
	CPUID[1] = *(u32 *)0x1FFF7A14;
	CPUID[2] = *(u32 *)0x1FFF7A18;


	SCB->VTOR = FLASH_BASE | 0x11000;				//set user app start address 

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	delay_init();								   
	reset_all_gpio();
	
	uart1_init(115200);
	mem_init(SRAMIN);		//internal SRAM Init
	exfuns_init();

	HeadType();
	App_StartInf();
	ADC1_DMA_Config();
	App_SystemPortConfig();
	app_timer_init();
	laser_init();

	printinfo_init();
	init_fatfs();
	IIC_Init();					//IIC_Config Init
	LCD_Init();					//LCD Display Init
	tp_dev.init();			//Touch screen Config
	gui_init();					//gui_system config
	piclib_init();			//Picture display Init
	B_LED = TRUE;			// turn on the lcd screen black light
	//read config files
	USR_UsrLog("Start Read EEPROM Config Data...");
	res = ConfigurationWrite2EEPROM();
	if(res == RDCONFIG_OK || res == PARAM_NONALL)
	{
		save_configfile();
		Config_StoreSettings();
		Config_RetrieveSettings();
		Config_PrintSettings();
	}
	PowerOff_Init();
	check_update_resource();
	
	setup();
	set_display_language();
	if((LevelMode == 1) && (PrinterMode == 0))
	{
		bl_touch_init();
	}
	//IWDG_Init(6, 2500); //16S
	if(PrinterMode == 0)
		RePrint();
	loop();

}


/**
  * @brief  Decrements the TimingDelay variable.
  * @param  None
  * @retval None
  */
void TimingDelay_Decrement(void)
{
	if(uwTimingDelay != 0x00)
	{
		uwTimingDelay--;
	}
}

#ifdef  USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
	/* User can add his own implementation to report the file name and line number,
	   ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

	/* Infinite loop */
	while(1)
	{
	}
}
#endif

/**
  * @}
  */


/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
