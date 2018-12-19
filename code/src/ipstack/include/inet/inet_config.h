/*
**   Inet Configure File
**Author: sunJH
**Date: 2007-08-13
�޸���ʷ��
080624 sunJH
1.����TCP_RCV_QUEUELEN��TCP_SND_QUEUELEN�궨��
2.ȡ��MAX_1024_COUNT����
3.MAX_512_COUNT��200����Ϊ90����ͨ���궨�壩
4.MAX_1518_COUNT��50����Ϊ120��
5.����ȫ��ռ�õ��ڴ�Ϊ222KB
**/
#ifndef _INET_CONFIG_H_
#define _INET_CONFIG_H_

/*
** Э��ջ�������ò���
**
**/

/* TCP Receiver buffer space
*/
#ifndef TCP_RCV_QUEUELEN
#define TCP_RCV_QUEUELEN 10
#endif
	
/* TCP sender buffer space (pbufs). This must be at least = 2 *
   TCP_SND_BUF/TCP_MSS for things to work. */
#ifndef TCP_SND_QUEUELEN
#define TCP_SND_QUEUELEN                10
#endif

/*
 TCP Max connect control block
*/
#ifndef MAX_TCP_PCB_COUNT
#define MAX_TCP_PCB_COUNT 10
#endif

/*
TCP Max Listen control block
*/
#ifndef MAX_TCP_LISTEN_PCB_COUNT
#define MAX_TCP_LISTEN_PCB_COUNT 2
#endif

/*
UDP MAX control block
*/
#ifndef MAX_UDP_COUNT
#define MAX_UDP_COUNT 2
#endif

/*
Socket MAX control block
*/
#ifndef MAX_SOCK_COUNT
#define MAX_SOCK_COUNT (MAX_TCP_PCB_COUNT+MAX_TCP_LISTEN_PCB_COUNT+MAX_UDP_COUNT)
#endif

/*
** Skbuff ��Ŀ���ò���
**
**/
#define MAX_512_COUNT  (TCP_RCV_QUEUELEN+TCP_SND_QUEUELEN+1)*MAX_TCP_PCB_COUNT /* packet size = 512 */
#define MAX_1518_COUNT (TCP_RCV_QUEUELEN+TCP_SND_QUEUELEN+4)*MAX_TCP_PCB_COUNT /* packet size = 1518 */

#endif/* _INET_CONFIG_H_ */
