/************************************************
Ӳ���汾���ü��汾����
������:wanlm
��������:20101129

�޸ļ�¼:
�޸���:
�޸�ʱ��:
************************************************/
#include "cfgmanage.h"
#include "base.h"
#include "..\flash\nand.h"
#include "..\puk\puk.h"
#include "keyword.c"

#define ARM_PLAT
#define printf                    //   s_uart0_printf
#define VALID_CFG_PARAM  "valid_cfg_param"
/************************************************************************
				�ؼ��ּ��ؼ��ֶ�Ӧ�ļ����--����
************************************************************************/
//���ڴ洢ת�����Ӳ��������Ϣ
S_HARD_INFO s_HardwareInfo[sizeof(s_Versicon_Keyword)/sizeof(S_VERSION_CONFIG)];
static tSignFileInfo s_CfgFileSigInfo;
/************************************************************************
����ԭ��: static int Fill_HardwareInfo(char *keyword,char *context)
����    : ��д�ؼ��ֶ�Ӧ��������Ϣ�������ڲ�ʹ��         
������
	keyword(input): ����ŵ�Ӳ��������Ϣ�Ĺؼ���				 
	context(input): ����ؼ��ֶ�Ӧ��������Ϣ���ַ���
����ֵ��
	<0      ʧ��
	����    �ɹ�
************************************************************************/
static int Fill_HardwareInfo(char *keyword,char *context,int type)
{
	int i,len;
	for(i=0;i<sizeof(s_HardwareInfo)/sizeof(S_HARD_INFO);i++)
	{
		if(strcmp(s_HardwareInfo[i].keyword,keyword)) continue;
		memset(s_HardwareInfo[i].context,0x00,sizeof(s_HardwareInfo[i].context));
		len=strlen(context);
		if(len>ITEM_SIZE) len=ITEM_SIZE;
		memcpy(s_HardwareInfo[i].context,context,len);
		s_HardwareInfo[i].type=type;  
		return i;
	}

	return -1;
}

/************************************************************************
����ԭ�ͣ�int ReadCfgInfo(char *keyword,char *context)           
������
	keyword(input): ģ�����ڲ�ѯӲ��������Ϣ�Ĺؼ��֣��ùؼ��ּ�����ĵ�				 
	context(ouput): ����ؼ��ֶ�Ӧ��������Ϣ���ַ���
����ֵ��
	<0      ʧ��
	         -1:�����ļ���δд��ùؼ��ֶ�Ӧ����Ϣ
	         -2:����Ĺؼ��ֲ��ڹؼ����б���
	         -3:�������Ϊ��ָ��
	>0    �ɹ�,��ֵ��ʾ���صĳ���
************************************************************************/
int ReadCfgInfo(char *keyword,char *context)
{
	int i;
	if((keyword==NULL) || (context ==NULL))
	{
		return -3;
	}
	
	for(i=0;i<sizeof(s_HardwareInfo)/sizeof(S_HARD_INFO);i++)
	{
		if(strcmp(s_HardwareInfo[i].keyword,keyword)) continue;
		if(!strlen(s_HardwareInfo[i].context))  return -1;
		strcpy(context,s_HardwareInfo[i].context);
		return strlen(s_HardwareInfo[i].context);
	}

	return -2;
}


/************************************************************************
����ԭ�ͣ�int GetHardwareConfig(const char *Keyword,char *ContextOut,int ContextOutSize)           
������
         keyword(input):       ���ڲ�ѯӲ��������Ϣ�Ĺؼ��֣��ùؼ��ּ�����ĵ�                             
         context(ouput):       ����ؼ��ֶ�Ӧ��������Ϣ���ַ���
         ContextOutSize(input): ContexOut �������������󳤶�    
����ֵ��
         <0      ʧ��
                  -1:�����ļ���δд��ùؼ��ֶ�Ӧ����Ϣ
                  -2:����Ĺؼ��ֲ��ڹؼ����б���
                  -3:��������Ƿ�
                  -4:���������Ȳ���
         >0      �ɹ�,��ֵ��ʾ���صĳ���

************************************************************************/
int GetHardwareConfig(const char *Keyword,char *ContextOut,int ContextOutSize)
{
	int i,len;
	if((Keyword==NULL) || (ContextOut ==NULL)  ||(ContextOutSize<=0))
	{
		return -3;
	}
	ContextOut[0]=0;
	
	for(i=0;i<sizeof(s_HardwareInfo)/sizeof(S_HARD_INFO);i++)
	{
		if(strcmp(s_HardwareInfo[i].keyword,Keyword)) continue;
		if(!strlen(s_HardwareInfo[i].context))  return -1;
		len=strlen(s_HardwareInfo[i].context);
		if((len+1)>ContextOutSize)  return -4;
			
		strcpy(ContextOut,s_HardwareInfo[i].context);
		return len;
	}

	return -2;
}


int CfgTotalNum()
{
	return sizeof(s_HardwareInfo)/sizeof(S_HARD_INFO);
}

int CfgGet(int num,int type,char *keyword,char *context)
{
	int totalnum=CfgTotalNum();
	
	if(num>=totalnum)
		return -1;
	if(type!=s_HardwareInfo[num].type)
		return -2;

	strcpy(keyword,s_HardwareInfo[num].keyword);
	strcpy(context,s_HardwareInfo[num].context);
	return 0;

}

/************************************************************************
����ԭ�ͣ�static int verify_symbol(char *src,int item_num)           
������
	src(input/output) input�� Ϊ�������ķ����ַ���
					  output���������ַ�����ȥ����ͷβ�Ŀո�TAB�ַ�����ַ�����
	item:   <0ʱ�� ����ֻ���keyword
			>=0ʱ�����keyword��Ӧ�������Ƿ�Ϸ�,������src��Ӧ����Ҫ�����keywordû������Ӧ�ļ�����ݣ�ֱ�ӷ��� 0
����ֵ��
	<0       �����ִ���
	>=0ʱ    src��Ӧ�����ݽṹ�е����
************************************************************************/
static int verify_symbol(char *src,int item_num)
{
	int total_items_num;
	int num_per_item,type;
	S_ITEMS_VALUE *items_temp;
	int i,j;
	char buf[ITEM_SIZE*2];

	total_items_num=sizeof(s_Versicon_Keyword)/sizeof(S_VERSION_CONFIG);
	for(i=0;i<strlen(src);i++)
	{
		if(src[i]!=0x20 && src[i]!=0x09) break;  //���˵�ͷ���Ŀո�
	}

	for(j=0;i<strlen(src);i++)
	{
		if(src[i]==0x20 || src[i]==0x09) break;  //����β���Ŀո�
		buf[j++]=src[i];
	}
	buf[j]=0;
	memcpy(src,buf,strlen(buf)+1); //��src�е���Ч���������

	if(item_num<0)  //��ʱֻ�ҷ���ͷ
	{
		for(i=0;i<total_items_num;i++)
		{
			if(strcmp(buf,(const char *)s_Versicon_Keyword[i].items)) continue;
			return i;
		}
		return -1; //û�ҵ����ش���
	}

	num_per_item= s_Versicon_Keyword[item_num].num_per_item;
	items_temp=s_Versicon_Keyword[item_num].items;
	type = s_Versicon_Keyword[item_num].type;
	
	if(CFG_TYPE!=type) //��Ӳ��ģ��
		return 0;

	for(i=0;i<num_per_item;i++)  //�Ƚ�context�����Ƿ�Ϸ�
	{
		if(!strstr(buf,(const char *)(items_temp+i))) continue;
		return i;
	}
	
	if(!strcmp(buf,"NULL"))   //����ؼ��ֺ�����дNULL�ַ���
		return 0;
	else 
		return -2;

}


/************************************************************************
����ԭ�ͣ�int s_CfgInfo_Init()        
������
	        ��
����ֵ��
	<0      ��ʼ������
	����    ��ʼ���ɹ�
************************************************************************/
int s_CfgInfo_Init()
{
	int i,j,jj;
	int total_items_num;
	int cnt,iret;
	uchar *pt;
	char buf[400];   
	int last_LF,last_equal,item_num,ret;
	int quote_cnt;

	total_items_num=sizeof(s_Versicon_Keyword)/sizeof(S_VERSION_CONFIG);

	iret=ReadTerminalInfo(INDEX_CFGFILE,(uchar*)SAPP_RUN_ADDR,PAGE_SIZE);
	if(iret<0)
		return NO_CFG_FILE;
	pt=(uchar *)SAPP_RUN_ADDR;
	cnt =iret;

    if (s_iVerifySignature(SIGN_FILE_MON, pt, cnt, &s_CfgFileSigInfo)) return CFGFILE_SIG_ERR;	
	if (memcmp(pt+cnt-16,SIGN_FLAG,16)==0) cnt -= SIGN_INFO_LEN;
	for(i=0;i<total_items_num;i++)
	{
		strcpy(s_HardwareInfo[i].keyword,(char*)s_Versicon_Keyword[i].items);
		memset(s_HardwareInfo[i].context,0,sizeof(s_HardwareInfo[i].context));
	}

	last_equal=0;
	last_LF=0;
	item_num=-1; //��¼keyword�������е��кţ�С��0��ʾ��ǰ��Ч��
	for(i=0;i<cnt;i++)
	{
		if('=' == pt[i])
		{
			last_equal=i;
			if(last_equal<last_LF) 
				return FILE_FORMAT_ERR;
			
			for(j=last_LF;j<last_equal;j++)
			{
				if((0x2F==pt[j]) && (0x2F==pt[j+1])) break;  //��������������˫б��
			}
			if(j!=last_equal) continue;  //�п�ͷ����˫б�ܣ�ֱ�ӽ�����һ��ѭ��
		

			memset(buf,0x00,sizeof(buf));
			if(!last_LF)
				memcpy(buf,pt+last_LF,last_equal-last_LF);
			else
				memcpy(buf,pt+last_LF+1,last_equal-(last_LF+1));

			item_num =verify_symbol(buf,-1);  //find keyword symbol 

			if(item_num<0) return KEYWORD_ERR;			
		}

		if('\n'  == pt[i] ||(i==(cnt-1))) //�������з������ļ�����
		{
			last_LF=i;
			
			//�����һ��û�лس�ʱ����Ҫ���س�����λ��ָ�����һ���ַ�֮��
			if (i==(cnt-1) && ('\n' != pt[i]))  
				last_LF=cnt;				

			if(last_equal>last_LF) return FILE_FORMAT_ERR;
			

			quote_cnt=0;   //˫���ŵĸ�������
			memset(buf,0x00,sizeof(buf));

			for(j=last_equal,jj=0;j<last_LF;j++)
			{

				if( '"'==pt[j])	quote_cnt++;  //ͳ��˫���Ÿ���
				if((0x2F==pt[j]) && (0x2F==pt[j+1])) break;  //��������������˫б��

				if(1==quote_cnt) buf[jj++]=pt[j+1];  //�ҵ���һ˫����ʱ��ʼ���������ڵ�����

				if(2==quote_cnt)
				{
					buf[jj-1]=0;  //��buf��ȥ��˫����
					break;
				}
			}

			if(!quote_cnt && (item_num!=-1))  //�����йؼ��ֵ�û��contextʱ������
				return NO_CONTEXT_ERR;
			if(-1==item_num) continue;        //û�йؼ���ʱ��������һ��ɨ��
			
			if(1==quote_cnt) //ֻ�е���˫����ʱ����ȷ
				return ONLY_ONE_QUOTE_ERR;
			ret = verify_symbol(buf,item_num);  //verify para vaild
			if(ret<0) return CONTEXT_ERR;

			ret=Fill_HardwareInfo((char*)s_Versicon_Keyword[item_num].items,buf,
									s_Versicon_Keyword[item_num].type); //write para to hardware info table
			if(ret <0) return FILL_ERR;
			item_num=-1;   //��ȡ��һ�κ󣬽��к���Ϊ��Ч
		}
	}

	return 0;
}

void AdjustCfg(int status)
{
    int i, total_num, item_num, ret, reinit_lcd_flag;
    char keyword[ITEM_SIZE+1],context[ITEM_SIZE+1],buf[ITEM_SIZE+1];    
    if(!status)
    {
        ret=CheckCfgParam();
        if(ret) return;
        reinit_lcd_flag=0;
        total_num = CfgTotalNum();
        for (i = 0; i < total_num; i++)
        {
            memset(keyword, 0, sizeof(keyword));
            memset(context, 0, sizeof(context));
            memset(buf, 0, sizeof(buf));
            ret = CfgGet(i, CFG_TYPE, keyword, context);
            if (ret) continue;
            ret = s_CfgReadSysParam(keyword, buf);
            if (ret) continue;
            if(!strcmp(buf, context)) continue;
            item_num = verify_symbol(keyword,-1);
            if (item_num < 0) continue;
            ret = verify_symbol(buf, item_num);
            if (ret<0) continue;
			ret=Fill_HardwareInfo((char*)s_Versicon_Keyword[item_num].items,buf,
									s_Versicon_Keyword[item_num].type); //write para to hardware info table
			if(ret <0) continue;
			if(!strcmp((char*)s_Versicon_Keyword[item_num].items, "LCD")) reinit_lcd_flag = 1;
        }  
        if(reinit_lcd_flag) s_ScrInit();
        return ;  //��ʼ��ͨ��
    }    

	ScrCls();
	switch(status)
	{
		case FILE_FORMAT_ERR:
			ScrPrint(0,3,0,"File Format Error");
			break;
		case KEYWORD_ERR:
			ScrPrint(0,3,0,"KeyWord Error");
			break;
		case CONTEXT_ERR:
			ScrPrint(0,3,0,"Context Error");
			break;
		case FILL_ERR:
			ScrPrint(0,3,0,"Write Config Error");
			break;
		case NO_CONTEXT_ERR:
			ScrPrint(0,3,0,"No Context");
			break;
		case ONLY_ONE_QUOTE_ERR:
			ScrPrint(0,3,0,"Only One Quote");			
			break;
		case CFGFILE_SIG_ERR:
			ScrPrint(0,3,0,"CfgFile Sig Error");			
			break;				
	}
	ScrPrint(0,4,0,"Init Config Error");
	while(1);
}

void GetCfgFileSig(tSignFileInfo *info)
{
    info[0] = s_CfgFileSigInfo;
}
/*
��ʽ:
|ITEM_SIZE                  |ITEM_SIZE Byte |ITEM_SIZE Byte('0x04' EOT���)|ITEM_SIZE |ITEM_SIZE   |ITEM_SIZE |...
|flag:VALID_CFG_PARAM|   Page No.        |keyword\x04\x04\x04\x04\x04\x04 |context\x04|keyword\x04|context\x04|

*/
int s_CfgWriteSysParam(char *keyword,char *context)
{
    int ret, item_num;
    uint PageNo[CONFIG_BLOCK_NUM];
    char buf[2048], value[2048], *pt=NULL;
    if (strlen(context)>ITEM_SIZE) return -1;
    item_num = verify_symbol(keyword,-1);
    if (item_num < 0) return -2;
    ret = verify_symbol(context, item_num);
    if (ret < 0) return -3;    
    memset(value, 0, sizeof(value));
    memset(buf, 0x04, sizeof(buf));
    ret = SysParaRead(MODIFY_CFG_SETTING, value);
    if(!ret && !memcmp(value, VALID_CFG_PARAM, strlen(VALID_CFG_PARAM)))
    {
        pt = strstr(value+ITEM_SIZE*2, keyword);
        if(pt!=NULL)
        {
            if(!memcmp(context, pt+ITEM_SIZE, strlen(context))) return 0;/*the same context*/
            memcpy(buf, context, strlen(context));        
            memcpy(pt+ITEM_SIZE, buf, ITEM_SIZE);
        }    
        else 
        {
            memcpy(buf, keyword, strlen(keyword));
            memcpy(buf+ITEM_SIZE, context, strlen(context));
            memcpy(value+strlen(value), buf, ITEM_SIZE*2);            
        }
    }
    else/*the first item*/
    {
        s_GetCfgPageNo(INDEX_CFGFILE, PageNo);
        memcpy(buf, VALID_CFG_PARAM, strlen(VALID_CFG_PARAM));
        buf[ITEM_SIZE] = PageNo[0]+'0';
        buf[ITEM_SIZE+1] = PageNo[1]+'0';
        buf[ITEM_SIZE+2] = PageNo[2]+'0';        
        memcpy(buf+ITEM_SIZE*2, keyword, strlen(keyword));
        memcpy(buf+ITEM_SIZE*3, context, strlen(context));
        memcpy(value, buf, ITEM_SIZE*4);      
    }
    ret = SysParaWrite(MODIFY_CFG_SETTING, value);

    return ret;
}

int s_CfgReadSysParam(char *keyword,char *outdata)
{
    int ret,i;
    char buf[ITEM_SIZE+1], value[2048], *pt=NULL;
    
    memset(value, 0, sizeof(value));
    memset(buf, 0, sizeof(buf));
    ret = SysParaRead(MODIFY_CFG_SETTING, value);
    if (ret!=0) return -1;
    if (memcmp(value, VALID_CFG_PARAM, strlen(VALID_CFG_PARAM))) return -1;
    pt = strstr(value+ITEM_SIZE*2, keyword);
    if (pt==NULL) return -2;

    for(i=0; i<ITEM_SIZE; i++)
    {
        if(*(pt+ITEM_SIZE+i)==0x04) break;
        buf[i] = *(pt+ITEM_SIZE+i);
    }
    strcpy(outdata,buf);
    return 0;
}

int CheckCfgParam()
{
    int ret;
    uint PageNo[CONFIG_BLOCK_NUM],curPageNo[CONFIG_BLOCK_NUM];
    char value[2048], buf[ITEM_SIZE+1];
    
    memset(value, 0, sizeof(value));
    memset(PageNo, 0, sizeof(PageNo));
    memset(curPageNo, 0, sizeof(curPageNo));
    
    ret = SysParaRead(MODIFY_CFG_SETTING, value);
    if (ret!=0) return -1;
    if (memcmp(value, VALID_CFG_PARAM, strlen(VALID_CFG_PARAM))) return -2;    

    /*���������ļ��洢��ҳ�ű�Ǩ
        �ж������ļ��Ƿ��������ع�*/
    PageNo[0]=value[ITEM_SIZE]-'0';     
    PageNo[1]=value[ITEM_SIZE+1]-'0';
    PageNo[2]=value[ITEM_SIZE+2]-'0';    
    s_GetCfgPageNo(INDEX_CFGFILE, curPageNo);
    if (!memcmp(curPageNo, PageNo, sizeof(PageNo))) return 0;/*δ�������ع�*/

    /*�������������,�������ļ�δ�������ع�*/
    if (curPageNo[0]<=PageNo[0] && curPageNo[1]<=PageNo[1] 
        && curPageNo[2]<=PageNo[2] )
    {
        value[ITEM_SIZE] = curPageNo[0]+'0';
        value[ITEM_SIZE+1] = curPageNo[1]+'0';
        value[ITEM_SIZE+2] = curPageNo[2]+'0';    
        SysParaWrite(MODIFY_CFG_SETTING, value);   
  
        return 0;
    }

    /*�������ع������ļ�,ʹ֮ǰ��������Ч*/
    memset(value, 0, sizeof(value));
    memset(buf, 0x04, sizeof(buf));    
    memcpy(value, buf, ITEM_SIZE);
    SysParaWrite(MODIFY_CFG_SETTING, value);   
      
    return -3;       
}
void s_SetHardWareInfo(char *keyword,char *context, int type)
{
    uchar ch;
    int i, j, num, ret,row, ROWS_PER_PAGE;
    S_ITEMS_VALUE *pt;

    ScrCls();
    SCR_PRINT(0,0,0x81,"  �޸�������Ϣ  "," Modify CFG INFO");
    ScrPrint(0,2,0,"%s (%s)", keyword, context);    
    j = sizeof(s_Versicon_Keyword)/sizeof(S_VERSION_CONFIG);
    for (i=0; i<j; i++)
    {
        if(s_Versicon_Keyword[i].type != type) continue;
        if(!strcmp((char*)s_Versicon_Keyword[i].items, keyword))break;
    }
    if (i>=j) return;
    num = s_Versicon_Keyword[i].num_per_item -1;
    pt = s_Versicon_Keyword[i].items + 1;
    ROWS_PER_PAGE = 5;
    for (i=0; i<num;)
    {
        row = i%ROWS_PER_PAGE;
        ScrPrint(0,row+3,0,"[%d] %s", row+1, pt+i);    
        i++;
        row = i%ROWS_PER_PAGE;
        while(!row || i==num)
        {
            ch=getkey();
            switch(ch)
            {
                case KEYCANCEL:
                case KEYENTER:
                return ;
                case KEYDOWN:
                case KEYF2:
                    if (i >= num) continue;
                break;
                case KEYUP:
                case KEYF1:
                    if (i <= ROWS_PER_PAGE) continue;
                    if (row) i-=(row+ROWS_PER_PAGE);
                    else i-=(ROWS_PER_PAGE*2);                        
                break;
                case KEY1:
                case KEY2:
                case KEY3:
                case KEY4:
                case KEY5:                    
                    if (row!=0 && (ch-KEY0)>row) continue;
                    if (row) j=i-(row-(ch-KEY1));
                    else j=i-(ROWS_PER_PAGE-(ch-KEY1));
                    ScrClrLine(2,7);
                    ret = s_CfgWriteSysParam(keyword, (char*)(pt+j));
                    if (ret) continue;
                    strcpy(context, (char*)(pt+j));
                    ScrPrint(0,2,0,"%s (%s)", keyword, context);
                    ScrPrint(0,3,0,"Modify Cfg Success!");
                    ScrPrint(0,4,0,"Reboot...");                   
                    DelayMs(1000);
                    Soft_Reset();
                    i=0;
                break;
                default: continue;                    
            }
            ScrClrLine(3,7);
            break;                        
        }        
    }    
}
void SetHardWareInfo()
{
    uchar buf[16], ch, sn[33], CfgNo[sizeof(s_HardwareInfo)/sizeof(S_HARD_INFO)];
    char keyword[ITEM_SIZE+1],context[ITEM_SIZE+1];    
    int i, j, num, row, ROWS_PER_PAGE;

    while(1)
    {
        memset(sn, 0x00, sizeof(sn));
        ReadSN(sn);
        if(strlen(sn)==0) return;        
        memset(buf,0,sizeof(buf));
        ScrCls();
        SCR_PRINT(0,0,0x81,"  �޸�������Ϣ  "," Modify CFG INFO");
        SCR_PRINT(0,3,1,"  ����������:","INPUT PASSWORD:");
        ScrGotoxy(0,5);
        ch = GetString(buf,0x69,4,4);
        if(ch == 0xff) return;/*KEYCANCEL*/
        if(ch == 0)
        {
            if(!strcmp(buf+1,sn+strlen(sn)-4)) break;
            ScrClrLine(2,7);
            SCR_PRINT(0,3,0x01,"   �������!","PASSWORD ERROR!");
            if(GetKeyMs(1200) == KEYCANCEL) return;
        }
    }

    memset(CfgNo, 0x00, sizeof(CfgNo));
    j = CfgTotalNum();
    num = 0;
    for (i = 0; i < j; i++)
    {
        if (CfgGet(i, CFG_TYPE, keyword, context)) continue;
        /*The first page display LCD,PRINTER,TOUCH_SCREEN*/
        if (strcmp(keyword, "PRINTER")
            &&strcmp(keyword, "TOUCH_SCREEN")
            &&strcmp(keyword, "LCD")) continue;
        CfgNo[num] = i;
        num++;
    }    
    for (i = 0; i < j; i++)
    {
        if (CfgGet(i, CFG_TYPE, keyword, context)) continue;
        if (!strcmp(keyword, "PRINTER")
            || !strcmp(keyword, "TOUCH_SCREEN")
            || !strcmp(keyword, "LCD")) continue;
        if (!strcmp(keyword, "MODEM")) continue;/*POS ��оƬ��ģ���ֹ�޸�*/
        CfgNo[num] = i;
        num++;
    }
    if (num==0) return;

    ROWS_PER_PAGE = 3;
    kbflush(); 
    ScrClrLine(2,7);    
    for (i = 0; i < num;)
    {
        row = i%ROWS_PER_PAGE;
        CfgGet(CfgNo[i], CFG_TYPE, keyword, context);
        ScrPrint(0,row*2+2,0,"%d-%s(%s)", row+1, keyword, context);    
        i++;
        row = i%ROWS_PER_PAGE;
        while(!row || i==num)
        {
            ch=getkey();
            switch(ch)
            {
                case KEYCANCEL:
                return;
                case KEYDOWN:
                case KEYF2:
                    if (i >= num) continue;
                break;
                case KEYUP:
                case KEYF1:
                    if (i<=ROWS_PER_PAGE) continue;
                    if (row) i -= (row+ROWS_PER_PAGE);
                    else i -= (ROWS_PER_PAGE*2);                        
                break;
                case KEY1:
                case KEY2:
                case KEY3:
                    if (row!=0 && (ch-KEY0)>row) continue;
                    if (row) j=i-(row-(ch-KEY1));
                    else j=i-(ROWS_PER_PAGE-(ch-KEY1));
                    CfgGet(CfgNo[j], CFG_TYPE, keyword, context);                    
                    s_SetHardWareInfo(keyword, context, CFG_TYPE);
                    i=0;                        
                break;
                default:
                continue;
            }
            ScrClrLine(2,7);
            break;
        }
    }
}

int DLGetCfgInfo(uchar* outdata)
{
    int i, j, cnt, total_num, num, ret, len;
    char keyword[ITEM_SIZE+1],context[ITEM_SIZE+1];    
    S_ITEMS_VALUE *pt;

    total_num = CfgTotalNum();
    len = 1;
    cnt = 0;    
    for (i = 0; i < total_num; i++)
    {
        memset(keyword, 0, sizeof(keyword));
        memset(context, 0, sizeof(context));        
        ret = CfgGet(i, CFG_TYPE, keyword, context);
        if (ret) continue;
        if (!strcmp(keyword, "MODEM")) continue;/*POS ��оƬ��ģ���ֹ�޸�*/
        num = s_Versicon_Keyword[i].num_per_item -1;
        pt = s_Versicon_Keyword[i].items + 1;
        outdata[len++] = num+2;       
        memcpy(outdata+len, keyword, ITEM_SIZE+1);
        len += ITEM_SIZE+1;
        memcpy(outdata+len, context, ITEM_SIZE+1);
        len += ITEM_SIZE+1;        
        for (j = 0; j < num; j++)
        {
            memcpy(outdata+len, (char*)(pt+j), ITEM_SIZE+1);
            len += ITEM_SIZE+1;                
        }
        cnt++;
    }
    outdata[0]=cnt;

    return len;
}
int DLSetCfgInfo(uchar* data)
{
    int i, num, offset, ret;
    
    if(s_GetBootSecLevel()==0) return -2;   
    offset = 0;
    num = data[offset];
    for (i=0; i<num; i++)
    {
        offset = 1+((ITEM_SIZE+1)*2*i);
        ret = s_CfgWriteSysParam(data+offset, data+offset+ITEM_SIZE+1);
        if (ret) return -1;
    }
    return 0;
}

