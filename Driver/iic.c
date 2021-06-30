#include "iic.h"
#include "delay.h"


//init iic
void IIC_Init(void)
{
	IIC_SCL = 1;
	IIC_SDA = 1;
}
//creat iic start signal
void IIC_Start(void)
{
	SDA_OUT();     
	IIC_SDA = 1;
	IIC_SCL = 1;
	delay_us(4);
	IIC_SDA = 0; //START:when CLK is high,DATA change form high to low
	delay_us(4);
	IIC_SCL = 0; 
}
//creat iic stop signal
void IIC_Stop(void)
{
	SDA_OUT();
	IIC_SCL = 0;
	IIC_SDA = 0; //STOP:when CLK is high DATA change form low to high
	delay_us(4);
	IIC_SCL = 1;
	IIC_SDA = 1; 
	delay_us(4);
}
//wait for ack signal
//return£º1£¬fail
//        0£¬success
u8 IIC_Wait_Ack(void)
{
	u8 ucErrTime = 0;
	SDA_IN();      
	IIC_SDA = 1;
	delay_us(1);
	IIC_SCL = 1;
	delay_us(1);
	while(READ_SDA)
	{
		ucErrTime++;
		if(ucErrTime > 250)
		{
			IIC_Stop();
			return 1;
		}
	}
	IIC_SCL = 0; 
	return 0;
}
//creat ACK
void IIC_Ack(void)
{
	IIC_SCL = 0;
	SDA_OUT();
	IIC_SDA = 0;
	delay_us(2);
	IIC_SCL = 1;
	delay_us(2);
	IIC_SCL = 0;
}
//do not creat ack
void IIC_NAck(void)
{
	IIC_SCL = 0;
	SDA_OUT();
	IIC_SDA = 1;
	delay_us(2);
	IIC_SCL = 1;
	delay_us(2);
	IIC_SCL = 0;
}
//IIC send one byte
//return:
//1£¬have a ack
//0£¬no ack
void IIC_Send_Byte(u8 txd)
{
	u8 t;
	SDA_OUT();
	IIC_SCL = 0; 
	for(t = 0; t < 8; t++)
	{
		IIC_SDA = (txd & 0x80) >> 7;
		txd <<= 1;
		delay_us(2);   
		IIC_SCL = 1;
		delay_us(2);
		IIC_SCL = 0;
		delay_us(2);
	}
}
//read one byte£¬when ack=1£¬send ACK£¬ack=0£¬send nACK
u8 IIC_Read_Byte(unsigned char ack)
{
	unsigned char i, receive = 0;
	SDA_IN();
	for(i = 0; i < 8; i++)
	{
		IIC_SCL = 0;
		delay_us(2);
		IIC_SCL = 1;
		receive <<= 1;
		if(READ_SDA)receive++;
		delay_us(1);
	}
	if(!ack)
		IIC_NAck();
	else
		IIC_Ack(); 
	return receive;
}



























