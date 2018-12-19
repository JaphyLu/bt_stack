/*
�޸���ʷ:
20080605:
1.#define __LITTLE_ENDIAN_BITFIELD������cpu��ص��ļ�(arch_s80.h)��
2.�����˶�ϵͳ�ⲿ�ӿ�ʹ�õ������������޶��徯��
3.ȥ�������õ�����constant_htons��constant_htonl
4.������sp30�ĺ궨��
5.ǿ�ƴ��ڴ�ӡͳһ����s_uart0_printf
080624 sunJH
1.����MAX_IPV4_ADDR_STR_LEN�궨��
2.��TCP_SND_QUEUELEN��TCP_RCV_QUEUELEN����inet_config.h
3.����str_check_max����
080820 sunJH
1.ȥ����PortTx��PortOpen���������ⲻͬƽ̨��ͻ
2.S3C2410ƽ̨��memcpy�Ż�����ĺ�������
P80��ΪS3C2410������������
081108 sunJH
ScrPrint��NO_LED�����Ϊ�պ���
090507 sunJH
TCP_MAXRTX ������Դ�������12Ϊ120��
��ǿGPRS��TCP�ȶ���
*/
#ifndef _INET_H_
#define _INET_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inet/inet_config.h>
#include <netapi.h>
#include <inet/inet_type.h>
#include <inet/ip_addr.h>
#include <inet/inet_irq.h>


#if defined(__HAS_NETWORK_OP)
//DON'T define htonl and htons
#elif defined(__LITTLE_ENDIAN_BITFIELD)
static inline u32 htonl(u32 l)
{
	return ((((l)&0xff)<<24)|(((l)&0xff00)<<8)|
					(((l)&0xff0000)>>8)|(((l)&0xff000000)>>24));
}
static inline u16 htons(u16 s)
{
	return (u16)((((s)&0xff)<<8)|(((s)&0xff00)>>8));
}
#define ntohl(l) htonl(l)
#define ntohs(s) htons(s)
#elif defined(__BIG_ENDIAN_BITFIELD)
#define htonl(l) (l)
#define htons(s) (s)
#define ntohl(l) (l)
#define ntohs(s) (s)
#else
#error	"Adjust your byte orders defines"
#endif


typedef int err_t;

#ifdef NET_DEBUG
#define TCP_DEBUG  1
#define UDP_DEBUG  2
#define ARP_DEBUG  3
#define IP_DEBUG   4
#define IPSTACK_DEBUG ipstack_print
int ipstack_print(int module_id, char *fmt, ...);
#else
#define TCP_DEBUG  0
#define UDP_DEBUG  0
#define ARP_DEBUG  0
#define IP_DEBUG   0
#define IPSTACK_DEBUG 
#endif





/* ---------- TCP options ---------- */
#define IP_DEFAULT_TTL 64
#ifndef TCP_TTL
#define TCP_TTL                         (IP_DEFAULT_TTL)
#endif

#define MAX_IPV4_ADDR_STR_LEN            15

/* TCP Maximum segment size. */
#ifndef TCP_MSS
#define TCP_MSS                         1400 /* A *very* conservative default. */
#endif

#ifndef TCP_WND
#define TCP_WND                         TCP_RCV_QUEUELEN*TCP_MSS
#endif 

#ifndef TCP_MAXRTX
#define TCP_MAXRTX                      120
#endif

#ifndef TCP_SYNMAXRTX
#define TCP_SYNMAXRTX                   100
#endif

/* Controls if TCP should queue segments that arrive out of
   order. Define to 0 if your device is low on memory. */
#ifndef TCP_QUEUE_OOSEQ
#define TCP_QUEUE_OOSEQ                 0
#endif


/* TCP sender buffer space (bytes). */
#ifndef TCP_SND_BUF
#define TCP_SND_BUF                     TCP_MSS*TCP_SND_QUEUELEN
#endif

#define IPSTACK_MAX(x , y)  (x) > (y) ? (x) : (y)
#define IPSTACK_MIN(x , y)  (x) < (y) ? (x) : (y)

#ifndef NULL
#define NULL ((void *)0)
#endif

#define U16_F "uh"
#define U32_F "ul"
#define S16_F "d"
#define S32_F "d"

u16 inet_chksum(void *dataptr, int len);

struct net_stats
{
	u32 recv_pkt;
	u32 recv_err_pkt;
	u32 recv_err_proto;
	u32 recv_err_check;
	u32 recv_null;
	u32 drop;
	u32 snd_pkt;

};

#define NET_STATS_DROP(stats) ((stats)->drop++)
#define NET_STATS_RCV(stats) ((stats)->recv_pkt++)
#define NET_STATS_SND(stats) ((stats)->snd_pkt++)

void ipstack_init(void);

#ifdef NET_DEBUG
#define s80_printf printk
#define err_printf printk
#else
#define s80_printf
#define err_printf 
#endif
char* s80_gets(void);
void inet_delay(u32 ms);

#ifdef NET_DEBUG
#define assert(expr) \
do{	\
	if ((!(expr))) \
	{	\
		char *str = __FILE__; \
		int i; \
		for(i=strlen(str);i>=0;i--) \
			if(str[i]=='\\'||str[i]=='/') \
				break; \
		printk("ASSERTION FAILED (%s) at: %s:%d\n", #expr,	\
			str+i+1, __LINE__);				\
		ScrPrint(0, 2, 0, "ASSERTION FAILED (%s) at: %s:%d\n", #expr,	\
					str+i+1, __LINE__); 			\
		asm("mov r2, #0\n bx r2");\
	}									\
} while (0)
#else/* NET_DEBUG */
#define assert(expr) do{ \
}while(0)
#endif/* NET_DEBUG */



#define INET_MEMCMP memcmp
#define INET_MEMCPY memcpy


void show_time(void);/* time for debug */

extern u8 bc_mac[6];/* broadcast mac */
extern u8 zero_mac[6];/* ȫ0 mac address */
extern volatile u32  inet_jiffier;

#define IPSTACK_CALLBACK_API 1
#define IPSTACK_IN_CHECK 1

#define DHCPS_PORT 67
#define DHCPC_PORT 68
#define DHCPC 1

#define DNS_PORT 53


int net_has_init(void);
void s_InetSoftirqInit(void);
void s_DevInit(void);
void s_initEth(void);
void s_initPPP(void);
void s_InitWxNetPPP(void);
void s_initPPPoe(void);

extern int toupper(int c);

#endif/* _INET_H_ */
