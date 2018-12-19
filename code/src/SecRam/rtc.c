#include "base.h"
#include "bcm5892-bbl.h"

#define BBL_READ_INDREG          0x21
#define BBL_WRITE_INDREG         0x61

static void bbl_rtc_control(uint start) {
	uint val, addr, status;
	if (start)  val = BBL1_R_BBL_CONTROL_INIT & ~BBL1_F_bbl_rtc_stop_MASK;
	else    val = BBL1_R_BBL_CONTROL_INIT | BBL1_F_bbl_rtc_stop_MASK;

	addr = BBL0_R_BBL_ACCDATA_MEMADDR;
	writel(val, addr);

	val = (BBL1_R_BBL_CONTROL_SEL << BBL0_F_indaddr_R) | (BBL_WRITE_INDREG);
	addr = BBL0_R_BBL_CMDSTS_MEMADDR;
	writel(val, addr);
	while(readl(addr) & BBL0_F_rdyn_go_MASK );

}

static void bbl_set_rtc_div(uint rtc_div) 
{
	uint val, addr;
	val = rtc_div;
	addr = BBL0_R_BBL_ACCDATA_MEMADDR;
	writel(val, addr);
	val = (BBL1_R_BBL_RTC_DIV_SEL << BBL0_F_indaddr_R) | (BBL_WRITE_INDREG);
	addr = BBL0_R_BBL_CMDSTS_MEMADDR;
	writel(val, addr);
	while(readl(addr) & BBL0_F_rdyn_go_MASK );
}

static void bbl_set_rtc_secs(uint rtc_secs) 
{
	uint val, addr;

	val = rtc_secs;
	addr = BBL0_R_BBL_ACCDATA_MEMADDR;
	writel(val, addr);
	val = (BBL1_R_BBL_RTC_SECONDS_SEL << BBL0_F_indaddr_R) | (BBL_WRITE_INDREG);
	addr = BBL0_R_BBL_CMDSTS_MEMADDR;
	writel(val, addr);
	while(readl(addr) & BBL0_F_rdyn_go_MASK );
}


static void bbl_set_time(uint rtc_div, uint rtc_secs) 
{
    bbl_set_rtc_div(rtc_div);
    bbl_set_rtc_secs(rtc_secs);
}

uint bbl_get_rtc_div() 
{
	uint val, addr, status;

	val = (BBL1_R_BBL_RTC_DIV_SEL << BBL0_F_indaddr_R) | (BBL_READ_INDREG);
	addr = BBL0_R_BBL_CMDSTS_MEMADDR;
	writel(val, addr);
	while(readl(addr) & BBL0_F_rdyn_go_MASK );
	status = readl(BBL0_R_BBL_ACCDATA_MEMADDR);

	return status;
}
uint bbl_get_rtc_secs() 
{
	uint val, addr, status;

	val = (BBL1_R_BBL_RTC_SECONDS_SEL << BBL0_F_indaddr_R) | (BBL_READ_INDREG);
	addr = BBL0_R_BBL_CMDSTS_MEMADDR;
	writel(val, addr);
	while(readl(addr) & BBL0_F_rdyn_go_MASK );
	addr = BBL0_R_BBL_ACCDATA_MEMADDR;
	status = readl(BBL0_R_BBL_ACCDATA_MEMADDR);

    return status;
}


static void bbl_set_rtc(uint second)
{
	bbl_rtc_control(0);
	bbl_set_time(0,second);
	bbl_rtc_control(1);
//	while(1)printk("sec:%d,div:%d!\r\n",bbl_get_rtc_secs(),bbl_get_rtc_div());
}

/**************************************************************************
* ��������:   uchar   BcdToBin(uchar ucData)
* ��������:   BCD��ת2������
****************************************************************************/
static uchar BcdToBin(uchar ucData)
{
	uchar ucTemp1 = 0, ucTemp2 = 0;
	uchar ucRet = 0;

	ucTemp1 = ucData & 0x0F;
	ucTemp2 = (ucData >> 4) & 0x0F;

	ucRet = ucTemp2 * 10 + ucTemp1;

	return ucRet;
}

/**************************************************************************
* ��������: uchar   BinToBcd(uchar ucData)
* ��������: 2������תBCD��
****************************************************************************/
static uchar BinToBcd(uchar ucData)
{
	uchar ucTemp1 = 0, ucTemp2 = 0;
	uchar ucRet = 0;

	ucTemp1 = ucData % 10;
	ucTemp2 = ucData / 10;

	ucRet = (ucTemp2 << 4) + ucTemp1;

	return ucRet;
}

static unsigned char CheckBCD(uchar str)
{
	uchar ch;
	ch=str & 0x0f;
	if(ch>=0x0a) return 1;
	ch=str & 0xf0;
	if(ch>=0xa0) return 1;
	return 0;
}

static uchar CheckTimeFormat(uchar * str)
{
	long days;
	uchar ch,day,ucBuf[12];
	int i,j,k;
	uchar ret=0;
	if(CheckBCD(str[0])) return 1;  // ret |= 0x01;
	if(CheckBCD(str[1])) return 2;  //ret |= 0x02;
	if(CheckBCD(str[2])) return 3;  //ret |= 0x04;
	if(CheckBCD(str[3])) return 4;  //ret |= 0x08;
	if(CheckBCD(str[4])) return 5;  //ret |= 0x10;
	if(CheckBCD(str[5])) return 6;  //ret |= 0x20;

	if(str[1]==0 || str[1]>0x12) return 2; //ret |= 0x02;
	if(str[2]==0 || str[2]>0x31) return 3; //ret |= 0x04;
	if(str[3]>0x23) return 4;  //ret |= 0x08;
	if(str[4]>0x59) return 5;  //ret |= 0x10;
	if(str[5]>0x59) return 6;  //ret |= 0x20;

	i=(str[0]>>4)*10+(str[0] & 0x0f);
	if(i<50) i+=2000;
	else i+=1900;
	memcpy(ucBuf,"\x1f\x1c\x1f\x1e\x1f\x1e\x1f\x1f\x1e\x1f\x1e\x1f",12);
	if(i%4==0) ucBuf[1]=0x1d;
	k=(str[1]>>4)*10+(str[1] & 0x0f)-1;
	day=(str[2]>>4)*10+(str[2] & 0x0f);
	if(day>ucBuf[k]) return 3; //ret |= 0x04;
	return 0x00;
}

#undef uint 
#undef ushort
#include <time.h> 
/***************************************************************************
* ��������:    uchar SetTime(uchar *str)
* ��������:    ����RTCʱ��
* �������:
*              str              ʱ�Ӳ�����ȡֵ����:
str[0]=YY   year(00-99)		str[1]=MM  month(01-12)
str[2]=DD   day(01-31)        str[3]=hh   hour(00-23)
str[4]=mm  minute(00-59)   str[5]=ss    second(00-59)
* ����ֵ:
0 ���óɹ�	;	1 ��ݴ�
2 �·ݴ�		;	3 ���ڴ�
4 Сʱ��		;	5 ���Ӵ�
6 ������        ;	0xFF ����ʧ��
****************************************************************************/
uchar SetTime(uchar *str)
{
	uchar ucTemp1 = 0, ucTemp2 = 0,ucRet;
	uchar ucBuff[6];
	uint uiTemp = 0,seconds;
	uint i = 0;
	struct tm tm1;

	if(ucRet = CheckTimeFormat(str)) return ucRet;

	// ��ʽת��
	if (str[0] < 50)	ucBuff[0] = BcdToBin(str[0])+100;  //start from 1900 year
	else 				ucBuff[0] = BcdToBin(str[0]);  //start from 1900 year
	
	ucBuff[1] = BcdToBin(str[1])-1;
	ucBuff[2] = BcdToBin(str[2]);
	ucBuff[3] = BcdToBin(str[3]);
	ucBuff[4] = BcdToBin(str[4]);
	ucBuff[5] = BcdToBin(str[5]);

	// �жϲ�������Ч��
	if(ucBuff[0] > 150) return 1;//>99	// ��ݳ���
	if(ucBuff[1] > 0x0C) return 2; 		// �·ݳ���
	if(ucBuff[2] > 0x1F) return 3;		// ���ӳ���
	if(ucBuff[3] > 0x18) return 4;		// ����24Сʱ��
	if(ucBuff[4] > 0x3B) return 5;         	// �������Ϊ59
	if(ucBuff[5] > 0x3B) return 6;         	// �����Ϊ59

	tm1.tm_year = ucBuff[0];
	tm1.tm_mon =ucBuff[1];
	tm1.tm_mday =ucBuff[2];
	tm1.tm_hour=ucBuff[3];
	tm1.tm_min=ucBuff[4];
	tm1.tm_sec=ucBuff[5];

	seconds=mktime(&tm1); //seconds =0 is 1970-1-1 00:00:00
	bbl_set_rtc(seconds);

    	return 0x00;
}

/***************************************************************************
* ��������:	void GetTime(uchar *str)
* ��������:	��ȡRTCʱ��
* �������:
str     ��ȡ��ʱ�������ȡֵ����:
str[0]=YY   year(00-99)		str[1]=MM  month(01-12)
str[2]=DD   day(01-31)        str[3]=hh   hour(00-23)
str[4]=mm  minute(00-59)   str[5]=ss    second(00-59)
str[6]=wk       week(01-07)
****************************************************************************/
void GetTime(uchar *str)
{
	uchar ucBuff[7];
	uchar uiTemp1 = 0, uiTemp2 = 0;
	uint sec;
	int ret = 0;
	struct tm *tm1;
	
      // ��ȡʱ��
	sec=bbl_get_rtc_secs();
	tm1=gmtime((long*)&sec);

	if (tm1->tm_year >= 100)	ucBuff[0] = tm1->tm_year-100;  //start from 1900 year
	else 				ucBuff[0] = tm1->tm_year;  //start from 1900 year

	ucBuff[1]= tm1->tm_mon +1;
	ucBuff[2]= tm1->tm_mday;
	ucBuff[3]= tm1->tm_hour;
	ucBuff[4]= tm1->tm_min;
	ucBuff[5]= tm1->tm_sec;
	ucBuff[6]= tm1->tm_wday;
	
	if (!ucBuff[6]) ucBuff[6] = 7;
	
	str[0] = BinToBcd(ucBuff[0]);
	str[1] = BinToBcd(ucBuff[1]);
	str[2] = BinToBcd(ucBuff[2]);
	str[3] = BinToBcd(ucBuff[3]);
	str[4] = BinToBcd(ucBuff[4]);
	str[5] = BinToBcd(ucBuff[5]);
	str[6] = BinToBcd(ucBuff[6]);
}


/*Ҫ����������ʱ������*/
int RequireSetTimePassword(void)
{
    unsigned char correct_password[8];
    unsigned char password[8];
    unsigned char getstring_res;
    int ret;

    memset(correct_password,0,sizeof(correct_password));
    if(ret = SysParaRead(SET_SYSTEM_TIME_PASSWORD,correct_password))
    {
        memcpy(correct_password,"9876",4);/*use default password*/
#if 0
        ScrPrint(0,7,0,"Read,ret:%d",ret);
        getkey();
#endif
    }

    ScrRestore(0);
    while(1)
    {
        memset(password,0,sizeof(password));
        ScrCls();
        SCR_PRINT(0,0,0x81,"  ����ϵͳʱ��  ","SET SYSTEM TIME ");
        SCR_PRINT(0,3,1,"  ����������:","INPUT PASSWORD:");
        ScrGotoxy(0,5);
        getstring_res = GetString(password,0x69,4,6);
        if(getstring_res == 0xff)
        {
            ret = -1;
            break;
        }
        if(getstring_res == 0)
        {
            if(!strcmp(password+1,correct_password))
            {
                ret = 0;
                break;
            }
            else
            {
                ret = -1;
                ScrClrLine(2,7);
                SCR_PRINT(0,3,0x01,"   �������!","PASSWORD ERROR!");
                if(GetKeyMs(1200) == KEYCANCEL)
                    break;
            }
        }
    }

    ScrRestore(1);
    return ret;
}

/*��������ʱ������*/
void ChangeSetTimePassword(void)
{
    unsigned char password1[8];
    unsigned char password2[8];
    unsigned char getstring_res;

    while(1)
    {
        memset(password1,0,sizeof(password1));
        memset(password2,0,sizeof(password2));
        ScrCls();
        SCR_PRINT(0,0,0x81,"    �޸�����     ","CHANGE PASSWORD  ");
        ScrClrLine(2,3);

        SCR_PRINT(0,3,0x01,"  ������������:","INPUT NEW PASSWORD:");
        ScrGotoxy(64,5);
        getstring_res = GetString(password1,0x69,4,6);
        if(getstring_res == 0xff)/*�û�����cancel��ȡ��,������һ���˵�*/
            return ;
        else if(getstring_res)/*��ʱ����,Ҫ��������������*/
            continue;

        ScrClrLine(3,7);
        SCR_PRINT(0,3,0x01,"���ٴ���������:","INPUT PASSWORD AGAIN:");
        ScrGotoxy(64,5);
        getstring_res = GetString(password2,0x69,4,6);
        if(getstring_res == 0xff)/*�û�����cancel��ȡ��,������һ���˵�*/
            return;
        else if(getstring_res)/*��ʱ����,Ҫ��������������*/
            continue;

        if(!memcmp(password1,password2,sizeof(password1)))
            break;
        else
        {
            ScrClrLine(2,7);
            SCR_PRINT(0,3,0x01,"  ���벻ƥ�䣡","PASSWORD NOT MATCH!");
            GetKeyMs(1000);
        }
    }

    ScrClrLine(2,7);
    if(!SysParaWrite(SET_SYSTEM_TIME_PASSWORD,password1+1))
        SCR_PRINT(0,3,0x01,"   �޸ĳɹ�  ","    SUCCESS  ");
    else
        SCR_PRINT(0,3,0x01,"д�����","ERROR");
    GetKeyMs(1000);
}

/***************************************************************************
* ��������:
*               void SetSystemTime(void)
* ��������:
*                ����ϵͳʱ��
* �����º�������:
*                main_code
* �������º���:
*                SetTime     ����ʱ��
* �������:
*                ��
* �������:
*                ��
* ����ֵ:
*                ��
* ��ʷ��¼:
*          �޸���       �޸�����                 �޸�����              �汾��
*         ��ƽ��          2007-06-20                        ����                    01-01-01
****************************************************************************/
void SetSystemTime(void)
{
    uchar ucRet = 0,i;
    uchar ucBuffer[256];
    uchar str[6],timebf[8],timeAsc[16];
    unsigned char key;

    if(RequireSetTimePassword())
       return ;	

    while(1)
    {
        ScrCls();
        SCR_PRINT(0,0,0x81,"    ϵͳʱ��     ","  SYSTEM TIME    ");
        SCR_PRINT(0,2,0x01,"1-����ʱ��","1-SET TIME");
        SCR_PRINT(0,4,0x01,"2-�޸�����","2-CHANGE PASSWD");

        while(1)
        {
            key = getkey();
            if(key == KEYCANCEL)
                return ;
            if(key == '1' || key == '2')
                break;
        }


        if(key == '1')
        {
            memset(ucBuffer, 0x00, sizeof(ucBuffer));
            memset(str, 0x00, sizeof(str));
            ScrCls();
            //  ScrPrint(0,0,0x81,"SET SYSTEM TIME ");
            SCR_PRINT(0,0,0x81,"  ����ϵͳʱ��  ","SET SYSTEM TIME ");
            ScrPrint(0,2,1,"YYMMDDHHMMSSWEEK");
            GetTime(timebf);
            for(i = 0; i < 7; i++)
            {
                HexToAscii(timebf[i],timeAsc + i*2);
            }

            timeAsc[12] = 0x00;
            ScrPrint(0,4,1,"%s[%02x]",timeAsc,timebf[6]);
            ScrFontSet(1);
            ScrGotoxy(0,6);
            ucRet = GetString(ucBuffer,0xE5,12,12);

            if(ucRet == 0)
            {
                str[0] = AsciiToHex(ucBuffer+1);
                str[1] = AsciiToHex(ucBuffer+3);
                str[2] = AsciiToHex(ucBuffer+5);
                str[3] = AsciiToHex(ucBuffer+7);
                str[4] = AsciiToHex(ucBuffer+9);
                str[5] = AsciiToHex(ucBuffer+11);
                ucRet = SetTime(str);
				ScrClrLine(2,7);
				if(ucRet == 0)
				{
					SCR_PRINT(0,3,0x01,"   ���óɹ�  ","    SUCCESS  ");
				}
				else
				{
					SCR_PRINT(0,3,0x01,"   ����ʧ��  ","     FAIL     ");
				}
				GetKeyMs(1000);
            }
        }
        else
            ChangeSetTimePassword();
    }
}


