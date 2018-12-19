#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "Lcdapi.h"
#include "base.h"
#include "LcdDrv.h"

/* LCD logic buffers */
static ushort guaBackBuf[TOTAL_PIXEL_CNT];
static ushort guaForeBuf[TOTAL_PIXEL_CNT];
static uchar guaAlphaBuf[TOTAL_PIXEL_CNT]; 

int lcd_logic_max_x;
int lcd_logic_max_y;
LCD_T gtLcdInfo;
LCD_ATTR_T *GetLcdInfo(int role);
// ���Ͻ���ʵ����Ļ�ϵ������ɴ˶��塣
app_origin_t gstAppScrOrg = {0, 40}; 

static volatile uchar default_gray = 8;
static uchar k_LcdGrayBaseVal=1,k_LcdGrayCurVal=1;

volatile uchar k_TftWrite_Lock=0;
volatile uchar gucTftBusy = 0;
uchar LcdCheckBusy(void)
{
	return gucTftBusy;
}
void s_LcdBufInit(void)
{
	memset(guaBackBuf, 0xff, sizeof(guaBackBuf));
	memset(guaAlphaBuf, 0, sizeof(guaAlphaBuf));
	memset(guaForeBuf, 0xff, sizeof(guaForeBuf));
}
extern uchar k_Charging;
void s_LcdConfigInit(void)
{
	int mach = get_machine_type();
	// ����PWM����
	pwm_config(PWM3, 40, 0, 0);
    if(k_Charging)//���ģʽʱ�����رձ���
        pwm_duty(PWM3,s_LcdGetGray(0));
    else//����ģʽʱ�򿪱���
        pwm_duty(PWM3,s_LcdGetGray(MAX_GRAY));
	pwm_enable(PWM3, 1);

	//����Ӧ�Ŀռ�(FLASH)����LCD�Աȶȵ�����ֵ����������Ҷȡ�
	if(mach == S800) default_gray = 8;
    else if (mach == S300 || mach == S900) default_gray = 9;
	else default_gray = 10;
	
	k_LcdGrayBaseVal = ReadLCDGray();
	if(k_LcdGrayBaseVal <MIN_GRAY ||
        k_LcdGrayBaseVal >MAX_GRAY) 
	{
        k_LcdGrayBaseVal =default_gray;
	}
    k_LcdGrayCurVal = k_LcdGrayBaseVal;

	s_LcdGetAreaSize(&lcd_logic_max_x, &lcd_logic_max_y);
	if (lcd_logic_max_x == 320 && lcd_logic_max_y == 240)
	{
		BatterIconInit(0);
	}
	else if (lcd_logic_max_x == 240 && lcd_logic_max_y == 320)
	{
		BatterIconInit(1);
	}
	
}
#define s_ForeBuf_WritePixel(x, y,data) guaForeBuf[(y)*lcd_logic_max_x+(x)] = data

#define s_ForeBuf_ReadPixel(x, y) guaForeBuf[(y)*lcd_logic_max_x+(x)]

#define s_BackBuf_WritePixel(x, y,data) guaBackBuf[(y)*lcd_logic_max_x+(x)] = data
#define s_BackBuf_ReadPixel(x, y) guaBackBuf[(y)*lcd_logic_max_x+(x)]

#define s_AlphaBuf_WritePixel(x, y,data) guaAlphaBuf[(y)*lcd_logic_max_x+(x)] = data
#define s_AlphaBuf_ReadPixel(x,y) guaAlphaBuf[(y)*lcd_logic_max_x+(x)]

static void s_ForeBuf_FillRect(int color, int left,int top, int right,int bottom)
{
	int x,y;
	ushort usColor;
	usColor = (ushort)(color&0xffff);
	for (y = top;y <= bottom;y++)
	{
		for (x = left;x <=right;x++)
		{
			s_ForeBuf_WritePixel(x, y, usColor);
		}
	}
}
static void s_BackBuf_FillRect(int color, int left,int top, int right,int bottom)
{
	int x,y;
	ushort usColor;
	usColor = (ushort)(color&0xffff);
	for (y = top;y <= bottom;y++)
	{
		for (x = left;x <=right;x++)
		{
			s_BackBuf_WritePixel(x, y, usColor);
		}
	}
}

static void s_AlphaBuf_FillRect(uchar mode, int left,int top, int right,int bottom)
{
	int x,y;
	for (y = top;y <= bottom;y++)
	{
		for (x = left;x <=right;x++)
		{
			s_AlphaBuf_WritePixel(x, y, mode);
		}
	}
}

void s_LcdClrRect(int color, int left,int top, int right,int bottom)
{
    uint y,x;  
    if(k_TftWrite_Lock)return;
    gucTftBusy = 1;

	if (left < 0) left = 0;
	if (top < 0) top = 0;
	if (right >= lcd_logic_max_x) right = lcd_logic_max_x - 1;
	if (bottom >= lcd_logic_max_y) bottom = lcd_logic_max_y - 1;
	s_LcdFillColor(left, top, right, bottom,color);
	gucTftBusy = 0;
	return;
}

static void s_LcdWritePixels(int scol, int sline, int ecol, int eline, int *data)
{
    int i;
    if(k_TftWrite_Lock)return;
    gucTftBusy = 1;
    s_LcdSetGram(scol, sline, ecol, eline);
    for (i = 0; i < (ecol-scol+1)*(eline-sline+1); i++) s_LcdWrLData(*(data+i));
    
    gucTftBusy = 0;
}
void s_LcdWritePixel(int x, int y, int Color)
{	
    if(k_TftWrite_Lock)return;
	//set tft busy
	gucTftBusy = 1;
	s_LcdSetGram(x, y, x, y);
	s_LcdWrLData(Color);
	gucTftBusy = 0;
	return;
}


static void s_FillBufToLcd(int scol, int sline, int ecol, int eline)
{
	if(k_TftWrite_Lock)return;
	gucTftBusy = 1;
	s_LcdFillBuf(scol, sline, ecol, eline, guaForeBuf, guaBackBuf, guaAlphaBuf);
    gucTftBusy = 0;
}

// s_LcdBrightness(): �趨��������, ��ΧMIN_GRAY ~ MAX_GRAY.
static int s_LcdBrightness(unsigned int level)
{
    static unsigned int lastLevel=100;//��ʼ��Ϊ����һ��0��MAX_GRAY�����һ��ֵ

    if(lastLevel==level)
		return 0;
    
    if(level>MAX_GRAY)
		return 0;

    lastLevel = level;

    pwm_enable(PWM3, 0);
	DelayUs(10);//��ʱһ��ʱ�䱣֤pwm�ر�

	pwm_duty(PWM3, s_LcdGetGray(level));

    pwm_enable(PWM3, 1);
    
	return 0;
}

/***************************************
level:
        0: �ر��⣬�����ɫģʽ
        1: �����⣬����ϵͳ���õı���ֵ
        2: ���׹رձ���
***************************************/
void s_ScrLightCtrl(int level)
{
    if(level==0)
        s_LcdBrightness(MIN_GRAY);
    else if(level==1)
    	s_LcdBrightness(k_LcdGrayCurVal);
    else
        s_LcdBrightness(0);
}

uchar GetDefaultGray(void)
{
	return default_gray;
}

void s_ScrSetGrayBase(uchar val)
{
    if(val > MAX_GRAY) val = MAX_GRAY;
    k_LcdGrayCurVal = k_LcdGrayBaseVal = val;
}

/******************************************
level: 0~7 4: Ĭ��ֵ�� 0: � 7: ����
******************************************/
void s_LcdGray(unsigned char level)
{
    int n;
    
    if(level>=8)return;

    n=k_LcdGrayBaseVal+(level-4);

    if(n>MAX_GRAY)n=MAX_GRAY;
    else if(n<MIN_GRAY)n=MIN_GRAY;

    k_LcdGrayCurVal = n;
    
    s_LcdBrightness(k_LcdGrayCurVal);
}


/*�������lcd��Ļ����*/
void s_LcdClear(void)
{
    s_LcdClrRect(COLOR_WHITE, 0, 0, lcd_logic_max_x-1, lcd_logic_max_y-1);
}

void s_ScrRect(int left, int top, int right, int bottom, int rgb, int operation)
{
	int color, i, j;
	int fullW=lcd_logic_max_x, fullH=lcd_logic_max_y;
    

	if (right < left) return;
    if (bottom < top) return;

	if (left >= fullW) return;
    if (top >= fullH) return;

    if (left < 0) left = 0;
    if (top < 0) top = 0;

	if (right >= fullW) 
		right = fullW-1; 								

	if (bottom >= fullH) 
		bottom = fullH-1;
	
	color = RGB((rgb>>16)&0xff, (rgb>>8)&0xff, rgb&0xff);
	if(operation == 1)
	{
		for(i = left; i <= right; i++)
		{
			for(j = top; j <= bottom; j++)
			{         
				s_AlphaBuf_WritePixel(i, j, ALPHA_MODE_USEFORE);
				s_ForeBuf_WritePixel(i, j, (ushort)(color&0xffff));
			}
		}
		s_FillBufToLcd(left, top, right, bottom);
	}
	else /* clear */
	{
		for(i = left; i <= right; i++)
		{
			for(j = top; j <= bottom; j++)
			{         
				s_AlphaBuf_WritePixel(i, j, ALPHA_MODE_USEBACK);
			}
		}
		s_FillBufToLcd(left, top, right, bottom);
	}
}
static int Matrix_x25(int left, int top, int realW, int realH, int dataLine, uchar *data, int color)
{
	int line, col, i = 0, j = 0;
	int caseType = 0;
	const int *p;
	int d1, d2, d3, d4;
	int nlx, nly;

	const int uiModleArray[] = { 
		0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 
		0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 
		0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 
		0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 
		0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 

		0x0000, 0x0000, 0x0000, 0xffff, 0xffff, 
		0x0000, 0x0000, 0x0000, 0xffff, 0xffff, 
		0x0000, 0x0000, 0x0000, 0xffff, 0xffff, 
		0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 
		0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 

		0xffff, 0xffff, 0x0000, 0x0000, 0x0000, 
		0xffff, 0xffff, 0x0000, 0x0000, 0x0000, 
		0xffff, 0xffff, 0x0000, 0x0000, 0x0000, 
		0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 
		0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 

		0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
		0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
		0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
		0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 
		0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 

		0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 
		0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 
		0x0000, 0x0000, 0x0000, 0xffff, 0xffff, 
		0x0000, 0x0000, 0x0000, 0xffff, 0xffff, 
		0x0000, 0x0000, 0x0000, 0xffff, 0xffff, 

		0x0000, 0x0000, 0x0000, 0xffff, 0xffff, 
		0x0000, 0x0000, 0x0000, 0xffff, 0xffff, 
		0x0000, 0x0000, 0x0000, 0xffff, 0xffff, 
		0x0000, 0x0000, 0x0000, 0xffff, 0xffff, 
		0x0000, 0x0000, 0x0000, 0xffff, 0xffff, 

		0xffff, 0xffff, 0x0000, 0x0000, 0x0000, 
		0xffff, 0xffff, 0x0000, 0x0000, 0x0000, 
		0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
		0x0000, 0x0000, 0x0000, 0xffff, 0xffff, 
		0x0000, 0x0000, 0x0000, 0xffff, 0xffff, 

		0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
		0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
		0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
		0x0000, 0x0000, 0x0000, 0xffff, 0xffff, 
		0x0000, 0x0000, 0x0000, 0xffff, 0xffff, 

		0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 
		0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 
		0xffff, 0xffff, 0x0000, 0x0000, 0x0000, 
		0xffff, 0xffff, 0x0000, 0x0000, 0x0000, 
		0xffff, 0xffff, 0x0000, 0x0000, 0x0000, 

		0x0000, 0x0000, 0x0000, 0xffff, 0xffff, 
		0x0000, 0x0000, 0x0000, 0xffff, 0xffff, 
		0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
		0xffff, 0xffff, 0x0000, 0x0000, 0x0000, 
		0xffff, 0xffff, 0x0000, 0x0000, 0x0000, 

		0xffff, 0xffff, 0x0000, 0x0000, 0x0000, 
		0xffff, 0xffff, 0x0000, 0x0000, 0x0000, 
		0xffff, 0xffff, 0x0000, 0x0000, 0x0000, 
		0xffff, 0xffff, 0x0000, 0x0000, 0x0000, 
		0xffff, 0xffff, 0x0000, 0x0000, 0x0000, 

		0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
		0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
		0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
		0xffff, 0xffff, 0x0000, 0x0000, 0x0000, 
		0xffff, 0xffff, 0x0000, 0x0000, 0x0000, 

		0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 
		0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 
		0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
		0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
		0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
		
		0x0000, 0x0000, 0x0000, 0xffff, 0xffff, 
		0x0000, 0x0000, 0x0000, 0xffff, 0xffff, 
		0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
		0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
		0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 

		0xffff, 0xffff, 0x0000, 0x0000, 0x0000, 
		0xffff, 0xffff, 0x0000, 0x0000, 0x0000, 
		0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
		0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
		0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 

		0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
		0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
		0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
		0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
		0x0000, 0x0000, 0x0000, 0x0000, 0x0000,  
	};

	for(i = 0; i < realW - 1; i += 2) /* ignore the data in the margin */
	{
		nlx = (((left + i) * 5) >> 1) + gstAppScrOrg.x;
		
		for(j = 0; j < realH - 1; j += 2) /* ignore the data in the margin */
		{
			nly = (((top + j) * 5) >> 1) + gstAppScrOrg.y;

			d1 = (j / 8) * dataLine + i;
			d2 = 1 << (j % 8);
			d3 = ((j+1) / 8) * dataLine + i;
			d4 = 1 << ((j + 1) % 8);
			
			caseType = 0; 
			if(data[d3+1] & d4)		caseType |= 8;
			if(data[d3]   & d4) 	caseType |= 4;
			if(data[d1+1] & d2)		caseType |= 2;
			if(data[d1]   & d2) 	caseType |= 1;
			
			p = &uiModleArray[caseType * 25];
			
			for(line = 0; line < 5; line++)
			{
				for(col = 0; col < 5; col++, p++)
				{
					
					if(*p == 0xffff)
					{
                        s_AlphaBuf_WritePixel(nlx+col,nly+line, ALPHA_MODE_USEBACK);
					}	
					else
					{
                        s_AlphaBuf_WritePixel(nlx+col,nly+line, ALPHA_MODE_USEFORE);
                        s_ForeBuf_WritePixel(nlx+col,nly+line, (ushort)color);
					}
				}
			}
            s_FillBufToLcd(nlx, nly,nlx+4, nly+4);
		}
	}
}

/***************************************************************************************
����ԭ��	int Rect_x25(uint x, uint y, uint width, uint height, int rgb)
��������	���ηŴ�2.5��
����˵��	���	x -- ���Ͻ�x����
                    y -- ���Ͻ�y����
                    width -- �Ŵ�ǰ���ο��
                    height -- �Ŵ�ǰ���θ߶�
                    rgb -- ���������ɫ
			����	��
����ֵ		��
����˵��	
***************************************************************************************/
static int Rect_x25(uint x, uint y, uint width, uint height, int rgb)
{
	int color;
	uint nlx = 0, nly = 0;
	uint i = 0, j = 0, k=0, q=0;
	
	color = RGB((rgb>>16)&0xff, (rgb>>8)&0xff, rgb&0xff);
	
	if((width - 1) % 2 == 0 && (height - 1) % 2 == 0)
	{
		nlx = (((x + width - 1) * 5) >> 1) + gstAppScrOrg.x;		
		nly = (((y + height - 1) * 5) >> 1) + gstAppScrOrg.y;		

		for (q = 0; q < 3; q++)//������
		{
			for (k = 0; k < 3; k++)
			{
				s_AlphaBuf_WritePixel(nlx+q,nly+k, ALPHA_MODE_USEFORE);
                s_ForeBuf_WritePixel(nlx+q,nly+k, (ushort)color);
			}
		}
        s_FillBufToLcd(nlx, nly,nlx+2, nly+2);
        
		if(width == 1 && height == 1) return 0;
	}

	if ((width - 1) % 2 == 0)
	{
		nlx = (((x + width - 1) * 5) >> 1) + gstAppScrOrg.x;		
		
		for(j = 0; j < height-1; j += 2)	
		{
			nly = (((y + j) * 5) >> 1) + gstAppScrOrg.y;		
			
			for (q = 0; q < 3; q++)//������
			{
				for (k = 0; k < 5; k++)
				{
					s_AlphaBuf_WritePixel(nlx+q,nly+k, ALPHA_MODE_USEFORE);
                    s_ForeBuf_WritePixel(nlx+q,nly+k, (ushort)color);
				}
			}
            s_FillBufToLcd(nlx, nly,nlx+2, nly+4);
		}

		if(width == 1) return 0;
	}

	if ((height - 1) % 2 == 0)
	{
		nly = (((y + height - 1) * 5) >> 1) + gstAppScrOrg.y;		
		for(i = 0; i < width-1; i += 2)	
		{
			nlx = (((x + i) * 5) >> 1) + gstAppScrOrg.x;		
			
			for (q = 0; q < 5; q++)//������
			{
				for (k = 0; k < 3; k++)//������
				{
					s_AlphaBuf_WritePixel(nlx+q,nly+k, ALPHA_MODE_USEFORE);
                    s_ForeBuf_WritePixel(nlx+q,nly+k, (ushort)color);
				}
			}
            s_FillBufToLcd(nlx, nly,nlx+4, nly+2);
		}

		if(height == 1) return 0;
	}

	for(i = 0; i < width-1; i += 2)	
	{
		nlx = (((x + i) * 5) >> 1) + gstAppScrOrg.x;		
		
		for(j = 0; j < height-1; j += 2)	
		{
			nly = (((y + j) * 5) >> 1) + gstAppScrOrg.y;		
			
			for (q = 0; q < 5; q++)//������
			{
				for (k = 0; k < 5; k++)
				{
					s_AlphaBuf_WritePixel(nlx+q,nly+k, ALPHA_MODE_USEFORE);
                    s_ForeBuf_WritePixel(nlx+q,nly+k, (ushort)color);
				}
			}
            s_FillBufToLcd(nlx, nly,nlx+4, nly+4);
		}
	}

	return 0;
}

/* Note: ����
 *   1. ��Ļ���߼��������������֮�֣���λ��Ϊ����.
 *   2. �߼��������˵Ĳ����ӽ�Ϊ�ο������Ͻ�Ϊԭ��(0, 0)������������������
 *      ����������������
 *   3. ��������������Ļ���Դ�Ķ�Ӧ��ϵ�������Դ��һ�����ݶ�Ӧ�ĵ�Ϊԭ��(0, 0),
 *      ���������������Ҳ���Դ���ء�
 *   4. ����������߼������ԭ�㡢�����᷽�����κβ�һ�£�����Ҫ������ת����
 *   5. �����е�API�ӿھ�ʹ���߼����ꡣ
 *   6. S300/900��������, S800�������ã���˶���Ҫ������ת������ת�����̲�һ����
 */

/******************************************
 * x, y is the logic axis, and * px, py is the physic axis;
 *
 * attention: for higher efficiency, this function won't judge 
 * the margin value, but the function calling this should do.  
 *****************************************/
void s_LcdSetDot(int x, int y, int rgb)
{
	int r, g, b;

	r = (rgb >> 16) & 0xff;
	g = (rgb >> 8) & 0xff;
	b = rgb & 0xff;	

    s_AlphaBuf_WritePixel(x,y, ALPHA_MODE_USEFORE);
    s_ForeBuf_WritePixel(x,y, (ushort)RGB(r, g, b));
    s_FillBufToLcd(x, y,x, y);
}

/******************************************
 * x, y is the logic axis, and * px, py is the physic axis;
 *
 * attention: for higher efficiency, this function won't judge 
 * the margin value, but the function calling this should do.  
 *****************************************/
int s_LcdGetDot(int x, int y)
{
	uchar r, g ,b;
    int Color;

    s_LcdReadPixel(x, y, &Color);
    
	r = RGB_RED(Color);
	g = RGB_GREEN(Color);
	b = RGB_BLUE(Color);
	return (r<<16) | (g<<8) | b;
}

// s_LcdDrawLine(): ����ɫrgb����: (lx0, ly0) -> (lx1, ly1), ��Ϊ�߼�����.
void s_LcdDrawLine(int lx0, int ly0, int lx1, int ly1, ushort rgb)
{
	int x0 = lx0;
	int y0 = ly0;
	int x1 = lx1;
	int y1 = ly1;

	int i, xx0, xx1, yy0, yy1;
	int dx = x1 - x0;
	int dy = y1 - y0;

	if(dx < 0) dx = -dx;
	if(dy < 0) dy = -dy;

	if(dx >= dy)
	{
		xx0 = x0 > x1 ? x1 : x0;
		xx1 = x0 > x1 ? x0 : x1;
		yy0 = x0 > x1 ? y1 : y0;
		yy1 = x0 > x1 ? y0 : y1;
		for(i=0; i<=dx; i++)
		{
			int y = ((yy1-yy0) * i + yy0 * (xx1-xx0)) / (xx1 - xx0);
			int x = xx0 + i;
			s_LcdWritePixel(x, y, rgb);
		}
	}
	else
	{
		xx0 = y0 > y1 ? x1 : x0;
		xx1 = y0 > y1 ? x0 : x1;
		yy0 = y0 > y1 ? y1 : y0;
		yy1 = y0 > y1 ? y0 : y1;
		for(i=0; i<=dy; i++)
		{
			int x = ((xx1-xx0) * i + xx0 * (yy1-yy0)) / (yy1 - yy0);
			int y = yy0 + i;
			s_LcdWritePixel(x, y, rgb);
		}
	}
}
static void s_LcdReadMem(int left, int top,  int width, int height, uchar *data, int bitsPerPixel)
{
	int i,j, x, y;

	x = gstAppScrOrg.x + left;
	y = gstAppScrOrg.y + top;
	if (bitsPerPixel == 16)
	{
		COLORREF usColor;
		for (i = 0 ; i < height; i++)
		{
			for (j = 0 ; j < width; j++)
			{
				s_LcdReadPixel(x+j, y+i, &usColor);
				*((ushort *)data + i * width + j) = usColor;
			}
		}
	}
	else if (bitsPerPixel == 24)
	{
		COLORREF usColor;
		uchar r, g, b;
		for (i = 0 ; i < height; i++)
		{
			for (j = 0 ; j < width; j++)
			{
				s_LcdReadPixel(x+j, y+i, &usColor);
				r = RGB_RED(usColor);
				g = RGB_GREEN(usColor);
				b = RGB_BLUE(usColor);
				*((int *)data + i * width + j) = (r << 16) | (g << 8) | b;
			}
		}
	}
}
// s_LcdDrawIcon(): �����Ϊwidth, �߶�Ϊheight��Icon����buf��ʾ����Ļ.
//                  ���Ͻ�λ����Ļ�߼�����(x, y). Icon��������ר�ù���BMP24T16
//                  ����.

void s_LcdDrawIcon(int x, int y, int width, int height, uchar *buf)
{
	int i, j, offset, line_offset;
	int width2 = width;
	int height2 = height;
    
	if((x < 0) || (y < 0) || (x >= lcd_logic_max_x) || (y >= lcd_logic_max_y))
		return;

	if((x + width) > lcd_logic_max_x)
		width2 = lcd_logic_max_x - x;
	if((y + height) > lcd_logic_max_y)
		height2 = lcd_logic_max_y - y;

	for (i = 0; i < height2; i++)
	{
		line_offset = (height - i - 1) * width * 2;
		for (j = 0; j < width2; j++)
		{
			offset = line_offset + j * 2;
			s_AlphaBuf_WritePixel(x+j, y+i, ALPHA_MODE_USEFORE);
			s_ForeBuf_WritePixel(x+j, y+i, (buf[offset] << 8) | buf[offset+1]);
		}
	}
	s_FillBufToLcd(x, y, x+width2-1, y+height2-1);
}

int s_ScrClrRect(int left, int top, int width, int height, int role)
{
	LCD_ATTR_T *ptLcdAttr =GetLcdInfo(role);
	int right = left + width - 1;
	int bottom = top + height - 1;
	int x, y, i, j;
	int realW, realH;
	int tmpRight,tmpBottom,tmpx,tmpy;

    if (right < left) return -1;
    if (bottom < top) return -1;

	if (left >= ptLcdAttr->width) return 0;
    if (top >= ptLcdAttr->height) return 0;

    if (left < 0) left = 0;
    if (top < 0) top = 0;

    if (right >= ptLcdAttr->width)  right = ptLcdAttr->width - 1;
    if (bottom >= ptLcdAttr->height) bottom = ptLcdAttr->height - 1;

	if (left + width > ptLcdAttr->width) 
		realW = ptLcdAttr->width - left; 								
	else
		realW = width;

	if (top + height > ptLcdAttr->height) 
		realH = ptLcdAttr->height - top;
	else
		realH = height;

   	if(role == COLOR_ROLE) /* �������ӿ�ʱ������Ҫ�Ŵ� */
	{
		x = gstAppScrOrg.x + left;
		y = gstAppScrOrg.y + top;
        s_AlphaBuf_FillRect(ALPHA_MODE_USEBACK, x,y,x+realW-1,y+realH-1);
		s_FillBufToLcd(x,y,x+realW-1,y+realH-1);
	}
	else 
	{
		/*���ݺڰ���Ļ���Ŵ�2��*/
		if(ptLcdAttr->denominator == 1 && ptLcdAttr->numerator == 2)
		{
			x = gstAppScrOrg.x + left*2;
			y = gstAppScrOrg.y + top*2;
			
			tmpx = x - (x + 15)/16;//128->120
			tmpy = y;
			tmpRight = x+2*(realW-1)+1;
			tmpRight = tmpRight - (tmpRight + 15)/16;//128->120
			tmpBottom = y+2*(realH-1)+1;
			
			s_AlphaBuf_FillRect(ALPHA_MODE_USEBACK, tmpx,tmpy,tmpRight,tmpBottom);
			s_FillBufToLcd(tmpx,tmpy,tmpRight,tmpBottom);
		}
        /*���ݺڰ���Ļ���Ŵ�2.5��*/
		else if(ptLcdAttr->denominator == 2 && ptLcdAttr->numerator == 5)
		{
			int nlx, nly, k, q;
			if((realW - 1) % 2 == 0 && (realH - 1) % 2 == 0)
			{
				nlx = (((left + realW - 1) * 5) >> 1) + gstAppScrOrg.x;		
				nly = (((top + realH - 1) * 5) >> 1) + gstAppScrOrg.y;		

				for (q = 0; q < 3; q++)//������
				{
					for (k = 0; k < 3; k++)
					{
						s_AlphaBuf_WritePixel(nlx+q,nly+k, ALPHA_MODE_USEBACK);
					}
				}
                s_FillBufToLcd(nlx, nly, nlx+2, nly+2);
                
				if(realW == 1 && realH == 1) return 0;
			}

			if ((realW - 1) % 2 == 0)
			{
				nlx = (((left + realW - 1) * 5) >> 1) + gstAppScrOrg.x;		
				
				for(j = 0; j < realH-1; j += 2)	
				{
					nly = (((top + j) * 5) >> 1) + gstAppScrOrg.y;		
					
					for (q = 0; q < 3; q++)//������
					{
						for (k = 0; k < 5; k++)
						{
							s_AlphaBuf_WritePixel(nlx+q,nly+k, ALPHA_MODE_USEBACK);
						}
					}
                    s_FillBufToLcd(nlx, nly, nlx+2, nly+4);
				}

				if(realW == 1) return 0;
			}

			if ((realH - 1) % 2 == 0)
			{
				nly = (((top + realH - 1) * 5) >> 1) + gstAppScrOrg.y;		
				for(i = 0; i < realW-1; i += 2)	
				{
					nlx = (((left + i) * 5) >> 1) + gstAppScrOrg.x;		
					
					for (q = 0; q < 5; q++)
					{
						for (k = 0; k < 3; k++)
						{
							s_AlphaBuf_WritePixel(nlx+q,nly+k, ALPHA_MODE_USEBACK);
						}
					}
                    s_FillBufToLcd(nlx, nly, nlx+4, nly+2);
				}

				if(realH == 1) return 0;
			}

			for(i = 0; i < realW-1; i += 2)	
			{
				nlx = (((left + i) * 5) >> 1) + gstAppScrOrg.x;		
				
				for(j = 0; j < realH-1; j += 2)	
				{
					nly = (((top + j) * 5) >> 1) + gstAppScrOrg.y;		
					
					for (q = 0; q < 5; q++)//������
					{
						for (k = 0; k < 5; k++)
						{
							s_AlphaBuf_WritePixel(nlx+q,nly+k, ALPHA_MODE_USEBACK);
						}
					}
                    s_FillBufToLcd(nlx, nly, nlx+4, nly+4);
				}
			}
		}
	}

	return 0;
}
/************************************************************************************************************
����ԭ��	s_ScrFillRect(int left, int top, int width, int height, int rgb, int role)
��������	��ָ����ɫ������
����˵��	���	left -- ���Ͻ�x����
                    top -- ���Ͻ�y����
                    width -- ���ο��
                    height -- ���θ߶�
                    rgb -- ��ɫ
                    role -- COLOR_ROLE/MONO_ROLE �������ߺڰ���
			����	��
����ֵ:		
	 		0     ִ�гɹ�		
	 		-1    ��������Ƿ�

����˵��	
*************************************************************************************************************/
int s_ScrFillRect(int left, int top, int width, int height, int rgb, int role, int layer)
{
	LCD_ATTR_T *ptLcdAttr =GetLcdInfo(role);

	int right = left + width - 1;
	int bottom = top + height - 1;
	int x, y, i, j;
	int realW, realH;
	int tmpRight,tmpBottom,tmpx,tmpy;
	ushort color;

    if (right < left) return -1;
    if (bottom < top) return -1;

	if (left >= ptLcdAttr->width) return 0;
	if (left + width <= 0 || width <= 0) return 0;
    if (left < 0)
	{
		width = width + left;
	   	left = 0;
	}

    if (top >= ptLcdAttr->height) return 0;
	if (top + height <= 0 || height <= 0) return 0;
    if (top < 0) 
	{
		height = height + top;
		top = 0;
	}

    if (right >= ptLcdAttr->width)  right = ptLcdAttr->width - 1;
    if (bottom >= ptLcdAttr->height) bottom = ptLcdAttr->height - 1;

	if (left + width > ptLcdAttr->width) 
		realW = ptLcdAttr->width - left; 								
	else
		realW = width;

	if (top + height > ptLcdAttr->height) 
		realH = ptLcdAttr->height - top;
	else
		realH = height;

	color = RGB((rgb>>16)&0xff, (rgb>>8)&0xff, rgb&0xff);

   	if(role == COLOR_ROLE) /* �������ӿ�ʱ������Ҫ�Ŵ� */
	{
		x = gstAppScrOrg.x + left;
		y = gstAppScrOrg.y + top;

        if(layer == FORE_LAYER)
    	{
    		s_AlphaBuf_FillRect(ALPHA_MODE_USEFORE, x,y,x+realW-1,y+realH-1);
    		s_ForeBuf_FillRect(color, x,y,x+realW-1,y+realH-1);
    		s_FillBufToLcd(x,y,x+realW-1,y+realH-1);
    	}
    	else
    	{
    		s_BackBuf_FillRect(color, x,y,x+realW-1,y+realH-1);
    		s_FillBufToLcd(x,y,x+realW-1,y+realH-1);
    	}
	}
	else
	{
		/*���ݺڰ���Ļ���Ŵ�2��*/
		if(ptLcdAttr->denominator == 1 && ptLcdAttr->numerator == 2)
		{
			x = gstAppScrOrg.x + left*2;
			y = gstAppScrOrg.y + top*2;
			
			tmpx = x - (x + 15)/16;//128->120
			tmpy = y;
			tmpRight = x+2*(realW-1)+1;
			tmpRight = tmpRight - (tmpRight + 15)/16;//128->120
			tmpBottom = y+2*(realH-1)+1;
			
			s_AlphaBuf_FillRect(ALPHA_MODE_USEFORE, tmpx,tmpy,tmpRight,tmpBottom);
			s_ForeBuf_FillRect(color, tmpx,tmpy,tmpRight,tmpBottom);
			s_FillBufToLcd(tmpx,tmpy,tmpRight,tmpBottom);
		}
        /*���ݺڰ���Ļ���Ŵ�2.5��*/
		else if(ptLcdAttr->denominator == 2 && ptLcdAttr->numerator == 5)
		{
			Rect_x25(left, top, realW, realH, rgb);		
		}
	}

	return 0;
}

/************************************************************************************************************
����ԭ��	void s_ScrWriteBitmap(int x, int y, unsigned char *dat, int width, int height, int role, int zoom)
��������	д���ݵ�ָ����λ�ã�����������Ŵ�
����˵��	���	x -- ���Ͻ�x����
                    y -- ���Ͻ�y����
                    dat -- ���ݻ�������ַ
                    width -- ������
                    height -- ����߶�
                    role -- COLOR_ROLE/MONO_ROLE �������ߺڰ���
                    zoom -- 0-���Ŵ�1-�Ŵ�
			����	��
����ֵ		��
����˵��	
*************************************************************************************************************/
void s_ScrWriteBitmap(int left, int top, int width, int height, unsigned char *dat, 
						int bitsPerPixel, int role, int zoom, int layer)
{
	LCD_ATTR_T *ptLcdAttr =GetLcdInfo(role);
	
	int i, j, color,tmpx,tmpy;
	int realW, realH, val;
    int x, y;
	int tmpRight,tmpBottom;

	if(left > ptLcdAttr->width || top > ptLcdAttr->height) return ;
	if(zoom == 0 && role == MONO_ROLE)
	{
		if (left + width * ptLcdAttr->denominator / ptLcdAttr->numerator > ptLcdAttr->width) 
			realW = (ptLcdAttr->width - left) * ptLcdAttr->numerator / ptLcdAttr->denominator;
		else
			realW = width;

		if (top + height * ptLcdAttr->denominator / ptLcdAttr->numerator > ptLcdAttr->height) 
			realH = (ptLcdAttr->height - top) * ptLcdAttr->numerator / ptLcdAttr->denominator;
		else
			realH = height;
	}
	else
	{
		if (left + width > ptLcdAttr->width) 
			realW = ptLcdAttr->width - left; 								
		else
			realW = width;

		if (top + height > ptLcdAttr->height) 
			realH = ptLcdAttr->height - top;
		else
			realH = height;
	}

    /*�������ӿڣ�һһ��Ӧ��ʾ*/
	if(role == COLOR_ROLE)
	{
		if(bitsPerPixel != 1 && bitsPerPixel != 16 && bitsPerPixel != 24 && bitsPerPixel != 32) return ;

        x = gstAppScrOrg.x + left;
	    y = gstAppScrOrg.y + top;
        
		if(bitsPerPixel == 1)
    	{
    		color = ptLcdAttr->forecolor;
    		color = RGB(((color>>16)&0xff), ((color>>8)&0xff), (color&0xff));

			if(layer == FORE_LAYER) 
    		{
	    		for(i=0; i<realH; i++)
	    		{
	    			for(j=0; j<realW; j++)
	    			{
	    				val = dat[(i / 8) * width + j] & (1 << i % 8);
	    				if(val == 0) 
	    				{
	    					s_AlphaBuf_WritePixel(x+j, y+i, ALPHA_MODE_USEBACK);
	    					continue;
	    				}
	    				
	    				s_AlphaBuf_WritePixel(x+j, y+i, ALPHA_MODE_USEFORE);
	    				s_ForeBuf_WritePixel(x+j, y+i, (ushort)(color&0xffff));
	    			}
	    		}
			}
    		else if(layer == BACK_LAYER) 
    		{
    			for(i=0; i<realH; i++)
	    		{
	    			for(j=0; j<realW; j++)
	    			{
	    				val = dat[(i / 8) * width + j] & (1 << i % 8);
	    				if(val == 0) 
	    				{
	    					s_AlphaBuf_WritePixel(x+j, y+i, ALPHA_MODE_USEBACK);
	    					continue;
	    				}
    					s_BackBuf_WritePixel(x+j, y+i, (ushort)(color&0xffff));
	    			}
    			}
    		}
    		s_FillBufToLcd(x, y, x+realW-1, y+realH-1);
    	}
    	else if(bitsPerPixel == 16)
    	{
    		ushort usColor;
			if(layer == FORE_LAYER) 
    		{
	    		for(i=0; i<realH; i++)
	    		{
	    			for(j=0; j<realW; j++)
	    			{
	    				usColor = *((ushort *)dat + i * width + j);  
	    				s_AlphaBuf_WritePixel(x+j, y+i, ALPHA_MODE_USEFORE);
	    				s_ForeBuf_WritePixel(x+j, y+i, usColor);
	    			}
	    		}
			}
			else if(layer == BACK_LAYER) 
			{
				for(i=0; i<realH; i++)
	    		{
	    			for(j=0; j<realW; j++)
	    			{
	    				usColor = *((ushort *)dat + i * width + j);  
    					s_BackBuf_WritePixel(x+j, y+i, usColor);
    				}
    			}
    		}
			s_FillBufToLcd(x, y, x+realW-1, y+realH-1);
    	}
    	else if(bitsPerPixel == 24)
    	{
    		if(layer == FORE_LAYER) 
    		{
	    		for(i=0; i<realH; i++)
	    		{
	    			for(j=0; j<realW; j++)
	    			{
	    				val = *((int *)dat + i * width + j);  
	    				color = RGB(((val>>16)&0xff), ((val>>8)&0xff), (val&0xff));
	    				s_AlphaBuf_WritePixel(x+j, y+i, ALPHA_MODE_USEFORE);
	    				s_ForeBuf_WritePixel(x+j, y+i, (ushort)(color&0xffff));
	    			}
	    		}
    		}
    		else if(layer == BACK_LAYER) 
    		{
				for(i=0; i<realH; i++)
	    		{
	    			for(j=0; j<realW; j++)
	    			{
	    				val = *((int *)dat + i * width + j);  
	    				color = RGB(((val>>16)&0xff), ((val>>8)&0xff), (val&0xff));
    					s_BackBuf_WritePixel(x+j, y+i, (ushort)(color&0xffff));
    				}
    			}
    		}
    		s_FillBufToLcd(x, y, x+realW-1, y+realH-1);
    	}
    	else if(bitsPerPixel == 32)
    	{
    		if (layer == FORE_LAYER)
    		{
	    		for (i = 0; i < realH; i++)
	    		{
	    			for (j = 0; j < realW; j++)
	    			{
	    				val = *((int *)dat + i * width + j);  
	    				if(!(val >> 24)) continue;
	    				color = RGB(((val >> 16) & 0xff), ((val >> 8) & 0xff), (val & 0xff));
	    				
	    				s_AlphaBuf_WritePixel(x+j, y+i, ALPHA_MODE_USEFORE);
	    				s_ForeBuf_WritePixel(x+j, y+i, (ushort)(color&0xffff));
	    			}
	    		}
    		}
    		else if (layer == BACK_LAYER)
    		{
				for (i = 0; i < realH; i++)
	    		{
	    			for (j = 0; j < realW; j++)
	    			{
	    				val = *((int *)dat + i * width + j);  
	    				if(!(val >> 24)) continue;
	    				color = RGB(((val >> 16) & 0xff), ((val >> 8) & 0xff), (val & 0xff));

    					s_BackBuf_WritePixel(x+j, y+i, (ushort)(color&0xffff));
	    			}
    			}
    		}
    		s_FillBufToLcd(x, y, x+realW-1, y+realH-1);
    	}
	}
     /*���ڰ����ӿڣ�S800�Ŵ�2.5����S900/S300�Ŵ�2��,�д��ֿ�ʱ����Ҫ�Ŵ�*/   
	else 
	{
		color = ptLcdAttr->forecolor;
		color = RGB(((color>>16)&0xff), ((color>>8)&0xff), (color&0xff));

		/*���ݺڰ���Ļ���Ŵ�2��*/
		if(ptLcdAttr->denominator == 1 && ptLcdAttr->numerator == 2)
		{
			if(zoom == 0)
			{
				x = gstAppScrOrg.x + left*2;
				y = gstAppScrOrg.y + top*2;
				x = x - (x+15)/16;//128->120
				
				for (j = 0; j < realW; j++)
				{
					for(i = 0; i < realH; i++)
					{
						val = dat[(i / 8) * width + j] & (1 << i % 8);
						if(val == 0) 
						{
							s_AlphaBuf_WritePixel(x+j, y+i, ALPHA_MODE_USEBACK);
						}
						else
						{
							s_AlphaBuf_WritePixel(x+j, y+i, ALPHA_MODE_USEFORE);
                            s_ForeBuf_WritePixel(x+j, y+i, (ushort)color);
						}
					}
				}  
				s_FillBufToLcd(x, y, x+realW-1, y+realH-1);
			}
			else /*û�д��ֿ�ʱ����Ҫ�Ŵ�2�� */
			{
				x = gstAppScrOrg.x + left*2;
				y = gstAppScrOrg.y + top*2;

				for(i = 0; i < realH; i++)
				{
					tmpy = y + i*2;
					for (j = 0; j < realW; j++) 
					{
						tmpx = x + j*2;
						tmpx = tmpx - (tmpx + 15) / 16; //128->120
						
						val = dat[(i / 8) * width + j] & (1 << i % 8);
						if(val == 0) 
						{
							s_AlphaBuf_FillRect(ALPHA_MODE_USEBACK, tmpx, tmpy, tmpx+1, tmpy+1);
							continue;
						}
						s_AlphaBuf_FillRect(ALPHA_MODE_USEFORE, tmpx, tmpy, tmpx+1, tmpy+1);
						s_ForeBuf_FillRect(color, tmpx, tmpy, tmpx+1, tmpy+1);
					}
				}  
				tmpx = x - (x+15)/16;//128->120
				tmpy = y;
				tmpRight = x+2*(realW-1)+1;
				tmpRight = tmpRight - (tmpRight+15)/16;//128->120
				tmpBottom = y+2*(realH-1)+1;
				s_FillBufToLcd(tmpx, tmpy, tmpRight, tmpBottom);
			}
		}
		else if(ptLcdAttr->denominator == 2 && ptLcdAttr->numerator == 5)
		{
			if (zoom == 0)  /*S800�Ŵ�2.5�� */
			{
				x = gstAppScrOrg.x + left * 5 / 2;
				y = gstAppScrOrg.y + top * 5 / 2;
				
				for (i = 0; i < realH; i++) 	/*�д��ֿ�ʱ�����÷Ŵ� */
				{
					for (j = 0; j < realW; j++)
					{
						val = dat[(i / 8) * width + j] & (1 << i % 8);
						if(val == 0)
						{
							s_AlphaBuf_WritePixel(x+j, y+i, ALPHA_MODE_USEBACK);
                            
						}
						else
						{
							s_AlphaBuf_WritePixel(x+j, y+i, ALPHA_MODE_USEFORE);
                            s_ForeBuf_WritePixel(x+j, y+i, (ushort)color);
						}
					}
				}
                s_FillBufToLcd(x, y, x+realW-1, y+realH-1);
			}
			else if (zoom == 1) /* û�д��ֿ�ʱ����Ҫ�Ŵ�2.5�� */
			{
				Matrix_x25(left, top, realW, realH, width, dat, color);
			}
		}
	}

	return ;
}

void s_ScrReadBitmap(uint left, uint top, uint width, uint height, unsigned char *dat, 
						int bitsPerPixel, int zoom, int layer)
{
	LCD_ATTR_T *ptLcdAttr =GetLcdInfo(COLOR_ROLE);
	
	int i, j, tmpx, tmpy;

	if(left + width > ptLcdAttr->width || top + height > ptLcdAttr->height) return ;
	
	if(bitsPerPixel != 16 && bitsPerPixel != 24) return ;

	if (layer != FORE_LAYER && layer != BACK_LAYER)
	{
		s_LcdReadMem(left, top, width, height, dat, bitsPerPixel);
		return;
	}
		if(bitsPerPixel == 16)
		{
            tmpx = gstAppScrOrg.x + left; 
			tmpy = gstAppScrOrg.y + top;
			if(layer == FORE_LAYER) 
			{
	            for(i=0; i<height; i++)
	    		{
	    			for(j=0; j<width; j++)
	    			{
	    				*((ushort *)dat + i * width + j) = s_ForeBuf_ReadPixel(tmpx+j, tmpy+i);
	    			}
	    		}
			}
			else if(layer == BACK_LAYER) 
			{
				for(i=0; i<height; i++)
	    		{
	    			for(j=0; j<width; j++)
	    			{
	    				*((ushort *)dat + i * width + j) = s_BackBuf_ReadPixel(tmpx+j, tmpy+i);
	    			}
	    		}
			}
		}
		else  /* 24 bits */
		{
            tmpx = gstAppScrOrg.x + left; 
			tmpy = gstAppScrOrg.y + top;
            uchar r, g, b, alpha;
    		ushort usColor;
			if(layer == FORE_LAYER) 
    		{
	    		for(i=0; i<height; i++)
	    		{
	    			for(j=0; j<width; j++)
	    			{
    				
    					usColor = s_ForeBuf_ReadPixel(tmpx+j, tmpy+i);
    					r = RGB_RED(usColor);
    					g = RGB_GREEN(usColor);
    					b = RGB_BLUE(usColor);
    					alpha = s_AlphaBuf_ReadPixel(tmpx+j, tmpy+i) ? 0xff : 0;
    					*((int *)dat + i * width + j) = (alpha << 24) | (r << 16) | (g << 8) | b;
    				}
	    		}
			}
			else if(layer == BACK_LAYER) 
			{
				for(i=0; i<height; i++)
	    		{
	    			for(j=0; j<width; j++)
	    			{
    					usColor = s_BackBuf_ReadPixel(tmpx+j, tmpy+i);
    					r = RGB_RED(usColor);
    					g = RGB_GREEN(usColor);
    					b = RGB_BLUE(usColor);
    					*((int *)dat + i * width + j) = (r << 16) | (g << 8) | b;
    				}
    			}
    		}
		}
	
}

unsigned char s_ScrRestore(unsigned char mode)
{
	int i, j, size;
	static uchar k_ScrSave_X = 0;
	static uchar k_ScrSave_Y = 0;
	static uchar k_LcdAlphaRAM[TOTAL_PIXEL_CNT/8];
	static ushort k_LcdBackRAM[TOTAL_PIXEL_CNT];
	static uchar k_ScrRestoreEnable = 0;
	int bkOffset;
    LCD_ATTR_T *ptLcdAttr =GetLcdInfo(COLOR_ROLE);
	int x,y;

	if (mode) /* restore */
	{
		if (k_ScrRestoreEnable == 0) return 1;

		x = gstAppScrOrg.x;
		y = gstAppScrOrg.y;
		for(i=0; i<ptLcdAttr->width; i++)
		{
			for(j=0; j<ptLcdAttr->height; j++)
			{
				bkOffset = j * ptLcdAttr->width + i;
				if((k_LcdAlphaRAM[bkOffset / 8] & (1<<(bkOffset%8))) == 0)
				{
					s_AlphaBuf_WritePixel(x+i, y+j, ALPHA_MODE_USEBACK);
				}
				else
				{
					s_AlphaBuf_WritePixel(x+i, y+j, ALPHA_MODE_USEFORE);
					s_ForeBuf_WritePixel(x+i, y+j, k_LcdBackRAM[bkOffset]);
				}
			}
		}
		s_FillBufToLcd(x, y, x+ptLcdAttr->width-1, y+ptLcdAttr->height-1);

		gtLcdInfo.tLcdAttr[MONO_ROLE].cursor_x = k_ScrSave_X;
		gtLcdInfo.tLcdAttr[MONO_ROLE].cursor_y = k_ScrSave_Y;
		k_ScrRestoreEnable = 0;
	}
	else /* backup */
	{
		x = gstAppScrOrg.x;
		y = gstAppScrOrg.y;
		memset(k_LcdAlphaRAM, 0, sizeof(k_LcdAlphaRAM));
		for(i=0; i<ptLcdAttr->width; i++)
		{
			for(j=0; j<ptLcdAttr->height; j++)
			{
				if(s_AlphaBuf_ReadPixel(x+i,y+j) == 0) continue;
				bkOffset = j * ptLcdAttr->width + i;
				k_LcdAlphaRAM[bkOffset / 8] |= 1 << (bkOffset%8);
				k_LcdBackRAM[bkOffset] = s_ForeBuf_ReadPixel(x+i,y+j);
			}
		}

		k_ScrSave_X = gtLcdInfo.tLcdAttr[MONO_ROLE].cursor_x;
		k_ScrSave_Y = gtLcdInfo.tLcdAttr[MONO_ROLE].cursor_y;
		k_ScrRestoreEnable = 1;
	}
	return 0;
}

void s_ScrGetLcdArea(int *left, int *top, int *width, int *height)
{
	LCD_ATTR_T *ptLcdAttr =GetLcdInfo(COLOR_ROLE);

    *left = gstAppScrOrg.x;
    *top = gstAppScrOrg.y;
    *width  = ptLcdAttr->width;
    *height = ptLcdAttr->height;
}


#include "..\font\font.h"
void FrameBufferTextOut(int x, int y, int mode, int color, unsigned char *txt)
{
	LCD_ATTR_T *ptLcdAttr =GetLcdInfo(COLOR_ROLE);
	int cursor_x, cursor_y;
    int i, j, tmpx, tmpy;
	int lcd_height, lcd_width;	
    int width, height, lineHeight, charWidth, CodeBytes;
    unsigned char DotBuf[48 * 48 / 8];
	unsigned short rgb;

	if (x >= ptLcdAttr->width || y >= ptLcdAttr->height) return;
	if(x < 0 || y < 0) return ;
    if (txt == NULL) return;

	lcd_width = ptLcdAttr->width;
	lcd_height = ptLcdAttr->height;

	cursor_x = x;
	cursor_y = y;
	rgb = RGB(((color>>16)&0xff), ((color>>8)&0xff), (color&0xff));
	
    s_GetLcdFontCharAttr(&charWidth, &lineHeight, NULL, NULL);
    
    while (*txt)
    {
        /*��ȡ�ַ��������ݺ������ȡ��߶�*/
        CodeBytes = s_GetLcdFontDot(0, txt, DotBuf, &width, &height);

		/*���統ǰ�в�����ʾ���ַ�����������һ��*/
		if (cursor_x + width > lcd_width)
			return ;
		for(i=0; i<height; i++)
		{
			tmpy = gstAppScrOrg.y + cursor_y + i;
			for(j=0; j<width; j++)
			{
				tmpx = gstAppScrOrg.x + cursor_x + j; 
				if((DotBuf[(i / 8) * width + j] & (1 << i % 8)) == 0) continue;
				s_LcdWritePixel(tmpx, tmpy, rgb);
			}
		}
		cursor_x += width;
        
        if (lineHeight < height) 
			lineHeight = height;

        /*��ȡ��һ���ַ���ַ*/
        txt += CodeBytes;
    }
}
int FrameBufferFillRect(int left, int top, int width, int height, int rgb, int alpha)
{
	int x, y, i, j;
	uchar r, g, b;
	ushort color, readcolor;

	color = RGB((rgb>>16)&0xff, (rgb>>8)&0xff, rgb&0xff);

	x = gstAppScrOrg.x + left;
	y = gstAppScrOrg.y + top;

	for(j = 0; j < height; j++)
	{
		for(i=0; i<width; i++)
		{
			s_LcdReadPixel(x+i, y+j, &readcolor);
			r = RGB_RED(color) * alpha / 255 +  RGB_RED(readcolor) * (255 - alpha) / 255;
			g = RGB_GREEN(color) * alpha / 255 + RGB_GREEN(readcolor) * (255 - alpha) / 255;
			b = RGB_BLUE(color) * alpha / 255 +  RGB_BLUE(readcolor) * (255 - alpha) / 255;
			s_LcdWritePixel(x+i, y+j, RGB(r,g,b));
		}
	}
	

	return 0;
}
void CLcdFresh(int left, int top, int width, int height)
{
	LCD_ATTR_T *ptLcdAttr =GetLcdInfo(COLOR_ROLE);
	int x,y;

	if (left >= ptLcdAttr->width) return ;
    if (top >= ptLcdAttr->height) return ;
    if (left < 0 || top < 0 || width < 0 || height < 0) return ;

	x = gstAppScrOrg.x + left;
	y = gstAppScrOrg.y + top;

	s_FillBufToLcd(x, y, x+width-1, y+height-1);
}
unsigned int s_ScrGetLcdBufAddr(void)
{
	switch(get_lcd_type())
	{
	case DMA_LCD:
	case DMA_S90357_LCD:
	case DMA_TM035KBH08_LCD:
	case DMA_H28C69_00N_LCD:
	case DMA_ST7789V_TM_YB_LCD:
		return DMA_LCD_BASE;
	default:
		return 0;
	}
}
int s_ScrDrawBmp(uint left, uint top, uchar *bmp)
{
	int i, j, color;
	int realW, realH, val;
	unsigned short bitsPerPixel = 0;
	unsigned char ucR, ucG, ucB;
	int skip = 0;
	int residue = 0;
	int rowByte = 0;
	
	//�����Ӷ��ļ����͵��ж�
	realW = (bmp[21]<<24) + (bmp[20]<<16) + (bmp[19]<<8) + bmp[18]; 
	realH = (bmp[25]<<24) + (bmp[24]<<16) + (bmp[23]<<8) + bmp[22]; 

	bitsPerPixel = bmp[28];
	if(bitsPerPixel != 16 && bitsPerPixel != 24) return -1;
	
	if (bitsPerPixel == 24)
	{
		rowByte = realW * 3;
		residue = rowByte % 4;
		if (residue)
		{
			skip = 4 - residue;
		}
	}
	else if (bitsPerPixel == 16)
	{
		rowByte = realW * 2;
		residue = rowByte % 4;
		if (residue)
		{
			skip = 4 - residue;
		}
	}


	if(bitsPerPixel == 16)
	{
		ushort usColor;
		for(i = 0; i < realH; i++)
		{
			for(j = 0; j < realW; j++)
			{
				usColor = (unsigned short)(bmp[54+((realH-i-1)*(rowByte+skip)+2*j+1)]) << 9 
					| (unsigned short)(bmp[54+((realH-i-1)*(rowByte+skip)+2*j)] & 0xE0) << 1 
					|  (unsigned short)(bmp[54+((realH-i-1)*(rowByte+skip)+2*j)] & 0x1F) ;
				s_AlphaBuf_WritePixel(left+j, top+i, ALPHA_MODE_USEFORE);
	    		s_ForeBuf_WritePixel(left+j, top+i, usColor);
			
			}

		}
		s_FillBufToLcd(left, top, left+realW-1, top+realH-1);
    }
    else if(bitsPerPixel == 24)
    {
		for(i = 0; i < realH; i++)
		{
			for(j = 0; j < realW; j++)
			{
				ucB = bmp[54+((realH-left-1)*(rowByte+skip)+3*top)];
				ucG = bmp[54+((realH-left-1)*(rowByte+skip)+3*top+1)];
				ucR = bmp[54+((realH-left-1)*(rowByte+skip)+3*top+2)];
				color = RGB(ucR, ucG, ucB);
				s_AlphaBuf_WritePixel(left+j, top+i, ALPHA_MODE_USEFORE);
	    		s_ForeBuf_WritePixel(left+j, top+i, (ushort)(color&0xffff));
			}
		}
		s_FillBufToLcd(left, top, left+realW-1, top+realH-1);
    }
	return 0;
}


/**********************for B210 Handset**********************/
static ushort k_LcdBackRAM_Echo[TOTAL_PIXEL_CNT];
static uchar k_LcdAlphaRAM_Echo[TOTAL_PIXEL_CNT/8];
static volatile uchar k_ScrOpenEcho_Flag=0;
static volatile uchar k_ScrLock_Mutex=0;



enum {SCRECHODESABLE=0x01,SCRECHOCLOSE,OPERATERROR};
enum {ECHO_CLOSE=0,ECHO_OPEN,ECHO_LOCK};

void ScrSetOutput(int device)
{
	static uchar suStoreFlag=0, k_ScrSave_X = 0, k_ScrSave_Y = 0;
    static uchar k_LcdAlphaRAM[TOTAL_PIXEL_CNT/8];
    static ushort k_LcdBackRAM[TOTAL_PIXEL_CNT];
    int bkOffset, i, j, x, y;  
    LCD_ATTR_T *ptLcdAttr =GetLcdInfo(COLOR_ROLE);

    if(get_machine_type()!=D210)return;
	if(0x01==device)
	{
        k_TftWrite_Lock = 0;//Lcd Write enable
		if(0==suStoreFlag) return;//condition verify		
		//restore lcd        
		x = gstAppScrOrg.x;
		y = gstAppScrOrg.y;
		for(i=0; i<ptLcdAttr->width; i++)
		{
			for(j=0; j<ptLcdAttr->height; j++)
			{
				bkOffset = j * ptLcdAttr->width + i;
				if((k_LcdAlphaRAM[bkOffset / 8] & (1<<(bkOffset%8))) == 0)
				{
					s_AlphaBuf_WritePixel(x+i, y+j, ALPHA_MODE_USEBACK);
				}
				else
				{
					s_AlphaBuf_WritePixel(x+i, y+j, ALPHA_MODE_USEFORE);
					s_ForeBuf_WritePixel(x+i, y+j, k_LcdBackRAM[bkOffset]);
				}
			}
		}
		//s_FillBufToTft(x, y, x+ptLcdAttr->width-1, y+ptLcdAttr->height-1);
		s_FillBufToLcd(x, y, x+ptLcdAttr->width-1, y+ptLcdAttr->height-1);

		gtLcdInfo.tLcdAttr[MONO_ROLE].cursor_x = k_ScrSave_X;
		gtLcdInfo.tLcdAttr[MONO_ROLE].cursor_y = k_ScrSave_Y;
        suStoreFlag=0;
	}

	if(0x02==device)//if to cache
	{
        k_TftWrite_Lock = 1;//Lcd write disable
		if(1==suStoreFlag)return;//condition verify
		x = gstAppScrOrg.x;
		y = gstAppScrOrg.y;
		memset(k_LcdAlphaRAM, 0, sizeof(k_LcdAlphaRAM));
		for(i=0; i<ptLcdAttr->width; i++)
		{
			for(j=0; j<ptLcdAttr->height; j++)
			{
				if(s_AlphaBuf_ReadPixel(x+i,y+j) == 0) continue;
				bkOffset = j * ptLcdAttr->width + i;
				k_LcdAlphaRAM[bkOffset / 8] |= 1 << (bkOffset%8);
				k_LcdBackRAM[bkOffset] = s_ForeBuf_ReadPixel(x+i,y+j);
			}
		}

		k_ScrSave_X = gtLcdInfo.tLcdAttr[MONO_ROLE].cursor_x;
		k_ScrSave_Y = gtLcdInfo.tLcdAttr[MONO_ROLE].cursor_y;          
        suStoreFlag=1;
	}
	return ;
}

void ScrSetEcho(int mode)
{
	int i, j, x, y, bkOffset;   
    LCD_ATTR_T *ptLcdAttr =GetLcdInfo(COLOR_ROLE);

	if(get_machine_type()!=D210)return;
	if(ECHO_CLOSE==mode) k_ScrOpenEcho_Flag=0;
	if(ECHO_OPEN==mode) k_ScrOpenEcho_Flag=1;
	if(ECHO_LOCK==mode)
	{
		k_ScrLock_Mutex=1; //set protecting vector
		
		//copy display memory
		x = gstAppScrOrg.x;
		y = gstAppScrOrg.y;
		memset(k_LcdAlphaRAM_Echo, 0, sizeof(k_LcdAlphaRAM_Echo));
		for(i=0; i<ptLcdAttr->width; i++)
		{
			for(j=0; j<ptLcdAttr->height; j++)
			{
				if(s_AlphaBuf_ReadPixel(x+i,y+j) == 0) continue;
				bkOffset = j * ptLcdAttr->width + i;
				k_LcdAlphaRAM_Echo[bkOffset / 8] |= 1 << (bkOffset%8);
				k_LcdBackRAM_Echo[bkOffset] = s_ForeBuf_ReadPixel(x+i,y+j);
			}
		}		

		k_ScrLock_Mutex=0;
	}    
	return ;	
}

int s_ScrEcho(uchar mode)
{
	static ushort k_LcdBackRAM_Bak[TOTAL_PIXEL_CNT];
    static uchar k_LcdAlphaRAM_Bak[TOTAL_PIXEL_CNT/8];
    static uchar k_ScrSave_X = 0, k_ScrSave_Y = 0;
	volatile static uchar EchoEnable=0;
	int i,j, x, y, bkOffset;
    LCD_ATTR_T *ptLcdAttr =GetLcdInfo(COLOR_ROLE);

	if(!k_ScrOpenEcho_Flag)	return	SCRECHOCLOSE;
	if(k_ScrLock_Mutex==1)	return	SCRECHODESABLE;	//verify the condition
			
	if(0x01==mode)//store display memory ,and display echo
	{
        if(1==EchoEnable) return OPERATERROR;
        EchoEnable=1;	
        //store display memory
		x = gstAppScrOrg.x;
		y = gstAppScrOrg.y;
		memset(k_LcdAlphaRAM_Bak, 0, sizeof(k_LcdAlphaRAM_Bak));
		for(i=0; i<ptLcdAttr->width; i++)
		{
			for(j=0; j<ptLcdAttr->height; j++)
			{
				if(s_AlphaBuf_ReadPixel(x+i,y+j) == 0) continue;
				bkOffset = j * ptLcdAttr->width + i;
				k_LcdAlphaRAM_Bak[bkOffset / 8] |= 1 << (bkOffset%8);
				k_LcdBackRAM_Bak[bkOffset] = s_ForeBuf_ReadPixel(x+i,y+j);
			}
		}
		k_ScrSave_X = gtLcdInfo.tLcdAttr[MONO_ROLE].cursor_x;
		k_ScrSave_Y = gtLcdInfo.tLcdAttr[MONO_ROLE].cursor_y;   

        //display echo
		for(i=0; i<ptLcdAttr->width; i++)
		{
			for(j=0; j<ptLcdAttr->height; j++)
			{
				bkOffset = j * ptLcdAttr->width + i;
				if((k_LcdAlphaRAM_Echo[bkOffset / 8] & (1<<(bkOffset%8))) == 0)
				{
					s_AlphaBuf_WritePixel(x+i, y+j, ALPHA_MODE_USEBACK);
				}
				else
				{
					s_AlphaBuf_WritePixel(x+i, y+j, ALPHA_MODE_USEFORE);
					s_ForeBuf_WritePixel(x+i, y+j, k_LcdBackRAM_Echo[bkOffset]);
				}
			}
		}
		//s_FillBufToTft(x, y, x+ptLcdAttr->width-1, y+ptLcdAttr->height-1); 
		s_FillBufToLcd(x, y, x+ptLcdAttr->width-1, y+ptLcdAttr->height-1);
		
	}
	
	if(0x00==mode)//restore display memory.
	{
		if(!EchoEnable) return OPERATERROR;
		EchoEnable=0;
		x = gstAppScrOrg.x;
		y = gstAppScrOrg.y;		
		for(i=0; i<ptLcdAttr->width; i++)
		{
			for(j=0; j<ptLcdAttr->height; j++)
			{
				bkOffset = j * ptLcdAttr->width + i;
				if((k_LcdAlphaRAM_Bak[bkOffset / 8] & (1<<(bkOffset%8))) == 0)
				{
					s_AlphaBuf_WritePixel(x+i, y+j, ALPHA_MODE_USEBACK);
				}
				else
				{
					s_AlphaBuf_WritePixel(x+i, y+j, ALPHA_MODE_USEFORE);
					s_ForeBuf_WritePixel(x+i, y+j, k_LcdBackRAM_Bak[bkOffset]);
				}
			}
		}
		gtLcdInfo.tLcdAttr[MONO_ROLE].cursor_x = k_ScrSave_X;
		gtLcdInfo.tLcdAttr[MONO_ROLE].cursor_y = k_ScrSave_Y;
		//s_FillBufToTft(x, y, x+ptLcdAttr->width-1, y+ptLcdAttr->height-1);		
		s_FillBufToLcd(x, y, x+ptLcdAttr->width-1, y+ptLcdAttr->height-1);
	}

	return 0;
}
uchar s_GetScrEcho(void)
{
	return k_ScrOpenEcho_Flag;
}

void s_SetImplicitEcho ()
{	
	ScrSetOutput(2);	
	ScrCls();	
	//draw the what you want
    SCR_PRINT(0, 3, CFONT, " �� �� �� �� �� ", " RETURN TO BASE");
	ScrSetEcho(ECHO_LOCK);
	ScrSetOutput(1);
}

