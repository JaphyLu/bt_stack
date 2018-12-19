/***********************************************************
*   �ļ�����:
*
*   �ļ�����:���ܴ�ͷ����ʵ��
*
*   �޸ļ�¼:
	����		�޸���		�޸�����
	2012.2.29 Τ�׿�	֧��BCM5892 CPU			  	
***********************************************************/
#include <string.h>
#include <stdarg.h>

#include "enmagrd.h"
#include "base.h"
#include "magcard.h"
#define MCU_BCM5892
//ʹ��GPIOģ��SPIͨѶ
//#define SPI_IO 1

#undef DEBUG
#undef ENMAGFUN_DEBUG
//�����ã��򿪿��ԴӴ������SPIͨѶ����Ϣ
//#define DEBUG
//�����ã��򿪵��Դ�ͷ����
//#define ENMAGFUN_DEBUG

//�����ã�magtek��ͷ�������ݲ����棬��Կд�벻����
//#define MAGTEK_NO_SAV

//���ü��ܴ�ͷ����Կ����ʽ �̶���Կ��DUKPT
#define TYPE_FIXKEY    1
#define TYPE_DUKPTKEY  0

/**
��ȡ���ܴ�ͷ��Կ��Կ״̬
@retval # 0 ��ǰ��ͷδд����Կ��
@retval # 1 ��ǰ��ͷ��ע��̶���Կ��
@retval # 2 ��ǰ��ͷ��ע��DUKPT��Կ��
*/
#define MSR_KEY_STATUS_NOT_EN -1
#define MSR_KEY_STATUS_NO_KEY 0
#define MSR_KEY_STATUS_FIXED 1
#define MSR_KEY_STATUS_DUKPT 2

static void  spi_init(void);
static void  spi_senddata(uchar ucData);
static void  spi_revdata(uchar *pData);
void  encryptmag_INTinit(void);

//magtek��ͷ��������
static int magtek_cmdexg(unsigned char cmd,const unsigned char *data,int dl,
						 unsigned char *rsp,int mrl,int TimeOutms);

static int	get_notification_message(uchar *pData, int mrl);
static int	set_magtek_key_serial_number(const uchar *pucDeviceSN);
static int	lock_device_key(uchar ucFlag);
static int	load_device_key(uchar *oldkey, uchar *newkey, int mode);
static int	external_authenticate(uchar *key);
static int 	save_magtek_nonvalatile_data(void);

void  encrypt_msr_isr(void);
static void s_check_read_enmag_data(void);

#define safememsetzero(buf)		memset(buf,0,sizeof(buf))
// TCBC�ӽ����㷨
static int iTCBC(uchar * pucIV, uchar *pszIn, uchar *pszOut, int iDataLen, uchar *pszKey, int iKeyLen, uchar ucMode);
static int  iTdes(uchar * pszIn,uchar * pszOut,int iDataLen,uchar * pszKey,int iKeyLen,uchar ucMode);
static int MsrKeyDes(int iSelectKey,uchar*pucKsnIn,uchar *pucKeyVar,uchar *pucIV,int iDataLen,uchar *pucDataIn,uchar*pucDataOut,uchar Mode);

//magtek��ͷʹ�õĺ궨��
// Command Type ��������
#define GET_PROPERTY_CMD        0x00
#define SET_PROPERTY_CMD        0x01
#define SAVE_NONVAL_DATA_CMD    0x02
#define RESET_DEVICE_CMD        0x03
#define LOAD_DEV_KEY_CMD        0x04
#define EXT_AUTHENTICATE_CMD    0x05

// Start of Frame Character
#define SOF_CHARACTER           0x01

// Idle_Character
#define IDLE_CHARACTER          0xFF

// Message Type 
#define REQUEST_MESSAGE         0x00
#define RESPONSE_MESSAGE        0x01
#define NOTIFICATION_MESSAGE    0x02

#define NID                     0x00

// Property ID
#define SOFTWARE_ID             0x00
#define KEY_SERIAL_NUMBER		0x01
#define ENCRYPT_CARD_DATA       0x02
#define LOCK_DEVICE_KEY         0x03
#define ENCRYPTED_CHALLENGE     0x04
#define RIC_TIMEOUT             0x05
#define DA_TIMEOUT              0x06
#define TIC_TIMEOUT             0x07
#define SPI_PHASE_POLARITY      0x08
#define CC_TIMEOUT              0x09
#define KEY_CHECK_VALUE         0x0A


// Gerneric Result Cdoe
#define RET_SUCCESS             0x00
#define RET_FAILURE             0x01
#define RET_BAD_PARAMETER       0x02


//idtech��ͷʹ�õĺ���
static int idtech_cmdexg(uchar cmd,uchar fid,const uchar *data,int datalen, uchar *rspdt,int rmlen,int TimeOutMs,int mode);
//idtech��ͷʹ�õĺ�
#define IDTECH_MODE_ASCII       0x00
#define IDTECH_MODE_FIDLEN      0x01
#define IDTECH_MODE_FIX	             0x02	

#define IDTECH_STX 0x02
#define IDTECH_ETX 0x03
#define IDTECH_ACK 0x06
#define IDTECH_NAK 0x15


#define IDTECH_CMD_READ_VERSION	 	 0x22
#define IDTECH_CMD_READ_SN			 0x4E
#define IDTECH_CMD_KEYMAG_TYPE		 0x58		
#define IDTECH_CMD_EXAUTH			 0x74
#define IDTECH_CMD_SETEN_TYPE		 0x4C
#define IDTECH_CMD_SEC_LEVE			 0x7E
#define IDTECH_CMD_READ_SETTINGS	 0x1F
#define IDTECH_CMD_DECODING_METHOD	 0x1D
#define IDTECH_CMD_LOAD_FIXED_KEY    0x76
#define IDTECH_CMD_REVIEW_KSN        0x51
#define IDTECH_CMD_SERIAL_NUMBER_CMD 0x4E

// Error Return Code
#define RESPONSE_ERROR          -1
#define RESPONSE_ACKERR			-2
#define RESPONSE_ERRSTX			-3
#define RESPONSE_ERRETX			-4
#define RESPONSE_ERRLRC			-5
#define RESPONSE_TIMEOUT		-13
#define RESPONSE_SPI_OCCUPY		-10

#define COMPONENT_LENGTH        16      // Key Component����
#define KEY_LENGTH              16      // Key ����

#define	PED_ENCRYPT	            0x01    // ����
#define	PED_DECRYPT 	        0x00    // ����

#define CMD_STATUS              0x00    // ��ǰ��������̬,ģ��ͨ���������ö���������
#define DATA_STATUS             0x01    // ��ǰ��������̬,ģ�鴦�ڽ��մſ�����״̬

// ��ʶ���ܴ�ͷ����״̬
static uchar gucCurrent_Status = CMD_STATUS;
static volatile short have_enmag_data=0;

// ��ǰʹ�õ�key
static uchar gaucCurretKey[16];

// �Ƿ�ʹ�ý��ܹ��ܱ�ʶ
// 0-������ 1-����
static uchar gucEncryptFlag = 0;

//�����ʱʹ�õĻ���
static uchar bufcmd[128];

//��ͷ���к�
static uchar k_MsrSn[8];

//KEY״̬
static int k_KeyStatus;
//��ͷ����״̬
static int k_ConfigStatus;
//��ͷ�汾
static uchar k_MsrVer[64];
//��ͷ�����Ƿ�����
volatile static char k_DataFail;


#ifdef DEBUG

#define DB_COM 0
extern uchar   PortSends(uchar channel,uchar *str,ushort str_len);
extern uchar PortOpen (uchar channel, uchar *Attr);
static uchar flag_com0_communication;
static void see_prn(char *fmt,...)
{
	va_list varg;
	int retv;
	char sbuffer[512];

	memset(sbuffer, 0, sizeof(sbuffer));
    	va_start( varg, fmt );                  // ��ȡ���� ��ʼλ��
    	retv=vsprintf(sbuffer,  fmt,  varg);
    	va_end( varg );
	//PortTxs(DB_COM,(unsigned char *)sbuffer, strlen((char *)sbuffer));
		if(PortSends(DB_COM,(unsigned char *)sbuffer, strlen((char *)sbuffer)))
		{
			PortOpen(DB_COM, "115200,8,n,1");
			PortSends(DB_COM,(unsigned char *)sbuffer, strlen((char *)sbuffer));
		}
	//	s_Lcdprintf((unsigned char *)sbuffer);
}
#define see_ch(a) PortTx(DB_COM,a)
static void see_str(unsigned char *s,int len)
{
	while(len--)
	{
		see_prn("%02x ",*s++);
	}
}
static void see_hbin(const char *dp,uchar *inb,int len)
{
	int ii,jj,kk;
	uchar ch;

	for(ii=0;ii<len;ii++)
	{
		if(inb[ii]) break;
	}

	for(;ii<len;len--)
	{
		if(inb[len-1]) break;
	}
	see_prn("%s len=%3d >>",dp,len-ii);
	if(ii < len)
	{
		ch=0x80;
		for(jj=0;jj<8;jj++)
		{
			if(inb[ii]&ch) break;
			ch >>=1;
		}
		kk=8-jj;
		for(;ii<len;ii++)
		{
			ch=inb[ii] << jj;
			if(ii != (len-1))
			{
				ch=ch|(inb[ii+1]>>kk);
			}
			see_prn("%02x ",ch);
		}
	}
	
}
#else
#define see_prn
#define see_ch(a)
#define see_str(a,b) 
#define see_hbin(a,b,c)
#endif

#define set_command_status()		gucCurrent_Status = CMD_STATUS

/* 
	J3	1	-->SPI0_CLK		GPA4
		2	<--SPI0_MISO	GPA5
		3	-->SPI0_MOSI	GPA6
		4	<--MSR_DAV		GPB2_EINT2
		5	-->SPI_CS		GPA7
		6	--> VDD			GPA0
		7	---	GND
		8	--- GND
*/
#define clear_encryptmag_intr() 	  
#define disable_encryptmag_intr()     gpio_set_pin_interrupt(GPIOB,2,0) 
#define exit_encryptmag_intr()		 
extern void disable_ts_spi();
extern void enable_ts_spi();

static void enable_spi()
{
	if (GetTsType() > 0) disable_ts_spi();
	gpio_set_pin_val(GPIOA,7,0); 	 
}

static void disable_spi()
{
 	gpio_set_pin_val(GPIOA,7,1);
	if ((GetTsType()>0) && !have_enmag_data) enable_ts_spi();
}
#define is_magtek_header() (0)
#define is_idtech_header() (1)

static void config_encryptmag_port(void)
{
	gpio_set_pin_type(GPIOA,0,GPIO_OUTPUT);	
	gpio_set_pin_type(GPIOA,7,GPIO_OUTPUT);
	
	gpio_set_pull(GPIOB,2,0);	//DAV������
	gpio_enable_pull(GPIOB,2);
	gpio_set_pin_type(GPIOB,2,GPIO_INPUT);
	s_SetSoftInterrupt(IRQ_EXT_GRP2,NULL);//���жϹر�

}
#define enable_encryptmag_power()	gpio_set_pin_val(GPIOA,0,0)
#define disable_encryptmag_power()	gpio_set_pin_val(GPIOA,0,1) 
#define dav_ready() gpio_get_pin_val(GPIOB,2)

#define is_encryptmag_power_off()	gpio_get_pin_val(GPIOA,0)
static void spi_init(void)
{
	disable_spi();
	gpio_set_pin_type(GPIOA,4,GPIO_FUNC0);	
	gpio_set_pin_type(GPIOA,5,GPIO_FUNC0);	
	gpio_set_pin_type(GPIOA,6,GPIO_FUNC0);	
	spi_config(0,150*1000,8,0);
	disable_spi();
}

// SPI��������
static void spi_senddata(uchar ucData)
{
#ifdef DEBUG
	if(!flag_com0_communication)	see_prn("%02x ",ucData);
#endif

	spi_txrx(0,ucData,8);
}
static void spi_revdata(uchar *pData)
{
	unsigned short cc;
	cc=spi_txrx(0,0xffff,8);
	*pData=cc;
#ifdef DEBUG
	if(!flag_com0_communication)	see_prn("%02x ",*pData);
#endif
	
	DelayUs(10);
}

// �жϳ�ʼ��
void encryptmag_INTinit(void)
{
	s_setShareIRQHandler(GPIOB,2,INTTYPE_RISE,(void*)encrypt_msr_isr);
	s_SetSoftInterrupt(IRQ_EXT_GRP2,s_check_read_enmag_data);
}
#define is_spi_free() (1)
void down_encryptmag_pin(void)
{
	enable_spi(); //CS������
	if(is_spi_free())
	{
		gpio_set_pin_type(GPIOA,4,GPIO_OUTPUT);	
		gpio_set_pin_type(GPIOA,5,GPIO_OUTPUT);	
		gpio_set_pin_type(GPIOA,6,GPIO_OUTPUT);	
		gpio_set_pin_val(GPIOA,4,0);
		gpio_set_pin_val(GPIOA,5,0);
		gpio_set_pin_val(GPIOA,6,0);
	}
	disable_spi();
}	
//�ж��Ƿ��Ǽ��ܴ�ͷ
int is_encrypt_mag_reader(void)
{
	return 1;
}
//�ж��Ƿ���Ѷ��һ�����ܴ�ͷ
//MAG_CARD="MH1601-V01" ������һ�����ܴ�ͷ
int is_mh1601_v01(void)
{
	static int rfg=0;
    char context[64];
	if(rfg)
	{
		return (rfg&1);
	}
    memset(context,0,sizeof(context));
    if(ReadCfgInfo("MAG_CARD",context)>0) 
		if(strstr(context, "MH1601-V01")) 
		{
			rfg=1;
			return 1;
		}
	rfg=2;
	return 0;
}
/****************************************************
 * ���ܣ��򿪴ſ��Ķ���
 *       ���ſ����ݲ����жϷ�ʽ������һ���򿪴ſ���
 *       ��������ʹ�����ö�ȡ������ֻҪˢ������ͷһ
 *       ���ܶ���ſ����ݣ�����ڲ���Ҫʹ�ôſ��Ķ�
 *       ��ʱ����ý��ſ��Ķ����رա�
 ******************************************************/
static int config_encrypymag_parameter();
static void enmag_pwr_handle(void);
void encryptmag_open(void)
{
	uchar tempch=0;
	int ercnt=0;
	T_SOFTTIMER tmopen;

#ifdef DEBUG
	DelayMs(10);
	PortOpen(0,"115200,8,n,1");
#endif	
	enmag_pwr_handle();
	ReadMsrKey(0, gaucCurretKey, NULL);
	if(k_ConfigStatus!=0)
	{
		spi_init();
		set_command_status();
		k_ConfigStatus=config_encrypymag_parameter();
	}
    // �ſ���������̬,���ջ�������
	gucCurrent_Status = DATA_STATUS;

	if(dav_ready())
	{
		//����������ݣ��������,��ʱʱ��1��
		spi_init();
		ercnt=0;
		while(1)
		{
			s_TimerSetMs(&tmopen,1000);
			while(dav_ready())
			{
				enable_spi();
				spi_revdata(&tempch);
				if(s_TimerCheckMs(&tmopen) == 0)
				{
					break;
				}
			}
			if(!dav_ready()) break;
			disable_spi();
			if(ercnt++>30)
			{
				ScrCls();
				SCR_PRINT(0,3,0x41, "��ͷ����!", "MagHead Error!");
				while(1);
			}
			disable_encryptmag_power();
			DelayMs(30);
			enmag_pwr_handle();
		}
	}
	k_DataFail=0;
	gucCurrent_Status = DATA_STATUS;
	see_prn("\r\nenmag open");
}

/************************************************
 * ˵���� �رմſ��Ķ��������ָ����ж�Ϊ��ǰ״̬
 ************************************************/
void encryptmag_close(void)
{
    // �ر��ж�
	disable_encryptmag_intr();       
	disable_encryptmag_power();
	down_encryptmag_pin();
    DelayMs(30);
	// ���ж�
    clear_encryptmag_intr();    
}

void encryptmag_swiped(void)
{
	if(!k_DataFail) return;

    //�������ݴ��󣬸���ͷ�������µ�
    encryptmag_close();
    encryptmag_open();  
    k_DataFail=0;
}

/*
//����ʱ�ƻ���һ��ָ���ȡidtech��ͷ����������,ʹ���з���idtech��ͷ���᷵���������ã�
//��˲�ʹ��
//��tlv��ʽ���л�ȡָ��tlv����
static int get_one_byte_tlv(unsigned char *ov,int omaxl,unsigned char *buf,int bufl,unsigned int tag)
{
	int ii,cpl;
	for(ii=0;ii<bufl;)
	{
		if(buf[ii++]==tag)
		{
			cpl=buf[ii++];
			if(cpl>omaxl) cpl=omaxl;
			memcpy(ov,buf+ii,cpl);
			return cpl;
		}
		cpl=buf[ii++];
		ii+=cpl;
	}
	return 0;
}
*/

/* 
����: ���ܴ�ͷ��ʼ��
���: ��
����: ��		
˵��: ��ʼ��ʱû�м�Ⲣ���ü��ܴ�ͷ����������Ϊ��ͷ��ʼ����PED��ʼ��֮ǰ��
	  ��Ⲣ���ô�ͷ������ҪPED�洢�Ĵ�ͷ��Կ	  
*/

void encryptmag_init(void)
{
	spi_init();
	safememsetzero(k_MsrSn);
	safememsetzero(gaucCurretKey);
	safememsetzero(bufcmd);
	safememsetzero(k_MsrVer);
	k_KeyStatus=-1;
	k_DataFail=0;
	disable_encryptmag_power();
	disable_encryptmag_intr();	
	config_encryptmag_port();
	enmag_pwr_handle();
	k_ConfigStatus=config_encrypymag_parameter();
	if(k_ConfigStatus==RESPONSE_TIMEOUT) 
	{
		ScrPrint(128-6*5,6,0,"%d",k_ConfigStatus);
		DelayMs(3000);
	}
	disable_spi();
	encryptmag_close();
}
/* 
����: ���ô�ͷ����
֧��:	magtek��idtech
���: ��
����: 0		
	  
*/
static int config_encrypymag_parameter(void)
{
	uchar tbuf[1024];
	int auth_fail=1;
	int iResult=0,ii;
	int iReturnResult=0;
	int ercnt=0;

	safememsetzero(tbuf);
	
//	PedReadMsrKey(0, gaucCurretKey, NULL);
	gucEncryptFlag=1;
	if(is_magtek_header())
	{
		const unsigned char s_para_cfg_magtek[][2]=
		{
		//���ôŵ������������0x00 ���� 0x01����
			ENCRYPT_CARD_DATA,	0x01,
		};
		for(ii=0;ii<(sizeof(s_para_cfg_magtek)/2);ii++)
		{
			iResult=magtek_cmdexg(GET_PROPERTY_CMD,&s_para_cfg_magtek[ii][0],1,bufcmd,sizeof(bufcmd),3000);
			if(iResult<0) return iResult;
			if((iResult)&&(bufcmd[2]==s_para_cfg_magtek[ii][1]))
			{
				
			}
			else
			{
				if(auth_fail) auth_fail=external_authenticate(gaucCurretKey);
				iResult=magtek_cmdexg(SET_PROPERTY_CMD,&s_para_cfg_magtek[ii][0],2,bufcmd,sizeof(bufcmd),3000);
				save_magtek_nonvalatile_data();
				if(iResult>=2)
				{
					iReturnResult=bufcmd[1];
				}
				else
				{
					iReturnResult=iResult;
				}
				if(iReturnResult) return iReturnResult;
			}
		}
	}
	else if(is_idtech_header())
	{
		//��ͷӦ�õ�����
		const unsigned char r_para_cfg[][2]=
		{
#if TYPE_FIXKEY
		//���ܵȼ�4
			IDTECH_CMD_DECODING_METHOD,	'4',
		//��Կ���� 0Ϊ�̶���Կ 1-DUKPT
			IDTECH_CMD_KEYMAG_TYPE, '0',
		//�������� 0 ���� 1 TDES���� 2 AES����
			IDTECH_CMD_SETEN_TYPE,	'1',
#endif
#if TYPE_DUKPTKEY	
		//���ü��ܵȼ�4
			IDTECH_CMD_DECODING_METHOD,	'4',
		//������Կ���� 0Ϊ�̶���Կ 1-DUKPT
			IDTECH_CMD_KEYMAG_TYPE, '1',
		//���ü������� 0 ���� 1 TDES���� 2 AES����
			IDTECH_CMD_SETEN_TYPE,	'1',
#endif
		};
		//���ô�ͷ�����������
		const unsigned char s_para_cfg[][2]=
		{
#if TYPE_FIXKEY
		//�������� 0 ���� 1 TDES���� 2 AES����
			IDTECH_CMD_SETEN_TYPE,	'0',
		//���ܵȼ�4
			IDTECH_CMD_DECODING_METHOD,	'4',
		//��Կ���� 0Ϊ�̶���Կ 1-DUKPT
			IDTECH_CMD_KEYMAG_TYPE, '0',
		//�������� 0 ���� 1 TDES���� 2 AES����
			IDTECH_CMD_SETEN_TYPE,	'1',
#endif
#if TYPE_DUKPTKEY	
		//���ü������� 0 ���� 1 TDES���� 2 AES����
			IDTECH_CMD_SETEN_TYPE,	'0',
		//���ü��ܵȼ�4
			IDTECH_CMD_DECODING_METHOD,	'4',
		//������Կ���� 0Ϊ�̶���Կ 1-DUKPT
			IDTECH_CMD_KEYMAG_TYPE, '1',
		//���ü������� 0 ���� 1 TDES���� 2 AES����
			IDTECH_CMD_SETEN_TYPE,	'1',
#endif
		};

		auth_fail=1; ercnt = 0;
		//����ͷ���������Ƿ���ȷ
		DelayMs(1);
		for(ii=0;ii<(sizeof(r_para_cfg)/2);ii++)
		{
			iResult=idtech_cmdexg('R',r_para_cfg[ii][0],bufcmd,0,
				bufcmd,sizeof(bufcmd),100,IDTECH_MODE_FIDLEN);
			if(iResult<0) 
			{
				if(ercnt++ < 90)
				{
					ii--;
					continue;
				}
				return iResult;
			}

			if((iResult)&&(bufcmd[2]==r_para_cfg[ii][1]))
			{
				
			}
			else
			{
				see_prn("\r\nPara Error ii=%d iResult=%d,para=0x%02x",ii,iResult,bufcmd[0]);
				if(auth_fail) auth_fail=external_authenticate(gaucCurretKey);
				for(ii=0;ii<(sizeof(s_para_cfg)/2);ii++)
				{
					iResult=idtech_cmdexg('S',s_para_cfg[ii][0],&s_para_cfg[ii][1],1,
						NULL,0,3000,IDTECH_MODE_ASCII);
					//˵�� ��ʹ��ͷӦ��ʧ�ܣ�ҲҪ��������
					if(iResult<0) 
					{
						iReturnResult=iResult; 
					}
				}
				break;
			}
		}
	}
	return iReturnResult;
}
/*
����: ��ȡ���ܴ�ͷ�̼��汾
֧��: magtek idtech
���: verbuf �汾�������ָ�룬��С��64�ֽڿռ�
		TimeOutms ���ζ��ĳ�ʱʱ��
	  ReadCount ��������
���أ�>0�ɹ������ذ汾����
	  <= ʧ��
*/
static int encryptmag_read_firmware_version(unsigned char *verbuf,int TimeOutms,int ReadCount)
{
	int ii;
    uchar pValue=0;
	int iResult=0;
	
	verbuf[0]=0;
	for(ii=0;ii<ReadCount;ii++)
	{
		if(is_magtek_header())
		{
		    pValue = SOFTWARE_ID;
			iResult=magtek_cmdexg(GET_PROPERTY_CMD,&pValue,1,bufcmd,sizeof(bufcmd),TimeOutms);
			if(iResult==13)
			{
				memcpy(verbuf,bufcmd+2,11);
				verbuf[11]=0;
				return 11;
			}
		}
		else if(is_idtech_header())
		{
			memset(bufcmd,0,sizeof(bufcmd));
			iResult=idtech_cmdexg('R',IDTECH_CMD_READ_VERSION,bufcmd,0,
				bufcmd,sizeof(bufcmd),TimeOutms,IDTECH_MODE_ASCII);
			see_prn("\r\nRead IDTECH Ver result=%d Ver=%s\r\n",iResult,bufcmd);
			if(iResult==32)
			{
				memcpy(verbuf,bufcmd,8);
				memcpy(verbuf+8,bufcmd+32-6,6);
				verbuf[14]=0;
				return 14;
			}
		}
	}
	if(iResult>0) iResult=0;
	return iResult;
}

/***********************************************************
                ����ʵ��
***********************************************************/
/* 
����: magtek���ܴ�ͷ�����
֧��: magtek
���: cmd	�����ַ�
	  data 	��������� ���ΪNULL��ʾֱ�ӽ���ȴ�Ӧ��
	  datalen �������ݳ���
	  rsp	 ���ջ��壬ΪNULL��ʾ�����գ����մ�ŵ����ݲ�������ʼ�ַ������ֽڵĳ��ȼ�lrc
	  rsplen ���ջ���ռ�,����ռ䲻�����᷵�ش���ֻ�Ѷ������ݶ���
	  TimeOutms ��ʱʱ�䣬��λΪms	
����: >=0 �ɹ�ִ�в��յ������ݳ���
	  <0  ͨѶ����
	  
*/
static int magtek_cmdexg(unsigned char cmd,const unsigned char *data,int datalen,
						 unsigned char *rsp,int rsplen,int TimeOutms)
{
	unsigned short slen=0;
	unsigned char  tch=0;
	int ii;
	T_SOFTTIMER tmptm;

	if(data!=NULL)
	{

		//�����ݷ���
		see_prn("\r\nmagtek_command Len=%d cmdtype=0x%02x: ",datalen,cmd);
		enable_spi();
		spi_senddata(SOF_CHARACTER);
		slen=datalen+2;
		spi_senddata((unsigned char )(slen>>8));
		spi_senddata((unsigned char )(slen));
		spi_senddata(REQUEST_MESSAGE);
		spi_senddata(cmd);
		while(datalen--)
		{
			spi_senddata(*data++);
		}
	}
	if(rsp!=NULL)
	{
		enable_spi();
		see_prn("\r\nmagtek_cmdrsp Len=%d: ",rsplen);
		s_TimerSetMs(&tmptm,TimeOutms);
		while(!dav_ready())
		{
			if(s_TimerCheckMs(&tmptm)==0)
			{
				disable_spi();
				return RESPONSE_TIMEOUT;
			}
		}
		for(ii=0;ii<30;ii++)
		{
			spi_revdata(&tch);
			if(tch==SOF_CHARACTER)
			{
				break;
			}
		}
		if(tch!=SOF_CHARACTER)
		{
			disable_spi();
			return RESPONSE_ERRSTX;
		}
	    spi_revdata(&tch);
		slen =  tch << 8;
	    spi_revdata(&tch);
		slen += tch;
		for(ii=0;ii<slen;ii++)
		{
		    spi_revdata(&tch);
			if(ii<rsplen) rsp[ii]=tch;
		}
		disable_spi();
		return slen;
	}
	disable_spi();
	return 0;
}
/* 
����:	��ȡ��ͷ֪ͨ����(��ȡˢ������)
֧��:	magtek��idtech
���:	pData ����Ļ���
		pDataSize ���泤��
����: 0		�ɹ���������ȫ������
	  >0 	ʧ�ܣ�����ֵΪδ���������
	  <0	ͨѶʧ��
*/
static int get_notification_message(uchar *pData,int pDataSize)
{
    uchar  ucData = 0;
    ushort usLen = 0;
    uint   i = 0;
    uchar  aucTemp[512],tempch;

    safememsetzero(aucTemp);

	enable_spi();
    for (i = 0; i < 10; i++)
    {
        spi_revdata(&ucData);
        if (ucData == SOF_CHARACTER)
        {
            break;
        }
    }
    if (i == 10)
    {
		disable_spi();
        return RESPONSE_ERROR;
    }

    usLen = 0;
    spi_revdata(&ucData);
    usLen +=  ucData << 8;
    spi_revdata(&ucData);
    usLen += ucData;
    for (i = 0; i < usLen;i++)
    {
    	
        spi_revdata(&tempch);    
        if(i<sizeof(aucTemp))
        {
        	aucTemp[i]=tempch;
        }
    }

	disable_spi();
    if (aucTemp[0] != NOTIFICATION_MESSAGE)
    {
        return RESPONSE_ERROR;
    }

    if (aucTemp[1] != NID)
    {
        return RESPONSE_ERROR;
    }
    else 
    {
		usLen -=2;
		if(usLen>pDataSize)
		{
			memcpy(pData,&aucTemp[2],pDataSize);
			pDataSize=pDataSize-usLen;
		}
		else
		{
			memcpy(pData,&aucTemp[2],usLen);
			pDataSize=RET_SUCCESS;
		}
    }
    return pDataSize;
}  

// Set magtek Key Serial Number
static int set_magtek_key_serial_number(const uchar KeySn[8])
{
    uchar aucTemp[9];
    int iResult=0;
    safememsetzero(aucTemp);

    aucTemp[0] = KEY_SERIAL_NUMBER;
	iResult=magtek_cmdexg(SET_PROPERTY_CMD,aucTemp,9,bufcmd,sizeof(bufcmd),3000);
	if(iResult==9)
	{
    	memcpy(&aucTemp[1],KeySn,8);
		return bufcmd[1];
	}
	return iResult;
}

// Lock magtek Device Key
static int lock_device_key(uchar ucFlag)
{
    uchar aucTemp[2];
    int iResult=0;
    aucTemp[0] = LOCK_DEVICE_KEY;
    aucTemp[1] = ucFlag;
	iResult=magtek_cmdexg(SET_PROPERTY_CMD,aucTemp,2,bufcmd,sizeof(bufcmd),3000);
	if(iResult>=2)
	{
		return bufcmd[1];
	}
	return RESPONSE_ERROR;
}

// Encrpyt Chanllenge Property
static int encrypt_challenge(uchar *pData,int dl)
{
    int iResult = 0;
    uchar ucTemp = 0;
	if(is_idtech_header())
	{
		iResult=idtech_cmdexg('R',IDTECH_CMD_EXAUTH,bufcmd,0,
			pData,8,3000,IDTECH_MODE_FIX);
		if(iResult==8) return 0;
	}
	else
	{
		bufcmd[0]=ENCRYPTED_CHALLENGE;
		iResult=magtek_cmdexg(GET_PROPERTY_CMD,bufcmd,1,bufcmd,sizeof(bufcmd),3000);
		if(iResult==10)
		{
			memcpy(pData,bufcmd+2,8);
			iResult=bufcmd[1];
		}
	}
    return iResult;
}
// Save Non-volatile Data
static int save_magtek_nonvalatile_data(void)
{
	
#ifndef MAGTEK_NO_SAV
    int iResult = 0;
	iResult=magtek_cmdexg(SAVE_NONVAL_DATA_CMD,bufcmd,0,bufcmd,sizeof(bufcmd),3000);
	if(iResult>=2)
	{
		return bufcmd[1];
	}
	return RESPONSE_ERROR;
#endif
}
#if TYPE_DUKPTKEY

static int get_KSN(unsigned char *oksn)
{
	int iResult=0;
	iResult=idtech_cmdexg('R',IDTECH_CMD_REVIEW_KSN,bufcmd,0,
				bufcmd,sizeof(bufcmd),3000,IDTECH_MODE_FIDLEN);
	if(iResult>=13)	
	{
		memcpy(oksn,bufcmd+3,10);
		return bufcmd[2];
	}
	else
	{
		return RESPONSE_ERROR;
	}
}
/*******************************************************
                BASE64
*******************************************************/
static const char base64digits[] =
   "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

#define BAD	-1
static const char base64val[] = {
    BAD,BAD,BAD,BAD, BAD,BAD,BAD,BAD, BAD,BAD,BAD,BAD, BAD,BAD,BAD,BAD,
    BAD,BAD,BAD,BAD, BAD,BAD,BAD,BAD, BAD,BAD,BAD,BAD, BAD,BAD,BAD,BAD,
    BAD,BAD,BAD,BAD, BAD,BAD,BAD,BAD, BAD,BAD,BAD, 62, BAD,BAD,BAD, 63,
     52, 53, 54, 55,  56, 57, 58, 59,  60, 61,BAD,BAD, BAD,BAD,BAD,BAD,
    BAD,  0,  1,  2,   3,  4,  5,  6,   7,  8,  9, 10,  11, 12, 13, 14,
     15, 16, 17, 18,  19, 20, 21, 22,  23, 24, 25,BAD, BAD,BAD,BAD,BAD,
    BAD, 26, 27, 28,  29, 30, 31, 32,  33, 34, 35, 36,  37, 38, 39, 40,
     41, 42, 43, 44,  45, 46, 47, 48,  49, 50, 51,BAD, BAD,BAD,BAD,BAD
};
#define DECODE64(c)  (isascii(c) ? base64val[c] : BAD)

void to64frombits(unsigned char *out, const unsigned char *in, int inlen)
{
    for (; inlen >= 3; inlen -= 3)
    {
    	*out++ = base64digits[in[0] >> 2];
    	*out++ = base64digits[((in[0] << 4) & 0x30) | (in[1] >> 4)];
    	*out++ = base64digits[((in[1] << 2) & 0x3c) | (in[2] >> 6)];
    	*out++ = base64digits[in[2] & 0x3f];
    	in += 3;
    }
    if (inlen > 0)
    {
    	unsigned char fragment;
        
    	*out++ = base64digits[in[0] >> 2];
    	fragment = (in[0] << 4) & 0x30;
    	if (inlen > 1)
    	{
    	    fragment |= in[1] >> 4;
        }
    	*out++ = base64digits[fragment];
    	*out++ = (inlen < 2) ? '=' : base64digits[(in[1] << 2) & 0x3c];
    	*out++ = '=';
    }
    *out = '\0';
}

int from64tobits(unsigned char *out, const char *in, int maxlen)
{
    int len = 0;
    register unsigned char digit1, digit2, digit3, digit4;

    if (in[0] == '+' && in[1] == ' ')
    {
	    in += 2;
    }
    if (*in == '\r')
    {
	    return(0);
    }

    do {
    	digit1 = in[0];
    	if (DECODE64(digit1) == BAD)
    	{
    	    return(-1);
        }
    	digit2 = in[1];
    	if (DECODE64(digit2) == BAD)
    	{
    	    return(-1);
        }
    	digit3 = in[2];
    	if (digit3 != '=' && DECODE64(digit3) == BAD)
    	{
    	    return(-1); 
        }
    	digit4 = in[3];
    	if (digit4 != '=' && DECODE64(digit4) == BAD)
    	{
    	    return(-1);
        }
    	in += 4;
    	++len;
    	if (maxlen && len > maxlen)
    	{
    	    return(-1);
        }
    	*out++ = (DECODE64(digit1) << 2) | (DECODE64(digit2) >> 4);
    	if (digit3 != '=')
    	{
    	    ++len;
    	    if (maxlen && len > maxlen)
    	    {
    	        return(-1);
    	    }
    	    *out++ = ((DECODE64(digit2) << 4) & 0xf0) | (DECODE64(digit3) >> 2);
    	    if (digit4 != '=')
    	    {
    	        ++len;
        		if (maxlen && len > maxlen)
        		{
        		    return(-1);
        	    }
        		*out++ = ((DECODE64(digit3) << 6) & 0xc0) | DECODE64(digit4);
    	    }
    	}
    }while(*in && *in != '\r' && digit4 != '=');

    return (len);
}

/*************************************************
     ͨ��KSN��ȡ��ǰʹ�õ�KEY
*************************************************/
#define DUKPT_PIN_KEY  0X00
#define DUKPT_MAC_BOTH_KEY 0X01
#define DUKPT_MAC_RSP_KEY 0X02
#define DUKPT_DES_KEY		0X03
#define DUKPT_FUTURE_KEY		0XFF
static void StrXor(uchar *pucIn1,uchar *pucIn2,int iLen,uchar *pucOut)
{
    int iLoop;
    for(iLoop=0;iLoop<iLen;iLoop++)
    {
        pucOut[iLoop]=pucIn1[iLoop]^pucIn2[iLoop];
    }
}


static void  StrAND21Bits(uchar *pucIn1,uchar *pucIn2,uchar *pucOut)
{
	int iLoop;
	uchar ucTmp=pucOut[0];
	for(iLoop=0;iLoop<3;iLoop++)
	{
		pucOut[iLoop]=pucIn1[iLoop]&pucIn2[iLoop];
	}

	ucTmp&=0xE0;

	pucOut[0]|=ucTmp;

	return;
}

static void  StrOR21Bits(uchar *pucIn1,uchar *pucIn2,uchar *pucOut)
{
	int iLoop;
	uchar ucTmp=pucOut[0];
	for(iLoop=0;iLoop<3;iLoop++)
	{
		pucOut[iLoop]=pucIn1[iLoop]|pucIn2[iLoop];
	}

	ucTmp&=0xE0;

	pucOut[0]|=ucTmp;

	return;
}

/**
���ܣ�����DUKPT�ĳ�ʼ��ԿTIK���ն˷��صĵ�ǰKSN��������ǰ���ڼ���PIN�����MAC
�Ĺ�����Կ��
����DUKPT����֤��
uchar *pucTIK:���ص�PED�ĳ�ʼ��Կ,TIK
int iTikLen :TIK���ȣ�8/16
uchar *pucCurKsn:PED���صĵ�ǰKSN
uchar Mode:
#define DUKPT_PIN_KEY  0X00 PIN ��Կ
#define DUKPT_MAC_BOTH_KEY 0X01 
#define DUKPT_MAC_RSP_KEY 0X02
#define DUKPT_DES_KEY		0X03

uchar*pucOutKey:����ĵ�ǰ������Կ
*/
int s_PedGetDukptWorkKey(uchar *pucTIK,int iTikLen,uchar *pucCurKsn,uchar Mode
,uchar*pucOutKey)
{
	uchar aucR8[8],aucR8A[8],aucR8B[8],aucR3[3],aucSR[3],aucKsnR[8];
	uchar aucCurKey[24];
	uchar aucTIK[24];
	uint uiTmp;
	uchar aucTmp[128];
	int i;

	int iLoop;

	memcpy(aucCurKey,pucTIK,iTikLen);
	memcpy(aucR8,pucCurKsn+2,8);
	
	memset(aucR8+6,0x00,2);
	aucR8[5]&=0xE0;
	
	memcpy(aucR3,pucCurKsn+2+5,3);
	aucR3[0]&=0x1f;

	memset(aucSR,0x00,3);
	aucSR[0]=0x10;

TAG1:
	aucTmp[0]=0;
	StrAND21Bits(aucSR,aucR3,aucTmp);
	if(memcmp(aucTmp,"\x00\x00\x00",3)==0) goto TAG2;
	
	StrOR21Bits(aucSR,aucR8+5,aucR8+5);


	

	if(iTikLen==16)
	{
		StrXor(aucCurKey+8,aucR8,8,aucR8A);
		des(aucR8A,aucR8A,aucCurKey,1);
		
		StrXor(aucR8A,aucCurKey+8,8,aucR8A);

		StrXor(aucCurKey,"\xc0\xc0\xc0\xc0\x00\x00\x00\x00\xc0\xc0\xc0\xc0\x00\x00\x00\x00",iTikLen,aucCurKey);

		StrXor(aucCurKey+8,aucR8,8,aucR8B);

		des(aucR8B,aucR8B,aucCurKey,1);
		StrXor(aucR8B,aucCurKey+8,8,aucR8B);

		memcpy(aucCurKey+8,aucR8A,8);
		memcpy(aucCurKey,aucR8B,8);
	}
	else if(iTikLen==8)
	{
		StrXor(aucCurKey,aucR8,8,aucR8A);
		des(aucR8A,aucR8A,aucCurKey,1);
		StrXor(aucR8A,aucCurKey,8,aucCurKey);
	}

TAG2:

	uiTmp = (aucSR[0]<<16)
			+(aucSR[1]<<8)
			+(aucSR[2]<<0);
	uiTmp >>=1;
	aucSR[0]=(uiTmp>>16)&0x1f;
	aucSR[1]=uiTmp>>8;
	aucSR[2]=uiTmp;
	
	if(memcmp(aucSR,"\x00\x00\x00",3)!=0)goto TAG1;

	if(Mode==DUKPT_PIN_KEY)
	{
		StrXor(aucCurKey,"\x00\x00\x00\x00\x00\x00\x00\xff\x00\x00\x00\x00\x00\x00\x00\xff\x00\x00\x00\x00\x00\x00\x00\xff",iTikLen,pucOutKey);
	}
	else if(Mode==DUKPT_MAC_BOTH_KEY)
	{
	 	StrXor(aucCurKey,"\x00\x00\x00\x00\x00\x00\xff\x00\x00\x00\x00\x00\x00\x00\xff\x00\x00\x00\x00\x00\x00\x00\xff\x00",iTikLen,pucOutKey);
	}
	else if(Mode==DUKPT_MAC_RSP_KEY)
	{
	 	StrXor(aucCurKey,"\x00\x00\x00\x00\xff\x00\x00\x00\x00\x00\x00\x00\xff\x00\x00\x00\x00\x00\x00\x00\xff\x00\x00\x00",iTikLen,pucOutKey);
	}
	else if (Mode==DUKPT_DES_KEY)
	{
		StrXor(aucCurKey,"\x00\x00\x00\x00\x00\xff\x00\x00\x00\x00\x00\x00\x00\xff\x00\x00\x00\x00\x00\x00\x00\xff\x00\x00",iTikLen,aucTmp);
		
		for(i=0;i<iTikLen/8;i++)
		{
			iTdes(aucTmp+i*8,pucOutKey+i*8, 8,aucTmp , iTikLen, 1);
		}
	}
	else if (Mode==DUKPT_FUTURE_KEY)
	{
		memcpy(pucOutKey,aucCurKey,iTikLen);
	}
	else return -1;

exit:
	return 0;

}
#endif
static void binxor(uchar *od,uchar *sr,int len)
{
	while(len--)
	{
		*od++ ^=*sr++;
	}
}
//�����ͷ�ϵ�
static void enmag_pwr_handle(void)
{
	int ii,iResult=0;
	if(is_encryptmag_power_off())
	{
		if(is_mh1601_v01())
		{
			down_encryptmag_pin();
			DelayMs(30);
		}
		disable_encryptmag_intr();			
		for(ii=0;ii<3;ii++)
		{
			enable_encryptmag_power();
			spi_init();
			if(is_mh1601_v01())
				DelayMs(130);
			else
				DelayMs(35);
			set_command_status();
			iResult=encryptmag_read_firmware_version(k_MsrVer,100,3);
			if(iResult>0) break;
			disable_encryptmag_power();
			DelayMs(1000);
		}
		enable_encryptmag_power();
    	// ���ж�
		encryptmag_INTinit();
	}
	set_command_status();
}
//magtek ������Կ���ط�ʽ
//����
#define INJECTION_KEY_CLEAR		0
//����
#define INJECTION_KEY_ENCRYPT	1
/* 
���ܣ�		�̶���Կ��ʽ����Կ����
֧�ִ�ͷ��	magtek �� idtech ��ͷ
���:		oldkey 16�ֽھ���Կ
			newkey 16�ֽ�����Կ
			mode	magtek��ͷ��Կע��ģʽ
���أ�		0		�ɹ�
			����ֵ	ʧ��
*/
static int load_device_key(uchar oldkey[16],uchar newkey[16],int mode)
{
    int   iResult = 0;
    uchar aucTemp[18];
	uchar pData1[16],pData2[16];
	
	safememsetzero(aucTemp);
	safememsetzero(pData1);
	safememsetzero(pData2);
	
	see_prn("\r\noldkey ");
	see_str(oldkey,16);
	see_prn("\r\nnewkey ");
	see_str(newkey,16);
    // ��֤��Կ�����ߺϷ�
    iResult=external_authenticate(oldkey);
	if(iResult != RET_SUCCESS) return iResult;
	
	if(is_magtek_header())
	{
    // ����lock device false
	    lock_device_key(0);
		if(INJECTION_KEY_CLEAR==mode)
		{
			iTdes(aucTemp+16,pData1,16,oldkey,16,PED_DECRYPT);
			//memset(pData1,0,16);
			memcpy(pData2,newkey,16);
			binxor(pData2,pData1,16);
        	memset(aucTemp,0x00,sizeof(aucTemp));
        
	        // Load the first component
    	    // Component Number 01
        
	        aucTemp[0] = 0x01;
    	    memcpy(&aucTemp[1],pData1,COMPONENT_LENGTH);
	
			iResult=magtek_cmdexg(LOAD_DEV_KEY_CMD,aucTemp,COMPONENT_LENGTH+1,bufcmd,sizeof(bufcmd),3000);
			
			if((iResult>=2)&&(RET_SUCCESS==bufcmd[1]))
			{
				
			}
			else
    	    {
			    lock_device_key(1);	
			    save_magtek_nonvalatile_data();
            	return iResult;
        	}
			memset(aucTemp,0x00,sizeof(aucTemp));
			// Load the second component
			// Component Number 02
			aucTemp[0] = 0x02;
			memcpy(&aucTemp[1],pData2,COMPONENT_LENGTH);
			
			iResult=magtek_cmdexg(LOAD_DEV_KEY_CMD,aucTemp,COMPONENT_LENGTH+1,bufcmd,sizeof(bufcmd),3000);
			if(iResult>=2)	iResult=bufcmd[1];
			
			lock_device_key(1);	
			save_magtek_nonvalatile_data();
		}
		else if(INJECTION_KEY_ENCRYPT==mode)
		{
			aucTemp[0] = 0x02;
			iTdes(newkey,aucTemp+1,16,oldkey,16,PED_DECRYPT);
			aucTemp[17] = 0x01;
			iResult=magtek_cmdexg(LOAD_DEV_KEY_CMD,aucTemp,COMPONENT_LENGTH+2,bufcmd,sizeof(bufcmd),3000);
			if(iResult>=2)	iResult=bufcmd[1];
			
	    	lock_device_key(1);	
	    	save_magtek_nonvalatile_data();
		}
	}
	else
	{
		iResult=idtech_cmdexg('S',IDTECH_CMD_LOAD_FIXED_KEY,newkey,COMPONENT_LENGTH,NULL,0,3000,IDTECH_MODE_ASCII);
	}
    if (iResult == RET_SUCCESS)
	{
		//���԰汾���Դ򿪣���ʽ�汾���ñ�����Կ
		//memcpy(gaucCurretKey,newkey,16);
	}
	see_prn("\r\nNowKey iResult=%d ",iResult);
	see_str(gaucCurretKey,16);
	
	return iResult;  
  	
}
/* 
����:		�̶���Կ��ʽ���ⲿ��֤���Ի�÷��ʻ��޸Ĵ�ͷ������Ȩ��
֧�ִ�ͷ:	magtek��idtech
���:		AuthKey ��֤��Կ	
����:		0		�ɹ�
			����ֵ	ʧ��
*/
static int external_authenticate(uchar AuthKey[16])
{
    int iResult = 0;
    uchar aucEncryptData[16];
    uchar aucDencryptData[16];
    uchar aucKey[16];
    uchar aucTemp[8];
    uchar ucKey = 0;
    
	see_prn("\r\nAuthKey: ");
	see_str(AuthKey,16);

    memset(aucEncryptData,0x00,sizeof(aucEncryptData));
    memset(aucDencryptData,0x00,sizeof(aucDencryptData));
    memset(aucKey,0x00,sizeof(aucKey));
    safememsetzero(aucTemp);
    
    iResult = encrypt_challenge(aucEncryptData,sizeof(aucEncryptData));
    if (iResult != RET_SUCCESS)
    {
        return iResult;
    }

    iTdes(aucEncryptData,aucDencryptData,sizeof(aucDencryptData),AuthKey,KEY_LENGTH,PED_DECRYPT);
    memcpy(aucTemp,aucDencryptData,sizeof(aucTemp));
	if(is_idtech_header())
	{
		iResult=idtech_cmdexg('S',IDTECH_CMD_EXAUTH,aucTemp,8,
			NULL,0,3000,IDTECH_MODE_ASCII);
		if(iResult==RESPONSE_ACKERR)
		{
			iResult=1;
		}
	}
	else
	{
		iResult=magtek_cmdexg(EXT_AUTHENTICATE_CMD,aucTemp,4,bufcmd,2,3000);
		if(iResult>=2)	iResult=bufcmd[1];
		
//		send_command(0x06,EXT_AUTHENTICATE_CMD,aucTemp);
//		iResult = get_cmdresponse(NULL,0);
	}
    return iResult;
}
/*--------------------------------------------------
ID TECH��ͷ����ʵ��
----------------------------------------------------*/
/* 
����:		idtech��ͷ�����
֧�ִ�ͷ:	idtech
���:		cmd �����ַ���ֻ����Ϊ�����ַ� S R F���ֱ�Ϊ���á�����DUKPT��ʼ��ע��		
			fid idtech���ܱ��
			data ��������ָ�룬���ΪNULL��ʾû�����ݸ���ͷ��������ֱ�ӵȴ���ͷ���
			datalen �������ݳ���
			rspdt	������ݻ���ָ�룬��������ݲ���IDTECH_STX IDTECH_ETX��У����LRC
			rmlen	������ݻ�����󳤶�
			TimeOutMs �ȴ�Ӧ���ĳ�ʱʱ�䣬��λΪms
			mode	 �ȴ����յ�ģʽ
					IDTECH_MODE_ASCII	ASCIIģʽ��ʾ���յ�����û�г�����
										���յ����ݳ���ΪIDTECH_STX��ʼ��IDTECH_ETX����
					IDTECH_MODE_FIDLEN	FIDLEN��һ�ֽڹ��ܱ�� һ�ֽڳ��� ���ݸ�ʽ����
										���ָ�ʽ������1������������һ��
					IDTECH_MODE_FIX		�Թ̶��������ݷ�ʽ�������ݣ�����Ϊrmlen
		
����:		0		�ɹ�
			����ֵ	ʧ��

˵����		idtech��������Ƚϸ���(Ҳ����˵�Ƚϻ���)��
			�����������Լ���Ӧ��ʽ����������һ��������Ҫ�������ǵ��ֲ����������ͽ���
			
			���� ��cmd��fid��Ϊ��ĸFʱ��lcrУ�鲻����IDTECH_STX�ַ�
*/
static int idtech_cmdexg(uchar cmd,uchar fid,const uchar *data,int datalen,
						 uchar *rspdt,int rmlen,int TimeOutMs,int mode)
{
	uchar lrc,buf[128],tch;
	int ii,ercnt,rii,fg_ff;
	T_SOFTTIMER tmptm;

	if((cmd=='F')&&(fid=='F'))
	{
		//�˸�ʽΪidtech����Կ����
		fg_ff=1;
	}
	else 
	{
		fg_ff=0;
	}
	if(data!=NULL)
	{
		ercnt=0;
		enable_spi();
RE_SEND:
		spi_revdata(&tch);
		spi_revdata(&tch);
		s_TimerSetMs(&tmptm,TimeOutMs);
	    while(dav_ready())
		{
			spi_revdata(&tch);
			if(s_TimerCheckMs(&tmptm)==0)
			{
				disable_spi();
				return RESPONSE_TIMEOUT;
			}
		}
		//�����ݷ���
		see_prn("\r\nSend Cmd=0x%02x,fid=0x%03x slen=%d ",cmd,fid,datalen);
		spi_senddata(IDTECH_STX);
		if(fg_ff)	lrc=0;
		else		lrc=IDTECH_STX;
		if((cmd=='S')||(cmd=='R')||(cmd=='F'))
		{
			spi_senddata(cmd);
			lrc ^= cmd;
		}
		spi_senddata(fid);
		lrc ^= fid;
		if(datalen)
		{
			if(fg_ff)
			{
				for(ii=0;ii<datalen;ii++)
				{
					lrc^=data[ii];
					spi_senddata(data[ii]);
				}	
				spi_senddata(0x0d);
				spi_senddata(0x0a);
				lrc=lrc^0x0d^0x0a;
			}
			else
			{
				datalen&=0xff;
				spi_senddata(datalen);
				lrc^=datalen;
				for(ii=0;ii<datalen;ii++)
				{
					lrc^=data[ii];
					spi_senddata(data[ii]);
				}	
			}
		}
		spi_senddata(IDTECH_ETX);
		lrc^=IDTECH_ETX;
		spi_senddata(lrc);
		s_TimerSetMs(&tmptm,TimeOutMs);
	    while(!dav_ready())
		{
			if(s_TimerCheckMs(&tmptm)==0)
			{
				disable_spi();
				return RESPONSE_TIMEOUT;
			}
		}
		for(ii=0;ii<30;ii++)
		{
			spi_revdata(&tch);
			if(tch==IDTECH_ACK)
			{
				break;
			}
			else if(tch==IDTECH_NAK)
			{
				break;
			}
		}
		if(tch!=IDTECH_ACK)
		{
			disable_spi();
			return RESPONSE_ACKERR;
		}
	}
	if(rspdt!=NULL)
	{
		//��Ҫ����
		see_prn("\r\nRecv Rsp maxlen=%d ",rmlen);
		enable_spi();
		s_TimerSetMs(&tmptm,TimeOutMs);
	    while(!dav_ready())
		{
			if(s_TimerCheckMs(&tmptm)==0)
			{
				disable_spi();
				return RESPONSE_ERRSTX;
			}
		}
		
		for(ii=0;ii<30;ii++)
		{
			spi_revdata(&tch);
			if(tch==IDTECH_STX)
			{
				break;
			}
		}
		if(tch!=IDTECH_STX)
		{
			disable_spi();
			return RESPONSE_ERRSTX;
		}
		if(fg_ff)	lrc=0;
		else		lrc=IDTECH_STX;
		rii=0;
		while(1)
		{
			spi_revdata(&tch);
			lrc ^=tch;
			if(mode==IDTECH_MODE_FIX)
			{
				if(rii<rmlen)
				{
					if(rii<rmlen) 
					{
						rspdt[rii++]=tch;
						continue;
					}
				}
				else
				{
					break;
				}
			}
			else if(mode==IDTECH_MODE_ASCII)
			{
				if(tch==IDTECH_ETX) 
				{
					break;
				}
				if(rii<rmlen) 
				{
					rspdt[rii++]=tch;
				}
				else 
				{
					break;
				}
			}
			else 
			{
				//F L V��ʽ
				if(tch==IDTECH_ETX) 
				{
					break;
				}
				if(rii<rmlen) rspdt[rii++]=tch;
				else 
				{
					break;
				}
				spi_revdata(&tch);
				lrc ^=tch;
				if(rii<rmlen) 
				{
					rspdt[rii++]=tch;
				}
				datalen=tch;
				for(ii=0;ii<datalen;ii++)
				{
					spi_revdata(&tch);
					if(rii<rmlen) rspdt[rii++]=tch;
					lrc ^=tch;
				}
			}
		}
		spi_revdata(&tch);//����lrc
		lrc ^=tch;
		if(lrc) 
		{
			disable_spi();
			return RESPONSE_ERRLRC;
		}
		spi_senddata(IDTECH_ACK);
		disable_spi();
		return rii;
	}
	disable_spi();
	return 0;
}
/* 
����:		�Ѽ��ܴ�ͷ���ͳ����ַ������ܺ���ת���������ŵ����ַ���
֧�ִ�ͷ:	idtech magtek
���:		MagBitBuf �����ŵ����ַ���
			bitbuf	  ���ܴŵ����ַ���
����:		��
*/
static void transfer_bitdata(MSR_BitInfo MagBitBuf[3],unsigned char bitbuf[280])
{
	int i,j,kk,bn;
	uchar aucMessage[280];
    uchar aucDecryptMessage[280];

    memcpy(aucMessage,bitbuf,sizeof(aucMessage));
    if (gucEncryptFlag == 1)
    {
		uchar tbuf[128];
		if(ReadMsrKey(0,tbuf,NULL)>0)	
		{
			//PED�д洢��Կ����PED�����
			i=MsrKeyDes(0,NULL,NULL,NULL,272, &aucMessage[8], aucDecryptMessage, PED_DECRYPT);
		}
		else
		{
			//ʹ�ô�ͷĬ�ϳ�ʼ��Կ����
			memset(gaucCurretKey,0,sizeof(gaucCurretKey));
			iTCBC(NULL,&aucMessage[8],aucDecryptMessage,272,gaucCurretKey,KEY_LENGTH,PED_DECRYPT);
		}
        memcpy(&aucMessage[8],aucDecryptMessage,272);
    }
    //���ռ��ܴ�ͷ�����ʽ�ֵ�������ͬ�Ĵŵ� ��ϸ˵���뿴���ܴ�ͷ�����ʽ����
	for(kk=0;kk<3;kk++)
	{
		bn=0;
		memset(MagBitBuf[kk].buf,0,sizeof(MagBitBuf[kk].buf));
		see_prn("\r\ntrack %d bindata: ",kk+1);
		see_hbin("",&aucMessage[16+88*kk],88);
        for (i = 0; i < 88;i++)
        {
            for (j = 0; j < 8; j++)
            {   
                if ((aucMessage[i+16+88*kk] >> j) & 0x01)
                {
                    MagBitBuf[kk].buf[i * 8 + j + 20] = 1;
					bn++;
                }
            }
        }
		MagBitBuf[kk].BitOneNum=bn;
		see_prn("\r\nTrack %d BitNum=%d ",kk+1,bn);
	}
}
void encryptmag_read_rawdata(void)
{
	
}

//�ṩRF����
int s_read_enmag_data(void)
{
	if(dav_ready()&&(gucCurrent_Status == DATA_STATUS))
	{
		uchar gaucTrackRawData[300];

		spi_init();
		if(get_notification_message(gaucTrackRawData,sizeof(gaucTrackRawData))==0)
		{
			MSR_BitInfo k_MagBitBuf[3];
			transfer_bitdata(k_MagBitBuf,gaucTrackRawData);
			s_SetMagBitBuf(k_MagBitBuf);
		}
		else
		{
				k_DataFail=1;
		}
		//״̬����ָ�������PED����ʴ�ͷ����״̬���ó�����״̬
		gucCurrent_Status = DATA_STATUS;
		have_enmag_data=0;
		enable_ts_spi();
		return 0;
	}
	else
	{
		//״̬����ָ�������PED����ʴ�ͷ����״̬���ó�����״̬
		gucCurrent_Status = DATA_STATUS;
		have_enmag_data=0;
		enable_ts_spi();
	}
	return -1;
}
static void s_check_read_enmag_data(void)
{
	if((have_enmag_data)&&(is_spi_free()))
	{
		s_read_enmag_data();
	}
}
extern unsigned char get_scrbacklightmode(void);
extern void  ScrBackLight(unsigned char mode);
void encrypt_msr_isr(void)
{
    // ���ж�
    clear_encryptmag_intr();
    // ����̬��ʶ�����¼�
    if (gucCurrent_Status == DATA_STATUS)
    {
    	disable_ts_spi();
		have_enmag_data=1;
		if(get_scrbacklightmode() < 2) ScrBackLight(1);
		if(get_kblightmode() <2)kblight(1);
		if(get_touchkeylockmode()<2)s_KbLock(1); // For D200 touch key
    }
	exit_encryptmag_intr();
}

void encryptmag_reset(void)
{
	if(dav_ready())
	{
		{
			uchar tbuf[300];
			spi_init();
			while(dav_ready())
			{
				get_notification_message(tbuf,sizeof(tbuf));

			}
		}
	}
}
#if TYPE_DUKPTKEY
// ����LRCУ��
static uchar Calc_LRC(uchar *pucDataIn,uint uiLength)
{
    uint i ;
    uchar ucCrc = 0;

    for (i = 0; i < uiLength; i++)
    {
        ucCrc ^= pucDataIn[i];
    }
    return ucCrc;
}

// ����Sum
static uchar Calc_SUM(uchar *pucDataIn,uint uiLength)
{
    uint i;
    uchar ucSum = 0;
    for (i = 0; i < uiLength; i++)
    {
        ucSum += pucDataIn[i];
    }
    return ucSum;
}

const uchar gaucHex2Ascii[16] = {'0','1','2','3','4','5','6','7','8','9',
'A','B','C','D','E','F'};

/*
    DUKPT Management
    Load Key Serial Number    
    pucKSN -- 8bytes KSN,��ʼ��KSN
    �����ʽ����
    <STX><'F'><'F'><Command Data(BASE64)><0x0D><0x0A><ETX><LRC>
    Command Data��ʽ����:
    <FF><0A><11><KSN#><KSN Bytes><LRC>
    <KSN#>:TDES:0x32 DES:0x0A
    <KSN Bytes>Ϊ16���ֽڵ�ASCII
    pucKSNΪ8��Byte��Ҫת��ΪASCII
    ��������:
    0xFF,0xFF,0x98,0x76,0x54,0x32,0x10,0xE0
    ת����Ϊ
    'F' 'F' 'F' 'F' '9' '8' '7' '6' '5' '4' '3'
    '2' '1' '0' 'E' '0'
*/
int Load_KSN(uchar *pucKSN)
{
	int iResult;
    uint  i;
    uchar aucCmdData[21];
    uchar aucCmdBase64Data[28];
    uchar aucTotalData[35];
    uchar aucRetData[50];    
    memset(aucCmdData,0x00,sizeof(aucCmdData));
    memset(aucCmdBase64Data,0x00,sizeof(aucCmdBase64Data));
    memset(aucTotalData,0x00,sizeof(aucTotalData));

    // Command Data
    aucCmdData[0] = 0xFF;
    aucCmdData[1] = 0x0A;
    aucCmdData[2] = 0x11;
    
    // TDES
    aucCmdData[3] = 0x32;

    // ��HEX�ַ�װ��ΪASCII�ַ�
    for (i = 0; i < 8; i++)
    {
        aucCmdData[4+i*2]   =  gaucHex2Ascii[(pucKSN[i] >> 4) & 0x0F];
        aucCmdData[4+i*2+1] =  gaucHex2Ascii[pucKSN[i] & 0x0F];
    }
    
    // ����LRC
    aucCmdData[20] = Calc_LRC(aucCmdData,sizeof(aucCmdData) - 1);

    // װ��ΪBASE64��ʽ�ַ�
    to64frombits(aucCmdBase64Data,aucCmdData,sizeof(aucCmdData));

    // STX
	iResult=idtech_cmdexg('F','F',aucCmdBase64Data,sizeof(aucCmdBase64Data),bufcmd,sizeof(bufcmd),3000,IDTECH_MODE_ASCII);
	return iResult;
}

/*
    Load Init Key
    ����ANSI9.24�Ĺ���,��Key����Base Deviation Key
    �ͳ�ʼ��KSN��ɢ���ɵĳ�ʼKey
    �����ʽ����
    <STX><'F'><'F'><Command Data(BASE64)><0x0D><0x0A><ETX><LRC>
    Command Data��ʽ����:
    <FF><0A><11><LENGTH><KEY#><KEY Bytes><LRC>
    <LENGTH>:TDES:0x21 DES:0x11
    <KEY#>:TDES:0x20 DES:0x10
    <KEY Bytes>Ϊ32���ֽڵ�ASCII
    ת������ͬKSN
*/
int LoadDUKPTInitKey(uchar *pucInitiallyKey)
{
	int iResult;
    uint  i ;
    uchar aucCmdData[37];
    uchar aucCmdBase64Data[52];
    uchar aucTotalData[59];
    uchar aucRetData[50];    
    memset(aucCmdData,0x00,sizeof(aucCmdData));
    memset(aucCmdBase64Data,0x00,sizeof(aucCmdBase64Data));
    memset(aucTotalData,0x00,sizeof(aucTotalData));

    // Command Data
    aucCmdData[0] = 0xFF;
    aucCmdData[1] = 0x0A;
    // �����ֽ�
    aucCmdData[2] = 0x21;
    // TDES
    aucCmdData[3] = 0x33;

    // HEX to ASCII
    for (i = 0; i < 16; i++)
    {
        aucCmdData[4+i*2]   =  gaucHex2Ascii[(pucInitiallyKey[i] >> 4) & 0x0F];
        aucCmdData[4+i*2+1] =  gaucHex2Ascii[pucInitiallyKey[i] & 0x0F];
    }
    aucCmdData[36] = Calc_LRC(aucCmdData,sizeof(aucCmdData) - 1);


    // ת��ΪBASE64��ʽ
    to64frombits(aucCmdBase64Data,aucCmdData,sizeof(aucCmdData));
    // STX
	iResult=idtech_cmdexg('F','F',aucCmdBase64Data,sizeof(aucCmdBase64Data),bufcmd,sizeof(bufcmd),3000,IDTECH_MODE_ASCII);
	return iResult;
}
#endif
/**
��ʼ����ͷ��Կ
@param[in]iSelectKey 
         0:fixed key
         1:Dukpt key
@param[in]     iKeyLen  pucInitKey ����
@param[in]     pucInitKey д����ܴ�ͷ����Կ
@param[in]     ��ʼKSN����iSelectKeyΪ1ʱ��������
@retval # 0�ɹ�
*/
int MsrInitKey(int iSelectKey,int iKeyLen,uchar *pucInitKey,uchar *pucInitKsnIn)
{
	int iResult;
	unsigned char ikey[16];

	k_KeyStatus=-1; //����Key״̬��Ч
	memset(ikey,0,sizeof(ikey));
	enmag_pwr_handle();
	{
		spi_init();
		if(iSelectKey==0)	iResult=load_device_key(ikey,pucInitKey,INJECTION_KEY_CLEAR);
		else iResult=RESPONSE_ERROR;
	}
	return iResult;
}
/**
���´�ͷ��Կ
ֻfixed ֧�֡�
@param[in]iKeyLen ��Կ����
@param[in]     pucOldKey ����Կ
@param[in]     pucNewKey ����Կ
@retval # 0  �ɹ�
@retval # -1 ��ǰ��ͷʱDUKPT��Կ����֧�ָ���
*/
int MsrUpdateKey(int iKeyLen,uchar * pucOldKey,uchar *pucNewKey)
{
	int iResult;
	k_KeyStatus=-1; //����Key״̬��Ч

	enmag_pwr_handle();
	{
		spi_init();
		iResult=load_device_key(pucOldKey,pucNewKey,INJECTION_KEY_CLEAR);
	}
	return iResult;
}
/**
д���ͷ���к�
@param[out]  pucMsrSnOut ��ͷ���к�
@retval # >0 ��ͷ���кų���
@retval # =0 ��ͷû�д�ͷ���к�
@retval # <0 ��ȡ��ͷ��ź�ʧ��
*/
int MsrReadSn(uchar * pucMsrSnOut)
{
	int iResult;
	int ii;

	if(k_MsrSn[0]) 
	{
		//�Ѿ��������к�
		memcpy(pucMsrSnOut,k_MsrSn,8);
		return 8;
	}
	enmag_pwr_handle();
	{
		spi_init();
		if(is_idtech_header())
		{
			for(ii=0;ii<90;ii++)
			{
				iResult=idtech_cmdexg('R',IDTECH_CMD_SERIAL_NUMBER_CMD,bufcmd,0,bufcmd,32,100,IDTECH_MODE_ASCII);
				if(iResult > 0)	break;
			}
			if(iResult<=0)
			{
				iResult =-1;				
			}
			else
			{
				if(bufcmd[2]==8)
				{
					memcpy(pucMsrSnOut,&bufcmd[3],bufcmd[2]);
					iResult=bufcmd[2];
				}
				else if(bufcmd[2]==10)
				{
					memcpy(pucMsrSnOut,&bufcmd[3+2],8);
					iResult=8;
				}
				else return -2;
			}
		}
		else if(is_magtek_header())
		{
		    bufcmd[0] = KEY_SERIAL_NUMBER;
			iResult=magtek_cmdexg(GET_PROPERTY_CMD,bufcmd,1,bufcmd,sizeof(bufcmd),3000);
			if(iResult==10)	
			{
				memcpy(pucMsrSnOut,bufcmd+2,iResult-2);
				iResult=iResult-2;
			}
			else iResult=-3;
		}
		else iResult=-1;
		
	}
	if(memcmp(pucMsrSnOut,"\x00\x00\x00\x00\x00\x00\x00\x00",8)==0)
	{
		return 0;
	}
	if(iResult==8)
	{
		//���кŶ����ɹ�
		memcpy(k_MsrSn,pucMsrSnOut,8);
	}
	return iResult;
}
/**
д���ͷ���к�
@param[in]iMsrSnLen ��ͷ���кų��� 
@param[out]  pucMsnSnIn ��ͷ���к�
@retval # 0 �ɹ�
*/
int MsrWriteSn(int iMsrSnLen,uchar * pucMsnSnIn)
{
	int iResult;
	unsigned char buf[32];


	memset(k_MsrSn,0,sizeof(k_MsrSn)); //�������к���Ч
	
	enmag_pwr_handle();
	spi_init();
	if(is_idtech_header())
	{
		buf[0]=iMsrSnLen;
		memcpy(buf+1,pucMsnSnIn,sizeof(buf)-1);
		iResult=idtech_cmdexg('S',IDTECH_CMD_SERIAL_NUMBER_CMD,buf,iMsrSnLen+1,NULL,0,3000,IDTECH_MODE_ASCII);
	}
	else if(is_magtek_header())
	{
		iResult=set_magtek_key_serial_number(pucMsnSnIn);
		save_magtek_nonvalatile_data();
	}
	else 
	{
		iResult=RESPONSE_ERROR;
	}
	return iResult;
}
/**
��ȡ���ܴ�ͷ��Կ��Կ״̬
@retval # 0 ��ǰ��ͷδд����Կ��
@retval # 1 ��ǰ��ͷ��ע��̶���Կ��
@retval # 2 ��ǰ��ͷ��ע��DUKPT��Կ��
*/
int MsrReadKeyStatus(void)
{
	int iResult;
	uchar initkey[16];
	if(k_KeyStatus>=0) return k_KeyStatus;
	//id tech��magtek��ͷ��ʼ��Կ����0
	enmag_pwr_handle();
	spi_init();
#if TYPE_FIXKEY
		memset(initkey,0,sizeof(initkey));
		iResult=external_authenticate(initkey);
		k_KeyStatus=iResult;
		return iResult;		
#endif

	return RESPONSE_ERROR;
}
/**
��֤PED�д洢����Կ�ʹ�ͷ����Կ�Ƿ�һ�¡�
@retval # 0 �ɹ�
*/
int MsrVerifyKey(int iSelectKey,int iKeyLen,uchar *pucMsrKey,uchar *pucKsnIn)
{
#if TYPE_FIXKEY
		int iResult;
		enmag_pwr_handle();
		spi_init();
		iResult=external_authenticate(pucMsrKey);
		return iResult;
#endif
	return RESPONSE_ERROR;
}

static void spi_init_download(void)
{
    gpio_set_pin_type(GPIOA,4,GPIO_FUNC0);  
    gpio_set_pin_type(GPIOA,5,GPIO_FUNC0);  
    gpio_set_pin_type(GPIOA,6,GPIO_FUNC0);  
    spi_config(0,80*1000,8,1);
}

static int SendTo1601(uchar *Data,int len)
{
    volatile ushort data = 0;
    volatile uint timeOut = s_Get10MsTimerCount()+2000;//20���ӳ�ʱ
    uchar para[13],cmdbuf[128],cs;
    int i = 0,j=0,ret=0,count;

#define CSN_HIGH  gpio_set_pin_val(GPIOA,7,1)
#define CSN_LOW  gpio_set_pin_val(GPIOA,7,0)   

    if(len <= 0) return len;
    
    encryptmag_close();//����ͷ�µ�
    disable_ts_spi();//disable TouchScreen
    CSN_HIGH;//disable mag spi
    DelayMs(500);
    spi_init_download();
    enable_encryptmag_power();
    CSN_LOW;//enable mag spi
    DelayMs(2);

    //1 ��������10��0xFE����ͷ��������10��0xCC
    for(count=0;count<10;)
    {
        if(s_Get10MsTimerCount()>timeOut)
        {
            ret = -1;
            goto exit;
        }
        
        DelayUs(100);
        CSN_LOW;
        data = spi_txrx(0,0xFE,8);
        CSN_HIGH;
        if((data & 0xff) == 0xCC) count ++;
        else count = 0;
    }

    //2  ��������10��0xBB����ͷ����13���ֽ�оƬ����
    for(count = 0; count < 10; count ++)
    {
        DelayUs(100);
        CSN_LOW;
        data = spi_txrx(0,0xBB,8);
        CSN_HIGH;
    }

    //����оƬ������13�ֽ�,��˸�ʽ
    /*
    Chip ID(1B):Idle Char(1B):Code Size(2B):CRC(2B):Reserved(5B):Valid Flag(2B)

    Valid Flag:55 AA
    */
    for(count = 0,i=0; count < 200; count ++)
    {
        DelayUs(100);
        CSN_LOW;
        data = spi_txrx(0,0,8);
        CSN_HIGH;
        if(((data & 0xff) == 0xF1) && (count == 0)) continue;

        para[i++] = (0xff & data);
        if(i >= 13)
        {
            //if((para[11] == 0x55) && (para[12] == 0xAA))break;	//Modified by srain
            if (para[0] == 0xA8)break;	//�ж�оƬID
            ret = -2;
            goto exit;
        }                    
    }
                
    //3 ����11�ֽڲ���
    /*
    CMD(1B):Code Size(2B):CRC(2B):Reserved(5B):CS(1B)
    */

    cmdbuf[0] = 0;
    cmdbuf[1] = (len>>8)&0xff;
    cmdbuf[2] = len&0xff;
    data = Cal_CRC16_PRE(Data,len,0);
    cmdbuf[3] = (data >> 8)&0xff;
    cmdbuf[4] = data & 0xff;
                
    for(i = 0; i < 5; i++) cmdbuf[5+i]=para[6+i];    
    if (cmdbuf[9] == 0) cmdbuf[9] = 0xE2;    
    for(i=1,cmdbuf[10]=0;i<10;i++) cmdbuf[10] += cmdbuf[i];

    for(i = 0; i < 11; i++)
    {
        DelayUs(100);
        CSN_LOW;
        data = spi_txrx(0,cmdbuf[i],8);
        CSN_HIGH;
    }
                
    while((data & 0xff) == 0xF1)
    {
        if(s_Get10MsTimerCount()>timeOut)
        {
            ret = -3;
            goto exit;
        }
        DelayUs(100);
        CSN_LOW;
        data = spi_txrx(0,0xf1,8);
        CSN_HIGH;
    }

    //���մ�ͷ�ظ�,0xAAΪ�����ɹ�
    if((data & 0xff) != 0xAA)
    {
        ret = -4;
        goto exit;
    }
                
    //4 ���ͳ����
    /*
    CODE(128B):CS(1B)
    */
    for(count=0;count<len;count+=128)
    {
        j = len-count;
        if(j>128)j=128;
        for(i=0,cs=0;i<j;i++)
        {
            DelayUs(100);
            cs += Data[count+i] & 0xff;
            data = (ushort)Data[count+i];
            CSN_LOW;
            spi_txrx(0,data,8);
            CSN_HIGH;
        }

        DelayUs(100);
        CSN_LOW;
        data = spi_txrx(0,cs,8);
        CSN_HIGH;

        while((data & 0xff) == 0xF1)
        {
            if(s_Get10MsTimerCount()>timeOut)
            {
                ret = -5;
                goto exit;
            }
            DelayUs(100);
            CSN_LOW;
            data = spi_txrx(0,0xf1,8);
            CSN_HIGH;
        }

        if((data & 0xff) != 0xAA)
        {
            ret = -6;
            goto exit;
        }
    }
    
exit:
    CSN_HIGH;//disable mag spi
	if (GetTsType()>0) enable_ts_spi();  //enable TouchScreen  
    return ret;
}

//��������Ѷ��ͷ���¹̼�
int MsrLoadFirmware(uchar *data, int len)
{
    uchar ucSoftwareID[200];
    int ret;
    
    if (strlen(k_MsrVer) && 
        !strstr(k_MsrVer, "MEGAHUNT")) return 0; //����Ѷ��ͷ�������
    MsrReadSn(ucSoftwareID);
    ret=SendTo1601(data, len);
    encryptmag_close();//����ͷ�µ�
    spi_init();
    enable_encryptmag_power();    
    if(MsrReadSn(&ucSoftwareID[100]) <= 0)
    {
        MsrWriteSn(8, ucSoftwareID);
    }

    encryptmag_init();

    return ret;
}
int GetMsrVer(uchar *verbuf)
{
    strcpy(verbuf, k_MsrVer);
    return strlen(k_MsrVer);
}         

int iStrXor(ulong ulInOutLen,uchar * pszInOut,ulong ulInLen,uchar * pszIn) {
	int iLoop;
	if(ulInOutLen<ulInLen)
		return -1;
	for(iLoop=0;iLoop<ulInLen;iLoop++)
	{
		pszInOut[iLoop]=pszInOut[iLoop]^pszIn[iLoop];
	}
}

/**
	DEA CBC����
@param[in]pucIV ��ʼ����,��pucIV ΪNULLʱ����ȫ0x00��Ϊ��ʼ����
@param[in] pszIn  :�����ܻ��߽��ܵ�����
@param[in] iDataLen:�����ܻ��߽��ܵ����ݳ��ȣ�
@param[in] pszKey:8�ֽ���Կ
@param[in] ucMode: 
			\li #PED_ENCRYPT ����
			\li #PED_DECRYPT ����
			\li �Ƿ�
@param[out]pszOut:���ܻ��߼��ܺ������			
@retval 0			
*/
int  iDesCBC(uchar * pucIV,uchar * pszIn,uchar * pszOut,int iDataLen,uchar * pszKey,uchar ucMode)
{
	uchar aucIV[8];
	int iLoop;
	
	if(iDataLen%8!=0) return -1;
	if(pucIV==NULL)memset(aucIV,0x00,8);
	else memcpy(aucIV,pucIV,8);
	
	if(ucMode==PED_ENCRYPT)
	{
		for(iLoop=0;iLoop<iDataLen/8;iLoop++)
		{
			if(iLoop==0) memcpy(pszOut,aucIV,8);	
			else        memcpy(pszOut+iLoop*8,pszOut+(iLoop-1)*8,8);

			iStrXor(8, pszOut+iLoop*8, 8, pszIn+iLoop*8);
			des(pszOut+iLoop*8,pszOut+iLoop*8,pszKey,PED_ENCRYPT);
		}
	}
	else if(ucMode==PED_DECRYPT)
	{
		for(iLoop=0;iLoop<iDataLen/8;iLoop++)
		{
			des(pszIn+iLoop*8,pszOut+iLoop*8,pszKey,PED_DECRYPT);

			if(iLoop==0) iStrXor(8,pszOut,8,aucIV);
			else         iStrXor(8,pszOut+iLoop*8,8,pszIn+(iLoop-1)*8);
		}

	}
	else return -2;

	return 0;
}

/**
	TDEA TCBC����
@param[in]pucIV ��ʼ����,��pucIV ΪNULLʱ����ȫ0x00��Ϊ��ʼ����
@param[in] pszIn  :�����ܻ��߽��ܵ�����
@param[in] iDataLen:�����ܻ��߽��ܵ����ݳ��ȣ�
@param[in] pszKey:��Կ,16/24�ֽ�
@param[in] iKeyLen:��Կ����\li 8  \li 16 \li 24 \li �Ƿ�
@param[in] ucMode: 
			\li #PED_ENCRYPT ����
			\li #PED_DECRYPT ����
			\li �Ƿ�
@param[out]pszOut:���ܻ��߼��ܺ������			
@retval 0			
*/
static int  iTCBC(uchar * pucIV,uchar * pszIn,uchar * pszOut,int iDataLen,uchar * pszKey,int iKeyLen,uchar ucMode)
{
	uchar ucTDesMode, aucIV[8];
	int iLoop;

 	if(iDataLen%8!=0) return -1;
	if(iKeyLen==8)
	{
		return iDesCBC(pucIV,pszIn,pszOut,iDataLen,pszKey,ucMode);
	}
	
	if(16!=iKeyLen && 24!=iKeyLen) return -3;
	if(pucIV==NULL)memset(aucIV,0x00,8);
	else memcpy(aucIV,pucIV,8);
	
	if(ucMode==PED_ENCRYPT)
	{
		for(iLoop=0;iLoop<iDataLen/8;iLoop++)
		{
			if(iLoop==0) memcpy(pszOut,aucIV,8);	
			else         memcpy(pszOut+iLoop*8,pszOut+(iLoop-1)*8,8);

			iStrXor(8, pszOut+iLoop*8, 8, pszIn+iLoop*8);
			des(pszOut+iLoop*8,pszOut+iLoop*8,pszKey,PED_ENCRYPT);			
			des(pszOut+iLoop*8,pszOut+iLoop*8,pszKey+8,PED_DECRYPT);

			if(iKeyLen==24) des(pszOut+iLoop*8,pszOut+iLoop*8,pszKey+16,PED_ENCRYPT);
			else            des(pszOut+iLoop*8,pszOut+iLoop*8,pszKey,PED_ENCRYPT);
		}
	}
	else if(ucMode==PED_DECRYPT)
	{
		for(iLoop=0;iLoop<iDataLen/8;iLoop++)
		{
			if(iKeyLen==24) des(pszIn+iLoop*8,pszOut+iLoop*8,pszKey+16,PED_DECRYPT);
			else			des(pszIn+iLoop*8,pszOut+iLoop*8,pszKey,PED_DECRYPT);

			des(pszOut+iLoop*8,pszOut+iLoop*8,pszKey+8,PED_ENCRYPT);
			des(pszOut+iLoop*8,pszOut+iLoop*8,pszKey,PED_DECRYPT);

			if(iLoop==0) iStrXor(8,pszOut,8,aucIV);
			else         iStrXor(8,pszOut+iLoop*8,8,pszIn+(iLoop-1)*8);
		}
	}
	else return -2;

	return 0;
}


/**
	Des ���㣬�����ܻ��߽�������
@param[in] pszIn  :�����ܻ��߽��ܵ�����
@param[in] iDataLen:�����ܻ��߽��ܵ����ݳ��ȣ�
@param[in] pszKey:��Կ,8�ֽ�
@param[in] ucMode: 
			\li #PED_ENCRYPT ����
			\li #PED_DECRYPT ����
			\li �Ƿ�
@param[out]pszOut:���ܻ��߼��ܺ������			
@retval 0			
*/
int  iDes(uchar * pszIn,uchar * pszOut,int iDataLen,uchar * pszKey,uchar ucMode)
{
	int iLoop;
	
	if(pszIn==NULL||pszOut==NULL||pszKey==NULL) return -1;
	if(ucMode==PED_ENCRYPT)
	{
		for(iLoop=0;iLoop<iDataLen/8;iLoop++)
		{
			des(pszIn+iLoop*8,pszOut+iLoop*8,pszKey,PED_ENCRYPT);
		}
	}
	else if(ucMode==PED_DECRYPT)
	{
		for(iLoop=0;iLoop<iDataLen/8;iLoop++)
		{
			des(pszIn+iLoop*8,pszOut+iLoop*8,pszKey,PED_DECRYPT);
		}
	}
	else return -2;

	for(iLoop=0;iLoop<iDataLen%8;iLoop++)
	{
		pszOut[(iDataLen/8)*8+iLoop]=pszIn[(iDataLen/8)*8+iLoop]^0xff;
	}
	return 0;
}

/**
	3Des ���㣬�����ܻ��߽�������
	iKeyLen Ϊ8�ֽ�ʱ��DES�㷨
@param[in] pszIn  :�����ܻ��߽��ܵ�����
@param[in] iDataLen:�����ܻ��߽��ܵ����ݳ��ȣ�
@param[in] pszKey:��Կ,8/16/24�ֽ�
@param[in] iKeyLen:��Կ����\li 8  \li 16 \li 24 \li �Ƿ�
@param[in] ucMode: 
			\li #PED_ENCRYPT ����
			\li #PED_DECRYPT ����
			\li �Ƿ�
@param[out]pszOut:���ܻ��߼��ܺ������			
@retval 0			
*/
static int  iTdes(uchar * pszIn,uchar * pszOut,int iDataLen,uchar * pszKey,int iKeyLen,uchar ucMode)
{
	int iLoop;
	uchar ucTDesMode;
	if(iDataLen%8!=0) return -1;
	if(8==iKeyLen)
	{
		return iDes(pszIn,pszOut,iDataLen,pszKey,ucMode);
	}
	if(ucMode==PED_ENCRYPT)
	{
		for(iLoop=0;iLoop<iDataLen/8;iLoop++)
		{
			des(pszIn+iLoop*8,pszOut+iLoop*8,pszKey,PED_ENCRYPT);
			des(pszOut+iLoop*8,pszOut+iLoop*8,pszKey+8,PED_DECRYPT);
			if(iKeyLen==24) des(pszOut+iLoop*8,pszOut+iLoop*8,pszKey+16,PED_ENCRYPT);
			else            des(pszOut+iLoop*8,pszOut+iLoop*8,pszKey,PED_ENCRYPT);
		}
	}
	else if(ucMode==PED_DECRYPT)
	{
		for(iLoop=0;iLoop<iDataLen/8;iLoop++)
		{
			if(iKeyLen==24) des(pszIn+iLoop*8,pszOut+iLoop*8,pszKey+16,PED_DECRYPT);
			else			des(pszIn+iLoop*8,pszOut+iLoop*8,pszKey,PED_DECRYPT);
			des(pszOut+iLoop*8,pszOut+iLoop*8,pszKey+8,PED_ENCRYPT);
			des(pszOut+iLoop*8,pszOut+iLoop*8,pszKey,PED_DECRYPT);
		}
	}
	else return -2;

	return 0;
}


int GetMsrKey(uchar* pucMsrSnIn, uchar *pucMsrKeyOut,uchar *pucMsrKeyKsn)
{
    #define MSR_INIT_KSN "\xff\xff\x98\x76\x54\x32\x10\xe0\x00\x00"
    int iMsrKeyLen;
    uchar ucMsrRootKey1[24],ucMsrRootKey2[24],aucRootKey[24];

    iMsrKeyLen = GetMsrRootKey(ucMsrRootKey1,ucMsrRootKey2);
	memcpy(aucRootKey,ucMsrRootKey1,iMsrKeyLen);
	
	iTCBC(NULL,ucMsrRootKey1,aucRootKey,16,ucMsrRootKey2,16,PED_ENCRYPT);
	iTdes(pucMsrSnIn,pucMsrKeyOut,8,aucRootKey,iMsrKeyLen,PED_ENCRYPT);
	iTdes(pucMsrSnIn,pucMsrKeyOut+8,8,aucRootKey,iMsrKeyLen,PED_DECRYPT);
	memset(aucRootKey,0x00,sizeof(aucRootKey));
	memcpy(pucMsrKeyKsn,MSR_INIT_KSN,10);

	return iMsrKeyLen;	
}

/**
��ȡ���ܴ�ͷ��Կ��
�ýӿ�ֻ���ſ�����ʹ�ã����ṩ��Ӧ�ò㡣
@param[in]iSelectKey 
	0:fixed key
	1:Dukpt key
@param[out]	pucKeyOut ��ǰmsrkey
@param[in]	pucKsnIn,��iSelectKeyΪ1ʱ�������壬���뵱ǰKSN
@retval # >0 pucKeyOut����
*/
int ReadMsrKey(int iSelectKey,uchar*pucKeyOut,uchar*pucKsnIn)
{
#define MSR_SN_LEN 8
	int iRet,iLoop;
	uchar aucMsrSn[MSR_SN_LEN+1], aucInitKsn[10];
	uchar aucTmp[MSR_SN_LEN], ucKeyLen, aucKey[24];

	if(pucKeyOut==NULL) return -1;
	if(iSelectKey != 0) return -2;

	memset(aucKey,0x00,sizeof(aucKey));
    memset(aucMsrSn,0x00,sizeof(aucMsrSn));
    iRet = MsrReadSn(aucMsrSn);    //get msr sn
    if(iRet==0)
    {
        GetRandom(aucMsrSn);
        for(iLoop=0;iLoop<MSR_SN_LEN;iLoop++)
        {//ת���ɿɼ��ַ�
            aucMsrSn[iLoop]=aucMsrSn[iLoop]%10+0x30;
        }
        MsrWriteSn(MSR_SN_LEN, aucMsrSn);
        memset(aucTmp,0x00,sizeof(aucTmp));
        iRet = MsrReadSn(aucTmp);
        if(memcmp(aucTmp,aucMsrSn,MSR_SN_LEN)!=0) return -3;//MSR_STATUS_INVALID;
    }    
    else if(iRet != MSR_SN_LEN) return -3;//MSR_STATUS_INVALID;    

    //get fixed MSR key
    ucKeyLen = GetMsrKey(aucMsrSn, aucKey, aucInitKsn);
    iRet =MsrReadKeyStatus();
    if (iRet==MSR_KEY_STATUS_NO_KEY)//δע����Կ
    {
        MsrInitKey(iSelectKey, ucKeyLen, aucKey,aucInitKsn);
    }
    else if(iRet!=MSR_KEY_STATUS_FIXED && iRet!=MSR_KEY_STATUS_DUKPT) return  -3;//MSR_STATUS_INVALID;
  
	memcpy(pucKeyOut, aucKey, ucKeyLen);
	return ucKeyLen;
}

/**
�ô洢��PED�Ĵ�ͷ��Կ���ӽ�������
@param[in]iSelectKey 
	0:fixed key
	1:Dukpt key
@param[in]	pucKsnIn ��iSelectKeyΪ1ʱ�������壬���뵱ǰKSN
@param[in]	pucKeyVar,���ڼӽ������ݵ���Կ������pucKeyVarΪNULLʱ���ô洢����Կֱ�Ӽӽ���
@param[in] pucIV, ��ʼ��������ΪNULLʱ��ʹ��ȫ0��Ϊ��ʼ����
@param[in] iDataLen ���ݳ���
@param[in]pucDataIn �������ݣ������ܻ��߽��ܵ�����
@param[out]pucDataOut ������ܺ���߽��ܺ������
@param[in] Mode ���ܣ�0x01
				���ܣ�0x00 [����]
@retval # 0�ɹ�
*/
static int MsrKeyDes(int iSelectKey,uchar*pucKsnIn,uchar *pucKeyVar,uchar *pucIV,int iDataLen,uchar *pucDataIn,uchar*pucDataOut,uchar Mode)
{
	uchar aucMsrKey[24];
	int iRet,iKeyLen;
	
	iKeyLen =ReadMsrKey(iSelectKey,aucMsrKey,pucKsnIn);
	if(iKeyLen<0) return iKeyLen;
	if(pucKeyVar!=NULL)
	{
		iStrXor(iKeyLen,aucMsrKey,iKeyLen,pucKeyVar);
	}
	
    iRet=iTCBC(pucIV,pucDataIn,pucDataOut,iDataLen,aucMsrKey,iKeyLen,Mode);
    
	return iRet;
}

