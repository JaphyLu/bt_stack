#ifndef _Wl_FUN_
#define _Wl_FUN_

#ifndef uchar
#define uchar unsigned char
#endif
#ifndef uint
#define uint unsigned int
#endif
#ifndef ulong
#define ulong unsigned long
#endif
#ifndef ushort
#define ushort unsigned short
#endif


#ifndef	OK
	#define OK 0
#endif
#ifndef RET_OK 
	#define RET_OK 0
#endif

#ifndef	NET_ERR_TIMEOUT
	#define NET_ERR_TIMEOUT -13
#endif
#ifndef	NET_ERR_PPP
	#define NET_ERR_PPP -21
#endif
#ifndef	NET_ERR_CONN
	#define NET_ERR_CONN -6
#endif

#if 1
//ģ��ڱ�ռ��
#define WL_RET_ERR_PORTINUSE 	-201
//ģ��û��Ӧ��
#define WL_RET_ERR_NORSP		-202
//ģ��Ӧ�����
#define WL_RET_ERR_RSPERR		-203
//ģ�鴮��û�д�
#define WL_RET_ERR_PORTNOOPEN -204
//��ҪPIN��
#define WL_RET_ERR_NEEDPIN	-205
//��ҪPUK��PIN��
#define WL_RET_ERR_NEEDPUK	-206
//��������
#define WL_RET_ERR_PARAMER	-207
//�������
#define WL_RET_ERR_ERRPIN		-208
//û�в���SIM��
#define	WL_RET_ERR_NOSIM      -209
//��֧�ֵ�����
#define WL_RET_ERR_NOTYPE		-210
//����û��ע��
#define WL_RET_ERR_NOREG		-211
//ģ������ʼ��
#define WL_RET_ERR_INIT_ONCE	-212
//û������
#define WL_RET_ERR_NOCONNECT  -213
//��·�Ͽ�
//#define WL_RET_ERR_LINEOFF	-214
//û��socket����
#define WL_RET_ERR_NOSOCKETUSE	-215
//��ʱ
#define WL_RET_ERR_TIMEOUT		-216
//���ڲ�����
#define WL_RET_ERR_CALLING 		-217
//�ظ���socket����
#define WL_RET_ERR_REPEAT_SOCKET	-218
//socket �Ѿ��Ͽ�
#define WL_RET_ERR_SOCKET_DROP	-219
//socket ����������
#define WL_RET_ERR_CONNECTING     -220
//socket ���ڹر�
#define WL_RET_ERR_SOCKET_CLOSING	-221
//����ע����
#define WL_RET_ERR_REGING			-222
//�رմ��ڳ���
#define WL_RET_ERR_PORTCLOSE  	-223
//����ʧ��
#define WL_RET_ERR_SENDFAIL		-224
//�����ģ��汾
#define WL_RET_ERR_MODEVER		-225
//������
#define WL_RET_ERR_DIALING		-226
//�һ���
#define WL_RET_ERR_ONHOOK		-227
//����ģ�鸴λ
#define WL_RET_ERR_RST			-228

#define WL_RET_ERR_NOSIG         -229    /* Signal level is too  low */
//ģ�����µ�	
#define WL_RET_ERR_POWEROFF      -230    /* Power Off */
//ģ�鷱æ
#define WL_RET_ERR_BUSY			 -231
//�������ڴ���
#define WL_RET_ERR_OPENPORT		-232			
// ģ��û�г�ʼ��
#define WL_RET_ERR_NOINIT		-233

#define WL_RET_ERR_OTHER			-300
#endif
int		WlInit(const uchar *SimPin);
int		WlGetSignal(uchar * SingnalLevel);
int		WlPppLogin(const uchar * APNIn,const uchar * UidIn,const uchar * PwdIn,  long Auth, int TimeOut,int ALiveInterVal);
void	WlPppLogout (void);
int		WlPppCheck(void);
int		WlSendCmd(const uchar * ATstrIn, uchar *RspOut,ushort Rsplen, ushort TimeOut, ushort Mode);
void	WlSwitchPower( unsigned char Onoff );
int		WlOpenPort(void);
int		WlClosePort(void);
int WlSelBand(unsigned char mod);

//�ṩ��monitor���õ�

//��ȡģ��汾
int WlGetModuleInfo(uchar *modinfo,uchar *imei);
/* 
tpmsg ����ģ��������Ϣ EXTB CDMA GPRS WCDM WIFI
modulemsg ��ģ���������Ϣ
*/
void WlSetCtrl(unsigned char pin,unsigned char val);
int WlSelSim(uchar simnum);

void WlTcpRetryNum(int value);

void WlDetectTcpOpened(int value);


#define WLAN_PWR 		ptWlDrvConfig->pwr_port, ptWlDrvConfig->pwr_pin
#define WLAN_WAKE 		ptWlDrvConfig->wake_port, ptWlDrvConfig->wake_pin
#define WLAN_ONOFF 	ptWlDrvConfig->onoff_port, ptWlDrvConfig->onoff_pin
#define WLAN_DTR_CTS	ptWlDrvConfig->dtr_cts_port,ptWlDrvConfig->dtr_cts_pin
#define WLAN_DUAL_SIM	ptWlDrvConfig->dual_sim_port,ptWlDrvConfig->dual_sim_pin
#endif


