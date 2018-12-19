#include "Base.h"
/**************************************************
������ʱ������λ�ٷ�֮һ��
��ʱ����ΪӦ�����ʹ��
��ʱ����Ϊ�ڲ���ʱ����
��ʱ����Ϊ�ڲ���ʱ�¼�
***************************************************/
#define  MON_TIMER    TM_LIMIT   // monitor������ʱ��
#define  APP_TIMER	  5	  //  5��Ӧ�ü�����ʱ�� 
#define  EVENT_TIMER  4     //�¼�ʹ�õĶ�ʱ��
#define  CNT_TIMER    (MON_TIMER+APP_TIMER)        // ������ʱ��������
#define  MAX_TIMER    (EVENT_TIMER+CNT_TIMER)    //��ʱ������

static volatile uint TimerCount[MAX_TIMER];
static volatile uint OldCount[4];
static void (*TimerProc[4])(void);
static volatile uchar LowEventRun[4];  //�����̶��ĵײ��¼����б�־
static T_SOFTTIMER App_SoftTimer[APP_TIMER];
void s_TimerInit(void);
void s_TimerProc(void);
void TimerSet(unsigned char TimerID, unsigned short Cnts);
unsigned short TimerCheck(unsigned char TimerID);
void  ClearTimerIntFlag(int id);

volatile unsigned int   k_TimerCount10MS;

 unsigned char    k_ScrBackLightMode;
 extern volatile unsigned int    k_ScrBackLightTime;
 extern volatile unsigned int    k_LightTime;
 extern volatile unsigned int    k_KBLightTime;
 extern volatile unsigned char   k_KBLightMode;
 extern volatile uint BeepfUnblockCount;
 extern volatile unsigned int touchkey_lock_mode;
 extern volatile unsigned int k_TouchLockTime;
void s_softTimer(void);

extern void timer_irq_Handler(void);

#define TIMER_CONTROL_OFFSET		0x08
#define TIMER_VALUE_OFFSET			0x04
#define TIMER_LOAD_OFFSET			0x00
#define TIMER_INTCLR_OFFSET		    0x0C
#define TIMER_INTSTA_OFFSET         0X10
#define TIMER_CTRL_ONESHOTMODE		(1 << 0)
#define TIMER_CTRL_SIZE_16		(0 << 1)
#define TIMER_CTRL_SIZE_32		(1 << 1)
#define TIMER_CTRL_DIV1			(0 << 2)
#define TIMER_CTRL_PREBY16		(1 << 2)
#define TIMER_CTRL_PREBY256		(2 << 2)
#define TIMER_CTRL_CLK2			(1 << 9)

/* * Indexes into timers_map[]*/
#define DMU_TIMER_ENABLE_SLE  0x0102404c
#define BCM5892_NB_TIMERS		8

typedef struct {
    uint     vic_base;    /* VIC controller */
    uint    base;           /* timer base address */
    uint    vec;            /* interrupt vector */
} timer_map_t;

static const timer_map_t timers_map[BCM5892_NB_TIMERS] = {
    /* VIC1 timers */
    {VIC1_REG_BASE_ADDR, TIM0_REG_BASE_ADDR + 0x00, IRQ_OTIMER0},        /* open or secure */
    {VIC1_REG_BASE_ADDR, TIM0_REG_BASE_ADDR + 0x20, IRQ_OTIMER1},        /* open or secure */
    {VIC1_REG_BASE_ADDR, TIM1_REG_BASE_ADDR + 0x00, IRQ_OTIMER2},        /* open only */
    {VIC1_REG_BASE_ADDR, TIM1_REG_BASE_ADDR + 0x20, IRQ_OTIMER3},        /* open only */
    /* VIC2 timers */
    {VIC2_REG_BASE_ADDR, TIM2_REG_BASE_ADDR + 0x00, IRQ_OTIMER4},        /* open only */
    {VIC2_REG_BASE_ADDR, TIM2_REG_BASE_ADDR + 0x20, IRQ_OTIMER5},        /* open only */
    {VIC2_REG_BASE_ADDR, TIM3_REG_BASE_ADDR + 0x00, IRQ_OTIMER6},        /* open only */
    {VIC2_REG_BASE_ADDR, TIM3_REG_BASE_ADDR + 0x20, IRQ_OTIMER7},        /* open only */
};


static struct platform_timer timers[BCM5892_NB_TIMERS];

#if 1 //shics
typedef struct 
{
	int call_num;
	int * func_addr;
} FUNC_CALL_INFO_T;
FUNC_CALL_INFO_T  func_call_info[128];
#endif



uint hw_timer_base(int timer_id)
{	
	return timers_map[timer_id].base;
}


/* Returns timer's counter */
uint hw_timer_get_counter(int timer_id)
{
	uint counter_addr;

	counter_addr = hw_timer_base(timer_id) + TIMER_VALUE_OFFSET;
	return readl((counter_addr));
}

struct platform_timer *platform_get_timer(struct platform_timer_param *param)
{
	struct platform_timer *timer;
	int i;
	uint timer_base;
	uint val;

	/* Sanity checks */
	if (param == NULL)	return NULL;
	if (param->id >= BCM5892_NB_TIMERS) return NULL;
	
	timer = &timers[param->id];
	timer->handler = param->handler;
	timer->id = param->id;
	timer->mode =param->mode;
	
	if(param->interrupt_prior > INTC_PRIO_15)	timer->interrupt_prior = INTC_PRIO_15;
	else timer->interrupt_prior =param->interrupt_prior;
	//MAX timeout time is 715 second
	timer->reload = param->timeout*TICKS_PER_uSEC;

	timer_base = hw_timer_base(timer->id);
	writel(timer->reload, (timer_base + TIMER_LOAD_OFFSET));

	val = timer->mode | TIMER_CTRL_SIZE_32 ;
	writel(val, (timer_base + TIMER_CONTROL_OFFSET));

	if(timer->handler)
	{
		s_SetInterruptMode(timers_map[timer->id].vec,timer->interrupt_prior, INTC_IRQ);
		s_SetIRQHandler(timers_map[timer->id].vec, (void (*)(int))timer->handler);
		enable_irq(timers_map[timer->id].vec);
	}
	else
	{
		s_SetIRQHandler(timers_map[timer->id].vec,NULL);
		disable_irq(timers_map[timer->id].vec);
	}

//printk("timer id:%d,reload:%d,handler:%08X,div:%d!\r\n"
//	,timer->id,timer->reload,timer->handler);

	return timer;
}


/** 
** platform_start_timer - Start the countdown of the timer.
 **	If the countdown reaches 0, the interrupt handler is called.
 **/
int platform_start_timer(struct platform_timer *timer)
{
	unsigned long flags;
	uint timer_base;
	uint val;

	if (timer == NULL)	return -1;
	timer_base = hw_timer_base(timer->id);
	/* Enable the corresponding interrupt and start countdown */
	val =readl(timer_base + TIMER_CONTROL_OFFSET);
	val |= TIMER_CTRL_EN | TIMER_CTRL_IE;
	writel(val, (timer_base + TIMER_CONTROL_OFFSET));
	return 0;
}


/* platform_stop_timer - Stop the countdown of the timer.*/

int platform_stop_timer(struct platform_timer *timer)
{
	uint timer_base, timer_ctrl;

	if (timer == NULL)	return -1;
	/* Disable the corresponding interrupt and stop countdown*/

	timer_base = hw_timer_base(timer->id);
	timer_ctrl = readl((timer_base + TIMER_CONTROL_OFFSET));
	timer_ctrl &= ~TIMER_CTRL_EN;
	writel(timer_ctrl, (timer_base + TIMER_CONTROL_OFFSET));
	/* Clear pending interrupt */
	writel(1, (timer_base + TIMER_INTCLR_OFFSET));

	return 0;
}

/*--------------------------------------------------
void platform_timer_IntClear(struct platform_timer *timer)
���timer��Ӧ���жϵ��жϱ�־
----------------------------------------------------*/
void platform_timer_IntClear(struct platform_timer *timer)
{
	writel(1, (timers_map[timer->id].base+ TIMER_INTCLR_OFFSET));
}

void s_TimerInit(void)
{
	int i;
	
	struct platform_timer *timer;
	struct platform_timer_param  param;
	
#if 1 //shics
memset(func_call_info, 0, sizeof(FUNC_CALL_INFO_T)*128);
#endif
	
	writel(0,DMU_TIMER_ENABLE_SLE);  //use REFCLK/4 for reference clock,it is 6MHZ 
	
	param.handler=s_TimerProc;
	param.id		=TIMER_TICK;
	param.timeout=10000;
	param.mode =TIMER_PERIODIC;
	param.interrupt_prior =INTC_PRIO_15;

	timer = platform_get_timer(&param);
		
	for(i = 0; i < MAX_TIMER; i++)
	{
		TimerCount[i] = 0;
	}
	for(i=0;i<4;i++)
	{
		TimerProc[i]=NULL;
		LowEventRun[i]=0;
	}
	k_TimerCount10MS = 0;
	platform_start_timer(timer);
    s_SetSoftInterrupt(timers_map[TIMER_TICK].vec,s_softTimer);

}


void SetLightTime(unsigned short LightTime)
{
    if(LightTime == 0)
        return;
    k_LightTime = (unsigned int)LightTime * 100;
    k_ScrBackLightTime = k_LightTime;
    k_KBLightTime = k_LightTime;
}


/********************************************************************
 * ԭ�ͣ�	void ScrBackLight(unsigned char mode)
 * ���ܣ�	������Ļ����
 * ������	mode - ���⿪���ķ�ʽ.
 *          0 ---�رձ��⣬���а������忨��ˢ��ʱ������Զ���, ��1������û
 *               �а����Ļ�,�Զ��رձ��⣨�ϵ�Ĭ�Ϸ�ʽ��.
 *          1 ---��ʱ������1���Ӻ��Զ��رգ�
 *		 2 ---���ⳣ����
 *          3 ---���ⳣ����
 *********************************************************************/
void  ScrBackLight(unsigned char mode)
{
	if(mode > 2) return;
	if (get_machine_type() == D200)
	{
		if(mode != 0)
			s_KbLock(touchkey_lock_mode);
	}
	if(mode == 0 )s_ScrLightCtrl(0);   // �����
	else s_ScrLightCtrl(1);   // ���⿪
	k_ScrBackLightMode = mode;
	if(mode == 1) k_ScrBackLightTime = k_LightTime;
}

int ScrGetBackLight(void)
{
    if(k_ScrBackLightMode == 2) return 1;
    if(k_ScrBackLightMode == 1 && k_ScrBackLightTime) return 1;
    return 0;
}

void kblight(unsigned char mode)
{
	if(mode > 3) return;
	if(mode == 0 || mode == 3)	s_KBlightCtrl(0);  //���⿪
	else s_KBlightCtrl(1);   //�����
	k_KBLightMode = mode;
	if(mode==1)	k_KBLightTime = k_LightTime;
}


/********************************************************************************
����ԭ��:	int s_SetTimerEvent(int handle,void (*TimerFunc)(void))
��;		   :  ���ڼ�ز�ע�ᶨʱ�¼�
�������:
	handle      :   ���ҽӵ��¼�����ο�enum TIMER �е�ֵ,�����ģ�����
	TimerFunc :   ��ʱ�¼��Ĵ�����
����ֵ     :   �����0
********************************************************************************/
int s_SetTimerEvent(int handle,void (*TimerFunc)(void))
{
	TimerProc[handle]=TimerFunc;
	LowEventRun[handle]=0;
	return 0;
}

/********************************************************************************
����ԭ��:	void StartTimerEvent(int handle,ushort uElapse)
��;		   :  ��ʼ������ע��Ķ�ʱ�¼�
�������:
	handle      :  ��ʹ��s_SetTimerEvent ע��ľ��
	uElapse    :  �¼����еļ�϶�¼�����λ10ms
����ֵ	   :	��
********************************************************************************/
void StartTimerEvent(int handle,ushort uElapse)
{
    TimerCount[handle+CNT_TIMER]=uElapse;
    OldCount[handle]=uElapse;
    LowEventRun[handle]=1;
}

/********************************************************************************
����ԭ��:	void StopTimerEvent(int handle)
��;		   : ֹͣ������ע��Ķ�ʱ�¼�
�������:
	handle      :  ��ʹ��s_SetTimerEvent ע��ľ��
����ֵ	   :	��
*******************************************************************************/
void StopTimerEvent(int handle)
{
    TimerCount[handle+CNT_TIMER]= 0;	
    OldCount[handle]=0;
    LowEventRun[handle]=0;
}

// Cnts����λ100ms
void TimerSet(unsigned char TimerID, unsigned short Cnts)
{
	s_TimerSetMs(&App_SoftTimer[TimerID%APP_TIMER],Cnts*100);
}

unsigned short TimerCheck(unsigned char TimerID)
{
	if (!s_TimerCheckMs(&App_SoftTimer[TimerID%APP_TIMER])) return 0;
	if (s_TimerCheckMs(&App_SoftTimer[TimerID%APP_TIMER]) >= 100)
		return (s_TimerCheckMs(&App_SoftTimer[TimerID%APP_TIMER])/100);

	return 1;
}


void s_TimerProc(void)
{
	int i, j;
	uint IntState = 0;

	// ���TMR��IRQ�жϱ�־
	writel(1, (timers_map[TIMER_TICK].base+ TIMER_INTCLR_OFFSET));
//extern volatile uint g_interrupt_addr;
//if((k_TimerCount10MS%500)==1)printk("Time:%d,PC:%08X!\r\n",k_TimerCount10MS/100,g_interrupt_addr);
//extern volatile uint g_interrupt_addr;	if((k_TimerCount10MS%200)==1)ScrPrint(0,1,0, ":%d,:%08X!\r\n",k_TimerCount10MS/100,g_interrupt_addr);//lanwq
	k_TimerCount10MS++;
    watch_usb_interrupt();

	if((k_ScrBackLightMode == 1) && (k_ScrBackLightTime > 0))
	{
		if(--k_ScrBackLightTime == 0) s_ScrLightCtrl(0);
	}
	
	for(i=0;i<MAX_TIMER;i++)
	{
		if(!TimerCount[i]) 
			continue;
		TimerCount[i]--;                                    

		if((i>=CNT_TIMER)&&(!TimerCount[i]))
		{
			if((TimerProc[i-CNT_TIMER]==NULL) || ! LowEventRun[i-CNT_TIMER])
				continue;
			TimerProc[i-CNT_TIMER]();                             //���м�ز��¼�

			if(OldCount[i-CNT_TIMER]!=0)
				TimerCount[i]=OldCount[i-CNT_TIMER];  //ʱ��ѭ������
		}
	}

	// POWER���ػ��¼�
	
	s_KeyPowerOffHandler();
	
	
    //�첽beep
    if(BeepfUnblockCount)
    {
        BeepfUnblockCount--;
        if(BeepfUnblockCount==0)
            s_BeepUnblockAbort();
    }
}

volatile int count_2=0;
//#define BT_DEBUG_COMM
#ifdef BT_DEBUG_COMM //SHICS
extern volatile uchar shics_debug_buff[8][64];
extern volatile int shics_bt_comm_flag;

void bt_uart_printf()
{
	int i,j;
	static int count=0;
	
#if 0
	if(shics_bt_comm_flag > 0)
	{	
		printk("DATA[%d]:\r\n",shics_bt_comm_flag);
		shics_bt_comm_flag=0;
		
		for(i=0; i<8; i++)
		{
			for(j=0; j<64; j++)
			{
				printk("%02x, ", shics_debug_buff[i][j]);
			}
			printk("\r\n");
		}
	}
#endif

	//if(count++ > 2000)
	if(0)
	{
		count = 0;
		printk("CALL[]:\r\n");
		for(i=0;i<128;i++)
		{
			if(func_call_info[i].call_num>0)
			{
				printk(":%d,%d,%x: \r\n", i,  func_call_info[i].call_num, func_call_info[i].func_addr);
			}
		}
		printk("\r\n");
		printk("count_2=%d\r\n",count_2);
	}
	
}
#else
void bt_uart_printf()
{
}
#endif

void s_softTimer(void)
{
	//unsigned int connectTimeT1 = 0;
	//unsigned int connectTimeT2 = 0;
	//unsigned int connectTimeT3 = 0;

    static uint timer10ms=0;
	
	//connectTimeT1 = GetTimerCount();


    if(timer10ms ==k_TimerCount10MS) return;
    timer10ms = k_TimerCount10MS;
	if((k_KBLightMode == 1) && (k_KBLightTime > 0))
	{
		if(--k_KBLightTime == 0)s_KBlightCtrl(0);
	}
	if((touchkey_lock_mode== 1) && (k_TouchLockTime > 0))
	{
		if(--k_TouchLockTime == 0) s_KbLock(0);
	}

#if 1//shics
extern void bt_timer_handler();	
bt_timer_handler();//added by shics
#endif	

	//just for D200
	s_KeySoundCheck();
	s_UsbState();
	
    Battery_Check_Handler();   
	ScrUpdateIcon();
	timer_irq_Handler(); 
    s_SoundPlayCheck();
	//s_BtTimerDone(); deleted by shics
	s_Kb_Touchkey_handler();
	s_GpsTimerDone();
	UsbHcdTask();
	//connectTimeT2 = GetTimerCount();
	//connectTimeT3 = connectTimeT2 - connectTimeT1;
	//printk("softTimer:%d", connectTimeT3);
	
#if 0//added by shics
extern void btstack_run_loop_execute(void);
btstack_run_loop_execute();
#endif
#if 0//shics
	bt_uart_printf();
#endif
}


unsigned int s_Get10MsTimerCount(void)
{
    return k_TimerCount10MS;
}

unsigned int GetTimerCount(void)
{
	volatile uint iResult,t1,t2;
	uint TMR_VALUE_REG;
	TMR_VALUE_REG=timers_map[TIMER_TICK].base +TIMER_VALUE_OFFSET;
	t1 = k_TimerCount10MS;
	iResult = readl(TMR_VALUE_REG);
	t2 = k_TimerCount10MS;
	if( t1!=t2 )
	{
		iResult = readl(TMR_VALUE_REG);
	}
	iResult = 10 - iResult /TICKS_PER_mSEC;
	iResult = iResult + t2*10;
	
	return iResult;
}

unsigned int GetMs(void)//for base timing
{
	volatile uint iResult,cur_10ms,tmpul,cur_nibble;
	uint TMR_VALUE_REG;
	static uint last_ms=0;
	
	TMR_VALUE_REG=timers_map[TIMER_TICK].base +TIMER_VALUE_OFFSET;

	cur_10ms = k_TimerCount10MS;
	
	tmpul = readl(TMR_VALUE_REG);
	cur_nibble = (10 - tmpul /TICKS_PER_mSEC)%10;

	iResult = cur_10ms*10+cur_nibble;
	if(iResult<last_ms)iResult=last_ms;
	last_ms=iResult;
	
	return iResult;
}


void DelayMs(uint ms)
{
	volatile int i, j;
	for(i=0;i<ms;i++)
	//	for(j=0;j<23500;j++);	/*400MHz */
		for(j=0;j<16621;j++);	/*266MHz */
}

void DelayUs(uint us)
{
    volatile uint i = 0,j = 0;

    for (i = 0; i < us; i++)
    {
        //for (j = 0; j < 23; j++); /*400MHz */
        for (j = 0; j < 17; j++);	/*266MHz */
    }

}

uint GetUsClock(void)
{
	volatile uint t_10ms,t_us,t1,t2,IntSta;
	uint TMR_VALUE_REG,TMP_INT_STATUS;
    
	TMR_VALUE_REG=timers_map[TIMER_TICK].base +TIMER_VALUE_OFFSET;
    TMP_INT_STATUS=timers_map[TIMER_TICK].base +TIMER_INTSTA_OFFSET;

	t_10ms = k_TimerCount10MS;
	t_us = readl(TMR_VALUE_REG);
    t1 = k_TimerCount10MS;
    IntSta = readl(TMP_INT_STATUS)&0x01;
    t2 = readl(TMR_VALUE_REG);

    if(!IntSta)//ϵͳ��ʱ��û�з�ת
    {
        if(t_10ms!=t1)//ϵͳ��ʱ�������˷�ת,���������ڷ��ж�
        {
            t_us = 10000 - t2/TICKS_PER_uSEC;
            t_10ms=t1;
        }
        else
        {
            t_us = 10000 - t_us/TICKS_PER_uSEC;
        }
    }
    else//ϵͳ��ʱ�������˷�ת,�����������ж�
    {        
        if(!t_us || !t2)
        {
            t_us = 0;
        }
        else
        {
            t_us = 10000 - t_us/TICKS_PER_uSEC; 
        }
        t_10ms+=1;
    }
    	
	return (t_10ms*10000+t_us);
}

/*------------------------------------------------
         ����: ����һ�����붨ʱ��ʱ��
         ����: tm         ��ʱ��ָ��
                  	  lms   ��ʱʱ�䣬��λ=����
         ����: ��
--------------------------------------------------*/
void s_TimerSetMs(T_SOFTTIMER *tm,uint lms)
{
         tm->start =GetTimerCount();
         tm->timeout   = lms;
}
/*---------------------------------------------------
         ����: ���һ�����붨ʱ��ʣ��ʱ��
         ����: tm ��ʱ��ָ��
         ����: ��ʱ��ʣ��ʱ�䣬��λ����
----------------------------------------------------*/
uint s_TimerCheckMs(T_SOFTTIMER *tm)
{
         
         unsigned int ulRet,Clk;
         
         if (tm->timeout== 0)    return 0;
         
         Clk = GetTimerCount();  //��ȡ��ǰʱ��
         
         //�����Ѿ����е�ʱ�Ӹ���
         if(Clk >= tm->start)
         {
                  Clk = Clk-tm->start;
         }
         else 
         {
                  Clk =(0xffffffff-tm->start+Clk+1);
         }
         
         
         if ( tm->timeout <=  Clk)
         {
                  ulRet =0;
                  tm->timeout = 0; //��֤�ٴμ�鶨ʱ�� ����ֱ�ӷ��ص��˶����ؽ�������
         }
         else
         { 
                  ulRet= tm->timeout - Clk;
         }
         return ulRet;
}

/*����pwm*/
void pwm_enable(PWM_TYPE pwm,unsigned char isEnalbe)
{
    unsigned int flag;

    irq_save(flag);
    if(isEnalbe)
        reg32(PWM_R_PWMCTL_MEMADDR)|=1<<pwm;
    else
        reg32(PWM_R_PWMCTL_MEMADDR)&=~(1<<pwm);
    irq_restore(flag);
}
/*����pwmռ�ձ�������λ΢��*/
void pwm_duty(PWM_TYPE pwm,unsigned int dutyUs)
{
    reg32(PWM_R_DUTY_CNT_0_MEMADDR+pwm*8)=dutyUs;
}

/*����pwm
periodUs:pwm���ڣ����ֵ0xffff ΢��
isOpenDrain: 1��ʾʹ��pwm������0��ʾ��ֹpwm����
isHighPolarity:1��ʾpwm��ʼ��ƽΪ�ߵ�ƽ��0��ʾpwm��ʼ��ƽΪ��
*/
void pwm_config(PWM_TYPE pwm,unsigned int periodUs,
    unsigned char isOpenDrain,unsigned char isHighPolarity)
{
    unsigned int flag;
    unsigned int ctrl;

    if(periodUs>0xffff)return;

    irq_save(flag);

    gpio_set_pin_type(GPIOD,pwm, GPIO_FUNC0);
    writel(reg32(PWM_R_PRESCALE_MEMADDR)&(~(0x3f<<18-(pwm-PWM0)*6)),
        PWM_R_PRESCALE_MEMADDR);
    reg32(PWM_R_PERIOD_CNT_0_MEMADDR+pwm*8) = periodUs;
    
    ctrl = reg32(PWM_R_PWMCTL_MEMADDR);
    if(isOpenDrain) ctrl |= 1<<(15+pwm);
    else ctrl &= ~(1<<(15+pwm));

    if(isHighPolarity)ctrl |= 1<<(8+pwm);
    else ctrl &= ~(1<<(8+pwm));
    
    reg32(PWM_R_PWMCTL_MEMADDR) = ctrl;
    
    irq_restore(flag);
}

