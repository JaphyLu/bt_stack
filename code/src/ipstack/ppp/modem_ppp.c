/*
** ��ppp.c�������modem data link ��صĲ���
** modem_ppp.cΪ��modem��صĲ���
	Modemģ��PPP���
    Date: 080623
    Author: sunJH
080722 sunJH
�ṩ��MODEM_PPP_ENABLE��modem_PPP.c���вü��Ĺ���
080917 sunJH
��֤s_initPPPֻ�ܳ�ʼ��һ��
090423 sunJH
s_initPPP�ı���ϵͳ��ʼ���жϷ�ʽ
100902 sunJH
1.����������modem ppp(��s60�������ͨ��);
2.�������ݲ����µ���ϵ�ܹ���Ϊ�˼��ݲ��ú궨�忪��
**/
#include <inet/inet.h>
#include <inet/dev.h>
#include <inet/inet_softirq.h>
#ifdef MODEM_PPP_ENABLE
#include "ppp.h"

extern int modem_get_comm(void);
extern void ppp_com_irq_disable(int comm);
extern void ppp_com_irq_enable(int comm);
extern void ppp_com_irq_tx(int comm);
extern int ppp_com_check(int comm);
extern int ppp_modem_check(int comm, long ppp_state);

static PPP_DEV_T ppp_dev={0,};
static INET_DEV_T ppp_inet_dev={0,};


static struct inet_softirq_s  ppp_timer;
static void ppp_modem_timeout(unsigned long mask)
{
	ppp_timeout(&ppp_dev, mask);
	#ifdef MODEM_PPP_ARCH_2
	/*
	���Ͳ���comm�Ĵ����жϴ���;
	*/
	{
		int cc, comm=ppp_dev.comm;
		if(comm>=0&&comm<=2&&ppp_comm[comm])
		{
			while(1)
			{
				if(ppp_com_can_tx(comm))
					break;
				cc = ppp_xmit_char(ppp_comm[comm]);
				if(cc<0)
					break;
				PortTx(4, cc&0xff);
			}
		}
	}
	#endif
	
	#if S90_SOFT_MODEMPPP
	{
		char send_c;
		
		int ret, cc, comm;
		comm=ppp_dev.comm;

		if(comm == 3 && ppp_comm[comm])
		{
			while(1)
			{
				ret = mdm_can_tx();
				if(ret<1)
					break;
				cc = ppp_xmit_char(ppp_comm[comm]);
				if(cc<0)
					break;
				send_c=cc&0xff;
				MdmTx(&send_c, 1);
			}
		}
	}	
	#endif
}
static void modem_irq_disable(PPP_DEV_T *ppp)
{
	ppp_com_irq_disable(ppp->comm);
}

static void modem_irq_enable(PPP_DEV_T *ppp)
{
	ppp_com_irq_enable(ppp->comm);
}

static void modem_irq_tx(PPP_DEV_T *ppp)
{
	ppp_com_irq_tx(ppp->comm);
}

static int  modem_ready(PPP_DEV_T *ppp)
{
	return ppp_com_check(ppp->comm);
}
/* check: link�豸�Ƿ��ѵ���
**	0: ok, <0 fail
*/
static int  modem_check(PPP_DEV_T *ppp, long ppp_state)
{
	int ret = ppp_modem_check(ppp->comm, ppp_state);
	return ret;
}

/* force_down: ǿ��link�豸����*/
static void modem_force_down(PPP_DEV_T *ppp)
{
	OnHook();
}

static PPP_LINK_OPS  modem_link_ops=
{
	modem_irq_disable,
	modem_irq_enable,
	modem_irq_tx,
	modem_ready,
	modem_check,
	modem_force_down,
};
#endif/* MODEM_PPP_ENABLE */

void s_initPPP(void)
{
#ifdef MODEM_PPP_ENABLE
	PPP_DEV_T *ppp = &ppp_dev;
	INET_DEV_T *dev = &ppp_inet_dev;
	static u8 bInit=0;

	if(net_has_init())
		return ;

	ppp_dev_init(ppp);
	ppp_inet_init(ppp, dev, "Modem_PPP", MODEM_IFINDEX);

	ppp->link_ops = &modem_link_ops;
	ppp->init = 1;

	ppp->comm = -1;

	ppp_timer.mask = INET_SOFTIRQ_TIMER;
	ppp_timer.h = ppp_modem_timeout;
	inet_softirq_register(&ppp_timer);
	ppp_printf("PPP module Init!\n");
#endif/* MODEM_PPP_ENABLE */	
}

err_t PPPLogin_std(char *name, char *passwd, long auth, int timeout)
{
#ifdef MODEM_PPP_ENABLE
	PPP_DEV_T *ppp = &ppp_dev;

#if S90_SOFT_MODEMPPP
	ppp->comm = 3;
#else
	ppp->comm = modem_get_comm();
#endif


	ppp_prepare(ppp);
	return ppp_login(ppp, name, passwd, auth, timeout);
#else
	return NET_ERR_SYS;
#endif/* MODEM_PPP_ENABLE */
}

int PPPCheck_std(void)
{
#ifdef MODEM_PPP_ENABLE
	return ppp_check(&ppp_dev);
#else
	return NET_ERR_SYS;
#endif/* MODEM_PPP_ENABLE */
}

void PPPLogout_std(void)
{
#ifdef MODEM_PPP_ENABLE

	ppp_logout(&ppp_dev, NET_ERR_LOGOUT);
#endif/* MODEM_PPP_ENABLE */
}

