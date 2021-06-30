/**
  ******************************************************************************
  * @file    Project/STM32F4xx_StdPeriph_Templates/main.h 
  * @author  MCD Application Team
  * @version V1.8.0
  * @date    04-November-2016
  * @brief   Header for main.c module
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2016 STMicroelectronics</center></h2>
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software 
  * distributed under the License is distributed on an "AS IS" BASIS, 
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */
  
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx.h"


#define SM_VERSION 020
/*
版本说明：
V012:增加了参数保存到eeprom功能
V013:增加断电续打功能，该功能目前只支持sd卡断电续打，宏定义为 REPRINTSUPPORT
新加功能说明：
1、开机上电读取sd卡打印完成标志位
2、选择是否续打
3、如果续打则读取文件名、断电保存行数、底板目标温度、挤出头目标温度、gcode命令、e轴绝对坐标、当前打印速率
4、在process_command函数里做定位断电行处理，如果读取的gcode命令没达到指定行数，则不做处理，达到指定行号则开始做
续打准备工作；
5、做续打准备工作：等待挤出头和热床达到目标温度，x、y、z轴归零，添加z轴打印命令

V014:新板程序，做了些对触摸屏引脚的调整
V015:1、新增spiFlash的fatfs系统，在sm_plus文件里添加从sd卡复制icon文件到spiflash中去。
	 2、新增了quicklz压缩解压算法，实现能从sd卡解压最大10240byte块的流数据文件 目前解压20M左右的数据大概用时35s。
V016:2019-3-16初始化串口1作为与wifi通信接口，添加与外部交互动作，主要添加内容有个usart_update函数负责更新打印状态
在gcodeplayer.c的file_read函数添加“ReadChar++;”句话记录读取文件有多少字节，做打印进度使用
具体命令内容查看svn上的文档。

V017:2019-3-22
1、添加自动调平功能，采用三点计算平面公式，通过x,y轴算出z轴的偏差进行补偿，补偿代码在get_coordinates函数里做。
2、添加二维码生成功能，使用rqencode库。

V018:2019-4-12
运动方式改为core_xy方式，x轴和y轴联动 
在configuration.h 里    #define COREXY  使能该运动方式  

V019:2019-4-19
1、添加自动调平功能

V020:2019-5-8
1、增加一些上位机的命令支持，和增加上位机退料时的百分比支持，限位开关判断方式和电机方向可通过命令配置；
需要说明的是使用M4009和M4010这个命令时没用完成动作最好不要发送其他命令，避免不可预料的事发生。

2019-7-26
1、重新实现M4008命令
2、增加M4012命令
3、增加接近开关的自动调平功能

*/


/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
void TimingDelay_Decrement(void);

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
