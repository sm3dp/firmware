#include "lcd.h"
#include "stdlib.h"
#include "font.h"
#include "usart.h"
#include "delay.h"

u16 POINT_COLOR = 0x0000;	//paintbrush color
u16 BACK_COLOR = 0xFFFF; //backgroung color

_lcd_dev lcddev;

#define MODE_DATA 1 //0: 2data mode; 1 1data mode



/*
*write lcd register mode
*/
void LCD_WR_REG(u8 regval)
{
	WriteComm(regval);

}
/*
*write lcd data mode
*/
void LCD_WR_DATA(u8 data)
{
	WriteData(data);

}


/*
*write lcd register data
*/
void LCD_WriteReg(u16 LCD_Reg, u16 LCD_RegValue)
{

	LCD_WR_REG((u8)LCD_Reg);
	LCD_WR_DATA((u8)(LCD_RegValue >> 8));
	LCD_WR_DATA((u8)LCD_RegValue);
}

void LCD_WriteRAM_Prepare(void)
{
	WriteComm(lcddev.wramcmd);
}
void LCD_WriteRAM(u16 RGB_Code)
{

	LCD_WR_DATA((u8)(RGB_Code >> 8));
	LCD_WR_DATA((u8)RGB_Code);
}

void LCD_Scan_Dir(u8 dir)
{
	u16 regval = 0;
	u16 dirreg = 0;
	u16 temp;

	switch(dir)
	{
		case L2R_U2D:
			regval |= (0 << 7) | (0 << 6) | (0 << 5);
			break;
		case L2R_D2U:
			regval |= (1 << 7) | (0 << 6) | (0 << 5);
			break;
		case R2L_U2D:
			regval |= (0 << 7) | (1 << 6) | (0 << 5);
			break;
		case R2L_D2U:
			regval |= (1 << 7) | (1 << 6) | (0 << 5);
			break;
		case U2D_L2R:
			regval |= (0 << 7) | (0 << 6) | (1 << 5);
			break;
		case U2D_R2L:
			regval |= (0 << 7) | (1 << 6) | (1 << 5);
			break;
		case D2U_L2R:
			regval |= (1 << 7) | (0 << 6) | (1 << 5);
			break;
		case D2U_R2L:
			regval |= (1 << 7) | (1 << 6) | (1 << 5);
			break;
	}
	if(lcddev.id == 0X5510)dirreg = 0X3600;
	else dirreg = 0X36;


	if((lcddev.id != 0X5310) && (lcddev.id != 0X5510))regval |= 0X08;
	if(lcddev.id == 0X6804)regval |= 0x02;
	LCD_WriteReg(dirreg, regval);
	if((regval & 0X20) || lcddev.dir == 1)
	{
		if(lcddev.width < lcddev.height)
		{
			temp = lcddev.width;
			lcddev.width = lcddev.height;
			lcddev.height = temp;
		}
	}
	else
	{
		if(lcddev.width > lcddev.height)
		{
			temp = lcddev.width;
			lcddev.width = lcddev.height;
			lcddev.height = temp;
		}
	}
	if(lcddev.id == 0X5510)
	{
		LCD_WR_REG(lcddev.setxcmd);
		LCD_WR_DATA(0);
		LCD_WR_REG(lcddev.setxcmd + 1);
		LCD_WR_DATA(0);
		LCD_WR_REG(lcddev.setxcmd + 2);
		LCD_WR_DATA((lcddev.width - 1) >> 8);
		LCD_WR_REG(lcddev.setxcmd + 3);
		LCD_WR_DATA((lcddev.width - 1) & 0XFF);
		LCD_WR_REG(lcddev.setycmd);
		LCD_WR_DATA(0);
		LCD_WR_REG(lcddev.setycmd + 1);
		LCD_WR_DATA(0);
		LCD_WR_REG(lcddev.setycmd + 2);
		LCD_WR_DATA((lcddev.height - 1) >> 8);
		LCD_WR_REG(lcddev.setycmd + 3);
		LCD_WR_DATA((lcddev.height - 1) & 0XFF);
	}
	else
	{
		LCD_WR_REG(lcddev.setxcmd);
		LCD_WR_DATA(0);
		LCD_WR_DATA(0);
		LCD_WR_DATA((lcddev.width - 1) >> 8);
		LCD_WR_DATA((lcddev.width - 1) & 0XFF);
		LCD_WR_REG(lcddev.setycmd);
		LCD_WR_DATA(0);
		LCD_WR_DATA(0);
		LCD_WR_DATA((lcddev.height - 1) >> 8);
		LCD_WR_DATA((lcddev.height - 1) & 0XFF);
	}

}

void LCD_DrawPoint(u16 x, u16 y)
{
	LCD_SetCursor(x, y);		
	WriteComm(lcddev.wramcmd);	

	LCD_WR_DATA((u8)(POINT_COLOR >> 8));
	LCD_WR_DATA((u8)POINT_COLOR);

}
void LCD_Fast_DrawPoint(u16 x, u16 y, u16 color)
{
	LCD_WR_REG(lcddev.setxcmd);
	LCD_WR_DATA(x >> 8);
	LCD_WR_DATA(x & 0XFF);
	LCD_WR_REG(lcddev.setycmd);
	LCD_WR_DATA(y >> 8);
	LCD_WR_DATA(y & 0XFF);
	LCD_WR_REG(lcddev.wramcmd);

	LCD_WR_DATA((u8)(color >> 8));
	LCD_WR_DATA((u8)color);

}


void LCD_Display_Dir(u8 dir)
{
	if(dir == 0)			
	{
		lcddev.dir = 0;	
		lcddev.width = 320;
		lcddev.height = 480;
		lcddev.wramcmd = 0X2C;
		lcddev.setxcmd = 0X2A;
		lcddev.setycmd = 0X2B;


	}
	else 				
	{
		lcddev.dir = 1;	
		lcddev.width = 480;
		lcddev.height = 320;
		lcddev.wramcmd = 0X2C;
		lcddev.setxcmd = 0X2A;
		lcddev.setycmd = 0X2B;

	}
	LCD_Scan_Dir(DFT_SCAN_DIR);	
}

void LCD_Set_Window(u16 sx, u16 sy, u16 width, u16 height)
{
	width = sx + width - 1;
	height = sy + height - 1;

	LCD_WR_REG(lcddev.setxcmd);
	LCD_WR_DATA(sx >> 8);
	LCD_WR_DATA(sx & 0XFF);
	LCD_WR_DATA(width >> 8);
	LCD_WR_DATA(width & 0XFF);
	LCD_WR_REG(lcddev.setycmd);
	LCD_WR_DATA(sy >> 8);
	LCD_WR_DATA(sy & 0XFF);
	LCD_WR_DATA(height >> 8);
	LCD_WR_DATA(height & 0XFF);

}

void delay(u8 len)
{
	u8 i;
	for(i = 0;i < len; i++)
	{
	    	__NOP();
		
	}
	
}
void  SendDataSPI(unsigned char dat)
{
	unsigned char i;

	for(i = 0; i < 8; i++)
	{
		if((dat & 0x80) != 0) SDA = 1;
		else SDA = 0;

		dat <<= 1;

		SCL = 0;
		delay(1);
		SCL = 1;
		delay(1);
	}
	SCL = 0;
}

void WriteComm(unsigned char i)
{
	CS0 = 0;
	delay(1);
	RS  = 0;
	SendDataSPI(i);

	CS0 = 1;

}
void WriteData(unsigned char i)
{
	CS0 = 0;
	delay(1);
	RS  = 1;
	SendDataSPI(i);

	CS0 = 1;
}
void LCD_Init(void)
{
	delay_ms(120); // Delay 120ms
	WriteComm(0x11); // Sleep Out
	delay_ms(120); // Delay 120ms

	WriteComm(0x3A);
	WriteData(0x55);


	WriteComm(0xf0) ;
	WriteData(0xc3) ;
	WriteComm(0xf0) ;
	WriteData(0x96) ;

	WriteComm(0x36);

	WriteData(0x88);

	WriteComm(0xB4);
	WriteData(0x01);

	WriteComm(0xB7) ;
	WriteData(0xC6) ;

	WriteComm(0xe8);
	WriteData(0x40);
	WriteData(0x8a);
	WriteData(0x00);
	WriteData(0x00);
	WriteData(0x29);
	WriteData(0x19);
	WriteData(0xa5);
	WriteData(0x33);

	WriteComm(0xc1);
	WriteData(0x06);

	WriteComm(0xc2);
	WriteData(0xa7);

	WriteComm(0xc5);
	WriteData(0x18);

	WriteComm(0xe0); //Positive Voltage Gamma Control
	WriteData(0xf0);
	WriteData(0x09);
	WriteData(0x0b);
	WriteData(0x06);
	WriteData(0x04);
	WriteData(0x15);
	WriteData(0x2f);
	WriteData(0x54);
	WriteData(0x42);
	WriteData(0x3c);
	WriteData(0x17);
	WriteData(0x14);
	WriteData(0x18);
	WriteData(0x1b);

	WriteComm(0xe1); //Negative Voltage Gamma Control
	WriteData(0xf0);
	WriteData(0x09);
	WriteData(0x0b);
	WriteData(0x06);
	WriteData(0x04);
	WriteData(0x03);
	WriteData(0x2d);
	WriteData(0x43);
	WriteData(0x42);
	WriteData(0x3b);
	WriteData(0x16);
	WriteData(0x14);
	WriteData(0x17);
	WriteData(0x1b);

	WriteComm(0xf0);
	WriteData(0x3c);

	WriteComm(0xf0);
	WriteData(0x69);



	delay_ms(120); //Delay 120ms
	WriteComm(0x29); //Display O


	//====================end gamma=========================
	LCD_WR_REG(0x11);
	delay_ms(120);
	LCD_WR_REG(0x29);
	LCD_WR_REG(0x2c);


	lcddev.dir = 0;		
	lcddev.height = 480;
	lcddev.width = 320;

	lcddev.id = 0x7796;
	lcddev.setxcmd = 0x2A;
	lcddev.setycmd = 0x2B;
	lcddev.wramcmd = 0X2C;

	LCD_Clear(WHITE);

}


u16 LCD_ReadPoint(u16 x, u16 y)
{
	return 0xffff;
}

void LCD_SetCursor(u16 Xpos, u16 Ypos)
{
	WriteComm(lcddev.setxcmd);
	WriteData(Xpos >> 8);
	WriteData(Xpos);
	WriteComm(lcddev.setycmd);
	WriteData(Ypos >> 8);
	WriteData(Ypos);
}

void BlockWrite(unsigned int Xstart, unsigned int Xend, unsigned int Ystart, unsigned int Yend)
{
	//ILI9163C

	WriteComm(0x2A);
	WriteData(Xstart >> 8);
	WriteData(Xstart);
	WriteData(Xend >> 8);
	WriteData(Xend);

	WriteComm(0x2B);
	WriteData(Ystart >> 8);
	WriteData(Ystart);
	WriteData(Yend >> 8);
	WriteData(Yend);

	WriteComm(0x2c);
}
void LCD_Clear(u16 color)
{

	unsigned int i, j;

	BlockWrite(0, 319, 0, 479);
	CS0 = 0;
	RS = 1;

	for(i = 0; i < 320; i++)
	{
		for(j = 0; j < 480; j++)
		{
			SendDataSPI(color >> 8);
			SendDataSPI(color);
		}
	}
	CS0 = 1;
}




void LCD_Fill(u16 sx, u16 sy, u16 ex, u16 ey, u16 color)
{
	u16 i, j;
	u16 xlen = 0;

	xlen = ex - sx + 1;
	for(i = sy; i <= ey; i++)
	{
		LCD_SetCursor(sx, i);      				
		WriteComm(lcddev.wramcmd);     			
		for(j = 0; j < xlen; j++)
		{

			LCD_WR_DATA((u8)(color >> 8));	
			LCD_WR_DATA((u8)color);

		}
	}

}

void LCD_Color_Fill(u16 sx, u16 sy, u16 ex, u16 ey, u16 *color)
{
	u16 height, width;
	u16 i, j;
	width = ex - sx + 1; 			
	height = ey - sy + 1;			
	for(i = 0; i < height; i++)
	{
		LCD_SetCursor(sx, sy + i);   	
		WriteComm(lcddev.wramcmd);    
		for(j = 0; j < width; j++)
		{

			LCD_WR_DATA((u8)(color[i * height + j] >> 8));
			LCD_WR_DATA((u8)color[i * height + j]);

		}
	}
}

void LCD_DrawLine(u16 x1, u16 y1, u16 x2, u16 y2)
{
	u16 t;
	int xerr = 0, yerr = 0, delta_x, delta_y, distance;
	int incx, incy, uRow, uCol;
	delta_x = x2 - x1;
	delta_y = y2 - y1;
	uRow = x1;
	uCol = y1;
	if(delta_x > 0)incx = 1; 
	else if(delta_x == 0)incx = 0;
	else
	{
		incx = -1;
		delta_x = -delta_x;
	}
	if(delta_y > 0)incy = 1;
	else if(delta_y == 0)incy = 0;
	else
	{
		incy = -1;
		delta_y = -delta_y;
	}
	if(delta_x > delta_y)distance = delta_x;
	else distance = delta_y;
	for(t = 0; t <= distance + 1; t++)
	{
		LCD_DrawPoint(uRow, uCol);
		xerr += delta_x ;
		yerr += delta_y ;
		if(xerr > distance)
		{
			xerr -= distance;
			uRow += incx;
		}
		if(yerr > distance)
		{
			yerr -= distance;
			uCol += incy;
		}
	}
}
void LCD_DrawRectangle(u16 x1, u16 y1, u16 x2, u16 y2)
{
	LCD_DrawLine(x1, y1, x2, y1);
	LCD_DrawLine(x1, y1, x1, y2);
	LCD_DrawLine(x1, y2, x2, y2);
	LCD_DrawLine(x2, y1, x2, y2);
}

void Draw_Circle(u16 x0, u16 y0, u8 r)
{
	int a, b;
	int di;
	a = 0;
	b = r;
	di = 3 - (r << 1);     
	while(a <= b)
	{
		LCD_DrawPoint(x0 + a, y0 - b);        //5
		LCD_DrawPoint(x0 + b, y0 - a);        //0
		LCD_DrawPoint(x0 + b, y0 + a);        //4
		LCD_DrawPoint(x0 + a, y0 + b);        //6
		LCD_DrawPoint(x0 - a, y0 + b);        //1
		LCD_DrawPoint(x0 - b, y0 + a);
		LCD_DrawPoint(x0 - a, y0 - b);        //2
		LCD_DrawPoint(x0 - b, y0 - a);        //7
		a++;
		if(di < 0)di += 4 * a + 6;
		else
		{
			di += 10 + 4 * (a - b);
			b--;
		}
	}
}

void LCD_ShowChar(u16 x, u16 y, u8 num, u8 size, u8 mode)
{
	u8 temp, t1, t;
	u16 y0 = y;
	u16 colortemp = POINT_COLOR;
	num = num - ' '; 
	if(!mode) 
	{
		for(t = 0; t < size; t++)
		{
			if(size == 12)temp = asc2_1206[num][t];
			else temp = asc2_1608[num][t];		 
			for(t1 = 0; t1 < 8; t1++)
			{
				if(temp & 0x80)POINT_COLOR = colortemp;
				else POINT_COLOR = BACK_COLOR;
				LCD_DrawPoint(x, y);
				temp <<= 1;
				y++;
				if(x >= lcddev.width)
				{
					POINT_COLOR = colortemp;    
					return;
				}
				if((y - y0) == size)
				{
					y = y0;
					x++;
					if(x >= lcddev.width)
					{
						POINT_COLOR = colortemp;   
						return;
					}
					break;
				}
			}
		}
	}
	else
	{
		for(t = 0; t < size; t++)
		{
			if(size == 12)temp = asc2_1206[num][t]; 
			else temp = asc2_1608[num][t];		
			for(t1 = 0; t1 < 8; t1++)
			{
				if(temp & 0x80)LCD_DrawPoint(x, y);
				temp <<= 1;
				y++;
				if(x >= lcddev.height)
				{
					POINT_COLOR = colortemp;    
					return;
				}
				if((y - y0) == size)
				{
					y = y0;
					x++;
					if(x >= lcddev.width)
					{
						POINT_COLOR = colortemp;    
						return;
					}
					break;
				}
			}
		}
	}
	POINT_COLOR = colortemp;
}
u32 LCD_Pow(u8 m, u8 n)
{
	u32 result = 1;
	while(n--)result *= m;
	return result;
}

void LCD_ShowNum(u16 x, u16 y, u32 num, u8 len, u8 size)
{
	u8 t, temp;
	u8 enshow = 0;
	for(t = 0; t < len; t++)
	{
		temp = (num / LCD_Pow(10, len - t - 1)) % 10;
		if(enshow == 0 && t < (len - 1))
		{
			if(temp == 0)
			{
				LCD_ShowChar(x + (size / 2)*t, y, ' ', size, 0);
				continue;
			}
			else enshow = 1;

		}
		LCD_ShowChar(x + (size / 2)*t, y, temp + '0', size, 0);
	}
}


void LCD_ShowString(u16 x, u16 y, u16 width, u16 height, u8 size, u8 *p)
{
	u8 x0 = x;
	width += x;
	height += y;
	while((*p <= '~') && (*p >= ' '))
	{
		if(x >= width)
		{
			x = x0;
			y += size;
		}
		if(y >= height)break; 
		LCD_ShowChar(x, y, *p, size, 0);
		x += size / 2;
		p++;
	}
}

void lcdtest(u16 color)
{
	int i, j;
	for(i = 0; i < 320; i++)
	{
		LCD_SetCursor(0, i);			
		WriteComm(lcddev.wramcmd);     
		for(j = 0; j < 240; j++)
		{
			LCD_WR_DATA((u8)(color >> 8));
			LCD_WR_DATA((u8)color);
			delay_ms(1);
		}
	}
}
























