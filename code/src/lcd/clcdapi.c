#include <posapi.h>
#include "base.h"
#include "LcdDrv.h"
#include "LcdApi.h"
#include "clcdApi.h"
#include <stdarg.h>
#include "..\font\font.h"

extern LCD_ATTR_T *GetLcdInfo(int role);
extern app_origin_t gstAppScrOrg;

/**************************************************************************************
����ԭ��	void ScrCls(void) 
��������	���������Ļ
����˵��	���	��
			����	��
����ֵ		��
����˵��	
***************************************************************************************/
void ScrCls(void)
{
	ST_LCD_INFO stLcdInfo;

	CLcdGetInfo(&stLcdInfo);
    s_ScrClrRect(0, 0, stLcdInfo.width, stLcdInfo.height, COLOR_ROLE);
	s_ScrCursorSet(0, 0);
}

/***************************************************************************************
����ԭ��	void LcdClrLine(int startline, int endline)
��������	���ָ����һ�л������У�����������ʱ�޶���
����˵��	���	startline -- ��ʼ��
					endline -- ������
			����	��
����ֵ		��
����˵��	��������ѡ���������СΪ׼��������Ϊ8X16,��һ��Ϊ16������
***************************************************************************************/
int CLcdClrLine(uint startline, uint endline)
{
	LCD_ATTR_T *ptLcdAttr =GetLcdInfo(COLOR_ROLE);
	ST_LCD_INFO stLcdInfo;
	int LineHeight, totalLineIndex;

    s_GetLcdFontCharAttr(NULL, &LineHeight, NULL, NULL);

	totalLineIndex = (ptLcdAttr->height+LineHeight-1) / LineHeight - 1;
	if (startline > totalLineIndex && endline > totalLineIndex)
		return -1;

    if (startline > endline)
    	return -1;

    if (endline > totalLineIndex)
        endline = totalLineIndex;
	
   	s_ScrClrRect(0, startline*LineHeight, ptLcdAttr->width, (endline-startline+1)*LineHeight, COLOR_ROLE);
	return 0;
}

/***************************************************************************************
����ԭ��	int CLcdClrRect(uint left, uint top, uint right, uint bottom, uint mode)

��������	��������������ʾ��Ϣ
����˵��	���	left -- ��
                    top  -- ��
                    right -- ��
                    bottom -- ��
����ֵ		
			0      ִ�гɹ�
		   -1      ��������Ƿ�	
����˵��	
***************************************************************************************/
int CLcdClrRect(uint left, uint top, uint right, uint bottom, uint mode)
{
	LCD_ATTR_T *ptLcdAttr =GetLcdInfo(COLOR_ROLE);
	int MAX_X = ptLcdAttr->width - 1;
	int MAX_Y = ptLcdAttr->height - 1;

	if (mode != 0 && mode != 1) return -1;
    if (right > MAX_X || bottom > MAX_Y) return -1;
	if (left > right || top > bottom) return -1;

	if(mode == 0)
	{
		s_ScrClrRect(left, top, right-left+1, bottom-top+1, COLOR_ROLE);
	}
	else if(mode == 1)
	{
		s_ScrClrRect(left, top, 1, bottom - top + 1, COLOR_ROLE);
		s_ScrClrRect(right, top, 1, bottom - top + 1, COLOR_ROLE);
		s_ScrClrRect(left, top, right - left+ 1, 1, COLOR_ROLE);
		s_ScrClrRect(left, bottom, right - left+ 1, 1, COLOR_ROLE);
	}
	return 0;
}

/***************************************************************************************
����ԭ��	int CLcdBgDrawBox(uint left, uint top, uint right, uint bottom, uint color)

��������	����ɫrgb����������: 
����˵��	���	left -- ��
                    top  -- ��
                    right -- ��
                    bottom -- ��
����ֵ		
			0      ִ�гɹ�
		   -1      ��������Ƿ�	
����˵��	
***************************************************************************************/

int CLcdBgDrawBox(uint left, uint top, uint right, uint bottom, uint color)
{
	LCD_ATTR_T *ptLcdAttr =GetLcdInfo(COLOR_ROLE);
	int MAX_X = ptLcdAttr->width - 1;
	int MAX_Y = ptLcdAttr->height - 1;

    if (right > MAX_X || bottom > MAX_Y) return -1;
	if (left > right || top > bottom) return -1;

	s_ScrFillRect(left, top, right - left + 1, bottom - top + 1, color, COLOR_ROLE, BACK_LAYER);
	return 0;
}

/***************************************************************************************
����ԭ��	int CLcdDrawRect(uint left, uint top, uint right, uint bottom, uint rgb)

��������	����ɫrgb�����ο�: 
����˵��	���	left -- ��
                    top  -- ��
                    right -- ��
                    bottom -- ��
����ֵ		
			0      ִ�гɹ�
		   -1      ��������Ƿ�	
����˵��	
***************************************************************************************/
int CLcdDrawRect(uint left, uint top, uint right, uint bottom, uint rgb)
{
	LCD_ATTR_T *ptLcdAttr =GetLcdInfo(COLOR_ROLE);
	int MAX_X = ptLcdAttr->width - 1;
	int MAX_Y = ptLcdAttr->height - 1;

	if (right > MAX_X || bottom > MAX_Y) return -1;
    if (left > right || top > bottom) return -1;

    s_ScrFillRect(left, top, 1, bottom - top + 1, rgb, COLOR_ROLE, FORE_LAYER);
    s_ScrFillRect(right, top, 1, bottom - top + 1, rgb, COLOR_ROLE, FORE_LAYER);
    s_ScrFillRect(left, top, right - left+ 1, 1, rgb, COLOR_ROLE, FORE_LAYER);
    s_ScrFillRect(left, bottom, right - left+ 1, 1,rgb, COLOR_ROLE, FORE_LAYER);
	return 0;
}

int CLcdSetBgColor(uint color)
{
	int bkColor;
	LCD_ATTR_T *ptLcdAttr;

	ptLcdAttr = GetLcdInfo(MONO_ROLE);
	ptLcdAttr->backcolor = color;
	
	ptLcdAttr =GetLcdInfo(COLOR_ROLE);
	bkColor = ptLcdAttr->backcolor;
	ptLcdAttr->backcolor = color;

	s_ScrFillRect(0, 0, ptLcdAttr->width, ptLcdAttr->height, color, COLOR_ROLE, BACK_LAYER);
	return bkColor;
}

int CLcdSetFgColor(uint color)
{
	int bkColor;
	LCD_ATTR_T *ptLcdAttr;

	ptLcdAttr = GetLcdInfo(MONO_ROLE);
	ptLcdAttr->forecolor = color;
	
	ptLcdAttr = GetLcdInfo(COLOR_ROLE);
	bkColor = ptLcdAttr->forecolor;
	ptLcdAttr->forecolor = color;

	return bkColor;
}

int CLcdWriteBitmap(uint left, uint top, uint width, uint height, ushort bitsPerPixel, uchar *bitmap, uint layer)
{
	if (left < 0 || top < 0) return -1;
	while(LcdCheckBusy())
	{
		OsSleep(1);
	}
	s_ScrWriteBitmap(left, top, width, height, bitmap, bitsPerPixel, COLOR_ROLE, 0, layer);
	return 0;
}

int CLcdReadBitmap(uint left, uint top, uint width, uint height, ushort bitsPerPixel, uchar *bitmap, uint layer)
{
    LCD_ATTR_T *ptLcdAttr =GetLcdInfo(COLOR_ROLE);

	if (left + width > ptLcdAttr->width) return -1;
	if (top + height > ptLcdAttr->height) return -1;

	s_ScrReadBitmap(left, top, width, height, bitmap, bitsPerPixel, 0, layer);
	return 0;
}

int CLcdDrawPixel(uint x, uint y, uint color)
{
    LCD_ATTR_T *ptLcdAttr =GetLcdInfo(COLOR_ROLE);
	
    if(!isValidPixel(x, y)) return -1;

	s_LcdSetDot(gstAppScrOrg.x + x, gstAppScrOrg.y + y, color);
	return 0;
}

int CLcdGetPixel(uint x, uint y, uint *color)
{
    LCD_ATTR_T *ptLcdAttr =GetLcdInfo(COLOR_ROLE);
	if(color == NULL) return -1;
    if(!isValidPixel(x, y)) return -1;

	*color = s_LcdGetDot(gstAppScrOrg.x + x, gstAppScrOrg.y + y);
	return 0;
}

/***************************************************************************************
����ԭ��	void LcdTextOut(int x, int y, uchar *fmt,...)
��������	����Ļָ��λ�ø�ʽ����ʾ�ַ���
����˵��	���	x -- ��ʼ��x����
					y -- ��ʼ��y����
					fmt -- ��ʾ������ַ�������ʽ
			����	��
����ֵ		��
����˵��	
***************************************************************************************/
int s_CLcdTextOut(uint x, uint y, char *str)
{
	if (str == NULL) return -1;

    s_ScrTextOut(x, y, 0, 0, str, COLOR_ROLE);
	return 0;
}

int CLcdTextOut(uint x, uint y, char *fmt,...)
{
    int i;
    char sbuffer[1028];
    va_list varg;

	if(fmt == NULL) return -1;

    va_start( varg, fmt );
    i = vsnprintf(sbuffer, sizeof(sbuffer)-4,  fmt,  varg);
    va_end( varg );
    if(i == -1) i = sizeof(sbuffer) - 4;
	sbuffer[i]=0;
    
	if (sbuffer == NULL) return -1;

    s_ScrTextOut(x, y, 0, 0, sbuffer, COLOR_ROLE);
	return 0;
}

/***************************************************************************************
����ԭ��	void LcdPrint(int offset,int line,int mode,uchar *fmt,...)
��������	����Ļָ���и�ʽ����ʾ�ַ���
����˵��	���	offset -- ��ʾ��ʼ�к�(�ַ����)
                    line -- ��ʾ��ʼ�к�(�ַ��߶�)
					mode -- ����룬���С�1--���ж���
					fmt -- ��ʾ������ַ�������ʽ
			����	��
����ֵ		��
����˵��	��������ѡ���������СΪ׼��������Ϊ8X16,��һ��Ϊ16������
***************************************************************************************/
int CLcdPrint(uint offset, uint line, uint mode, char *fmt,...)
{
    int LineHeight,CharWidth;
    int i;
    char sbuffer[1028];
    va_list varg;

	if(fmt == NULL) return -1;
	if(mode > 2) return -1;

    va_start( varg, fmt );
    i = vsnprintf(sbuffer, sizeof(sbuffer)-4,  fmt,  varg);
    va_end( varg );
    if(i == -1) i = sizeof(sbuffer) - 4;
	sbuffer[i]=0;
	
    s_GetLcdFontCharAttr(&CharWidth, &LineHeight, NULL, NULL);
    s_ScrTextOut(offset*CharWidth, line*LineHeight, mode, 0, sbuffer, COLOR_ROLE);
	return 0;
}

int s_CLcdPrint(uint offset, uint line, uint mode, char *str)
{
    int LineHeight,CharWidth;

	if(str == NULL) return -1;
	if(mode > 2) return -1;

    s_GetLcdFontCharAttr(&CharWidth, &LineHeight, NULL, NULL);
    s_ScrTextOut(offset*CharWidth, line*LineHeight, mode, 0, str, COLOR_ROLE);
	return 0;
}

/***************************************************************************************
����ԭ��	int LcdGetInfo(ST_ LCDINFO *stLcdInfo)
��������	��ȡ������Ϣ
����˵��	���	��
			����	stLcdInfo -- ������Ϣ����

����ֵ		0 	��ȡ�ɹ�
		   -1  	���������Ч 
����˵��	
***************************************************************************************/
int CLcdGetInfo(ST_LCD_INFO *stLcdInfo)
{
	LCD_ATTR_T *ptLcdAttr =GetLcdInfo(COLOR_ROLE);
    int LineHeight, CharWidth;

	if(stLcdInfo == NULL) return -1;

    s_GetLcdFontCharAttr(&CharWidth, &LineHeight, NULL, NULL);

	memset(stLcdInfo, 0, sizeof(ST_LCD_INFO));
    stLcdInfo->width = ptLcdAttr->width;
    stLcdInfo->height = ptLcdAttr->height;
    stLcdInfo->ppl = LineHeight;
    stLcdInfo->ppc = CharWidth;
	stLcdInfo->fgColor = ptLcdAttr->forecolor;
	stLcdInfo->bgColor = ptLcdAttr->backcolor;
	return 0;
}

/* end of lcdapi_c.c */
