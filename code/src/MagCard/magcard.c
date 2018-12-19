#include <string.h>
#include <stdarg.h>

#include "base.h"
#include "posapi.h"
#include "magcard.h"


static MSR_BitInfo             k_MagBitBuf[3]; //  ����Ӵſ���ȡ�Ļ���
static MSR_TrackInfo           k_TrackData[6]; //  �����������������Ϣ,0,1��ʾһ�ŵ�����,����;2,3��ʾ���ŵ�����,����;4,5��ʾ���ŵ�����,����;
static uchar                   k_flag_swipe;   //  ���廮����־
static int                     k_MagOpenFlag;
uchar  Mag_Parity(uchar cpd);
int    Mag_Encode(void);

#define MAG_NONE          		  	0x00
#define MAG_EN_IDTECH               0x01
#define MAG_MCR 				    0x02

uchar GetMsrType()
{
    char context[64];
    static uchar type =MAG_NONE;

    if(type) return type;
    memset(context,0,sizeof(context));
    if(ReadCfgInfo("MAG_CARD",context)<=0) type = MAG_EN_IDTECH;
    else if(strstr(context, "NULL")) type=MAG_NONE;
    else if(strstr(context, "MH1601-V01")) type=MAG_EN_IDTECH;        
    else type=MAG_EN_IDTECH;
    
    return type;
}
// �ſ���ʼ��
void s_MagInit(void)
{
    if(GetMsrType()==MAG_NONE) return;
	encryptmag_init();
    k_flag_swipe  = 0;
    k_MagOpenFlag=0;
}
//
void s_SetMagBitBuf(MSR_BitInfo msr_bt[3])
{
	k_MagBitBuf[0]=msr_bt[0];
	k_MagBitBuf[1]=msr_bt[1];
	k_MagBitBuf[2]=msr_bt[2];
	k_flag_swipe = 1;
}

// �ж�����У�黹��żУ��
// ����0-żУ��,1-��У��
unsigned char Mag_Parity(unsigned char cpd)
{
    unsigned char i=0,j=1,k;

    for(k=0; k<8; k++)
    {
        if((cpd & j) != 0) i++;
        j <<= 1;
     }
     //return(i % 2);
     return(i & 0x01);
}

/**********************************
*   ���ŵ�����
*   direction   ->  �����־��0=����ȡֵ��1=����
*   CodeSize    ->  �ôŵ��ı���λ��,1�ŵ�=7, 2/3�ŵ�=5
*   StartCode   ->  ��ʼ��,һ�ŵ�=0x45, �����ŵ�=0x0B;
*   StopCode    ->  ������,һ�ŵ�=0x1F�������ŵ�=0x1F;
*   BitBuf      ->  ����Դ��Ϣ
*   TrackData   ->  ��ų������붨λ����Ϣ
**********************************/
void  Mag_EncodeTrack(uchar direction,uint CodeSize,uchar StartCode, uchar StopCode,MSR_BitInfo *BitBuf,MSR_TrackInfo *TrackData)
{
    int     i, b, j, step/*, FirstOddErr*/;
    uchar   data,templrc,cal_lrc,zeroflag;

    //  init dest buf
    TrackData->error  = 0;
    TrackData->len    = 0;
    // Modified by yangrb 2007-11-12 
    TrackData->odd    = 1;
    TrackData->lrc    = 0;
    TrackData->start  = 0;
    TrackData->stop   = 0;
    memset(TrackData->buf, 0, sizeof(TrackData->buf));
    //  check data empty
    if(BitBuf->BitOneNum <= 8)  return;

    if(direction == 0)  //  ����
    {
        b = 0;
        step = 1;
    }
    else                
    {//  ����
        b = sizeof(BitBuf->buf) - 1;
        step = -1;
    }
    //  Ѱ����ʼ�ַ�
    data    = 0;
    cal_lrc = 0;
    TrackData->error  = 1;
	
    while(1)
    {
        data >>= 1;
        data  |= (BitBuf->buf[b]) << (CodeSize - 1);
        b     += step;
        if(data == StartCode)   //  �ҵ���ʼ��
        {
            TrackData->start  = 1;
            TrackData->buf[0] = data;
            cal_lrc           = data;
            break;
        }
        if(((direction==0)&&(b>=sizeof(BitBuf->buf))) || ((direction!=0)&&(b<=0)))  return;
    }
    
	//  ��ʼ����
	// Modified by yangrb 2007-11-12
    //FirstOddErr = 0;
    j = 1;

    while(1)
    {
        if(((direction==0)&&(b>sizeof(BitBuf->buf)-CodeSize)) || ((direction!=0)&&(b<CodeSize)))    break;
        data = 0;
        for(i=0; i<CodeSize; i++)
        {
            data |= (BitBuf->buf[b]) << i;
            b    += step;
        }
        TrackData->buf[j++] = data;
        if(data == 0)
        {
            // Modified by yangrb 2007-11-12
            //if(TrackData->OddErr >= 1)
                break;
        }
        cal_lrc ^= data;
        if(Mag_Parity(data) == 0)
        {
            // Modified by yangrb 2007-11-12
            /*(TrackData->OddErr)++;
            if(TrackData->OddErr == 1)    FirstOddErr = j-1;*/
            TrackData->odd = 0;
        }
        if(data == StopCode)        //  �ҵ�������
        {
            TrackData->stop = 1;
            templrc = 0;
            for(i=0; i<CodeSize; i++)
            {
                templrc |= (BitBuf->buf[b]) << i;
                b       += step;
            }
            TrackData->buf[j++] = templrc;
            if(Mag_Parity(templrc) == 0)
            {
                // Modified by yangrb 2007-11-12
                /*(TrackData->OddErr)++;
                if(TrackData->OddErr == 1)    FirstOddErr = j-1;*/
                TrackData->odd = 0;
            }
#ifdef MAGCARD_DEBUG
            ScrCls();
            ScrPrint(0,1,0,"templrc:%x",templrc);
            ScrPrint(0,2,0,"cal_lrc:%x",cal_lrc);
            getkey();
#endif
            templrc = (cal_lrc ^ templrc) & (~(0xff<<(CodeSize-1)));  //  %(1<<(CodeSize-1));
#ifdef MAGCARD_DEBUG
            ScrCls();
            ScrPrint(0,1,0,"templrc2:%x",templrc);
            getkey();
#endif
            if(templrc == 0)   TrackData->lrc = 1;      //  У����ȷ
            break;
        }
        (TrackData->len)++;
    }

    zeroflag = 0;
    for(i=0; i<CodeSize; i++)
    {
        zeroflag |= (BitBuf->buf[b]) << i;
        b        += step;
    }
    
    // Modified by yangrb 2007-11-12
    //if((TrackData->OddErr==0) && (TrackData->stop==1) && (TrackData->lrc==1) && (!zeroflag))
    if((TrackData->odd==1) && (TrackData->stop==1) && (TrackData->lrc==1) && (!zeroflag))
    {
        TrackData->error = 0;
        return;
    }

    /*if((TrackData->OddErr==1) && (TrackData->stop==1))
    {
#ifdef  MAGCARD_DEBUG
        ScrCls();
        ScrPrint(0,1,0,"FirstOddErr:%d",FirstOddErr);
        ScrPrint(0,2,0,"Err Data:%x",TrackData->buf[FirstOddErr]);
        getkey();
#endif
        zeroflag = 0;
        for(i=0; i<CodeSize; i++)
        {
            zeroflag |= (BitBuf->buf[b]) << i;
            b        += step;
        }
        if(zeroflag)    return;
#ifdef MAGCARD_DEBUG
            ScrCls();
            ScrPrint(0,1,0,"templrc:%x",templrc);
            getkey();
#endif
        data = 1;
        for(i=0; i<CodeSize; i++)
        {
            if(templrc == data)
            {
#ifdef MAGCARD_DEBUG
                ScrCls();
                ScrPrint(0,1,0,"Before:%x",TrackData->buf[FirstOddErr]);
                getkey();
#endif
                TrackData->buf[FirstOddErr] ^= (1<<i);
#ifdef MAGCARD_DEBUG
                ScrCls();
                ScrPrint(0,1,0,"After:%x",TrackData->buf[FirstOddErr]);
                getkey();
#endif
                TrackData->error = 0;
                TrackData->lrc   = 1;
                break;
            }
            data <<= 1;
            data  &= ~(0xff<<(CodeSize-1));
#ifdef MAGCARD_DEBUG
            ScrCls();
            ScrPrint(0,1,0,"i:%d",i);
            ScrPrint(0,2,0,"data:%x",data);
            getkey();
#endif        
        }
    }*/
}

//  ����������Ŀ����ԣ�����ֵԽ����÷��������Խ��
int AnalyseDirection(int direction)
{
    int i,j,mark;

    mark = 0;

    for(i=0; i<3; i++)
    {
        j = i * 2 + direction;
        if(k_TrackData[j].start)        mark += 10;
        if(k_TrackData[j].stop)         mark += 10;
        if(k_TrackData[j].lrc)          mark += 10;
        // added by yangrb 2007-11-12 
        if(k_TrackData[j].odd)          mark += 10;
        if(k_TrackData[j].error == 0)    //  û�г��ִ���
        {
            mark += 100 * k_TrackData[j].len;
        }
        // Modified by yangrb 2007-11-13
        /*if(k_TrackData[j].len)
        {
            // Modified by yangrb 2007-11-12
            //mark += 100 * (k_TrackData[j].len-k_TrackData[j].OddErr) / k_TrackData[j].len;
            mark += 100;
        }*/
    }
    return(mark);
}

//  ����0=���򣬷���1=����
int  Mag_Encode(void)
{
    int     fwdmark,revmark;
    int     i,j;

    Mag_EncodeTrack(0, 7, 0x45, 0x1F, &(k_MagBitBuf[0]), &(k_TrackData[0]));
    Mag_EncodeTrack(1, 7, 0x45, 0x1F, &(k_MagBitBuf[0]), &(k_TrackData[1]));
    Mag_EncodeTrack(0, 5, 0x0B, 0x1F, &(k_MagBitBuf[1]), &(k_TrackData[2]));
    Mag_EncodeTrack(1, 5, 0x0B, 0x1F, &(k_MagBitBuf[1]), &(k_TrackData[3]));
    Mag_EncodeTrack(0, 5, 0x0B, 0x1F, &(k_MagBitBuf[2]), &(k_TrackData[4]));
    Mag_EncodeTrack(1, 5, 0x0B, 0x1F, &(k_MagBitBuf[2]), &(k_TrackData[5]));

    fwdmark = AnalyseDirection(0);
    revmark = AnalyseDirection(1);

#ifdef MAGCARD_DEBUG       
    {
		s_UartPrint("\r\n\n==============Magtek Flag===================\r\n");        
        for (i = 0; i < 16; i++)
        {
            s_UartPrint("%d ",g_ucMagtekFlag[16-1-i]);
        }
        for(i=0; i<3; i++)
        {
            s_UartPrint("\r\n\n==============trackbit %d===================\r\n",i);
            for(j=0; j<sizeof(k_MagBitBuf[i].buf);j++)
            {
                s_UartPrint("%d ",k_MagBitBuf[i].buf[j]);
            }
        }
        for(i=0; i<6; i++)
        {
            s_UartPrint("\r\n=================trackdata %d=================\r\n",i);
            s_UartPrint("start = %u \r\n",k_TrackData[i].start);
            s_UartPrint("stop  = %u \r\n",k_TrackData[i].stop);
            s_UartPrint("lrc   = %u \r\n",k_TrackData[i].lrc);
            s_UartPrint("error = %d \r\n",k_TrackData[i].error);
            s_UartPrint("len   = %d \r\n",k_TrackData[i].len);
            s_UartPrint("odderr= %d \r\n",k_TrackData[i].odd);
            for(j=0; j<sizeof(k_TrackData[i].buf);j++)
            {
                s_UartPrint("%02X ",k_TrackData[i].buf[j]);
            }
        }
        s_UartPrint("\r\nfwdmark=%d, revmark=%d\r\n",fwdmark,revmark);
    }
#endif



    if(fwdmark >= revmark)   //  ��������Դ�
        return(0);
    else
        return(1);
}

//  ����0=���򣬷���1=����
int  Mag_ibm_Encode(void)
{
    int     fwdmark,revmark;
    int     i,j;

    Mag_EncodeTrack(0, 7, 0x45, 0x1C, &(k_MagBitBuf[0]), &(k_TrackData[0]));
    Mag_EncodeTrack(1, 7, 0x45, 0x1C, &(k_MagBitBuf[0]), &(k_TrackData[1]));
    Mag_EncodeTrack(0, 5, 0x0B, 0x1C, &(k_MagBitBuf[1]), &(k_TrackData[2]));
    Mag_EncodeTrack(1, 5, 0x0B, 0x1C, &(k_MagBitBuf[1]), &(k_TrackData[3]));
    Mag_EncodeTrack(0, 5, 0x0B, 0x1C, &(k_MagBitBuf[2]), &(k_TrackData[4]));
    Mag_EncodeTrack(1, 5, 0x0B, 0x1C, &(k_MagBitBuf[2]), &(k_TrackData[5]));

    fwdmark = AnalyseDirection(0);
    revmark = AnalyseDirection(1);

#ifdef MAGCARD_DEBUG       
    {
		s_UartPrint("\r\n\n==============Magtek Flag===================\r\n");        
        for (i = 0; i < 16; i++)
        {
            s_UartPrint("%d ",g_ucMagtekFlag[16-1-i]);
        }
        for(i=0; i<3; i++)
        {
            s_UartPrint("\r\n\n==============trackbit %d===================\r\n",i);
            for(j=0; j<sizeof(k_MagBitBuf[i].buf);j++)
            {
                s_UartPrint("%d ",k_MagBitBuf[i].buf[j]);
            }
        }
        for(i=0; i<6; i++)
        {
            s_UartPrint("\r\n=================trackdata %d=================\r\n",i);
            s_UartPrint("start = %u \r\n",k_TrackData[i].start);
            s_UartPrint("stop  = %u \r\n",k_TrackData[i].stop);
            s_UartPrint("lrc   = %u \r\n",k_TrackData[i].lrc);
            s_UartPrint("error = %d \r\n",k_TrackData[i].error);
            s_UartPrint("len   = %d \r\n",k_TrackData[i].len);
            s_UartPrint("odderr= %d \r\n",k_TrackData[i].odd);
            for(j=0; j<sizeof(k_TrackData[i].buf);j++)
            {
                s_UartPrint("%02X ",k_TrackData[i].buf[j]);
            }
        }
        s_UartPrint("\r\nfwdmark=%d, revmark=%d\r\n",fwdmark,revmark);
    }
#endif



    if(fwdmark >= revmark)   //  ��������Դ�
        return(0);
    else
        return(1);
}

uchar Mag_ibm_Read(uchar *Track1, uchar *Track2, uchar *Track3)
{
    unsigned char UCret;
    int     direction;
    int     i,j;

    direction = Mag_ibm_Encode();
    errno     = 0;
    UCret     = 0;
    //  ������һ�ŵ�����
    if(Track1 != NULL)
    {
        if(k_TrackData[direction].error == 0)   //  ֻ��2�����: û������,������ȷ
        {
            if(strlen(k_TrackData[direction].buf))
                UCret |= 0x01;
        }
        else        //  ��У���,lrcУ���,û����ʼ��,û�н�����,
        {
            UCret |= 0x10;
            if(k_TrackData[direction].lrc == 0)
            {
                errno |= 0x10;
            }
        }
        for(i=0; i<k_TrackData[direction].len; i++)
        {
            if(Mag_Parity(k_TrackData[direction].buf[i+1]) == 0)    //  ��У���
            {
                errno |= 0x01;
                break;
            }
            Track1[i] = ((k_TrackData[direction].buf[i+1]) & 0x3F) + 0x20;
        }
        Track1[i] = 0x00;

        if ((errno & 0x11) || (UCret & 0x10))
        {
            Track1[0] = 0;
        }
    }
    //  �����ڶ��ŵ�����
    if(Track2 != NULL)
    {
        if(k_TrackData[2+direction].error == 0)   //  ֻ��2�����: û������,������ȷ
        {
            if(strlen(k_TrackData[2+direction].buf))
                UCret |= 0x02;
        }
        else        //  ��У���,lrcУ���,û����ʼ��,û�н�����,
        {
            UCret |= 0x20;
            if(k_TrackData[2+direction].lrc == 0)
            {
                errno |= 0x20;
            }
        }

        for(i=0; i<k_TrackData[2+direction].len; i++)
        {
            if(Mag_Parity(k_TrackData[2+direction].buf[i+1]) == 0)    //  ��У���
            {
                errno |= 0x02;
                break;
            }
            Track2[i]   = ((k_TrackData[2+direction].buf[i+1]) & 0x0F) + 0x30;
        }
        Track2[i] = 0x00;

        if ((errno & 0x22) || (UCret & 0x20))
        {
            Track2[0] = 0;
        }
    }
    //  ���������ŵ�����
    if(Track3 != NULL)
    {
        if(k_TrackData[4+direction].error == 0)   //  ֻ��2�����: û������,������ȷ
        {
            if(strlen(k_TrackData[4+direction].buf))
                UCret |= 0x04;
        }
        else        //  ��У���,lrcУ���,û����ʼ��,û�н�����,
        {
            UCret |= 0x40;
            if(k_TrackData[4+direction].lrc == 0)
            {
                errno |= 0x40;
            }
        }
        for(i=0; i<k_TrackData[4+direction].len; i++)
        {
            if(Mag_Parity(k_TrackData[4+direction].buf[i+1]) == 0)    //  ��У���
            {
                errno |= 0x04;
                break;
            }
            Track3[i]   = ((k_TrackData[4+direction].buf[i+1]) & 0x0F) + 0x30;
        }
        Track3[i] = 0x00;

        if ((errno & 0x44) || (UCret & 0x40))
        {
            Track3[0] = 0;
        }
    }

    return UCret;
}
void  Mag_JcbEncodeTrack(uchar direction,uint CodeSize,uchar StartCode, uchar StopCode,MSR_BitInfo *BitBuf,MSR_TrackInfo *TrackData)
{
    int     i, b, j, step;
    uchar   data,templrc,cal_lrc,zeroflag;

    //  init dest buf
    TrackData->error  = 0;
    TrackData->len    = 0;
    TrackData->odd    = 1;
    TrackData->lrc    = 0;
    TrackData->start  = 0;
    TrackData->stop   = 0;
    memset(TrackData->buf, 0, sizeof(TrackData->buf));
    if(BitBuf->BitOneNum <= 8)  return;     //  check data empty

    if(direction == 0)  //  ����
    {
        b = 0;
        step = 1;
    }
    else                
    {//  ����
        b = sizeof(BitBuf->buf) - 1;
        step = -1;
    }
    //  Ѱ����ʼ�ַ�
    data    = 0;
    cal_lrc = 0;
    TrackData->error  = 1;
	
    while(1)
    {
        data >>= 1;
        data  |= (BitBuf->buf[b]) << (CodeSize - 1);
        b     += step;
        if(data == StartCode)   //  �ҵ���ʼ��
        {
            TrackData->start  = 1;
            TrackData->buf[0] = data;
            break;
        }
        if(((direction==0)&&(b>=sizeof(BitBuf->buf))) || ((direction!=0)&&(b<=0)))  return;
    }
    
	//  ��ʼ����
    j = 1;
    while(1)
    {
        if(((direction==0)&&(b>sizeof(BitBuf->buf)-CodeSize)) || ((direction!=0)&&(b<CodeSize)))    break;
        data = 0;
        for(i=0; i<CodeSize; i++)
        {
            data |= (BitBuf->buf[b]) << i;
            b    += step;
        }
        TrackData->buf[j++] = data;
        if(data == 0) break;
        cal_lrc ^= data;
        if(Mag_Parity(data) == 1) TrackData->odd = 0;
        
        if(data == StopCode)        //  �ҵ�������
        {
            TrackData->stop = 1;
            templrc = 0;
            for(i=0; i<CodeSize; i++)
            {
                templrc |= (BitBuf->buf[b]) << i;
                b       += step;
            }
            TrackData->buf[j++] = templrc;
            if(Mag_Parity(templrc) == 1) TrackData->odd = 0;
            templrc = (cal_lrc ^ templrc) & (~(0xff<<(CodeSize-1)));  //  %(1<<(CodeSize-1));
            if(templrc == 0)   TrackData->lrc = 1;      //  У����ȷ
            break;
        }
        (TrackData->len)++;
    }

    zeroflag = 0;
    for(i=0; i<CodeSize; i++)
    {
        zeroflag |= (BitBuf->buf[b]) << i;
        b        += step;
    }
    
    if((TrackData->odd==1) && (TrackData->stop==1) && (TrackData->lrc==1) && (!zeroflag))
    {
        TrackData->error = 0;
    }
}

//  ����0=���򣬷���1=����
int  Mag_Jcb_Encode(void)
{
    int     fwdmark,revmark;

    Mag_JcbEncodeTrack(0, 8, 0xff, 0xff, &(k_MagBitBuf[0]), &(k_TrackData[0]));
    Mag_JcbEncodeTrack(1, 8, 0xff, 0xff, &(k_MagBitBuf[0]), &(k_TrackData[1]));

    fwdmark = AnalyseDirection(0);
    revmark = AnalyseDirection(1);

    if(fwdmark >= revmark) return(0);//  ��������Դ�
    else        return(1);
}
uchar Mag_Jcb_Read(uchar *Track1, uchar *Track2, uchar *Track3)
{
    uchar UCret;
    int direction, i;

    direction = Mag_Jcb_Encode();
    errno     = 0;
    UCret     = 0;
    //  ������һ�ŵ�����
    if(Track1 != NULL)
    {
        if(k_TrackData[direction].error == 0)   //  ֻ��2�����: û������,������ȷ
        {
            if(strlen(k_TrackData[direction].buf))
                UCret |= 0x01;
        }
        else        //  ��У���,lrcУ���,û����ʼ��,û�н�����,
        {
            UCret |= 0x10;
            if(k_TrackData[direction].lrc == 0) errno |= 0x10;
        }
        for(i=0; i<k_TrackData[direction].len; i++)
        {
            if(Mag_Parity(k_TrackData[direction].buf[i+1]) == 1)    //  ��У���
            {
                errno |= 0x01;
                break;
            }
            Track1[i] = ((k_TrackData[direction].buf[i+1]) & 0x7F);
        }
        Track1[i] = 0x00;

        if ((errno & 0x11) || (UCret & 0x10)) Track1[0] = 0;
    }

    return UCret;
}

uchar MagRead(uchar *Track1, uchar *Track2, uchar *Track3)
{
    unsigned char UCret;
    int     direction;
    int     i,j;

    uint    uiTemp = 0;
    uchar   ucTemp = 0;

    if(GetMsrType()==MAG_NONE) return 0;
    if(0==k_MagOpenFlag)
    {
        k_flag_swipe = 0;
        MagReset();
        return 0;
    }

	encryptmag_read_rawdata();
    direction = Mag_Encode();
    errno     = 0;
    UCret     = 0;
    //  ������һ�ŵ�����
    if(Track1 != NULL)
    {
        if(k_TrackData[direction].error == 0)   //  ֻ��2�����: û������,������ȷ
        {
            if(strlen(k_TrackData[direction].buf))
                UCret |= 0x01;
        }
        else        //  ��У���,lrcУ���,û����ʼ��,û�н�����,
        {
            UCret |= 0x10;
            if(k_TrackData[direction].lrc == 0)
            {
                errno |= 0x10;
            }
        }
        for(i=0; i<k_TrackData[direction].len; i++)
        {
            if(Mag_Parity(k_TrackData[direction].buf[i+1]) == 0)    //  ��У���
            {
                errno |= 0x01;
                break;
            }
            Track1[i] = ((k_TrackData[direction].buf[i+1]) & 0x3F) + 0x20;
        }
        Track1[i] = 0x00;

        if ((errno & 0x11) || (UCret & 0x10))
        {
            Track1[0] = 0;
        }
    }
    //  �����ڶ��ŵ�����
    if(Track2 != NULL)
    {
        if(k_TrackData[2+direction].error == 0)   //  ֻ��2�����: û������,������ȷ
        {
            if(strlen(k_TrackData[2+direction].buf))
                UCret |= 0x02;
        }
        else        //  ��У���,lrcУ���,û����ʼ��,û�н�����,
        {
            UCret |= 0x20;
            if(k_TrackData[2+direction].lrc == 0)
            {
                errno |= 0x20;
            }
        }

        for(i=0; i<k_TrackData[2+direction].len; i++)
        {
            if(Mag_Parity(k_TrackData[2+direction].buf[i+1]) == 0)    //  ��У���
            {
                errno |= 0x02;
                break;
            }
            Track2[i]   = ((k_TrackData[2+direction].buf[i+1]) & 0x0F) + 0x30;
        }
        Track2[i] = 0x00;

        if ((errno & 0x22) || (UCret & 0x20))
        {
            Track2[0] = 0;
        }
    }
    //  ���������ŵ�����
    if(Track3 != NULL)
    {
        if(k_TrackData[4+direction].error == 0)   //  ֻ��2�����: û������,������ȷ
        {
            if(strlen(k_TrackData[4+direction].buf))
                UCret |= 0x04;
        }
        else        //  ��У���,lrcУ���,û����ʼ��,û�н�����,
        {
            UCret |= 0x40;
            if(k_TrackData[4+direction].lrc == 0)
            {
                errno |= 0x40;
            }
        }
        for(i=0; i<k_TrackData[4+direction].len; i++)
        {
            if(Mag_Parity(k_TrackData[4+direction].buf[i+1]) == 0)    //  ��У���
            {
                errno |= 0x04;
                break;
            }
            Track3[i]   = ((k_TrackData[4+direction].buf[i+1]) & 0x0F) + 0x30;
        }
        Track3[i] = 0x00;

        if ((errno & 0x44) || (UCret & 0x40))
        {
            Track3[0] = 0;
        }
    }

    if (!(UCret & 0x07))
    {
        uiTemp = errno;
        // ����IBM��ʽ����
        ucTemp = Mag_ibm_Read(Track1,Track2,Track3);
        // ���������ȷ,����IBM��ʽ�ſ�����
        if (ucTemp & 0x07)
        {
            UCret = ucTemp;
        }
        else
        {
            errno = uiTemp;
        }
    }
    if (!(UCret & 0x07))
    {
        uiTemp = errno;
        ucTemp = Mag_Jcb_Read(Track1,Track2,Track3);// ����JCB��ʽ����
        // ���������ȷ,����JCB��ʽ�ſ�����
        if (ucTemp & 0x07) UCret = ucTemp;
        else errno = uiTemp;
    }    
    k_flag_swipe = 0;
    MagReset();
    return UCret;
}

uchar MagSwiped(void)
{
    if(GetMsrType()==MAG_NONE) return 0xff;

    encryptmag_swiped();
    if(0==k_MagOpenFlag) return 0xff;
    if (k_flag_swipe == 1) return 0x00;
    return 0xff;
}

void MagReset(void)
{
    int     i=0;
    if(GetMsrType()==MAG_NONE) return;    
	encryptmag_reset();
    for(i=0;i<3;i++)
    {
        k_MagBitBuf[i].BitOneNum = 0;
        memset(k_MagBitBuf[i].buf, 0, sizeof(k_MagBitBuf[i].buf));
    }
    k_flag_swipe = 0;
}

/****************************************************
 * ���ܣ��򿪴ſ��Ķ���
 *       ���ſ����ݲ����жϷ�ʽ������һ���򿪴ſ���
 *       ��������ʹ�����ö�ȡ������ֻҪˢ������ͷһ
 *       ���ܶ���ſ����ݣ�����ڲ���Ҫʹ�ôſ��Ķ�
 *       ��ʱ����ý��ſ��Ķ����رա�
 ******************************************************/
void MagOpen(void)
{
    if(GetMsrType()==MAG_NONE) return;

    encryptmag_open();    
	MagReset();
	k_MagOpenFlag = 1;
}

/************************************************
 * ˵���� �رմſ��Ķ��������ָ����ж�Ϊ��ǰ״̬
 ************************************************/
void MagClose(void)
{
    if(GetMsrType()==MAG_NONE) return;

	MagReset();
	k_MagOpenFlag = 0;
}

//============================== test code ====================================


#ifdef TEST_MD_DETAIL
void TestMagCard(void)
{
	uchar   ucRet;
	uchar   Track1[256],Track2[256],Track3[256];
	int     i,MagError;
    uint    event;

    k_MagTestMode = ON;  
	MagOpen();

	while(1)
	{
		ScrCls();
		ScrPrint(0,0,0x01,"  MagCard Test");
		ScrPrint(0,2,0x01,"PLS SWIPE<<<");
		
		while(2)
		{
			if(!MagSwiped()) break;
			if(!kbhit()&&getkey()==KEYCANCEL)
			{
				MagClose();
                k_MagTestMode = OFF;
                return;
			}
		}// 2
		
		ucRet = MagRead((unsigned char*)Track1,(unsigned char*)Track2,(unsigned char*)Track3);
        MagError = GetLastError();

        ScrPrint(0,5,0x00,"ret=%02xH, Err=%02xH",ucRet, MagError);

		Beep();
		switch(ucRet)
		{
		case 0x01:	ScrPrint(0,6,0x01,"Read Track 1");		break;
		case 0x02:	ScrPrint(0,6,0x01,"Read Track 2");		break;
		case 0x04:	ScrPrint(0,6,0x01,"Read Track 3");		break;
		case 0x03:	ScrPrint(0,6,0x01,"Read Track 1,2"); 	break;
		case 0x05:	ScrPrint(0,6,0x01,"Read Track 1,3"); 	break;
		case 0x06:	ScrPrint(0,6,0x01,"Read Track 2,3"); 	break;
		case 0x07:	ScrPrint(0,6,0x01,"Read Track 1,2,3"); 	break;
		default: 	ScrPrint(0,6,0x01,"Read Error!");
					Beep();  MagReset(); break;
		}
        if(getkey() == KEYCANCEL) continue;

		Beep(); //WaitEsc(0,2);
		if(1)
        {
			ScrClrLine(0,7);
			ScrGotoxy(0,0);
			ScrFontSet(0);
            if(ucRet & 0x01)        Lcdprintf("Track1:OK   Len=%d\n",strlen(Track1));
            else if(ucRet & 0x10)   Lcdprintf("Track1:ERR  Len=%d\n",strlen(Track1));
            else                    Lcdprintf("Track1:NULL Len=%d\n",strlen(Track1));
			Lcdprintf("%s", Track1);
            if(getkey() == KEYCANCEL) continue;
            //kbflush_getkey();
		}
		if(2)       //(ucRet & 0x02)     //��ʾ2������
        {
            ScrClrLine(0,7);
			ScrGotoxy(0,0);
			ScrFontSet(0);
            if(ucRet & 0x02)        Lcdprintf("Track2:OK   Len=%d\n",strlen(Track2));
            else if(ucRet & 0x20)   Lcdprintf("Track2:ERR  Len=%d\n",strlen(Track2));
            else                    Lcdprintf("Track2:NULL Len=%d\n",strlen(Track2));
			Lcdprintf("%s",Track2);
            if(getkey() == KEYCANCEL) continue;
            //			kbflush_getkey();
		}
		if(3)       //(ucRet & 0x04) //��ʾ3������
        {
			ScrClrLine(0,7);
			ScrGotoxy(0,0);
			ScrFontSet(0);
            if(ucRet & 0x04)        Lcdprintf("Track3:OK   Len=%d\n",strlen(Track3));
            else if(ucRet & 0x40)   Lcdprintf("Track3:ERR  Len=%d\n",strlen(Track3));
            else                    Lcdprintf("Track3:NULL Len=%d\n",strlen(Track3));
			Lcdprintf("%s",Track3);
            if(getkey() == KEYCANCEL) continue;
            //			kbflush_getkey();
		}
		Beep();
	}// 1
	k_MagTestMode = OFF;
}
#endif

//================================ end ========================================

