#include "base.h"

/*
//PRN-HEAD#1----GPE6    ������źţ������ƴ�ӡͷ��1��
//PRN-HEAD#2----GPE7    ������źţ������ƴ�ӡͷ��2��
//PRN-HEAD#3----GPE8    ������źţ������ƴ�ӡͷ��3��
//PRN-HEAD#4----GPE9    ������źţ������ƴ�ӡͷ��4��
//PRN-HEAD#5----GPC24    ������źţ������ƴ�ӡͷ��5��
//PRN-HEAD#6----GPC25    ������źţ������ƴ�ӡͷ��6��
//PRN-HEAD#7----GPC26    ������źţ������ƴ�ӡͷ��7��
//PRN-HEAD#8----GPC27    ������źţ������ƴ�ӡͷ��8��
//PRN-HEAD#9----GPC28    ������źţ������ƴ�ӡͷ��9��
//PRN-PHA-------GPB10    ������źţ������Ʋ��������λ;
//PRN-PHB-------GPB11   ������źţ������Ʋ��������λ��
//PRN-STEP------GPE5   ������źţ���������ֽ������
//PRN-MOTOR-----GPE4   ������źţ������ƴ�ӡ��
//PRN-POWER-----GPE3   ������źţ������ƴ�ӡģ���Դ��
//PRN-PIN-------PWM2/GPD2   ������źţ������ƴ�ӡ��ͷ�����巽ʽ��
//PRN-HOME------GPB8   �������źţ������homeλ�ã�
//PRN-ENCODE----GPB6   �������źţ��жϣ���encode��
//PRN-PAPER-----GPB7   �������źţ���ȱֽ��⣻
*/
#define CFG_PRN_CHKPAPER()  {\
    gpio_set_pin_val(GPIOB,7,1);gpio_set_pin_type(GPIOB,7,GPIO_INPUT);}

#define CFG_PRN_CHKHOME()	{\
    gpio_set_pin_val(GPIOB,8,1);gpio_set_pin_type(GPIOB,8,GPIO_INPUT);}  

#define CFG_PRN_HEAD()		{\
    gpio_set_pin_val(GPIOE,6,1);gpio_set_pin_type(GPIOE,6,GPIO_OUTPUT);\
    gpio_set_pin_val(GPIOE,7,1);gpio_set_pin_type(GPIOE,7,GPIO_OUTPUT);\
    gpio_set_pin_val(GPIOE,8,1);gpio_set_pin_type(GPIOE,8,GPIO_OUTPUT);\
    gpio_set_pin_val(GPIOE,9,1);gpio_set_pin_type(GPIOE,9,GPIO_OUTPUT);\
    gpio_set_pin_val(GPIOC,24,1);gpio_set_pin_type(GPIOC,24,GPIO_OUTPUT);\
    gpio_set_pin_val(GPIOC,25,1);gpio_set_pin_type(GPIOC,25,GPIO_OUTPUT);\
    gpio_set_pin_val(GPIOC,26,1);gpio_set_pin_type(GPIOC,26,GPIO_OUTPUT);\
    gpio_set_pin_val(GPIOC,27,1);gpio_set_pin_type(GPIOC,27,GPIO_OUTPUT);\
    gpio_set_pin_val(GPIOC,28,1);gpio_set_pin_type(GPIOC,28,GPIO_OUTPUT);}

#define PRN_HEAD1_H()       gpio_set_pin_val(GPIOE,6,1)      // (����ź�),���ƴ�ӡͷ#1
#define PRN_HEAD1_L()       gpio_set_pin_val(GPIOE,6,0)

#define PRN_HEAD2_H()       gpio_set_pin_val(GPIOE,7,1)      // (����ź�),���ƴ�ӡͷ#2
#define PRN_HEAD2_L()       gpio_set_pin_val(GPIOE,7,0)

#define PRN_HEAD3_H()       gpio_set_pin_val(GPIOE,8,1)      // (����ź�),���ƴ�ӡͷ#3
#define PRN_HEAD3_L()       gpio_set_pin_val(GPIOE,8,0)

#define PRN_HEAD4_H()       gpio_set_pin_val(GPIOE,9,1)      // (����ź�),���ƴ�ӡͷ#4
#define PRN_HEAD4_L()       gpio_set_pin_val(GPIOE,9,0)

#define PRN_HEAD5_H()       gpio_set_pin_val(GPIOC,24,1)      // (����ź�),���ƴ�ӡͷ#5
#define PRN_HEAD5_L()       gpio_set_pin_val(GPIOC,24,0)

#define PRN_HEAD6_H()       gpio_set_pin_val(GPIOC,25,1)      // (����ź�),���ƴ�ӡͷ#6
#define PRN_HEAD6_L()       gpio_set_pin_val(GPIOC,25,0)

#define PRN_HEAD7_H()       gpio_set_pin_val(GPIOC,26,1)      // (����ź�),���ƴ�ӡͷ#7
#define PRN_HEAD7_L()       gpio_set_pin_val(GPIOC,26,0)

#define PRN_HEAD8_H()       gpio_set_pin_val(GPIOC,27,1)      // (����ź�),���ƴ�ӡͷ#8
#define PRN_HEAD8_L()       gpio_set_pin_val(GPIOC,27,0)

#define PRN_HEAD9_H()       gpio_set_pin_val(GPIOC,28,1)      // (����ź�),���ƴ�ӡͷ#9
#define PRN_HEAD9_L()       gpio_set_pin_val(GPIOC,28,0)

#define CFG_PRN_PHAB()		{\
    gpio_set_pin_val(GPIOB,10,0);gpio_set_pin_type(GPIOB,10,GPIO_OUTPUT);\
    gpio_set_pin_val(GPIOB,11,0);gpio_set_pin_type(GPIOB,11,GPIO_OUTPUT);}

#define PRN_PHA_H()         gpio_set_pin_val(GPIOB,10,1)      // (����ź�),���Ʋ��������λA
#define PRN_PHA_L()         gpio_set_pin_val(GPIOB,10,0)    

#define PRN_PHB_H()         gpio_set_pin_val(GPIOB,11,1)      // (����ź�),���Ʋ��������λB
#define PRN_PHB_L()         gpio_set_pin_val(GPIOB,11,0)


#define CFG_PRN_STEP()		{\
    gpio_set_pin_val(GPIOE,5,1);gpio_set_pin_type(GPIOE,5,GPIO_OUTPUT);}

#define PRN_STEP_H()        gpio_set_pin_val(GPIOE,5,1)       // (����ź�),������ֽ�������
#define PRN_STEP_L()        gpio_set_pin_val(GPIOE,5,0)      


#define CFG_PRN_MOTOR()		{\
    gpio_set_pin_val(GPIOE,4,1);gpio_set_pin_type(GPIOE,4,GPIO_OUTPUT);}

#define PRN_MOTOR_H()       gpio_set_pin_val(GPIOE,4,1)      // (����ź�),���ƴ�ӡ���;����Ч
#define PRN_MOTOR_L()       gpio_set_pin_val(GPIOE,4,0)


#define CFG_PRN_POWER()		{\
    gpio_set_pin_val(GPIOE,3,0);gpio_set_pin_type(GPIOE,3,GPIO_OUTPUT);}

#define PRN_POWER_H()       gpio_set_pin_val(GPIOE,3,1)      // (����ź�),���ƴ�ӡģ���Դ,����Ч
#define PRN_POWER_L()       gpio_set_pin_val(GPIOE,3,0)

////////////////////
//��ʱ����Դʹ��
///1. TIMER5 ���Ʋ�����ֽ
///2. TIMER6 ������ֽ
////////////////////

static struct platform_timer_param PaperMStepTimer_set;
static struct platform_timer *pPaperMStepTimer;
static struct platform_timer_param GuardTimer_set;
static struct platform_timer *pGuardTimer;

//PWM config
#define rPWMCTL         (*(volatile unsigned int*)0x000D6000)
#define rPERIOD_CNT_2   (*(volatile unsigned int*)0x000D6014)
#define rDUTYHI_CNT_2   (*(volatile unsigned int*)0x000D6018)
#define rPRESCALE       (*(volatile unsigned int*)0x000D6024)

//����ʱ�� 340us
#define PRN_TIME_US 340
//#define TOTOL_PRN_TIME 833	//1200hz
//�����޸�Ϊ40ms,��ֹen_code�źű�����ʱ���ҳ���
#define TOTOL_PRN_TIME 40000	


extern void s_PrnEncoderInt(void);
extern void s_PaperMStepTimer_Int(void);
extern void s_GuardTimer_Int(void);
extern int k_PrnSmallFontFlag;

void DISABLE_PRNOUT_PWM(void)
{
    rPWMCTL &= ~(1<<2);
    gpio_set_pin_val(GPIOD, 2, 0);
    gpio_set_pin_type(GPIOD,2,GPIO_OUTPUT);
}

void ENABLE_PRNOUT_PWM(void)
{
    gpio_set_pin_type(GPIOD,2,GPIO_FUNC0);
    rPWMCTL|=1<<2;
}

void s_CfgPrnPinOut(void)
{    
	// ���ñȽ�ֵ
	// ������Ƶ��1200HZ��833us�� �ߵ�ƽռ��ʱ��PRN_TIME_US(340us)�� 
	DISABLE_PRNOUT_PWM();
    rPERIOD_CNT_2 = TOTOL_PRN_TIME;
    rPRESCALE &= (~(0x3f<<6));
    rDUTYHI_CNT_2 = PRN_TIME_US;
    rPWMCTL &=~(1<<17);//pwm2 set to driver mode 
    // ���ó�ʼ״̬ΪHIGH
    rPWMCTL |=1<<10;
}

void s_CfgPrnEncode(uchar mode)
{
    int TrigType;
    if(mode) TrigType = INTTYPE_RISE;
    else     TrigType = INTTYPE_FALL;  
    s_setShareIRQHandler(GPIOB,6,TrigType,s_PrnEncoderInt);
}

void DISABLE_ENCODE_INT(void)
{
    gpio_set_pin_interrupt(GPIOB,6,0);
}

void ENABLE_ENCODE_INT(void)
{
    gpio_set_pin_interrupt(GPIOB,6,1);
}

void CLEAR_ENCODE_INT(void)      //no need in bcm5892
{

}

//������ͷ1~8
void s_SetPrnHeader1_8(unsigned char value)
{
    gpio_set_mpin_val(GPIOE,0x0F<<6,value<<6);
    gpio_set_mpin_val(GPIOC,0x0F<<24, value<<20);
}

//������ͷ9�����������ź�
void s_SetPrnControl(unsigned char value)
{
    // k_PortPrnCtrl
	// Bit0 ~ PRN_HEAD9
	// Bit1 ~ PRN_PHA
	// Bit2 ~ PRN_PHB
	// Bit3 ~ PRN_STEPMOTOR
	// Bit4 ~ PRN_MOTOR
	// Bit5 ~ PRN_PWR
	// Bit6 ~ PRN_PIN
	// Bit7 ~ Reverse

	// Bit0 ~ PRN_HEAD9
	if (value & 0x01) PRN_HEAD9_H();
	else if(k_PrnSmallFontFlag == 1)  // ��ʹ�õھ��룬����������ԭ��δ֪
        PRN_HEAD9_H();  
    else PRN_HEAD9_L();

	// Bit1 ~ PRN_PHA
	if (value & 0x02)PRN_PHA_H();
    else PRN_PHA_L();
    
	// Bit2 ~ PRN_PHB
	if (value & 0x04) PRN_PHB_H();
	else PRN_PHB_L();
    
	// Bit3 ~ PRN_STEPMOTOR
	if (value & 0x08) PRN_STEP_H();
	else PRN_STEP_L();

	// Bit4 ~ PRN_MOTOR
	if (value & 0x10) PRN_MOTOR_H();
	else PRN_MOTOR_L();
	
	// Bit5 ~ PRN_PWR
	if (value & 0x20) PRN_POWER_H();
	else PRN_POWER_L();
}

void s_PaperMStepTimerInit(void)
{
	PaperMStepTimer_set.timeout=10000;            	/* uint is 1us */
	PaperMStepTimer_set.mode=TIMER_PERIODIC;			/*timer periodic mode. TIMER_PERIODIC or TIMER_ONCE*/
	PaperMStepTimer_set.handler=s_PaperMStepTimer_Int;    /* interrupt handler function pointer */
	PaperMStepTimer_set.interrupt_prior=INTC_PRIO_7;	/*timer's interrupt priority*/
	PaperMStepTimer_set.id=TIMER_PRN_0;            		/* timer id number */
	pPaperMStepTimer=platform_get_timer(&PaperMStepTimer_set);
	platform_stop_timer(pPaperMStepTimer);
}

void s_PaperMStepTimerStart(uint TimeUs)
{
    PaperMStepTimer_set.timeout=TimeUs;            	/* uint is 1us */
    pPaperMStepTimer=platform_get_timer(&PaperMStepTimer_set);
    platform_start_timer(pPaperMStepTimer);
}

void s_PaperMStepTimerStop(void)
{
    platform_stop_timer(pPaperMStepTimer);
}

void CLEAR_MSTEP_TIMER_INT(void)
{
    platform_timer_IntClear(pPaperMStepTimer);
}

void s_GuardTimerInit(void)
{
	GuardTimer_set.timeout=10000;            	/* uint is 1us */
	GuardTimer_set.mode=TIMER_ONCE;			/*timer periodic mode. TIMER_PERIODIC or TIMER_ONCE*/
	GuardTimer_set.handler=s_GuardTimer_Int;    /* interrupt handler function pointer */
	GuardTimer_set.interrupt_prior=INTC_PRIO_7;	/*timer's interrupt priority*/
	GuardTimer_set.id=TIMER_PRN_1;            		/* timer id number */
	pGuardTimer=platform_get_timer(&GuardTimer_set);
	platform_stop_timer(pGuardTimer);
}

void s_GuardTimerStart(uint TimeUs)
{
    GuardTimer_set.timeout=TimeUs;            	/* uint is 1us */
    pGuardTimer=platform_get_timer(&GuardTimer_set);
    platform_start_timer(pGuardTimer);
}

void s_GuardTimerStop(void)
{
    platform_stop_timer(pGuardTimer);
}

void CLEAR_GUARD_TIMER_INT(void)
{
    platform_timer_IntClear(pGuardTimer);
}

void s_PrinterHardwareInit(void)
{
	CFG_PRN_CHKPAPER();//����ָʾ��ǰ��ӡֽ״̬����ֽ����ֽ��?
	CFG_PRN_CHKHOME();//���ڼ���ӡͷ�Ƿ��λ������ߣ�
	CFG_PRN_HEAD();//9���ӡͷ����
	CFG_PRN_PHAB();//���������λ����
	CFG_PRN_STEP();//������ֽ�������
	CFG_PRN_MOTOR();//���ƴ�ӡ���
	CFG_PRN_POWER();//���ƴ�ӡģ���Դ

	s_CfgPrnEncode(1);//�������źţ��жϣ���encode
	s_CfgPrnPinOut();//PWM����

	//����TIMER5,TIMER6
	s_PaperMStepTimerInit();
	s_GuardTimerInit();
}

uint have_paper(void)  
{
   return gpio_get_pin_val(GPIOB, 7);
}

uint s_CheckHome(void)
{
   return gpio_get_pin_val(GPIOB, 8); //��0��home��0����
}

#define TEST_PRINTER_SIGNAL_TEST

#ifdef TEST_PRINTER_SIGNAL_TEST
void TestPrinterSignal(void)
{
	uchar ucKey = 0;
	int   iTemp = 0;
	uchar ch;
    unsigned int cnt;

	s_PrnInit_stylus();
	PrnInit_stylus();

	while (1)
	{
        ScrCls();
		ScrPrint(0,0,0,"Singal 20121101_06");
		ScrPrint(0,1,0,"1-Head13579 2-2468");
		ScrPrint(0,2,0,"3-PHA   4-PHB");
        ScrPrint(0,3,0,"5-STEP  6-MOTOR");
		ScrPrint(0,4,0,"7-PWR   8-PRNOUT");
		ScrPrint(0,5,0,"9-Check 0-Feed");

		ucKey = getkey();
        ScrCls();
        if(ucKey == KEYCANCEL) return;
		switch(ucKey)
		{
		case '1'://Head1~8
            PRN_HEAD1_L();
            PRN_HEAD2_H();
            PRN_HEAD3_L();
            PRN_HEAD4_H();
            PRN_HEAD5_L();
            PRN_HEAD6_H();
            PRN_HEAD7_L();
            PRN_HEAD8_H();
            PRN_HEAD9_L();
            
            ENABLE_PRNOUT_PWM();
            DelayMs(100);
            DISABLE_PRNOUT_PWM();
            break;
		case '2'://Head9
            PRN_HEAD1_H();
            PRN_HEAD2_L();
            PRN_HEAD3_H();
            PRN_HEAD4_L();
            PRN_HEAD5_H();
            PRN_HEAD6_L();
            PRN_HEAD7_H();
            PRN_HEAD8_L();
            PRN_HEAD9_H();
            
            ENABLE_PRNOUT_PWM();
            DelayMs(100);
            DISABLE_PRNOUT_PWM();
            break;
		case '3':
			while (1)
			{
				ScrPrint(0,0,0,"1-Set PHA high");
				ScrPrint(0,1,0,"2-Set PHA low");
                ucKey = getkey();
                if(ucKey=='1')PRN_PHA_H();
				if(ucKey=='2')PRN_PHA_L();
				if(ucKey==KEYCANCEL) break;
			}
			break;
		case '4'://PHB
			while (1)
			{
				ScrPrint(0,0,0,"1-Set PHB high");
				ScrPrint(0,1,0,"2-Set PHB low");
                ucKey = getkey();
				if(ucKey=='1')PRN_PHB_H();
				if(ucKey=='2')PRN_PHB_L();
				if(ucKey==KEYCANCEL) break;
			}
			break;
		case '5'://STEP
			while (1)
			{
				ScrPrint(0,0,0,"Set STEP high");
				ScrPrint(0,1,0,"Set STEP low");
                ucKey = getkey();
				if(ucKey=='1')PRN_STEP_H();
				if(ucKey=='2')PRN_STEP_L();
				if(ucKey==KEYCANCEL) break;

			}
			break;
		case '6'://MOTOR
			ScrPrint(0,0,0,"6-MOTOR");
            DISABLE_ENCODE_INT();
			PRN_MOTOR_L();
            DelayMs(5);
			PRN_MOTOR_H();
            ENABLE_ENCODE_INT();
            getkey();
			break;
		case '7'://PWR
			while (1)
			{
				ScrPrint(0,0,0,"1-Set PWR high");
				ScrPrint(0,1,0,"2-Set PWR low");
                ucKey = getkey();
				if(ucKey=='1')PRN_POWER_H();
				if(ucKey=='2')PRN_POWER_L();
                if(ucKey==KEYCANCEL)break;
			}
			break;
		case '8':
			ScrPrint(0,0,0,"1-Enable PWM");
			ScrPrint(0,1,0,"2-Disable PWM");
            ucKey = getkey();
			if(ucKey=='1')ENABLE_PRNOUT_PWM();
            else DISABLE_PRNOUT_PWM();
            break;
            
		case '9':
			//PAPER
			if (have_paper()) ScrPrint(0,0,0,"Have paper!");
			else ScrPrint(0,0,0,"No paper!");

            if (!s_CheckHome())ScrPrint(0,2,0,"In Home!");
			else ScrPrint(0,2,0,"No Home!");
			getkey();
			break;
            
		case '0'://Feed
            ScrPrint(0,0,0,"1-Forward Feed");
            ScrPrint(0,1,0,"2-Reverse Feed");
            ucKey = getkey();
            if(ucKey=='1')
            {
                unsigned short i;
                ScrPrint(0,3,0,"Forward Feed");
                PRN_STEP_L();
                
                for(i=0;i<10;i++)
                {
                    PRN_PHA_L();//step1
                    PRN_PHB_H();
                    DelayMs(2);
                    
                    PRN_PHA_H();//step2
                    DelayMs(2);
                    
                    PRN_PHB_L();//step3
                    DelayMs(2);

                    PRN_PHA_L();//step4
                    DelayMs(2);
                }
                
                PRN_STEP_H();
            }
            else
            {
                unsigned short i;
                ScrPrint(0,3,0,"Reverse Feed");

                PRN_STEP_L();

                for(i=0;i<10;i++)
                {
                    PRN_PHA_H();//step1
                    PRN_PHB_L();
                    DelayMs(2);
                    
                    PRN_PHB_H();//step2
                    DelayMs(2);
                    
                    PRN_PHA_L();//step3
                    DelayMs(2);

                    PRN_PHB_L();//step4
                    DelayMs(2);
                }

                PRN_STEP_H();
            }
			break;
            
		default:
			break;
		}
	}
}
#endif
