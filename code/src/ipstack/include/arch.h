/*
author: colin
e-mail:zhaoyikecolin@gmail.com.cn
function:
	��ƽ̨��صĶ�ʱ����ʹ�ã����ڵĶ��壬�ӿں����ȡ�
*/
#ifndef _ARCH_ARM1136_H_
#define _ARCH_ARM1136_H_


#define NO_LED

/* ʱ������:10 ms*/
#define INET_TIMER_SCALE 10

/* CPU��س�ʼ��*/

#define INET_NIC_IRQ_NO 
#define INET_TIMER_IRQ_NO 

/* enable �����ж�*/
#define inet_nic_irq_enable(mask) 
/* disable �����ж�*/
#define inet_nic_irq_disable() 
/* enable inet timer irq */
#define inet_timer_irq_enable()
/* disable inet timer irq */
#define inet_timer_irq_disable()
#define inet_irq_enable()
#define inet_irq_disable()
/* used in irq service code */	
#define inet_irq_restore_flags(flags)
#define inet_irq_save_flags(flags)
#define arch_timer_init()
#define arch_timer_int_clear()
#define arch_timer_start(count, pres)
#define eth_irq_clear() 
#define sys_timer_int_clear()

/*
**  PPP macro define
**/
//extern ushort port_xtab[0];

#define __LITTLE_ENDIAN_BITFIELD

/* ��֤��ͬƽ̨����ʱIP��ַ��MAC��ַ�����ͻ */
#define ETH_TEST_IP "172.16.112.245"
#define ETH_TEST_MAC 0x06

/* Modem PPP Enable Flag */
#define MODEM_PPP_ENABLE

/* Wireless PPP Enable Flag */
#define WL_PPP_ENABLE

/* s_uart0_printf����ʵ�� */
#define DISABLE_UART0_PRINTF  1
/* OpenComm����ʵ�� */
#define DISABLE_OPEN_COMM     1

/* PPPoE Enable*/
#define PPPOE_ENABLE

//#define ETH_DISABLE


static inline void s_UartSend( unsigned char ch)
{
}
#define WL_CDMA_DTR_ENABLE  /* */
extern int WlGetInitStatus();
extern void WlSetCtrl(unsigned char pin,unsigned char val);

#define CDMA_DTR_ON() WlSetCtrl(0, 1)/* ����DTR��*/
#define CDMA_DTR_OFF() WlSetCtrl(0, 0)

#define 	S_PORTOPEN


#endif/* */
