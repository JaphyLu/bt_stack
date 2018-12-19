#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "Base.h"
#include "Kb.h"
#include "kb_touch.h"
#include "posapi.h"
#include "..\lcd\lcdapi.h"


//for Sxxx
//����ϼ��ļ����
static  uchar Key_Tab_Sxxx[]={
	KEY1,			KEY4,		KEY7,		KEYFN,      KEYCANCEL,
    KEY2,           KEY5,       KEY8,       KEY0,	    KEYCLEAR,
    KEY3,		    KEY6,       KEY9,		KEYALPHA,	0,
    KEYATM1,	    KEYUP, 		KEYDOWN,	KEYMENU,	KEYENTER,
    NOKEY,	        NOKEY, 		NOKEY,	    NOKEY,	    NOKEY,
};
//��ϰ����ļ����
static  uchar FKey_Tab_Sxxx[]={ 
	FNKEY1,			FNKEY4,		FNKEY7,		0,	        FNKEYCANCEL,
    FNKEY2,         FNKEY5,     FNKEY8,     FNKEY0,	    FNKEYCLEAR,
    FNKEY3,		    FNKEY6,	    FNKEY9,	    FNKEYALPHA,	0,
    FNKEYATM1,      FNKEYUP,	FNKEYDOWN,	FNKEYMENU,	FNKEYENTER,
    NOKEY,	        NOKEY, 		NOKEY,	    NOKEY,	    NOKEY,
};
//��ϰ����ļ����
static  uchar EKey_Tab_Sxxx[]={ 
    //ENTER
    ENKEY1,			ENKEY4,		ENKEY7,		FNKEYENTER,	ENKEYCANCEL,
    ENKEY2,         ENKEY5,     ENKEY8,     ENKEY0,	    ENKEYCLEAR,
    ENKEY3,		    ENKEY6,	    ENKEY9,	    ENKEYALPHA,	0,
    ENKEYATM1,      ENKEYUP,	ENKEYDOWN,	ENKEYMENU,	0,//KEYENTER
    NOKEY,	        NOKEY, 		NOKEY,	    NOKEY,	    NOKEY,
};

//for Sxx
//����ϼ��ļ����
static  uchar Key_Tab_Sxx[]={
	KEY1,			KEY4,		KEY7,		KEY0, 		
	KEY2,			KEY5,		KEY8,		0, 		 
	KEY3,			KEY6,		KEY9,		0,
	KEYCANCEL,		KEYENTER,	KEYCLEAR, 	0,
	KEYATM1, 		KEYATM3,	KEYFN,    		
	KEYATM2,		KEYATM4,	KEYMENU,		
	KEYUP,			KEYDOWN,	KEYALPHA 					
};
//��ϰ����ļ����
static  uchar FKey_Tab_Sxx[]={ 
	FNKEY1,			FNKEY4,		FNKEY7,		FNKEY0,
	FNKEY2,			FNKEY5,		FNKEY8,		0,      	
	FNKEY3,			FNKEY6,		FNKEY9,		0,
	FNKEYCANCEL,	FNKEYENTER,	FNKEYCLEAR,	0,
	FNKEYATM1,		FNKEYATM3,	0,		
	FNKEYATM2,		FNKEYATM4, 	FNKEYMENU,		
	FNKEYUP,		FNKEYDOWN,	FNKEYALPHA							
};

//��ϰ����ļ����
static  uchar EKey_Tab_Sxx[]={ 
	ENKEY1,			ENKEY4,		ENKEY7,		ENKEY0,
	ENKEY2,			ENKEY5,		ENKEY8,		0,      	
	ENKEY3,			ENKEY6,		ENKEY9,		0,
	ENKEYCANCEL,	0,	        ENKEYCLEAR,	0,
	ENKEYATM1,		ENKEYATM3,	0,		
	ENKEYATM2,		ENKEYATM4, 	ENKEYMENU,		
	ENKEYUP,		ENKEYDOWN,	ENKEYALPHA							
};

static void s_PowerKey_Int(void);
static void s_PowerKeyInit(void);

// ���ӵ���ϼ�
static unsigned int KEYFN_SCAN_BITMAP;		//FN ����Ӧ�İ���λͼ�е�λ�� ��s_Kb_Normal_Init()�г�ʼ��	   
//��ͨ��������

#define KEYFN_PRESS (1U <<31)
#define KEYENTER_PRESS  (1U <<30)
#define KEY_INVALID_BITMAP        0xFF   //��Ч����Ӧ��λͼ

#define WAITFOR_PRESS     0     // �ȴ������¼�
#define WAITFOR_RELEASE   1     // �ȴ������ͷ��¼�

#define  KB_BUF_MAX          (32)
volatile uchar    kb_buf[KB_BUF_MAX]; /* ��ȡ��ֵ������ */
volatile uchar    kb_in_ptr;          /* �����ַָ��   */
volatile static uchar    kb_out_ptr;         /* ������ַָ��   */
volatile static uchar    kb_buf_over;        /* ��ֵ����������� */
volatile static int   kb_Buffer_Busy;

volatile static int   kb_TimerStep;	   //��¼��ǰɨ�趨ʱ�����еĽ׶�	
volatile static int   kb_OffTimerCount;   //��¼ɨ�赽nokey�Ĵ���
volatile static int   kb_OnTimerCount;//��¼ɨ�赽key�Ĵ���

static volatile int   kb_keyCombFlag;		/* key combination on/off flag. 0-Off; 1-On. */
static volatile int   kb_EnterkeyCombFlag;		/* key combination on/off flag. 0-Off; 1-On. */
static volatile int   kb_save_keyCombFlag;		/*�������µ��ſ��ڼ���ϼ������ñ�־*/
static volatile int   kb_save_enterkeyCombFlag;		/*�������µ��ſ��ڼ���ϼ������ñ�־*/

extern const uint BeepFreqTab[8];
	
volatile static int   kb_PressStatus;  //�ṩ��ϵͳ����ʱʹ��

static volatile uchar gucPowerKeyDownFlag = 0;
static volatile uint guiPowerKeyCount = 0;
 volatile unsigned int    k_ScrBackLightTime;
 volatile unsigned int    k_LightTime;
 volatile unsigned int    k_KBLightTime;
 volatile unsigned char   k_KBLightMode;
static uchar kblightctrl = 0; // For D200 touch key backlight control
static uchar kblightstat = 0;

extern  unsigned char    k_ScrBackLightMode;
extern OS_TASK *g_taskpoweroff;


typedef struct {
	int fun_bitmap_num;
	uint key_gpioin_mask;
	uint key_gpioout_mask;
	int key_in_start;
	int key_in_end;
	int key_out_start;
	int key_out_end;
	int atm_in_start;
	int atm_in_end;
	int atm_out_start;
	int atm_out_end;
	uchar *Key_Tab;
	uchar *FKey_Tab;
	uchar *EKey_Tab;
}T_KbDrvConfig;
//for SXXX
T_KbDrvConfig Sxxx_KbDrvConfig = 
{
	.fun_bitmap_num = 3,
	.key_gpioin_mask = (0x00F00000),//bit20~23
	.key_gpioout_mask = (0x000F8000),//bit15~19
	.key_in_start = 20,
	.key_in_end = 23,
	.key_out_start = 15,
	.key_out_end = 19,
	.atm_in_start = 255,
	.atm_in_end = 0,
	.atm_out_start = 0,
	.atm_out_end = 0,
	.Key_Tab = Key_Tab_Sxxx,
	.FKey_Tab = FKey_Tab_Sxxx,
	.EKey_Tab = EKey_Tab_Sxxx,
};
//for SXX
T_KbDrvConfig Sxx_KbDrvConfig = 
{
	.fun_bitmap_num = 18,
	.key_gpioin_mask = (0x07F00000),//bit20~23
	.key_gpioout_mask = (0x380F0000),//bit15~19
	.key_in_start = 20,
	.key_in_end = 23,
	.key_out_start = 16,
	.key_out_end = 19,
	.atm_in_start = 24,
	.atm_in_end = 26,
	.atm_out_start = 27,
	.atm_out_end = 29,
	.Key_Tab = Key_Tab_Sxx,
	.FKey_Tab = FKey_Tab_Sxx,
	.EKey_Tab = EKey_Tab_Sxx,
};
T_KbDrvConfig *KbDrvConfig = NULL;
static  uchar Key_Tab[25];
static uchar FKey_Tab[25];
static uchar EKey_Tab[25];
static void s_Kb_Normal_Init(void);
static void s_KbInit_touchkey(void);
void s_KbInit(void)
{
	int mach = get_machine_type();
	//s_BeepInit();
	if(get_machine_type() == D200)
	{
		kb_i2c_config();
		if (s_GetBatteryEntryFlag() == 0)
		{
			s_PmuInit();
			return;
		}
		s_PowerInit();
		s_KbInit_touchkey();
		return ;
	}

	if (mach == S300 || mach == S800 || mach == S900 || mach == D210)
	{
		KbDrvConfig = &Sxxx_KbDrvConfig;
	}
	else if (mach == S500)
	{
		KbDrvConfig = &Sxx_KbDrvConfig;
	}
	if (KbDrvConfig != NULL)
	{
		memset(Key_Tab , 0, sizeof(Key_Tab));
		memcpy(Key_Tab, KbDrvConfig->Key_Tab, sizeof(Key_Tab));
		memset(FKey_Tab , 0, sizeof(FKey_Tab));
		memcpy(FKey_Tab, KbDrvConfig->FKey_Tab, sizeof(FKey_Tab));
        memset(EKey_Tab , 0, sizeof(EKey_Tab));
        memcpy(EKey_Tab, KbDrvConfig->EKey_Tab, sizeof(EKey_Tab));
    }
	
	/*ʹ�ܵ�Դ*/
	gpio_set_pin_val(POWER_HOLD, 1);
	DelayUs(1);
	gpio_set_pin_type(POWER_HOLD, GPIO_OUTPUT);
	s_Kb_Normal_Init();
	s_StopBeep();
        
	k_LightTime         = 6000;//��1���ӣ��Զ�Ϩ��
	k_ScrBackLightTime  = k_LightTime;//
	k_KBLightTime       = k_LightTime;//S80û�а�������
	k_KBLightMode       = 1;
	
	if (mach == S500)
	{
		s_PowerKeyInit();
	}
}


#define FUN_BITMAP_NUM (KbDrvConfig->fun_bitmap_num)//FN key BITMAP����λ��,�ӵ�·
									//ͼ��һ������ʼ������ʼΪ1
#define KEY_GPIOIN_MASK     (KbDrvConfig->key_gpioin_mask)    //������߶�Ӧ��GPIO	 BITs 
#define KEY_GPIOOUT_MASK    (KbDrvConfig->key_gpioout_mask)    //������߶�Ӧ��GPIO	 BITs 

#define KEY_IN_START  		(KbDrvConfig->key_in_start)       //��ͨ������������߶�Ӧ��GPIO����ʼ���
#define KEY_IN_END      	(KbDrvConfig->key_in_end)       //��ͨ������������߶�Ӧ��GPIO�Ľ������
#define KEY_OUT_START 		(KbDrvConfig->key_out_start)       //��ͨ������������߶�Ӧ��GPIO����ʼ���
#define KEY_OUT_END 		(KbDrvConfig->key_out_end)       //��ͨ������������߶�Ӧ��GPIO�Ľ������

#define ATM_IN_START  		(KbDrvConfig->atm_in_start)       //ATM����������߶�Ӧ��GPIO����ʼ���
#define ATM_IN_END      	(KbDrvConfig->atm_in_end)       //ATM����������߶�Ӧ��GPIO�Ľ������
#define ATM_OUT_START 		(KbDrvConfig->atm_out_start)       //ATM����������߶�Ӧ��GPIO����ʼ���
#define ATM_OUT_END 		(KbDrvConfig->atm_out_end)       //ATM����������߶�Ӧ��GPIO�Ľ������


static void s_TimerScanKey(void);
static uint ScanKey(int mode);
static void s_Key_Int(void);
extern uchar k_Charging;
//for Sxxx
//������ͨ�����ĳ�ʼ��
static void s_Kb_Normal_Init(void)
{
	int i;
	uint temp;
	
    if (k_Charging > 0)		//Joshua _a  don't scan the key while in charge mode
    {
		gpio_set_mpin_type(GPIOB,KEY_GPIOIN_MASK|KEY_GPIOOUT_MASK,GPIO_INPUT);
		for(i=0;i<32;i++) /*config internal pull-up for input ports */
		{
			if((KEY_GPIOIN_MASK|KEY_GPIOOUT_MASK) & (1<<i))
			{
				gpio_set_pull(GPIOB,i, 1);
				gpio_enable_pull(GPIOB,i);
			}
		}
		gpio_set_pin_type(GPIOB, KEY_OUT_END, GPIO_OUTPUT);
		gpio_set_pin_val(GPIOB, KEY_OUT_END, 0);
		gpio_disable_pull(GPIOB, KEY_OUT_END);
    }
	else
	{
		gpio_set_mpin_type(GPIOB,KEY_GPIOIN_MASK,GPIO_INPUT);
		gpio_set_mpin_type(GPIOB,KEY_GPIOOUT_MASK,GPIO_OUTPUT);
		 // Configure the GPIOs as Output Pin, and OutPut 0
		gpio_set_mpin_val(GPIOB,KEY_GPIOOUT_MASK,0);
		for(i=0;i<32;i++) /*config internal pull-up for input ports */
		{
			if((KEY_GPIOIN_MASK & (1<<i)))
			{
				gpio_set_pull(GPIOB,i, 1);
				gpio_enable_pull(GPIOB,i);
			}
	        
			if((KEY_GPIOOUT_MASK & (1<<i)))
			{
				gpio_disable_pull(GPIOB,i);
			}
		}	
	}
	for(i = 0; i < KB_BUF_MAX; i++)
	{
		kb_buf[i] = NOKEY;
	}
    
    KEYFN_SCAN_BITMAP=(1<<(FUN_BITMAP_NUM));
    
	kb_in_ptr = 0;
	kb_out_ptr = 0;
	kb_buf_over = 0;

	kb_PressStatus = WAITFOR_PRESS;
	kb_Buffer_Busy = 0;

	kb_TimerStep = 0;
	kb_OffTimerCount = 0;
	
	kb_keyCombFlag = 0;			//Ĭ�ϲ�������ϼ�
    kb_EnterkeyCombFlag = 0;         //Ĭ�ϲ�������ϼ�

	if (k_Charging > 0)		//Joshua _a  don't scan the key while in charge mode
		return;

	 // Configure the GPIOs as Interrupt Pin
	 for(i=0;i<32;i++)
	 {
	 	if(KEY_GPIOIN_MASK & (1<<i))
	 	{
	 		gpio_set_pin_interrupt(GPIOB,i,0); /*disable sub port interrupt*/
			writel((1<<i),GIO1_R_GRPF1_INT_CLR_MEMADDR);	// Clear Interrupt Pending
			s_setShareIRQHandler(GPIOB,i,INTTYPE_LOWLEVEL,s_Key_Int);
	 	}
	}
   	 
	 // ������ʱ���¼���20ms��ʼɨ�谴��
	s_SetTimerEvent(KEY_HANDLER,s_TimerScanKey); //4

    for(i=0;i<32;i++)
	{
	 	if(KEY_GPIOIN_MASK & (1<<i))
	 	{
	 		gpio_set_pin_interrupt(GPIOB,i,1); /*enable sub port interrupt*/
	 	}
	}
}

// �����жϷ������
static void s_Key_Int(void)
{
	uint temp,flag;
    
	irq_save(flag);
	temp =readl(GIO1_R_GRPF1_INT_MSK_MEMADDR);
	temp &=	~KEY_GPIOIN_MASK;
	writel(temp,GIO1_R_GRPF1_INT_MSK_MEMADDR);
	irq_restore(flag);

    
	kb_TimerStep = 0;
	kb_OffTimerCount = 0;
    kb_OnTimerCount = 0;
	kb_PressStatus = WAITFOR_RELEASE;
	
	/*
	�ڴ˴ΰ���ɨ������У�
	���ٽ�����ϼ�״̬�ĸı�
	*/
	kb_save_keyCombFlag = kb_keyCombFlag;
	kb_save_enterkeyCombFlag = kb_EnterkeyCombFlag;
	// ������ʱ���¼���20ms��ʼɨ�谴��
	StartTimerEvent(KEY_HANDLER,2);
}

//���̳�ʼ��ʱ���øú���
static void s_PowerKeyInit(void)
{
	gucPowerKeyDownFlag = 0;
	guiPowerKeyCount = 0;
	gpio_set_pin_type(POWER_KEY, GPIO_INPUT);
	
	writel((1<<15),GIO1_R_GRPF1_INT_CLR_MEMADDR);	// Clear Interrupt Pending
	s_setShareIRQHandler(POWER_KEY,INTTYPE_FALL,s_PowerKey_Int);
}

void s_EnterKeyCombConfig(int value)
{
    if(value != 0 && value != 1)
        return;

    kb_EnterkeyCombFlag = value;//��������
}

void s_KeyCombConfig(int value)
{
    if(value != 0 && value != 1)
        return;

    kb_keyCombFlag = value;//��������
}

/********************************************************************************
 *����ԭ��:unsigned int ScanKey(int mode)
 *�������:
 *  mode:0��ʾֻ�ǲ鿴�Ƿ��м����£���0����ϸɨ�谴��
 *
 *����:
 *  modeΪ0������0��ʾ�ް������£���0��ʾ�а������¡�
 *  modeΪ1:
 *      1.kb_save_keyCombFlagΪ0ʱ����ɨ����ϼ�������ֵΪ����ֵ�������ڶ�������򷵻�KEY_INVALID_BITMAP.
 *      2.kb_save_keyCombFlagΪ1ʱ��ɨ����ϼ�״̬��
 *          �������FN�����������£��򷵻�KEY_INVALID_BITMAP.
 *          KEY_FN_PRESSλ(31λ)��λ��ʾFN�����а���,��8λΪ����������ֵ
 *********************************************************************************/
static unsigned int ScanKey(int mode)
{
	uint temp1 = 0,temp2=0;
	uint ucRet = 0;
	volatile uint i = 0,j;
	uint invalid_flag=0,combin_flag = 0;
	int keynum=0;
	uint reg;
	
	gpio_set_mpin_val(GPIOB,KEY_GPIOOUT_MASK,0);
	temp1 = gpio_get_mpin_val(GPIOB,KEY_GPIOIN_MASK);
	temp2 = (temp1==KEY_GPIOIN_MASK);

	if(temp2)	return 0;  //no key
	if(!mode)   //�����ڷ����Ƿ��м�����
		return temp1;	//��ʾ���м�δ�ͷ�		

	//����ͨ����ɨ��
	for(i=KEY_IN_START;i<=KEY_IN_END;i++)  //����ɨ��
	{
		gpio_set_mpin_type(GPIOB,KEY_GPIOOUT_MASK,GPIO_INPUT);

		for(j=KEY_OUT_START;j<=KEY_OUT_END;j++)  //��ĳ�а���ɨ��
		{
			if(j!=KEY_OUT_START) 
			{
				gpio_set_pin_type(GPIOB,j-1,GPIO_INPUT);
			}
			gpio_set_pin_type(GPIOB,j,GPIO_OUTPUT);
			gpio_set_pin_val(GPIOB,j, 0);

			DelayUs(1);
			temp1 = gpio_get_mpin_val(GPIOB,KEY_GPIOIN_MASK);
			if(temp1 & (1<<i))  continue ;
			//ɨ�赽������������λͼ�������
			ucRet|= 1<<((i-KEY_IN_START)*(KEY_OUT_END-KEY_OUT_START+1)
			            +j-KEY_OUT_START); 
		}		
	}
	//��ATM��ɨ��
	for(i=ATM_IN_START;i<=ATM_IN_END;i++)  //����ɨ��
	{
		gpio_set_mpin_type(GPIOB,KEY_GPIOOUT_MASK,GPIO_INPUT);

		for(j=ATM_OUT_START;j<=ATM_OUT_END;j++)  //��ĳ�а���ɨ��
		{
			if(j!=ATM_OUT_START) 
			{
				gpio_set_pin_type(GPIOB,j-1,GPIO_INPUT);
			}
			gpio_set_pin_type(GPIOB,j,GPIO_OUTPUT);
			gpio_set_pin_val(GPIOB,j, 0);

			DelayUs(1);
			temp1 = gpio_get_mpin_val(GPIOB,KEY_GPIOIN_MASK);
//printk("sscc temp1:%08x,i:%d,j:%d\r\n",temp1,i,j);
			if(temp1 & (1<<i))  continue ;
//printk("temp1:%08x,i:%d,j:%d\r\n",temp1,i,j);
			 //ɨ�赽������������λͼ�������
			 temp2 = 1<<16;
			ucRet|= temp2<<((i-ATM_IN_START)*3+j-ATM_OUT_START); 
		}		
	}

	//��������߻ָ������״̬����������������ߵ�ƽ����
	gpio_set_mpin_type(GPIOB,KEY_GPIOOUT_MASK,GPIO_OUTPUT);
	gpio_set_mpin_val(GPIOB,KEY_GPIOOUT_MASK,0);

	//��key bitmap ���з�������������

    /*ֻ��FN������*/
    if((ucRet == KEYFN_SCAN_BITMAP) && (1 == kb_save_keyCombFlag))
        return KEYFN_PRESS;
    else if((ucRet == 0x80000) && (1 == kb_save_enterkeyCombFlag))//enter press
        return KEYENTER_PRESS;
	            
    if((ucRet & KEYFN_SCAN_BITMAP) && (1 == kb_save_keyCombFlag)) 
    {
        combin_flag = 2;
        ucRet &= ~(KEYFN_SCAN_BITMAP);
    }
    else if ((ucRet & 0x80000) && (1 == kb_save_enterkeyCombFlag))//enter press
    {
        combin_flag = 1;
        ucRet &= ~(0x80000);
    }

	for(i=0,j=0;i<32;i++)
	{
		if(ucRet&(1<<i)) 
		{
			j++; //��¼��������
			keynum=i+1;    //������λͼ�е�λ�ñ�ɱ��
		}
		if(j>1) //��������FN�����ж�Ϊ�Ƿ�����
		{
			invalid_flag=1;
			break;
		}
	}
//printk("scan ret:%08X,Invaild:%d,combin_flag:%d!\r\n",ucRet,invalid_flag, combin_flag);

	if(invalid_flag)  //����Ч�İ�����ϰ���
		ucRet =KEY_INVALID_BITMAP;
	else			   //�˴ΰ�����Ч����¼�°����ı��
		ucRet =(combin_flag<<30) + keynum;  //���λ�����ϼ���־������8λ��Ű������
	return ucRet;
}

static void s_TimerScanKey(void)
{
    static unsigned int kbCode0, kbCode1,kbCode2;  /*�ֱ��¼�׶�0-2�������е�ɨ����	*/
    static int fn_press = 0;
    static int enter_press = 0;
    static int exist_combine_key = 0;
    static int kb_invalid = 0;
    unsigned char uRet;
    uint reg;

    kb_OnTimerCount++;

    switch (kb_TimerStep)
    {
    case 0:  /*������������*/
        kbCode0 = ScanKey(1);
        if (kbCode0 == 0)
            kb_OffTimerCount++;
        else if (KEY_INVALID_BITMAP == kbCode0)
        {
            kb_OffTimerCount++;
            kb_invalid = 1;
        }
        else    /*ɨ�赽��Ч����*/
        {
            /*��ϼ�״̬�г�fn֮��İ������£������ϼ�״̬�м����£�
             *����case 1ȷ�ϰ���
             */
            if (kbCode0 & ~(KEYFN_PRESS | KEYENTER_PRESS))
            {
                kb_OffTimerCount = 0;
                kb_TimerStep = 1;
            }
            else
            {
                if (kbCode0 & KEYENTER_PRESS)
                {
                    enter_press = 1;
                }
                else /*ֻ��FN���£������ȴ���ϼ���һ�������º��ٽ���case 1ȷ�ϰ���*/
                {
                    fn_press = 1;
                }
                //����Fn��������
                if (k_ScrBackLightMode < 2)
                    ScrBackLight(1);
                if (k_KBLightMode < 2)
                    kblight(1);
            }
        }

        if (kb_OffTimerCount > 2)
        {
            if (fn_press | enter_press)
            {
                /*֮ǰFN���а��£�����û��������Ч��ϰ�����
                *û��ͬʱ���¶������������Ч���룬
                *��FN��ֵ������̻�����
                */
                if (!exist_combine_key && !kb_invalid)
                {
                    if ((kb_buf_over == 0) && (kb_Buffer_Busy == 0))
                    {
                        //д��FN����������
                        if (enter_press != 0)
                            kb_buf[kb_in_ptr] = 0x14; //
                        else
                            kb_buf[kb_in_ptr] = FUN_BITMAP_NUM + 1;

                        kb_in_ptr = (kb_in_ptr + 1) % KB_BUF_MAX;

                        if (kb_in_ptr == kb_out_ptr)
                            kb_buf_over = 1;
                    }

                    if (k_ScrBackLightMode < 2)
                        ScrBackLight(1);
                    if (k_KBLightMode < 2)
                        kblight(1);
                    s_TimerKeySound(0);
                }
                fn_press = 0;
                enter_press = 0;
                kb_TimerStep = 21;
            }
            else
                kb_TimerStep = 22;
            kb_OffTimerCount = 0;
        }
        break;

        case 1:      
        /*�ý׶ζ���Ч������ȷ��ɨ�裬����ȡ����ֵ*/
        kbCode1 = ScanKey(1);
        /*���û�м�ֵ����ɨ�赽���β�һ��ʱ,�˳�ɨ��*/
        if (!kbCode1 || (kbCode1 != kbCode0))
        {
            /*��case 0ɨ������һ�£���λkb_invalid*/
            kb_invalid = 1;
            if (fn_press | enter_press)
                kb_TimerStep = 21; /*FN�����£�����ȴ���ϼ��ͷŽ׶�*/
            else
                kb_TimerStep = 22; /*ֻ����ͨ�������£�����ȴ��ͷŰ����׶�*/
            break;
        }

        if ((kb_buf_over == 0) && (kb_Buffer_Busy == 0))
        {
            /*��Ű�������������*/
            /*�������ϼ������������ͨ��������FNKEY0*/
            if (kbCode1 & (1 << 31))
                kb_buf[kb_in_ptr] = (kbCode1 & (~(1 << 31))) + FNKEY0;
            else if (kbCode1 & (1 << 30))
            {
                kb_buf[kb_in_ptr] = (kbCode1 & (~(1 << 30))) + ENKEY0;
            }
            else
                kb_buf[kb_in_ptr] = kbCode1;

            kb_in_ptr = (kb_in_ptr + 1) % KB_BUF_MAX;

            if (kb_in_ptr == kb_out_ptr)
                kb_buf_over = 1;
        }

            s_TimerKeySound(0);
            if(k_ScrBackLightMode < 2)  ScrBackLight(1);
            if(k_KBLightMode < 2)	       kblight(1);

        if (kbCode1 & (1 << 31))
        {
            exist_combine_key = 1;
            kb_TimerStep = 21; /*FN�����£�����ȴ���ϼ��ͷŽ׶�*/
        }
        else if (kbCode1 & (1 << 30))
        {
            exist_combine_key = 1;
            kb_TimerStep = 21; /*FN�����£�����ȴ���ϼ��ͷŽ׶�*/
        }
        else
            kb_TimerStep = 22; /*ֻ����ͨ�������£�����ȴ��ͷŰ����׶�*/
        break;

        /* �ý׶����ڵȴ���ϼ��ͷŵ�ɨ��*/
    case 21:
        uRet = s_TimerKeySound(1);
        kbCode2 = ScanKey(1);
        /*�������Ѿ��ͷţ���FN���������£�����case 0����ɨ�谴��*/
        if (kbCode2 == KEYFN_PRESS && !uRet)
        {
            kb_TimerStep = 0;
        }
        else if (kbCode2 == KEYENTER_PRESS && !uRet)
        {
            kb_TimerStep = 0;
        }
        /*���м������ͷţ����������ɨ��*/
        else if (!kbCode2 && ++kb_OffTimerCount >= 2 && !uRet)
            goto quit_scan;
        break;

            /*�ý׶εȴ���ͨ�����ͷ�*/
        case 22:
            uRet = s_TimerKeySound(1);
            kbCode2 = ScanKey(0);
            if((kb_OnTimerCount==100)&&((get_machine_type()==S800)||(get_machine_type()==S900)||(get_machine_type()==D210)))
            {
                if((kbCode0==5)&&(kbCode1==5)&&(ScanKey(1)==5)&&g_taskpoweroff)
                    OsResume(g_taskpoweroff);
            }
            
            if(!kbCode2 && ++kb_OffTimerCount >= 2 && !uRet)
                goto quit_scan;		
            break;

        default:
            break;
    }

    return ;

    /*�����˴ΰ���ɨ�裬���жϲ��򿪶�ʱ��*/
quit_scan:
    s_TimerKeySound(2);
    StopTimerEvent(KEY_HANDLER);
    kb_PressStatus = WAITFOR_PRESS; 
    exist_combine_key = 0;
    kb_invalid = 0;
    
    /* �������ж�*/
	reg =readl(GIO1_R_GRPF1_INT_MSK_MEMADDR);
	reg |= KEY_GPIOIN_MASK;
	writel(reg,GIO1_R_GRPF1_INT_MSK_MEMADDR);
}

/*--------------------------------------------------
 ���ܣ�����ֵ���������Ƿ���δ��ȡ�ļ�ֵ
 ����ֵ�� 0XFF------�޼�ֵ
                        0X00------�м�ֵ
 ---------------------------------------------------*/
unsigned char kbhit(void)
{
	if(kb_buf[kb_out_ptr] != NOKEY)
		return 0x00;

	return NOKEY;
}


/*--------------------------------------------
  ���ܣ����ֵ�����������尴�����еĻ�����
 ---------------------------------------------*/
void kbflush(void)
{
     	uchar i;
	/* ��ע���ж�����ָ���� */
	kb_Buffer_Busy = 1;

	for(i = 0; i < KB_BUF_MAX; i++)
	{
	    kb_buf[i] = NOKEY;
	}

	kb_in_ptr = 0;
	kb_out_ptr = 0;
	kb_buf_over = 0;
	
	kb_Buffer_Busy = 0;
}


/*-----------------------------------------------------------
 ���ܣ��Ӽ��̻������ж�ȡһ��ֵ���޼�ʱ�ȴ�������ʾ����Ļ��
 ���أ�����ȡ�õļ�ֵ����
 ------------------------------------------------------------*/
unsigned char getkey(void)
{
	uchar temp,kbcode;

	while((kb_in_ptr == kb_out_ptr)&&(kb_buf_over == 0));
	kb_buf_over = 0;
	temp = kb_buf[kb_out_ptr]-1;
	if(temp<FNKEY0)
	{
		kbcode=Key_Tab[temp];
	}
	else if(temp<ENKEY0)
	{
		kbcode=FKey_Tab[temp-FNKEY0];
	}
    else
    {
		kbcode=EKey_Tab[temp-ENKEY0];
    }
	kb_buf[kb_out_ptr]=NOKEY;
	kb_out_ptr=(kb_out_ptr+1)%KB_BUF_MAX;
	return kbcode;
}

/*-----------------------------------------------------------
 ���ܣ��Ӽ��̻������ж�ȡһ��ֵ,������Ѷ�ȡ�ļ�ֵ
 ���أ�����ȡ�õļ�ֵ����
 ------------------------------------------------------------*/
unsigned char kbcheck(void)
{
	uchar temp,kbcode;

	while((kb_in_ptr == kb_out_ptr)&&(kb_buf_over == 0));
	kb_buf_over = 0;
	temp = kb_buf[kb_out_ptr]-1;
	if(temp<FNKEY0)
	{
		kbcode=Key_Tab[temp];
	}
    else if (temp<ENKEY0)
    {
		kbcode=FKey_Tab[temp-FNKEY0];
	}
    else
	{
		kbcode=EKey_Tab[temp-ENKEY0];
	}
	return kbcode;
}
/*�򰴼������������ֵ,����0��ʾ�ɹ���������ʾ����*/
int putkey(uchar key)
{
    int ret=0;
    uint x,i;

    if(!key)return 1;
    
    for(i=0; i<sizeof(Key_Tab); i++) //��������ֵ��������
	{
		if(Key_Tab[i] == key) break;
	}
	if(i == sizeof(Key_Tab)) return 2;

    irq_save(x);
    if((kb_buf_over == 0)&&(kb_Buffer_Busy == 0))
    {
        /*��Ű�������������*/
        /*�������ϼ������������ͨ��������FNKEY0*/
		kb_buf[kb_in_ptr] = i+1;
		kb_in_ptr=(kb_in_ptr+1)%KB_BUF_MAX;
		
		if(kb_in_ptr == kb_out_ptr)    
			kb_buf_over = 1;
    }
    else ret = 3;

    irq_restore(x);
    return ret;
}

/***************************************
*  UserGetKey()
*  GetKey For Application hierarchy call, with the display message register 
check.
***************************************/
uchar   UserGetKey(uchar NeedReg)
{
	return getkey();
}

uchar   PciGetKey(ucNeedDMRCheck)
{
    return(UserGetKey(ucNeedDMRCheck));
}

//  ��Ms�����ڵȴ�����������ʱ�򷵻�0. 0<= Ms <= 24*60*60*1000
//  Ms=0xFFFFFFFF��ʾ�����ó�ʱ���ܣ��˺�����Ч��getkey
//  Ms=0��ʾ������Ⲣ����

uchar GetKeyMs(uint Ms)
{
    volatile uint    Begin, Cur;

    if(Ms > 24*60*60*1000)
        Ms = 24*60*60*1000;

    Begin = GetTimerCount();

    if(Ms != 0xFFFFFFFF)
    {
        while(kbhit())
        {
            Cur = GetTimerCount();
            if((Cur-Begin) >= Ms)
                return(0);
        }
    }
    return(getkey());
}

int KbReleased(void)
{
	if(kb_PressStatus == WAITFOR_PRESS)return 1;
	return 0;
}
void s_KBlightCtrl(unsigned char OnOff)
{
	static int first_flag=0;
	if(get_machine_type() == D200)
	{
		kblightctrl = OnOff;
		return ;
	}
	if(!first_flag)
	{
		gpio_set_pin_type(GPIOD,0, GPIO_OUTPUT);
		first_flag=1;
	}
	if(!OnOff) gpio_set_pin_val(GPIOD,0, 1);
	else	gpio_set_pin_val(GPIOD,0, 0);
	kblightstat = OnOff;
}

unsigned char get_scrbacklightmode(void)
{
	return k_ScrBackLightMode;
}
unsigned char get_kblightmode(void)
{
	return k_KBLightMode;
}
static void s_PowerKey_Int(void)
{
    uint uiTemp = 0;
    
    uiTemp = gpio_get_pin_val(POWER_KEY) ;
    // �ػ���������
    if (uiTemp) return;
    // ���ùػ�����������־
    gucPowerKeyDownFlag = 1;
    // ���¼�ʱ,���������������в��ܹػ�
    guiPowerKeyCount = 0;
}


// �����ػ�����,�ú�����ͨ�ö�ʱ���б�����
void s_PowerOff(void)
{
	uint i;

    if(get_machine_type()==S300)return;

	// ��ͼ��
	for(i=0; i<ICON_NUMBER; i++)
	{
		if ((i+1) == ICON_BATTERY)
    	{
    		s_ScrSetIcon(ICON_BATTERY, 6);
			continue;
    	}
		ScrSetIcon(i+1, 0); 
	}   
    //ֹͣ��ӡ��
    s_PrnStop();
    //�ر�wifi
//    if(is_wifi_module())WifiClose();
	//��λwifiģ��  ����������ر�wifiģ��������⣬��Ϊֱ���µ磬����rs9110ֱ������������ֱ���µ�Ҳ��������
	if(is_wifi_module() == 3) s_WifiPowerSwitch(0);
    
    if(GetWlType() != 0)
    {
    	CLcdSetFgColor(COLOR_BLACK);
		CLcdSetBgColor(COLOR_WHITE);
		ScrSpaceSet(0,0);
        ScrCls();
        SCR_PRINT(0, 3, 0x1, "�ػ���,���Ժ�...", "Shutting Down...");
        WlPowerOff(); // close wnet power
    }

	// ����
	s_LcdClear();
	// ��Һ������
	s_ScrLightCtrl(2);
	// �ر��ⰴ��
	if(get_machine_type() == D200)
	{
		s_KbLock(0);
		s_TouchKeyStop();
	}
	else
	{
		s_KBlightCtrl(0);
	}
 
	//�ط�����
	s_StopBeep();

    if(GetBatteryChargeProcess()<0)
	{
		// D210û���ڳ�磬���Ƿ�������������뵽�ٹػ�״̬
		if(GetHWBranch() == D210HW_V2x && !OnBase())
		{
			OnBasePowerOffEntry();
		}
		else
		{
			gpio_set_drv_strength(POWER_HOLD, GPIO_DRV_STRENGTH_8mA);
			gpio_set_pin_val(POWER_HOLD, 0);
			while(1);
		}
	}
	else
	{
		k_Charging = 1;
		BatteryChargeEntry(2);
	}
}
// Power���ػ�����,�ú�����ͨ�ö�ʱ���б�����
/* ���жϵ��ػ���������ʱ,���ȿ�ʼ��ʱ,�����ʱʱ��
   ��������,�жϹػ����Ƿ��Դ��ڱ����µ�״̬,�����
   ��ػ�,�����ʱ����,�ȴ��ػ���������
*/ 
void s_KeyPowerOffHandler(void)
{
	uint uiTemp = 0,i;

	if(get_machine_type()!= S500)return;
	// �ػ���������,��ʼ��ʱ
	if (gucPowerKeyDownFlag == 0) return;
	guiPowerKeyCount++;
	
	// ��ʱʱ�䵽,׼���ػ�
	if (guiPowerKeyCount < 200) return;

	guiPowerKeyCount = 0;
	gucPowerKeyDownFlag = 0;
        
    uiTemp = gpio_get_pin_val(POWER_KEY) ;
    // �жϵ��ػ����Դ��ڰ���״̬
    if (uiTemp) return;
	OsResume(g_taskpoweroff);
	#if 0
	//�ط�����
	s_StopBeep();
	
    //ֹͣ��ӡ��
    s_PrnStop();
	
    if(GetWlType() != 0)
    {
    	CLcdSetFgColor(COLOR_BLACK);
		CLcdSetBgColor(COLOR_WHITE);
		ScrSpaceSet(0,0);
        ScrCls();
        SCR_PRINT(0, 3, 0x1, "�ػ���,���Ժ�...", "Shutting Down...");
        WlPowerOff(); // close wnet power
    }

	// ����
	s_LcdClear();

	// ����
	ScrCls();
	// ��Һ������
	s_ScrLightCtrl(2);
	// �ر��ⰴ��
	if(get_machine_type() == D200)
	{
		s_KbLock(0);
		s_TouchKeyStop();
	}
	else
	{
		s_KBlightCtrl(0);
	}
	// Output 0
	gpio_set_pin_val(POWER_HOLD, 0);
	while(1);
	#endif
}

void PowerOff(void)
{
	int i;

	if (get_machine_type()==D200)
	{
		s_AbortPowerOff();
	}
    if(get_machine_type()==S300)return;

	//s_Beep(BeepFreqTab[7],100,0);
    	//��ͼ��
	for(i=0; i<ICON_NUMBER; i++)
	{
		if ((i+1) == ICON_BATTERY)
    	{
    		s_ScrSetIcon(ICON_BATTERY, 6);
			continue;
    	}
		ScrSetIcon(i+1, 0); 
	}	
    //ֹͣ��ӡ��
    s_PrnStop();

    if(GetWlType() != 0)
    {
    	CLcdSetFgColor(COLOR_BLACK);
		CLcdSetBgColor(COLOR_WHITE);
		ScrSpaceSet(0,0);
        ScrCls();
        SCR_PRINT(0, 3, 0x1, "�ػ���,���Ժ�...", "Shutting Down...");
        WlPowerOff(); // close wnet power
    }

	//����Ļ
	s_LcdClear();
	//����Ļ����
	s_ScrLightCtrl(2);
	//�ؼ��̱���
	if(get_machine_type() == D200)
	{
		s_KbLock(0);
		s_TouchKeyStop();
	}
	else
	{
		s_KBlightCtrl(0);
	}

	if(GetBatteryChargeProcess()<0)
	{
		gpio_set_drv_strength(POWER_HOLD, GPIO_DRV_STRENGTH_8mA);
		gpio_set_pin_val(POWER_HOLD, 0);
		while(1);
	}
	else
	{
		k_Charging = 1;
		BatteryChargeEntry(2);
	}
}

// AbortHandler�ػ�����,�ú�����Abort�������б�����
void s_AbortPowerOff(void)
{
	uint i;

    if(get_machine_type()==S300)return;

	// ��ͼ��
	for(i=0; i<ICON_NUMBER; i++)
	{
		if ((i+1) == ICON_BATTERY)
    	{
    		s_ScrSetIcon(ICON_BATTERY, 6);
			continue;
    	}
		ScrSetIcon(i+1, 0); 
	}   
    //ֹͣ��ӡ��
    s_PrnStop();
    
    if(GetWlType() != 0)
    {
        CLcdSetFgColor(COLOR_BLACK);
		CLcdSetBgColor(COLOR_WHITE);
		ScrSpaceSet(0,0);
        ScrCls();
        SCR_PRINT(0, 3, 0x1, "�ػ���,���Ժ�...", "Shutting Down...");
        WlPowerOff(); // close wnet power
    }

	// ����
	s_LcdClear();
	// ��Һ������
	s_ScrLightCtrl(2);
	// �ر��ⰴ��
	if(get_machine_type() == D200)
	{
		s_KbLock(0);
		s_TouchKeyStop();
	}
	else
	{
		s_KBlightCtrl(0);
	}
	//�ط�����
	s_StopBeep();

	gpio_set_drv_strength(POWER_HOLD, GPIO_DRV_STRENGTH_8mA);
	gpio_set_pin_val(POWER_HOLD, 0);
	while(1);
}


/*********************************************/
//Just for D200 - Touch Keyboard
/*********************************************/
static  const char Key_Tab_D200[]={
	KEY0,			KEY1,		KEY2,		KEY3,
    KEY4,			KEY5,		KEY6,		KEY7,
    KEY8,			KEY9,		KEYF1,		KEYF2,
    KEYCLEAR,		KEYCANCEL,	KEYENTER,   KEYF5,
};  
volatile unsigned int guiCheckKeyFlag;
volatile unsigned int k_TouchLockTime; // = 3000;
/* 
 * touchkey_lock_mode = 0 : �����������̰���Դ������ˢ�ſ���IC���ɽ���
 * touchkey_lock_mode = 1 : ���ֽ���״̬30�룬30����Զ����������߶̰���Դ�����������ͽ������л���
 * touchkey_lock_mode = 2 : ���ְ�������״̬�����ٱ��̰���Դ������
 */
volatile unsigned int touchkey_lock_mode;
volatile int giLastKey=-1;
uchar gucKeyBuf[2];
static T_SOFTTIMER touckb_timer;

/*
 * ��KbLock ��֮ͬ����s_KbLock ֻ���Ƽ��̣������Ƽ��̱������Ļ���⣬
 * �÷�ͬKbLock��
 */
void s_KbLock(uchar mode)
{
	if (get_machine_type() != D200)
	{
		return;
	}
	
	if (mode > 2)
		return ;
	if (mode == 0) /* lock */
	{
		ScrSetIcon(5, 1);
		s_TouchKeyLockSwitch(1);
		touchkey_lock_mode = 1;
	}
	else if (mode == 1) /* unlock */
	{
		ScrSetIcon(5, 0);
		s_TouchKeyLockSwitch(0);
		k_TouchLockTime 	= k_LightTime;
		touchkey_lock_mode = 1;
	}
	else /* unlock always */
	{
		ScrSetIcon(5, 0);
		s_TouchKeyLockSwitch(0);
		touchkey_lock_mode = 2;
	}
}

/**
 * mode:
 * = 0 : �����������̰���Դ������ˢ�ſ���IC���ɽ���
 * = 1 : ���ֽ���״̬30�룬30����Զ����������߶̰���Դ�����������ͽ������л���
 * = 2 : ���ְ�������״̬�����ٱ��̰���Դ������
 */
void KbLock(uchar mode)
{
	if (get_machine_type() != D200)
	{
		return;
	}
	
	if (mode > 2)
		return ;
	if (mode == 0) /* lock */
	{
		s_ScrLightCtrl(0); //Always turn off the light.
		s_KBlightCtrl(0);
	}
	else/* unlock */
	{
		if(k_ScrBackLightMode<2) ScrBackLight(1);
		else ScrBackLight(2);

		if(k_KBLightMode<2)kblight(1);
		else if(k_KBLightMode==2)ScrBackLight(2);
	}

	s_KbLock(mode);
}

extern int giTouchKeyLockFlag;
int KbCheck(int iCmd)
{
	if (iCmd == 0)
	{
		if (get_machine_type() == D200)
		{
			return giTouchKeyLockFlag;
		}
		else
		{
			return -1;
		}
	}
	else if (iCmd == 1)
	{
		if(kb_buf_over == 1)
			return KB_BUF_MAX;
		else
			return (kb_in_ptr-kb_out_ptr + KB_BUF_MAX)%KB_BUF_MAX;
	}
	else if (iCmd == 2)
	{
		return s_GetKbmuteStatus();
	}
	else if (iCmd == 3)
	{
		return kblightstat;
	}
	return -1;
}

void s_Kb_Touchkey_Isr(void)
{
	gpio_set_pin_interrupt(KB_INT_GPIO,0);
	guiCheckKeyFlag = 1;
}
// called by the system general timer
void s_Kb_Touchkey_handler(void)
{
	int iKeyCode;
	unsigned short usRegVal;

	if (get_machine_type() != D200)
		return;
	if(kblightstat != kblightctrl)
	{
		s_TouchKeyBLightCtrl(kblightctrl);
		kblightstat = kblightctrl;
	}
	if(guiCheckKeyFlag == 0)
	{
		return ;
	}

	gucKeyBuf[0] = KB_I2C_ADDR_TOUCHKEY_VALUE;
	kb_i2c_write_str(KB_I2C_SLAVER_WRITE,gucKeyBuf,1);
	kb_i2c_read_str(KB_I2C_SLAVER_READ, gucKeyBuf,2);

	usRegVal = ((unsigned short)gucKeyBuf[1] << 8) | gucKeyBuf[0];
	iKeyCode = fls(usRegVal);
	
	#ifdef DEBUG_TOUCHKEY	
	printk("key=0x%02x,0x%02x\r\n",gucKeyBuf[0],gucKeyBuf[1]);
	printk("Fck=%x, iCode=%d\n", usRegVal, iKeyCode);
	#endif

	if((usRegVal & (usRegVal-1)) || (usRegVal & (1<<15)))
	{
		guiCheckKeyFlag = 0;
		gpio_set_pin_interrupt(KB_INT_GPIO,1);
		return ;
	}

	#ifdef DEBUG_TOUCHKEY
	printk( "s_ulDrvTimerCheck=%d\r\n",s_TimerCheckMs(&touckb_timer));
	#endif
	if ((s_TimerCheckMs(&touckb_timer) != 0) && (iKeyCode != giLastKey))
	{
		//�����л���������
		//���ٰ�ͬһ������ʱ������
		guiCheckKeyFlag = 0;
		gpio_set_pin_interrupt(KB_INT_GPIO,1);
		return ;
	}
	
	if (iKeyCode != giLastKey)
	{
		s_TimerSetMs(&touckb_timer,200);//200ms�ڲ������л�����
		giLastKey = iKeyCode;
	}

	//Push
	if((kb_buf_over == 0)&&(kb_Buffer_Busy == 0))
	{
		kb_buf[kb_in_ptr] = iKeyCode;
		kb_in_ptr=(kb_in_ptr+1)%KB_BUF_MAX;
		if(kb_in_ptr == kb_out_ptr)    
			kb_buf_over = 1;
	}
	
	if(k_ScrBackLightMode < 2)     ScrBackLight(1);
    if(k_KBLightMode < 2)             kblight(1);
    

	s_KbLock(touchkey_lock_mode);
	s_TimerKeySound(0);

	gpio_set_pin_interrupt(KB_INT_GPIO,1);
	guiCheckKeyFlag = 0;
}

void s_Kb_TouchKey_Init(void)
{
	int i;
	uint temp;
	
	memcpy(Key_Tab, Key_Tab_D200, sizeof(Key_Tab_D200));
	
	for(i = 0; i < KB_BUF_MAX; i++)
	{
		kb_buf[i] = NOKEY;
	}
	
	kb_in_ptr = 0;
	kb_out_ptr = 0;
	kb_buf_over = 0;

	kb_PressStatus = WAITFOR_PRESS;

	kb_TimerStep = 0;
	kb_OffTimerCount = 0;

	kb_Buffer_Busy = 0;
	kb_keyCombFlag = 0; 		//Ĭ�ϲ�������ϼ�
	guiCheckKeyFlag = 0;

	gpio_set_pin_interrupt(KB_INT_GPIO,0);
	gpio_set_pin_type(KB_INT_GPIO, GPIO_INPUT);

	if (s_TouchKeyVersion() >= 0x04)//����V03�津����������
	{
		s_TouchKeySetBaseLine();
	}
	s_TouchKeyStart();

	s_setShareIRQHandler(KB_INT_GPIO,INTTYPE_FALL,s_Kb_Touchkey_Isr);
	gpio_set_pin_interrupt(KB_INT_GPIO,1);
}
// ���ڴ��������ĳ�ʼ��
static void s_KbInit_wlankey(void);
static void s_KbInit_touchkey(void)
{
	k_LightTime         = 6000;//��1���ӣ��Զ�Ϩ��
	k_ScrBackLightTime  = k_LightTime;//
	k_KBLightTime       = k_LightTime;//S80û�а�������
	k_TouchLockTime		= k_LightTime;
	k_KBLightMode       = 1;

	s_Kb_TouchKey_Init();
	s_KbInit_wlankey();
	s_StopBeep();
}
// BT+WIFI �����жϷ������
static void s_WlanKey_Int(void)
{
	kb_TimerStep = 0;
	kb_OffTimerCount = 0;
    kb_OnTimerCount = 0;
	kb_PressStatus = WAITFOR_RELEASE;

	gpio_set_pin_interrupt(WLAN_EN_KEY,0);
	// ������ʱ���¼���20ms��ʼɨ�谴��
	StartTimerEvent(KEY_HANDLER,2);
}
static void s_TimerScanWlanKey(void)
{
    static unsigned int kbCode; 
    unsigned char uRet;

    kb_OnTimerCount++;

    switch(kb_TimerStep)
    {
        case 0:  /*������������*/
			if (gpio_get_pin_val(WLAN_EN_KEY) == 0)
				kbCode = 0x10;
			else
				kbCode = 0;
			
            if(kbCode == 0)
                kb_OffTimerCount++;
            else	/*ɨ�赽��Ч����*/
            {
				kb_OffTimerCount = 0; 
				kb_TimerStep =1;
            }

            if(kb_OffTimerCount > 2)
            {
				kb_TimerStep = 22;
				kb_OffTimerCount = 0;
            }
            break;

        case 1:      
			/*�ý׶ζ���Ч������ȷ��ɨ�裬����ȡ����ֵ*/
			if (gpio_get_pin_val(WLAN_EN_KEY) == 0)
				kbCode = 0x10;
			else
				kbCode = 0;
            /*���û�м�ֵ,�˳�ɨ��*/
            if(!kbCode)
            {
                kb_TimerStep = 22;/*ֻ����ͨ�������£�����ȴ��ͷŰ����׶�*/
                break;
            }

            if((kb_buf_over == 0)&&(kb_Buffer_Busy == 0))
            {
                /*��Ű�������������*/
        		kb_buf[kb_in_ptr] = kbCode;
        		kb_in_ptr=(kb_in_ptr+1)%KB_BUF_MAX;
        		
        		if(kb_in_ptr == kb_out_ptr)    
        			kb_buf_over = 1;
            }

            s_TimerKeySound(0);
            if(k_ScrBackLightMode < 2)  ScrBackLight(1);
            if(k_KBLightMode < 2)	       kblight(1);
            s_KbLock(touchkey_lock_mode);
           
            kb_TimerStep = 22; /*ֻ����ͨ�������£�����ȴ��ͷŰ����׶�*/
            break;	

            /*�ý׶εȴ���ͨ�����ͷ�*/
        case 22:
            uRet = s_TimerKeySound(1);
			if (gpio_get_pin_val(WLAN_EN_KEY) == 0)
				kbCode = 0x10;
			else
				kbCode = 0;
			
            if(!kbCode && ++kb_OffTimerCount >= 2 && !uRet)
                goto quit_scan;		
            break;

        default:
            break;
    }

    return ;

    /*�����˴ΰ���ɨ�裬���жϲ��򿪶�ʱ��*/
quit_scan:
    s_TimerKeySound(2);
    StopTimerEvent(KEY_HANDLER);
	gpio_set_pin_interrupt(WLAN_EN_KEY,1);
    kb_PressStatus = WAITFOR_PRESS; 
}
static void s_KbInit_wlankey(void)
{
	kb_TimerStep = 0;
	kb_OffTimerCount = 0;
	kb_OnTimerCount = 0;
	kb_PressStatus = WAITFOR_PRESS;
	
	gpio_set_pin_interrupt(WLAN_EN_KEY,0);
	gpio_set_pin_type(WLAN_EN_KEY, GPIO_INPUT);
	s_setShareIRQHandler(WLAN_EN_KEY,INTTYPE_FALL,s_WlanKey_Int);
	
	// ������ʱ���¼���20ms��ʼɨ�谴��
	s_SetTimerEvent(KEY_HANDLER,s_TimerScanWlanKey); //4
	
	gpio_set_pin_interrupt(WLAN_EN_KEY,1);
}
int get_touchkeylockmode(void)
{
	return touchkey_lock_mode;
}
int s_KbWaitRelease(void)
{
	int kb_in1;
	if (get_machine_type() == D200) return;
	while(1)	
	{
		kb_in1 = gpio_get_mpin_val(GPIOB,KEY_GPIOIN_MASK);
		if(kb_in1==KEY_GPIOIN_MASK) break;
	}
}
