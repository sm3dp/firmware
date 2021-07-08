#include "sys.h"
#include "delay.h"
#include "usart.h" 
#include "malloc.h"    
#include "ff.h"  
#include "exfuns.h"    
#include "stmflash.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "mmc_sd.h"
#include "sd_spi.h"

 
#define VERSIONADDR 0x08010000
#define SIZE		sizeof("SMPRINTERV0101.bin")
#define PATHUP		"0:/UPDATE/"

#define FLASH_APP1_ADDR		0x08011000  	//第一个应用程序的起始地址
#define UPDATEADDR 			0x08010000		//更新

typedef  void (*iapfun)(void);				//定义一个函数类型的参数
iapfun jump2app; 
u32 iapbuf[512];

//appxaddr:应用程序的起始地址
//appbuf:应用程序CODE.
//appsize:应用程序大小(字节).
void iap_write_appbin(u32 appxaddr,u8 *appbuf,u32 appsize)
{
	u32 t;
	u16 i=0;
	u32 temp;
	u32 fwaddr=appxaddr;//当前写入的地址
	u8 *dfu=appbuf;
	for(t=0;t<appsize;t+=4)
	{						   
		temp=(u32)dfu[3]<<24;   
		temp|=(u32)dfu[2]<<16;    
		temp|=(u32)dfu[1]<<8;
		temp|=(u32)dfu[0];	  
		dfu+=4;//偏移4个字节
		iapbuf[i++]=temp;	    
		if(i==512)
		{
			i=0; 
			STMFLASH_Write(fwaddr,iapbuf,512);
			fwaddr+=2048;//偏移2048   512*4=2048
		}
	} 
	if(i)STMFLASH_Write(fwaddr,iapbuf,i);//将最后的一些内容字节写进去.  
}


//跳转到应用程序段
//appxaddr:用户代码起始地址.
void iap_load_app(u32 appxaddr)
{
	if(((*(vu32*)appxaddr)&0x2FFE0000)==0x20000000)	//检查栈顶地址是否合法.
	{ 
		jump2app=(iapfun)*(vu32*)(appxaddr+4);		//用户代码区第二个字为程序开始地址(复位地址)		
		MSR_MSP(*(vu32*)appxaddr);					//初始化APP堆栈指针(用户代码区的第一个字用于存放栈顶地址)
		jump2app();									//跳转到APP.
	}
}	

void montor_init(void)
{
	GPIO_InitTypeDef  GPIO_InitStructure;
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6|GPIO_Pin_9|GPIO_Pin_12;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(GPIOE, &GPIO_InitStructure);

	GPIO_SetBits(GPIOE,GPIO_Pin_6|GPIO_Pin_9|GPIO_Pin_12);
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3|GPIO_Pin_15;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
	GPIO_SetBits(GPIOC,GPIO_Pin_3|GPIO_Pin_15);
}

int main(void)
{        
 	u32 total,free;
  	u8 updateflag=0;	
	u32 t;
	u8 res=0;	
	u8 flashtemp[SIZE]={0};
	char updatepath[64];
	u8 databuff[4];
	DIR dir;
	FILINFO fileinfo;
	FIL *filp=NULL;
	u8 *fn;
	u8 *strchr_pointer;
	long oldversion,newversion;
	int len=0;
	u16 i=0,j=0;
	u32 temp;
	u32 fwaddr = FLASH_APP1_ADDR;
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//设置系统中断优先级分组2
	montor_init();		//初始化电机使能引脚，设置为高电平
	delay_init(168);  //初始化延时函数
	uart_init(115200);		//初始化串口波特率为115200
	printf("start bootloader........\r\n");
	filp = (FIL*)mymalloc(SRAMIN,sizeof(FIL));
	if(filp==NULL)
	{
		printf("mymalloc filp error\r\n");
		goto exit;
	}
	printf("mymalloc filp successed!!!!!!\r\n");
	i=0;
	res = SD_Initialize();
	while(i<5 && res!=0)
	{
		res = SD_Initialize();
		delay_ms(300);
		i++;
	}
 	if(res)//检测不到SD卡
	{
		printf("SD Card Error!\r\n");
		delay_ms(300);					
		printf("Please Check!\r\n");
		delay_ms(300);
		goto exit;
	}
	printf("sd card init successed!!!!!!\r\n");
 	exfuns_init();							//为fatfs相关变量申请内存				 
  	f_mount(fs[0],"0:",1); 					//挂载SD卡 

	if(exf_getfree("0",&total,&free))	//得到SD卡的总容量和剩余容量
	{
		printf("SD Card Fatfs Error!\r\n");
		delay_ms(200);
		delay_ms(200);
		goto exit;
	}													  			    
	printf("FATFS OK!\r\n");	 
	printf("SD Total Size: %d MB",total>>10);	 
	printf("SD  Free Size: %d MB",free>>10); 
	res = f_open(filp,"0:SM3DP.bin",FA_READ);	//查询是否有升级包
	if(res)
	{
		printf("res=%d line=%d\r\n",res,__LINE__);
		goto exit;
	}
	printf("line:%d\r\n",__LINE__);
	res = f_stat("0:SM3DP.bin",&fileinfo);
	if(res)
	{
		printf("res=%d line=%d\r\n",res,__LINE__);
		goto exit;
	}
	printf("fileinfo.fsize=%d line:%d\r\n",fileinfo.fsize,__LINE__);

	j=0;
	for(t=0;t<fileinfo.fsize;t+=4)
	{	
		f_lseek(filp,t);
		res = f_read(filp,databuff,4,&br);
		temp=(u32)databuff[3]<<24;   
		temp|=(u32)databuff[2]<<16;    
		temp|=(u32)databuff[1]<<8;
		temp|=(u32)databuff[0];	  
		//dfu+=4;//偏移4个字节
		iapbuf[i++]=temp;	
		j++;
		if(i==512)
		{
			printf("j=%d line:%d\r\n",j,__LINE__);
			i=0; 
			STMFLASH_Write(fwaddr,iapbuf,512);
			fwaddr+=2048;//偏移2048   512*4=2048
		}
	}
	printf("line:%d\r\n",__LINE__);
	if(i)STMFLASH_Write(fwaddr,iapbuf,i);//将最后的一些内容字节写进去.  
	//更改文件名，防止下次开机再次升级

	f_rename("0:SM3DP.bin","0:OLDSM3DP.bin");

	printf("开始执行新FLASH用户代码！！！！\r\n");
	if(((*(vu32*)(FLASH_APP1_ADDR+4))&0xFF000000)==0x08000000)		//判断是否为0x08xxxxxx.
	{
		printf("line:%d\r\n",__LINE__);
		iap_load_app(FLASH_APP1_ADDR);//执行FLASH APP代码
	}
	else 
	{
		printf("非FLASH应用程序,无法执行!\r\n");
	}
	while(1)
	{
		;
	}
	exit:
	myfree(SRAMIN,filp);
	printf("exit\r\n");
	if(((*(vu32*)(FLASH_APP1_ADDR+4))&0xFF000000)==0x08000000)		//判断是否为0x08xxxxxx.
	{
		printf("执行原用户代码！！！！\r\n");
		iap_load_app(FLASH_APP1_ADDR);//执行FLASH APP代码
	}
}





