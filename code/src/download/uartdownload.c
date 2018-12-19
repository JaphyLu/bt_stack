/*-------------------------------------------------------------
* Copyright (c) 2001-2007 PAX Computer Technology (Shenzhen) Co., Ltd.
* All Rights Reserved.
*
* This software is the confidential and proprietary information of
* PAX ("Confidential Information"). You shall not
* disclose such Confidential Information and shall use it only in
* accordance with the terms of the license agreement you entered
* into with PAX.
*
* PAX makes no representations or warranties about the suitability of 
* the software, either express or implied, including but not limited to 
* the implied warranties of merchantability, fitness for a particular purpose, 
* or non-infrigement. PAX shall not be liable for any damages suffered 
* by licensee as the result of using, modifying or distributing this software 
* or its derivatives.
* 
*
*  FileName: uartdownload.c
*  Author:	PengRongshou	 
*  Version : 1.0		
*  Date:2007/7/12
*  Description: 	monitor��������
	

*  Function List:	

*  History: 		// ��ʷ�޸ļ�¼
   <author>  <time>   <version >   <desc>
   PengRongshou	07/05/28	 1.0	 build this moudle

---------------------------------------------------------------*/
 /** 
 * @addtogroup 
 * 
 * @{
 */
#include <stdio.h>
#include "base.h"
#include "localdownload.h"
#include "posapi.h"
#include "../encrypt/rsa.h"
#include "../puk/puk.h"
#include "../file/filedef.h"
#include "../comm/comm.h"
#include "../fat/fsapi.h"
#include "link_info.h"

#ifndef NULL
#define NULL 0
#endif

#define	SHAKE_HAND_TIMEOUT_DEFAULT	50
#define	SHAKE_HAND_TIMEOUT_BACKGROUND	300
#define	SHAKE_HAND_TIMEOUT	2000
#define	DOWNLOAD_TIMEOUT	10000// S
#define	DOWNLOAD_RECVPACK_TIMEOUT	500  // ms

//define RET CODE 
#define 		RET_UART_OK	0
#define 		RET_UART_ERR	-1
#define			RET_UART_SHAKE_TIMEOUT -2
#define			RET_UART_REQ_TIMEOUT	-3
#define			RET_UART_RCV_DATA_TIMEOUT -4
#define			RET_UART_LRC_ERR 	-5
#define			RET_UART_RCV_ERR	-6
#define			RET_UART_RCVBUFLEN_ERR	-7
#define			RET_UART_CHANGEBAUD_ERR -8

enum {
	IUARTDOWNLOADINIT=0 ,
	IUARTDOWNLOADSHAKE,
	IUARTDOWNLOADREQ,
	IUARTDOWNLOADDONE,
	IUARTDOWNLOADEXIT
};
typedef struct tagStatusTable{
	int iStatusNumber;
	int (*pFunction)( int * );
}STATUSTABLE;

static uchar gucComPort=0xff;
static const uchar gaucComPortArr[4]={COM1,COM2,PINPAD,P_USB_DEV};
static uchar gaucComPortList[sizeof(gaucComPortArr)];
static short gsLastMsgNo=-1;//�ϴΰ����
static short gsLastCmd=-1;//�ϴ�����
static short gsLastSndBufLen=-1;//�ϴη������ݰ�����
static uchar gucDownloadNeedPrompt = 0;
static int lastStep=0xff;//��¼����ʱ����ʾ�������Ƶ��ˢ����Ļ������˸
static int giShakeTimeOut=SHAKE_HAND_TIMEOUT_DEFAULT;
static uchar s_reboot=0;//s_reboot>=2 ��������
static uchar s_FirmwareType=0;
//firmware type
#define 	SM_THK88_FIRMWARE			0x01
/**����pedͨѶ������ ����������ͨѶ������
��������û�������С����ͬʱ����sp30api.h�е�����
*/
static uchar *gaucRcvBuf=(uchar *)MAPP_RUN_ADDR;
uchar gaucSndBuf[20480];
static T_SOFTTIMER tm_dl0;
void vUartDownloadingPromt(int iStep,int iCmd);
static int iRecvMsg(int *piResult);
static int iRecvMsgH(int *piResult);
static int iSndMsg(int *piResult,int iSndLen);
void vDownLoadPrtErr(int iErrCode);
int firmware_update(unsigned char type, unsigned char ver, unsigned char *dat, int len);

#define	TM_UART_DOWNLOAD_CMD	&tm_dl0

/**
 ��������״̬����ʼ������

 @param[in,out]  *piResult,��һ״̬�����룬�˳�ʱ���ر�״̬������
  
 @retval ��һ״̬
 \li #IUARTDOWNLOADDONE 
\li  #IUARTDOWNLOADSHAKE
 @remarks
	 \li ����-----------------ʱ��--------�汾---------�޸�˵�� 				   
	 \li PengRongshou---07/06/28--1.0.0.0-------����
 */
int iUartDownloadInit( int *piResult)
{
	int	iRet,iLoop,iFailCount=0;
	uchar aucComAttr[32];

    lastStep = 0xff;//ˢ�²˵�����
	if(gucDownloadNeedPrompt)
		vUartDownloadingPromt(IUARTDOWNLOADINIT,0);
	gsLastMsgNo=-1;//�ϴΰ����
	gsLastCmd=-1;//�ϴ�����
	gsLastSndBufLen=-1;//�ϴη������ݰ�����

	memcpy(gaucComPortList, gaucComPortArr, sizeof(gaucComPortArr));
	snprintf(aucComAttr,sizeof(aucComAttr),"%d%s",DEFAULT_BAUDRATE,COMATTR);
	gucComPort = 0xff;
	//open all comport in gaucComPortList
	for(iLoop=0;iLoop<sizeof(gaucComPortList);iLoop++)
	{
		if(gaucComPortList[iLoop]==0xff) continue;
		iRet =PortClose(gaucComPortList[iLoop]);
		iRet = PortOpen(gaucComPortList[iLoop], aucComAttr);
        
		if(iRet!=0)
		{	
			gaucComPortList[iLoop]=0xff;
			iFailCount++;
		}
	}
	
	if(iFailCount<sizeof(gaucComPortList))
	{
		*piResult = RET_UART_OK;	
		return	IUARTDOWNLOADSHAKE;
	}
	else
	{
		//all comport open err, 
		*piResult = RET_UART_ERR;
		return IUARTDOWNLOADDONE;
	}
}


/**
 ���֣�������ع��̵�����

 @param[in,out]  *piResult,��һ״̬�����룬�˳�ʱ���ر�״̬������
  
 @retval #IUARTDOWNLOADDONE ��һ״̬�����ؽ���
  @retval #IUARTDOWNLOADREQ	 ��һ״̬�Ǽ�����������
 @remarks
	 \li ����-----------------ʱ��--------�汾---------�޸�˵�� 				   
	 \li PengRongshou---07/06/28--1.0.0.0-------����
 */
int iUartDownloadShake( int *piResult)
{
	/************************************************/
	uchar ucTmp;
	int	iLoop,iFindLoop,iTmp;
	if(gucDownloadNeedPrompt)
		vUartDownloadingPromt(IUARTDOWNLOADSHAKE,0);
	s_TimerSetMs(TM_UART_DOWNLOAD_CMD,giShakeTimeOut);
	iFindLoop=-1;
	while(s_TimerCheckMs(TM_UART_DOWNLOAD_CMD))
	{	
		iFindLoop=(iFindLoop+1)%sizeof(gaucComPortList);
		if(gaucComPortList[iFindLoop]==0xff) continue;
		iTmp=PortRecv(gaucComPortList[iFindLoop], &ucTmp,0 );
        
		if(0==iTmp)
		{
			if(ucTmp==SHAKE_REQUEST)
			{
				gucComPort = gaucComPortList[iFindLoop];
                if(gucComPort==P_USB_DEV)
    				PortSend(gucComPort, SHAKE_REPLY_H);
                else
                    PortSend(gucComPort, SHAKE_REPLY);
				*piResult = RET_UART_OK;
				return  IUARTDOWNLOADREQ;
			}
		}
	}
	*piResult = RET_UART_SHAKE_TIMEOUT;
	return IUARTDOWNLOADDONE;
}


/**
 �������ص����󣬲������������ͣ������Ӧ����

 @param[in,out]  *piResult,��һ״̬�����룬�˳�ʱ���ر�״̬������
  
 @retval #IUARTDOWNLOADDONE ��һ״̬�����ؽ���
  @retval #IUARTDOWNLOADREQ   ��һ״̬�Ǽ�����������
 @remarks
	 \li ����-----------------ʱ��--------�汾---------�޸�˵�� 				   
	 \li PengRongshou---07/06/28--1.0.0.0-------����
 */
int iUartDownloadReq( int *piResult)
{
    #define	LD_FILE_TEMP_START_ADDR	SAPP_RUN_ADDR

	ushort usTmp;
	int iRet,iLoop,iTmp,iDataLen,iOffset;
	int iRcvLen;
	uchar aucTmp[128];
	static uchar iLastDLRsaIndex=-1;//�����ع�Կ�ļ�
	static int iLastDLFileCmd=-1;
	static uint lLastDlFileLen=0;
	static uchar *pucCurFileAddr=(uchar*)LD_FILE_TEMP_START_ADDR;
	static int  iLastAppNo=-1;//�ļ�����Ӧ�ñ�ţ����ز����ļ�������DMR	
	static int iLastFileType=-1;//�ļ����ͣ����ز����ļ�
	static uchar aucLastAppName[33];//���ز����ļ�
	tSignFileInfo stSigInfo;
    SO_HEAD_INFO *head;
    uchar *ptr, firmwaretype, firmwarever;
    int countnum;

    if(gucComPort==P_USB_DEV)
        iRcvLen = iRecvMsgH(piResult);
    else
    	iRcvLen = iRecvMsg(piResult);

	if(*piResult!=RET_UART_OK)
	{
		//PortClsRxBuf(gucComPort);
	}

	if(iRcvLen<=0)
	{			
		if(*piResult==RET_UART_REQ_TIMEOUT)
		{
			return IUARTDOWNLOADDONE;
		}
		else
		{
			return	IUARTDOWNLOADREQ;
		}
	}
	
	usTmp = gaucRcvBuf[1];
	
	if(usTmp==gsLastMsgNo)
	{
		iSndMsg(piResult,gsLastSndBufLen);
		return IUARTDOWNLOADREQ;		
	}
	gaucSndBuf[4]=0x00;
	gaucSndBuf[5]=0x01;
	gaucSndBuf[6]=LDERR_UNSUPPORTEDCMD;
	gsLastSndBufLen = 8;			
	
	switch(gaucRcvBuf[3])
	{
	case  LD_CMDCODE_SETBAUTRATE ://��������ͨѶ������
	{
		long lNewBaud;
		vUartDownloadingPromt(IUARTDOWNLOADREQ,LD_CMDCODE_SETBAUTRATE);	
        if(gucComPort==P_USB_DEV)
        {
            lNewBaud = ((uchar)gaucRcvBuf[8]<<24)
    					+((uchar)gaucRcvBuf[9]<<16)
    					+((uchar)gaucRcvBuf[10]<<8)
    					+((uchar)gaucRcvBuf[11]);
        }
        else
        {
    		lNewBaud = ((uchar)gaucRcvBuf[6]<<24)
    					+((uchar)gaucRcvBuf[7]<<16)
    					+((uchar)gaucRcvBuf[8]<<8)
    					+((uchar)gaucRcvBuf[9]);            
        }
	//	sprintf(szBtr,"%ld,8,n,1",lNewBaud);
		if(PortCheckBaudValid(lNewBaud))
		{
			gaucSndBuf[6] = LDERR_UNSUPPORTEDBAUDRATE;
			break;
		}
		else
		{
			gaucSndBuf[6] = LD_OK;
			gaucSndBuf[0]=gaucRcvBuf[0];
			gaucSndBuf[1]=gaucRcvBuf[1];	
			gaucSndBuf[2]=gaucRcvBuf[2];
			gaucSndBuf[3] = gaucRcvBuf[3];
			gsLastMsgNo = gaucRcvBuf[1];	
			gsLastCmd = gaucRcvBuf[3];
			iSndMsg(piResult,gsLastSndBufLen);
		}
		sprintf(aucTmp,"%d,8,N,1",lNewBaud);
		while(PortTxPoolCheck(gucComPort));
		if(P_USB_DEV != gucComPort)
		{
			DelayMs(50);
			if(0!=PortOpen(gucComPort,aucTmp))
			{
				*piResult = RET_UART_CHANGEBAUD_ERR;
				return IUARTDOWNLOADDONE;
			}
			DelayMs(5);
			PortReset(gucComPort);
		}
		return IUARTDOWNLOADREQ;
	}
    
	case  LD_CMDCODE_GETPOSINFO  ://��ȡ�ն���Ϣ
	{	
		vUartDownloadingPromt(IUARTDOWNLOADREQ,LD_CMDCODE_GETPOSINFO);

		if(gucComPort==P_USB_DEV)
		{
		    usTmp = GetPosInfo(gaucSndBuf+7,0x100000);//132;
			gaucSndBuf[4]=(usTmp>>8)&0xff;
			gaucSndBuf[5]=usTmp&0xff;
			gaucSndBuf[6]=LD_OK;
			gsLastSndBufLen = 7+usTmp;
        }
        else
        {
    		usTmp = GetPosInfo(gaucSndBuf+7,LD_MAX_RECV_DATA_LEN);//130;
    		gaucSndBuf[4]=(usTmp>>8)&0xff;
    		gaucSndBuf[5]=usTmp&0xff;
    		gaucSndBuf[6]=LD_OK;
    		gsLastSndBufLen = 7+usTmp;            
        }

		break;
	}
	case  LD_CMDCODE_GETAPPSINFO ://��ȡӦ����Ϣ
	{
		vUartDownloadingPromt(IUARTDOWNLOADREQ,LD_CMDCODE_GETAPPSINFO);

        if(gucComPort==P_USB_DEV)
		    iTmp = s_iGetAppInfo(gaucRcvBuf[8],gaucSndBuf+7);
        else
            iTmp = s_iGetAppInfo(gaucRcvBuf[6],gaucSndBuf+7);
		
		if(iTmp < 0)
		{
			gaucSndBuf[6]=LDERR_APPLCIATIONNOTEXIST;
			break;
		}
		
		iTmp++;
		gaucSndBuf[4]=(iTmp>>8)&0xff;
		gaucSndBuf[5]=iTmp&0xff;
		gaucSndBuf[6]=LD_OK;
		gsLastSndBufLen = 7+iTmp;
		break;
	}

	case LD_CMDCODE_GETSOINFO : //��ȡSO��Ϣ
	{
		vUartDownloadingPromt(IUARTDOWNLOADREQ, LD_CMDCODE_GETSOINFO);

        if(gucComPort==P_USB_DEV)
    		iTmp = GetSoInfo(gaucRcvBuf+8, gaucSndBuf + 7);
        else
            iTmp = GetSoInfo(gaucRcvBuf+6, gaucSndBuf + 7);
        
        if(iTmp > 0)
		{
		    iTmp++;
			gaucSndBuf[4] = (iTmp >> 8) & 0xff;
			gaucSndBuf[5] = iTmp & 0xff;
			gaucSndBuf[6] = LD_OK;
			gsLastSndBufLen = 7 + iTmp;
		}
		else
		{
			gaucSndBuf[6] = LDERR_GENERIC;
			gsLastSndBufLen = 7+1;
		}
		break;
	}

    case LD_CMDCODE_GETPUBFILE: /*��ȡ�����ļ���Ϣ*/
            
        vUartDownloadingPromt(IUARTDOWNLOADREQ,LD_CMDCODE_GETPUBFILE);
        iTmp = GetPubFile(gaucSndBuf + 8);
        if(iTmp >= 0)
		{
			gaucSndBuf[4] = ((iTmp*sizeof(PUBFILE_INFO)+2) >> 8) & 0xff;
			gaucSndBuf[5] = (iTmp*sizeof(PUBFILE_INFO)+2) & 0xff;
			gaucSndBuf[6] = LD_OK;
            gaucSndBuf[7] = iTmp;
			gsLastSndBufLen = 9 + iTmp*sizeof(PUBFILE_INFO);
		}
		else
		{
			gaucSndBuf[6] = LDERR_GENERIC;
			gsLastSndBufLen = 7+1;
		}
		break;
        
	case  LD_CMDCODE_REBUILDFS   ://�ؽ��ļ�ϵͳ
	{	
		vUartDownloadingPromt(IUARTDOWNLOADREQ,LD_CMDCODE_REBUILDFS);
		if(0==s_iRebuildFileSystem())
		{
			
			gaucSndBuf[6]=LD_OK;
		}
		else
		{
			gaucSndBuf[6]=LDERR_GENERIC;
		}
		break;
	}
    
	case  LD_CMDCODE_SETTIME     ://�����ն�ʱ��
	{	
		vUartDownloadingPromt(IUARTDOWNLOADREQ,LD_CMDCODE_SETTIME);
        if(gucComPort==P_USB_DEV)
    		iRet = SetTime(gaucRcvBuf+8);
        else
            iRet = SetTime(gaucRcvBuf+6);
		if(iRet==0)
		{
			gaucSndBuf[6]=LD_OK;
		}
		else 
		{
			gaucSndBuf[6]=LDERR_CLOCKHWERR;
		}
		break;
	}
    
    case LD_CMDCODE_GET_ALL_SONAME : 
	{ 
		vUartDownloadingPromt(IUARTDOWNLOADREQ, LD_CMDCODE_GET_ALL_SONAME);
		iRet = GetAllSoName(gaucSndBuf+7, &iTmp);
        iTmp++;
		if(iRet == 0)
		{
			gaucSndBuf[4] = (iTmp >> 8) & 0xff;
			gaucSndBuf[5] = iTmp & 0xff;
			gaucSndBuf[6] = LD_OK;
            
			gsLastSndBufLen = 7 + iTmp;
		}
		else
		{
			gsLastSndBufLen = 7+1;
			gaucSndBuf[6] = LDERR_GENERIC;
		}
		break;
	}	
		

	case  LD_CMDCODE_DLUPK ://���ع�Կ�ļ�����
	{	
		vUartDownloadingPromt(IUARTDOWNLOADREQ,LD_CMDCODE_DLUPK);
		iLastDLFileCmd =LD_CMDCODE_DLUPK;

        if(gucComPort==P_USB_DEV)
        {
    		lLastDlFileLen =  ((uchar)gaucRcvBuf[8]<<24)
    					+((uchar)gaucRcvBuf[9]<<16)
    					+((uchar)gaucRcvBuf[10]<<8)
    					+((uchar)gaucRcvBuf[11]);
    		iLastDLRsaIndex= gaucRcvBuf[12];
        }
        else
        {
    		lLastDlFileLen =  ((uchar)gaucRcvBuf[6]<<24)
					+((uchar)gaucRcvBuf[7]<<16)
					+((uchar)gaucRcvBuf[8]<<8)
					+((uchar)gaucRcvBuf[9]);
    		iLastDLRsaIndex= gaucRcvBuf[10];
        }
		pucCurFileAddr=(uchar*)LD_FILE_TEMP_START_ADDR;
		gaucSndBuf[6]=LD_OK;
		break;
	}
    
	case  LD_CMDCODE_DLAPP   ://����Ӧ�ó�������
	{	
		vUartDownloadingPromt(IUARTDOWNLOADREQ,LD_CMDCODE_DLAPP);
		iLastDLFileCmd =LD_CMDCODE_DLAPP;
        if(gucComPort==P_USB_DEV)
        {
    		lLastDlFileLen =  ((uchar)gaucRcvBuf[8]<<24)
    					+((uchar)gaucRcvBuf[9]<<16)
    					+((uchar)gaucRcvBuf[10]<<8)
    					+((uchar)gaucRcvBuf[11]);
        }
        else
        {
    		lLastDlFileLen =  ((uchar)gaucRcvBuf[6]<<24)
					+((uchar)gaucRcvBuf[7]<<16)
					+((uchar)gaucRcvBuf[8]<<8)
					+((uchar)gaucRcvBuf[9]);
        }
        
		if(lLastDlFileLen>MAPP_AREA_SIZE)
		{
		    gaucSndBuf[6]=LDERR_APP_TOO_BIG;
		    break;
		}
		pucCurFileAddr=(uchar*)LD_FILE_TEMP_START_ADDR;			
		gaucSndBuf[6]=LD_OK;
		break;
	}
    
	case  LD_CMDCODE_DLFONTLIB   ://�����ֿ��ļ�����
	{	
		vUartDownloadingPromt(IUARTDOWNLOADREQ,LD_CMDCODE_DLFONTLIB);
		iLastDLFileCmd =LD_CMDCODE_DLFONTLIB;
        if(gucComPort==P_USB_DEV)
        {
    		lLastDlFileLen =  ((uchar)gaucRcvBuf[8]<<24)
    					+((uchar)gaucRcvBuf[9]<<16)
    					+((uchar)gaucRcvBuf[10]<<8)
    					+((uchar)gaucRcvBuf[11]);
        }
        else
        {
    		lLastDlFileLen =  ((uchar)gaucRcvBuf[6]<<24)
					+((uchar)gaucRcvBuf[7]<<16)
					+((uchar)gaucRcvBuf[8]<<8)
					+((uchar)gaucRcvBuf[9]);
        }
		pucCurFileAddr=(uchar*)LD_FILE_TEMP_START_ADDR;
		gaucSndBuf[6]=LD_OK;	
		break;
	}
        
	case  LD_CMDCODE_DLMAGDRIVER:
	{	
		vUartDownloadingPromt(IUARTDOWNLOADREQ,LD_CMDCODE_DLMAGDRIVER);
		iLastDLFileCmd =LD_CMDCODE_DLMAGDRIVER;
        if(gucComPort==P_USB_DEV)
        {
    		lLastDlFileLen =  ((uchar)gaucRcvBuf[8]<<24)
					+((uchar)gaucRcvBuf[9]<<16)
					+((uchar)gaucRcvBuf[10]<<8)
					+((uchar)gaucRcvBuf[11]);
        }
        else
        {
    		lLastDlFileLen =  ((uchar)gaucRcvBuf[6]<<24)
					+((uchar)gaucRcvBuf[7]<<16)
					+((uchar)gaucRcvBuf[8]<<8)
					+((uchar)gaucRcvBuf[9]);            
        }
		pucCurFileAddr=(uchar*)LD_FILE_TEMP_START_ADDR;
		gaucSndBuf[6]=LD_OK;	
		break;
	}		


    case  LD_CMDCODE_DLPUBFILE    ://���ع����ļ�����
	{	
		vUartDownloadingPromt(IUARTDOWNLOADREQ,LD_CMDCODE_DLPUBFILE);

		iLastDLFileCmd = LD_CMDCODE_DLPUBFILE;
        if(gucComPort==P_USB_DEV)
        {
            iLastFileType = gaucRcvBuf[13]; 
    		lLastDlFileLen =  ((uchar)gaucRcvBuf[8]<<24)
					+((uchar)gaucRcvBuf[9]<<16)
					+((uchar)gaucRcvBuf[10]<<8)
					+((uchar)gaucRcvBuf[11]);
    		memset(aucLastAppName,0x00,sizeof(aucLastAppName));
    		memcpy(aucLastAppName,gaucRcvBuf+14,16);
        }
        else
        {
            iLastFileType = gaucRcvBuf[11]; 
    		lLastDlFileLen =  ((uchar)gaucRcvBuf[6]<<24)
					+((uchar)gaucRcvBuf[7]<<16)
					+((uchar)gaucRcvBuf[8]<<8)
					+((uchar)gaucRcvBuf[9]);

    		memset(aucLastAppName,0x00,sizeof(aucLastAppName));
    		memcpy(aucLastAppName,gaucRcvBuf+12,16);
        }

		pucCurFileAddr=(uchar*)LD_FILE_TEMP_START_ADDR;
		gaucSndBuf[6]=LD_OK;
		break;
	}
        
	case  LD_CMDCODE_DLPARA      ://���ز����ļ�����
	{	
		vUartDownloadingPromt(IUARTDOWNLOADREQ,LD_CMDCODE_DLPARA);

		iLastDLFileCmd =LD_CMDCODE_DLPARA;

        if(gucComPort==P_USB_DEV)
        {
    		lLastDlFileLen =  ((uchar)gaucRcvBuf[8]<<24)
    					+((uchar)gaucRcvBuf[9]<<16)
    					+((uchar)gaucRcvBuf[10]<<8)
    					+((uchar)gaucRcvBuf[11]);
    		iLastAppNo = gaucRcvBuf[12];
        }
        else
        {
    		lLastDlFileLen =  ((uchar)gaucRcvBuf[6]<<24)
					+((uchar)gaucRcvBuf[7]<<16)
					+((uchar)gaucRcvBuf[8]<<8)
					+((uchar)gaucRcvBuf[9]);
    		iLastAppNo = gaucRcvBuf[10];
        }

		if(iLastAppNo>=MAX_APP_NUM)
		{
			gaucSndBuf[6]=LDERR_GENERIC;
			break;
		}

        if(gucComPort==P_USB_DEV)
        {
    		iLastFileType = gaucRcvBuf[13];
    		if(iLastFileType==FILE_TYPE_USER_FILE)
    		{
    			memset(aucLastAppName,0x00,sizeof(aucLastAppName));
    			memcpy(aucLastAppName,gaucRcvBuf+14,16);
    		}
        }
        else
        {
    		iLastFileType = gaucRcvBuf[11];
    		if(iLastFileType==FILE_TYPE_USER_FILE)
    		{
    			memset(aucLastAppName,0x00,sizeof(aucLastAppName));
    			memcpy(aucLastAppName,gaucRcvBuf+12,16);
    		}
        }
		pucCurFileAddr=(uchar*)LD_FILE_TEMP_START_ADDR;
		gaucSndBuf[6]=LD_OK;	

		break;
	}
        
	case  LD_CMDCODE_DLFILEDATA  ://��ѹ����ʽ�����ļ�����
	{	
		vUartDownloadingPromt(IUARTDOWNLOADREQ,LD_CMDCODE_DLFILEDATA);

        if(gucComPort==P_USB_DEV)
        {
    		iRcvLen = gaucRcvBuf[7]|(gaucRcvBuf[6]<<8)|(gaucRcvBuf[5]<<16)|(gaucRcvBuf[4]<<24);
    		memcpy(pucCurFileAddr,gaucRcvBuf+8,iRcvLen);
        }
        else
        {
    		iRcvLen = ((uchar)gaucRcvBuf[4]<<8)
				+((uchar)gaucRcvBuf[5]);
    		memcpy(pucCurFileAddr,gaucRcvBuf+6,iRcvLen);
        }
		pucCurFileAddr+=iRcvLen;
        
		gaucSndBuf[6]=LD_OK;	
		break;
	}
    
	case  LD_CMDCODE_DLFILEDATAC ://ѹ����ʽ�����ļ�����
	{	
		vUartDownloadingPromt(IUARTDOWNLOADREQ,LD_CMDCODE_DLFILEDATAC);

        if(gucComPort==P_USB_DEV)//usb��֧��ѹ������
        {
    		gaucSndBuf[6]=LDERR_UNSUPPORTEDCMD;
        }
        else
        {
            //����ʹ��ѹ������
            iRcvLen = ((uchar)gaucRcvBuf[4]<<8)
				+((uchar)gaucRcvBuf[5]);
    		memcpy(pucCurFileAddr,gaucRcvBuf+6,iRcvLen);
    		if(Decompress(gaucRcvBuf+6, pucCurFileAddr, iRcvLen, &iTmp))
    		{
    			gaucSndBuf[6]=LDERR_DECOMPRESS_ERR;
    		}
    		else
    		{
    			pucCurFileAddr+=iTmp;
    			gaucSndBuf[6]=LD_OK;	
    		}	
        }
		break;
	}
    
	case  LD_CMDCODE_WRITEFILE   ://д���ļ�
	{	
		//uchar aucSignData[257];
		vUartDownloadingPromt(IUARTDOWNLOADREQ,LD_CMDCODE_WRITEFILE);
		
		if((uint)pucCurFileAddr-LD_FILE_TEMP_START_ADDR!=lLastDlFileLen)
		{
            //�յ���ʵ�ʳ��Ȳ������������ص��ļ����ȡ�
			gaucSndBuf[6] = LDERR_WRITEFILEFAIL;
			break;
		}
	    
		switch(iLastDLFileCmd)
		{
		int iFileId;
		//д�빫Կ�ļ�
		case  LD_CMDCODE_DLUPK         :
		{
		    iRet = s_WritePuk(iLastDLRsaIndex,(uchar*)LD_FILE_TEMP_START_ADDR,lLastDlFileLen);
            if (iRet == 0)
            {
                gaucSndBuf[6] = LD_OK;
                s_reboot = 3;
            }    
			else if(PUK_RET_SIG_ERR==iRet) gaucSndBuf[6] = LDERR_INVALIDSIGNATURE;
            else if(PUK_RET_LEVEL_ERR==iRet) gaucSndBuf[6] = LDERR_BOOT_TOO_OLD;
            else if(PUK_RET_NO_SUPPORT_ERR==iRet) gaucSndBuf[6] = LDERR_NOSUFFICIENTSPACE;
            else if(PUK_RET_SIG_TYPE_ERR==iRet) gaucSndBuf[6] = LDERR_SIGTYPEERR;
			else gaucSndBuf[6] = LDERR_WRITEFILEFAIL;
			break;
		}
        
				//д��Ӧ�ó����ļ�
				case LD_CMDCODE_DLAPP:
				{
					if(0==memcmp((uchar *)(LD_FILE_TEMP_START_ADDR+lLastDlFileLen-strlen(DATA_SAVE_FLAG)), DATA_SAVE_FLAG, strlen(DATA_SAVE_FLAG)))
					{
						//���ԶԿ���Ӧ�ó���β����16�ֽڵ�״̬��־��
						lLastDlFileLen -= 16;//APPFILE_TAIL_LEN;
					}
					if(IsFtestFile((uchar *)LD_FILE_TEMP_START_ADDR,lLastDlFileLen))
						iRet = WriteFtestAppFile((uchar*)LD_FILE_TEMP_START_ADDR, lLastDlFileLen);
					else
                    	iRet = s_iDLWriteApp(LD_FILE_TEMP_START_ADDR, lLastDlFileLen);
					gaucSndBuf[4]=0x00;
					gaucSndBuf[5]=0x02;
					if(iRet >= 0) gaucSndBuf[6]=LD_OK;					
					else if(iRet == -1) gaucSndBuf[6]=LDERR_INVALIDSIGNATURE;
					else if(iRet == -2) gaucSndBuf[6]=LDERR_UNKNOWNAPPTYPE;
					else if(iRet==-3 || iRet==-4) gaucSndBuf[6]=LDERR_REPEAT_APPNAME;
					else if(iRet == -5) gaucSndBuf[6]=LDERR_TOOMANYAPPLICATIONS;						
					else if(iRet == -6) gaucSndBuf[6]=LDERR_NOSUFFICIENTSPACE;
					else if(iRet == -8) gaucSndBuf[6]=LDERR_SIGTYPEERR;
					else
					{
                        if(errno==TOO_MANY_FILES) gaucSndBuf[6]=LDERR_TOOMANYFILES;
                        else gaucSndBuf[6]=LDERR_WRITEFILEFAIL;   
					}    

    				gaucSndBuf[7] = iRet&0xFF;
    				gsLastSndBufLen = 9;                       			
			
			break;
		}
			
		//д���ֿ��ļ�
		case LD_CMDCODE_DLFONTLIB:
		{
			if((freesize()+filesize(FONT_FILE_NAME))<lLastDlFileLen)
			{
			 	gaucSndBuf[6] = LDERR_NOSUFFICIENTSPACE;
			 	break;
			}
			s_remove(FONT_FILE_NAME,(uchar *)"\xff\x02");
			iFileId = s_open((char*)FONT_FILE_NAME,O_CREATE|O_RDWR,(uchar *)"\xff\x02");
			if(iFileId<0)
			{
				if(errno==TOO_MANY_FILES)
					gaucSndBuf[6]=LDERR_TOOMANYFILES;
				else 
                    gaucSndBuf[6]=LDERR_WRITEFILEFAIL;
				break;
			}
			seek(iFileId,0,SEEK_SET);
			if(lLastDlFileLen!=write(iFileId,(uchar*)LD_FILE_TEMP_START_ADDR,lLastDlFileLen))
			{
				gaucSndBuf[6]=LDERR_WRITEFILEFAIL;
				close(iFileId);
				s_remove((char*)FONT_FILE_NAME,(uchar *)"\xff\x02");
			}
			else 
			{
				gaucSndBuf[6]=LD_OK;	
				close(iFileId);
				LoadFontLib();//���¼����ֿ�
				lastStep = 0xff;//ˢ�²˵�����
			}
			break;
		}
            
		case LD_CMDCODE_DLMAGDRIVER:
		{
			//paxfirmware0101  01:sm chip    01:firmware version
			ptr = (uchar*)(LD_FILE_TEMP_START_ADDR+lLastDlFileLen-15);
			if( memcmp(ptr, "paxfirmware",  11) == 0)
			{
				//find firmware type
				ptr += 11;
				firmwaretype = ((ptr[0]-0x30)<<4) + (ptr[1]-0x30);
				//firmware version
				firmwarever = ((ptr[2]-0x30)<<4) + (ptr[3]-0x30);
				iRet = firmware_update(firmwaretype, firmwarever, (uchar*)LD_FILE_TEMP_START_ADDR, lLastDlFileLen-15);
				s_FirmwareType = firmwaretype;
			}
			else
			{
            	iRet = MsrLoadFirmware((uchar*)LD_FILE_TEMP_START_ADDR, lLastDlFileLen);
            }
            if (iRet) gaucSndBuf[6]=LDERR_WRITEFILEFAIL;                    
            else gaucSndBuf[6]=LD_OK;                        
            break;
		}		


		//д������ļ�
		case LD_CMDCODE_DLPARA:
		{
			uchar fname[30];
			uchar aucAttr[3];
    		int iType;
            
			aucAttr[2]=0;
			aucAttr[0] = iLastAppNo&0xff;
			if(iLastFileType>0)
			{	
				aucAttr[1]=iLastFileType&0xff;
			}
			else
			{
				gaucSndBuf[6]=LDERR_WRITEFILEFAIL;
				break;
			}
			//ɾ��ԭ���ļ�
			if(iLastFileType==FILE_TYPE_APP_PARA)
			{
				strcpy(aucLastAppName,ENV_FILE);
			}
			else if(iLastFileType!=FILE_TYPE_USER_FILE && iLastFileType!=LD_PARA_SO)
			{
				gaucSndBuf[6]=LDERR_WRITEFILEFAIL;
				break;
			}
		    else if (iLastFileType==LD_PARA_SO)
			{
				memset(fname,0x00,sizeof(fname));
                head = (SO_HEAD_INFO*)(LD_FILE_TEMP_START_ADDR+SOINFO_OFFSET);
                memcpy(fname,head->so_name,sizeof(fname)-1);
				UpperCase(fname, aucLastAppName, 16);

                aucAttr[0] = iLastAppNo&0xff;
                if(head->lib_type == LIB_TYPE_SYSTEM) 
                {
                    iType = SIGN_FILE_MON;
                    aucAttr[1] = FILE_TYPE_SYSTEMSO;
                }
                else 
                {
                    iType = SIGN_FILE_APP;
                    aucAttr[1] = FILE_TYPE_USERSO;
                }

                iRet = s_iVerifySignature(iType, LD_FILE_TEMP_START_ADDR, lLastDlFileLen, NULL);	
				if(iRet != 0)
				{
					gaucSndBuf[6] = LDERR_INVALIDSIGNATURE;
					break;
				}
                
			}
			s_remove(aucLastAppName, aucAttr);

			if(freesize()<lLastDlFileLen)
			{//�ռ䲻��
				gaucSndBuf[6] = LDERR_NOSUFFICIENTSPACE;
			 	break;
			}
			//д���ļ�
			iFileId = s_open(aucLastAppName,O_CREATE|O_RDWR,aucAttr);
			if(iFileId<0)
			{
				if(errno==TOO_MANY_FILES)
					gaucSndBuf[6]=LDERR_TOOMANYFILES;
				else 
                    gaucSndBuf[6]=LDERR_WRITEFILEFAIL;
				break;
			}
			seek(iFileId,0,SEEK_SET);
			if(lLastDlFileLen!=write(iFileId,(uchar*)LD_FILE_TEMP_START_ADDR,lLastDlFileLen))
			{
				gaucSndBuf[6]=LDERR_WRITEFILEFAIL;
				close(iFileId);
			}
			else 
			{
				gaucSndBuf[6]=LD_OK;	
				close(iFileId);
			}
			
			break;
		}
        
        case LD_CMDCODE_DLPUBFILE:
		{
			uchar aucAttr[3];
            uchar fname[30];
			uchar uaUpperName[30];
            int num,i,iType;
            FILE_INFO finfo[256];
			
			memset(fname, 0, sizeof(fname));
			memset(uaUpperName, 0, sizeof(uaUpperName));

			if(freesize()<lLastDlFileLen)
			{
				gaucSndBuf[6] = LDERR_NOSUFFICIENTSPACE;
			 	break;
			}
            
   			UpperCase(aucLastAppName, uaUpperName, 16);

			if(iLastFileType == LD_PUB_SO) //so�ļ�
            {
		        if(strstr(uaUpperName, FIRMWARE_SO_NAME))        
				{
					strcpy(uaUpperName,(uchar*)LD_FILE_TEMP_START_ADDR);
					iType = SIGN_FILE_MON; 	
                    aucAttr[0] = FILE_ATTR_PUBSO;
				    aucAttr[1] = FILE_TYPE_SYSTEMSO;
                    s_reboot=1;
				}
				else
				{
                    head = (SO_HEAD_INFO*)(LD_FILE_TEMP_START_ADDR+SOINFO_OFFSET);
                    memcpy(fname,head->so_name,sizeof(fname)-1);
					UpperCase(fname, uaUpperName, 16);
                    if(head->lib_type == LIB_TYPE_SYSTEM)
                    {
                    	//���Կ�WIFI_MPATCH
						if(is_wifi_mpatch(head->so_name))
                    	{
                       		gaucSndBuf[6] = LD_OK;	
                        	break;
                    	}
						iType = SIGN_FILE_MON; 	
                        aucAttr[0] = FILE_ATTR_PUBSO;
					    aucAttr[1] = FILE_TYPE_SYSTEMSO;
                    }
                    else
                    {
						iType = SIGN_FILE_APP; 	
                        aucAttr[0] = FILE_ATTR_PUBSO;
					    aucAttr[1] = FILE_TYPE_USERSO;
                    }
				}
                
                iRet = s_iVerifySignature(iType, LD_FILE_TEMP_START_ADDR, lLastDlFileLen, &stSigInfo);	
    			if(iRet != 0)
    			{
    				gaucSndBuf[6] = LDERR_INVALIDSIGNATURE;
                    if(s_reboot==1)s_reboot = 0;
    				break;
    			}
            }
            else //sharefile�ļ�
            {
                SO_HEAD_INFO *header = (SO_HEAD_INFO *)(LD_FILE_TEMP_START_ADDR+SOINFO_OFFSET);
                
                if(strstr(header->so_name, MPATCH_EXTNAME) && header->lib_type==LIB_TYPE_SYSTEM)//mpatch file
                {
                    //���ƷǱ������õ�wifiģ���mpatch����
					if((is_wifi_mpatch(header->so_name)==1 && is_wifi_module()!=1)||
						(is_wifi_mpatch(header->so_name)==2 && is_wifi_module()!=3)||
							strlen(header->so_name)>16)
                    {
                        gaucSndBuf[6] = LDERR_GENERIC;
                        break;
                    }
                    iType = SIGN_FILE_MON; 	
                    aucAttr[0] = FILE_ATTR_PUBSO;
				    aucAttr[1] = FILE_TYPE_SYSTEMSO;
                    memcpy(fname,header->so_name,16);
                    UpperCase(fname, uaUpperName, 16);
                    iRet = s_iVerifySignature(iType, LD_FILE_TEMP_START_ADDR, lLastDlFileLen, &stSigInfo);	
                    if(iRet != 0)
                    {
                        gaucSndBuf[6] = LDERR_INVALIDSIGNATURE;
                        break;
                    }

                    if(GetMpatchSysPara(uaUpperName)!=0)
                    {
                        if(SetMpatchSysPara(uaUpperName, MPATCH_MIN_PARA)!=0)
                            SetMpatchSysPara(uaUpperName, MPATCH_MIN_PARA);//���дpara���������дһ��
                    }
                    s_reboot=2;
                }
                else
                {
    				aucAttr[0] = FILE_ATTR_PUBFILE;
                    aucAttr[1] = FILE_TYPE_USER_FILE;
                }
            }
			s_remove(uaUpperName, aucAttr);

			iFileId = s_open(uaUpperName, O_CREATE|O_RDWR, aucAttr);
			if(iFileId<0)
			{
				if(errno==TOO_MANY_FILES)
					gaucSndBuf[6]=LDERR_TOOMANYFILES;
				else 
					gaucSndBuf[6]=LDERR_WRITEFILEFAIL;
                if(s_reboot==1)s_reboot = 0;
				break;
			}
			seek(iFileId, 0, SEEK_SET);
			if(lLastDlFileLen != write(iFileId,(uchar*)LD_FILE_TEMP_START_ADDR,lLastDlFileLen))
			{
				gaucSndBuf[6] = LDERR_WRITEFILEFAIL;
				close(iFileId);
                if(s_reboot==1)s_reboot = 0;
			}
			else 
			{
				gaucSndBuf[6] = LD_OK;	
				close(iFileId);
                if(s_reboot==1)
                {
                    if(s_BaseSoLoader())s_reboot = 2;
                    else s_reboot = 0;
                }
			}

			break;
		}
		
		default:
    		break;
		}

		iLastDLRsaIndex=-1;
		iLastDLFileCmd=-1;
		lLastDlFileLen=-1;
		pucCurFileAddr=(uchar*)LD_FILE_TEMP_START_ADDR;
		iLastAppNo=-1;
		iLastFileType=-1;
		break;
	}

    case  LD_CMDCODE_DELSO  :
	{	
		vUartDownloadingPromt(IUARTDOWNLOADREQ, LD_CMDCODE_DELSO);

		int iRet;
        if(gucComPort==P_USB_DEV)
        {
    		if(iRet = RemoveSo(&gaucRcvBuf[8],NULL))
            {
    			gaucSndBuf[6] = LDERR_NOPERMIT;
    		}
            else 
            {
                gaucSndBuf[6] = LD_OK;
            }
        }
        else
        {
    		if(iRet = RemoveSo(&gaucRcvBuf[6],NULL))
            {
    			gaucSndBuf[6] = LDERR_NOPERMIT;
    		}
            else 
            {
                gaucSndBuf[6] = LD_OK;
            }
        }
		break;
	}

	case  LD_CMDCODE_DELPUBFILE  ://ɾ����PAXBASE,WIFI_MPATCH��������й����ļ�(E0��E1����),�����ڶԿ�
	{	
		int filenum,i;
		FILE_INFO finfo[MAX_FILES];
		unsigned char aucAttr[2];

		vUartDownloadingPromt(IUARTDOWNLOADREQ, LD_CMDCODE_DELPUBFILE);
		
		filenum = GetFileInfo(finfo); 
		for( i=0;i<filenum;i++)
		{
			if (finfo[i].attr == FILE_ATTR_PUBSO || 
				finfo[i].attr == FILE_ATTR_PUBFILE )
			{
				if( finfo[i].attr == FILE_ATTR_PUBSO && 
					strstr(finfo[i].name, FIRMWARE_SO_NAME) )continue;//PAXBASE����(��ֹ��monitor������)
				//WIFI_MPATCH����
				if( finfo[i].attr == FILE_ATTR_PUBSO &&
					is_wifi_mpatch(finfo[i].name))continue;
				aucAttr[0] = finfo[i].attr;
				aucAttr[1] = finfo[i].type;
				s_remove(finfo[i].name, aucAttr);//ɾ���������й����ļ�
			}
		}
        gaucSndBuf[6] = LD_OK;
        
		break;
	}
	case LD_CMDCODE_WRITECSN:
	{
		gaucSndBuf[6]=LD_OK;
		vUartDownloadingPromt(IUARTDOWNLOADREQ,LD_CMDCODE_WRITECSN);

		if(gucComPort==P_USB_DEV)
		{
			iRcvLen =((uchar)gaucRcvBuf[4]<<24)+((uchar)gaucRcvBuf[5]<<16)+((uchar)gaucRcvBuf[6]<<8)
					+((uchar)gaucRcvBuf[7]);
			iOffset = 8;
		}
		else
		{
			iRcvLen = ((uchar)gaucRcvBuf[4]<<8)
					+((uchar)gaucRcvBuf[5]);
			iOffset = 6;
		}
		if(iRcvLen > 128)
		{
			gaucSndBuf[6]=LDERR_GENERIC;
			gaucSndBuf[0]=gaucRcvBuf[0];
			gaucSndBuf[1]=gaucRcvBuf[1];	
			gaucSndBuf[2]=gaucRcvBuf[2];
			gaucSndBuf[3] = gaucRcvBuf[3];
			gsLastMsgNo = gaucRcvBuf[1];	
			gsLastCmd = gaucRcvBuf[3];
			*piResult = 0;
			iSndMsg(piResult,gsLastSndBufLen);
			return IUARTDOWNLOADDONE;				
		}
		gaucRcvBuf[iOffset + iRcvLen] = 0;
		iRet = WriteCSN(gaucRcvBuf+iOffset);
		if(iRet != 0)
		{
			gaucSndBuf[6]=LDERR_GENERIC;
		}
		else
		{
			gaucSndBuf[6]=LD_OK;
		}
		gaucSndBuf[0]=gaucRcvBuf[0];
		gaucSndBuf[1]=gaucRcvBuf[1];	
		gaucSndBuf[2]=gaucRcvBuf[2];
		gaucSndBuf[3] = gaucRcvBuf[3];
		gsLastMsgNo = gaucRcvBuf[1];	
		gsLastCmd = gaucRcvBuf[3];
		*piResult = 0;
		iSndMsg(piResult,gsLastSndBufLen);
		return IUARTDOWNLOADDONE;	
		break;
	}
	case  LD_CMDCODE_DELAPP      ://ɾ��Ӧ�ó���
	{	
		vUartDownloadingPromt(IUARTDOWNLOADREQ,LD_CMDCODE_DELAPP);

        if(gucComPort==P_USB_DEV)
    		s_iDeleteApp(gaucRcvBuf[8]);
        else
            s_iDeleteApp(gaucRcvBuf[6]);
		gaucSndBuf[6]=LD_OK;
		break;
	}
    
	case LD_CMDCODE_DELALLAPP://ɾ������Ӧ�ü��ֿ�
	{
		for(iLoop = 0;iLoop<MAX_APP_NUM;iLoop++)
		{
			s_iDeleteApp(iLoop);
		}
		
		s_remove(FONT_FILE_NAME,(uchar *)"\xff\x02");
		gaucSndBuf[6]=LD_OK;
		break;
	}
    
	case LD_CMDCODE_GETCONFTABLE:
	{
		iRet=ReadSecuConfTab(gaucSndBuf+11);

		if(iRet<=0)
		{
			gaucSndBuf[6]=LDERR_GENERIC;
			break;
		}
		gaucSndBuf[6]=LD_OK;
		gaucSndBuf[7]=(iRet>>24)&0xff;
		gaucSndBuf[8]=(iRet>>16)&0xff;
		gaucSndBuf[9]=(iRet>>8)&0xff;
		gaucSndBuf[10]=(iRet>>0)&0xff;
		iRet +=4+1;//
		gaucSndBuf[4]=(iRet>>8)&0xff;
		gaucSndBuf[5]=iRet&0xff;
		gsLastSndBufLen = 7+iRet;
		break;
	}
	case LD_CMDCODE_DLPAUSE: //
	{
		vUartDownloadingPromt(IUARTDOWNLOADREQ,LD_CMDCODE_DLPAUSE);
		gaucSndBuf[6]=LD_OK;
		gaucSndBuf[0]=gaucRcvBuf[0];
		gaucSndBuf[1]=gaucRcvBuf[1];    
		gaucSndBuf[2]=gaucRcvBuf[2];
		gaucSndBuf[3] = gaucRcvBuf[3];
		*piResult = 0;
		iSndMsg(piResult,gsLastSndBufLen);

		//Beep();
		while(1)
		{
			ScrBackLight(0);
			DelayMs(500);
			ScrBackLight(1);
			DelayMs(500);
			if(!kbhit() && getkey() == KEYCANCEL) return IUARTDOWNLOADDONE;
		}
	}       	

	case  LD_CMDCODE_DLCOMPLETE  ://�������
	{	
		vUartDownloadingPromt(IUARTDOWNLOADREQ,LD_CMDCODE_DLCOMPLETE);
		gaucSndBuf[6]=LD_OK;
		gaucSndBuf[0]=gaucRcvBuf[0];
		gaucSndBuf[1]=gaucRcvBuf[1];	
		gaucSndBuf[2]=gaucRcvBuf[2];
		gaucSndBuf[3] = gaucRcvBuf[3];
		gsLastMsgNo = gaucRcvBuf[1];	
		gsLastCmd = gaucRcvBuf[3];
		*piResult = 0;
		iSndMsg(piResult,gsLastSndBufLen);
		return IUARTDOWNLOADDONE;
	}
		case  LD_CMDCODE_GETCFGINFO :
		{	
			vUartDownloadingPromt(IUARTDOWNLOADREQ,LD_CMDCODE_GETCFGINFO);
            usTmp = DLGetCfgInfo(gaucSndBuf+7);
            usTmp+=1;
            gaucSndBuf[4]=(usTmp>>8)&0xff;
            gaucSndBuf[5]=usTmp&0xff;
            gaucSndBuf[6]=LD_OK;
            gsLastSndBufLen = 7+usTmp;
            break;
		}
    	case  LD_CMDCODE_SETCFGINFO     :
    	{	
    		vUartDownloadingPromt(IUARTDOWNLOADREQ,LD_CMDCODE_SETCFGINFO);
            if(gucComPort==P_USB_DEV) iRet = DLSetCfgInfo(gaucRcvBuf+8);
            else iRet = DLSetCfgInfo(gaucRcvBuf+6);
    		if(iRet==0)
    		{
    		    gaucSndBuf[6]=LD_OK;
    		    s_reboot = 4;
    		}    
    		else if(iRet==-2) gaucSndBuf[6]=LDERR_BOOT_TOO_OLD;
    		else gaucSndBuf[6]=LDERR_CLOCKHWERR;
    		break;
    	}
		case LD_CMDCODE_SHUTDOWN://Ӳ�ػ�
		{
			if(s_FirmwareType == SM_THK88_FIRMWARE )//sm firmware update 
		{
			s_reboot=5;
		}
		gaucSndBuf[6]=LD_OK;
		gaucSndBuf[0]=gaucRcvBuf[0];
		gaucSndBuf[1]=gaucRcvBuf[1];	
		gaucSndBuf[2]=gaucRcvBuf[2];
		gaucSndBuf[3] = gaucRcvBuf[3];
		gsLastMsgNo = gaucRcvBuf[1];	
		gsLastCmd = gaucRcvBuf[3];
		*piResult = 0;
		iSndMsg(piResult,gsLastSndBufLen);
		return IUARTDOWNLOADDONE;
		}
		case LD_CMDCODE_GETSNKEYINFO://��ȡ����SN_KEY��Ϣ
		{
            usTmp = DLGetSNKeyInfo(gaucSndBuf+7);
            if (usTmp > 0) {
                gaucSndBuf[6]=LD_OK;
                usTmp+=1;
            }
            else  {
                gaucSndBuf[6]=LDERR_GENERIC;
                usTmp = 1;
            }
            gaucSndBuf[4]=(usTmp>>8)&0xff;
            gaucSndBuf[5]=usTmp&0xff;
            
            gsLastSndBufLen = 7+usTmp;
            gaucSndBuf[0]=gaucRcvBuf[0];
			gaucSndBuf[1]=gaucRcvBuf[1];	
			gaucSndBuf[2]=gaucRcvBuf[2];
			gaucSndBuf[3] = gaucRcvBuf[3];
			gsLastMsgNo = gaucRcvBuf[1];	
			gsLastCmd = gaucRcvBuf[3];
			*piResult = 0;
			iSndMsg(piResult,gsLastSndBufLen);
			return IUARTDOWNLOADDONE;
		}
		case LD_CMDCODE_SETSNKEYSTATE://���ü���SN_KEY�Ƿ��ϴ��ı�־λ
		{
            if(gucComPort==P_USB_DEV) iRet = DLSetSNKeyState(gaucRcvBuf+8);
            else iRet = DLSetSNKeyState(gaucRcvBuf+6);
    		if(iRet==0) gaucSndBuf[6]=LD_OK;
    		else gaucSndBuf[6]=LDERR_GENERIC;
			gaucSndBuf[0]=gaucRcvBuf[0];
			gaucSndBuf[1]=gaucRcvBuf[1];	
			gaucSndBuf[2]=gaucRcvBuf[2];
			gaucSndBuf[3] = gaucRcvBuf[3];
			gsLastMsgNo = gaucRcvBuf[1];	
			gsLastCmd = gaucRcvBuf[3];
			*piResult = 0;
			iSndMsg(piResult,gsLastSndBufLen);
			return IUARTDOWNLOADDONE;
		}
		case LD_CMDCODE_SETTUSNFLAG://����TUSN�ı�־λ
		{
            if(gucComPort==P_USB_DEV) iRet = DLSetTUSNFlag(gaucRcvBuf+8);
            else iRet = DLSetTUSNFlag(gaucRcvBuf+6);
    		if(iRet==0) gaucSndBuf[6]=LD_OK;
    		else gaucSndBuf[6]=LDERR_GENERIC;
			gaucSndBuf[0]=gaucRcvBuf[0];
			gaucSndBuf[1]=gaucRcvBuf[1];	
			gaucSndBuf[2]=gaucRcvBuf[2];
			gaucSndBuf[3] = gaucRcvBuf[3];
			gsLastMsgNo = gaucRcvBuf[1];	
			gsLastCmd = gaucRcvBuf[3];
			*piResult = 0;
			iSndMsg(piResult,gsLastSndBufLen);
			return IUARTDOWNLOADDONE;
		}			
		default:
		{
			break;
		}
	}
	gaucSndBuf[0]=gaucRcvBuf[0];
	gaucSndBuf[1]=gaucRcvBuf[1];	
	gaucSndBuf[2]=gaucRcvBuf[2];
	gaucSndBuf[3] = gaucRcvBuf[3];
	gsLastMsgNo = gaucRcvBuf[1];	
	gsLastCmd = gaucRcvBuf[3];
	iSndMsg(piResult,gsLastSndBufLen);
	vDownLoadPrtErr(gaucSndBuf[6]);
    
	return  IUARTDOWNLOADREQ;
}

/**
 ״̬������״̬��״̬���˳�ʱ������뱾״̬��
 �ڱ�״̬������piResultֵ��������Ӧ����
 @param[in,out]  *piResult,��һ״̬������
 @retval #IUARTDOWNLOADEXIT ��һ״̬�����ؽ���
 @remarks
	 \li ����-----------------ʱ��--------�汾---------�޸�˵�� 				   
	 \li PengRongshou---07/06/28--1.0.0.0-------����
 */
int iUartDownloadDone( int *piResult)
{	
	int iTmp;
	gucDownloadNeedPrompt=0;	
	giShakeTimeOut=SHAKE_HAND_TIMEOUT_DEFAULT;
	if(gucComPort!=0xFF)
	{
		gucComPort = 0xff;
	}
	for(iTmp=0;iTmp<sizeof(gaucComPortList);iTmp++)
	{
		if(gaucComPortList[iTmp]==0xff) continue;
		else 	PortClose(gaucComPortList[iTmp]);
	}
	switch(*piResult)
	{
		case RET_UART_OK:
		{
			if(gucDownloadNeedPrompt)
				vUartDownloadingPromt(IUARTDOWNLOADDONE,0);

			break;
		}
		case RET_UART_SHAKE_TIMEOUT:
		default:
			break;
	}
    
    switch(s_reboot)
    {
        case 2:
            ScrCls();
            SCR_PRINT(0, 3, 1, "  ���¹̼�",  "Update Firmware"); 
            SCR_PRINT(0, 5, 1, "   ������...", "   Reboot...");
            DelayMs(2000);
            Soft_Reset();           
        break;
        case 3:
            ScrCls();        
            SCR_PRINT(0, 3, 1, "  ���¹�Կ",  "Update Puk");
            SCR_PRINT(0, 5, 1, "   ������...", "   Reboot...");
            DelayMs(2000);
            Soft_Reset();            
        break;
        case 4:
            ScrCls();
            SCR_PRINT(0, 3, 1, "  ���������ļ�",  "Update CfgFile");
            SCR_PRINT(0, 5, 1, "   ������...", "   Reboot...");
            DelayMs(2000);
            Soft_Reset();            
        break;
        case 5:
            while(1) 
            {
                ScrCls();
                SCR_PRINT(0, 3, 1, "���¹̼����",  "Update Firmware OK");
                SCR_PRINT(0, 5, 1, "��ػ�", "Plz Shutdown");
                getkey();
            }        
        default:
        break;        
    }
    
	return IUARTDOWNLOADEXIT;
}


/**
 ״̬��������������״̬�����
 @param[in]iMode
	\li #FRONT_MODE 1 :ǰ̨ģʽ
	\li #BACKGOUND_MODE  0 :��̨ģʽ��û�н��յ�����ǰ������lcd������ʾ
@retval #RET_UART_REQ_TIMEOUT �������ʱ
@remarks
	\li ����-----------------ʱ��--------�汾---------�޸�˵��                    
 	\li PengRongshou---07/06/28--1.0.0.0-------����
*/
int iUartDownloadMain(int iMode)
{
	STATUSTABLE aUartDStatus[] =
	{
		{ IUARTDOWNLOADINIT, iUartDownloadInit},
		{ IUARTDOWNLOADSHAKE, iUartDownloadShake},
		{ IUARTDOWNLOADREQ, iUartDownloadReq},
		{ IUARTDOWNLOADDONE, iUartDownloadDone},
		{ -1, 0}
	};
	STATUSTABLE *p;
	int iResult=-1;
	int iStatusNumber;
	if(iMode!=0)
	{
		gucDownloadNeedPrompt =1;
		giShakeTimeOut = SHAKE_HAND_TIMEOUT;
	}
	else
	{
		gucDownloadNeedPrompt =0;
		giShakeTimeOut = SHAKE_HAND_TIMEOUT_BACKGROUND;

	}
	iStatusNumber = IUARTDOWNLOADINIT;
	while ( iStatusNumber != IUARTDOWNLOADEXIT )
	{
		for ( p = ( STATUSTABLE *)aUartDStatus ; p->pFunction != 0 ; p++ )
		{
			if ( iStatusNumber == p->iStatusNumber )
			{
				iStatusNumber = ( *p->pFunction)( &iResult );
				break;
			}
		}
		
		if ( p->iStatusNumber == -1 || p->pFunction == NULL )
		{
			iStatusNumber = IUARTDOWNLOADINIT;
		}
	}

	return iResult;
}

static uchar GetXOR(uchar sum,uchar *buf,uint len)
{
	uchar a;
	uint i;

	for(i=0,a=sum;i<len;i++)
	  a^=buf[i];

	return a;
}

/**
�����������ݰ���
@param[out]*piResult:������	
@retval >0���յ����ݰ�����
@retval <=0 ����
@remarks
	\li ����-----------------ʱ��--------�汾---------�޸�˵��                    
 	\li PengRongshou---07/06/28--1.0.0.0-------����
*/
static int iRecvMsg(int *piResult)
{
	int iRet,iTmp,iLen,iRcvLen;
    
	*piResult =RET_UART_REQ_TIMEOUT;
	if(gucComPort!=0xff)
	{
		s_TimerSetMs(TM_UART_DOWNLOAD_CMD,DOWNLOAD_TIMEOUT);
		while(s_TimerCheckMs(TM_UART_DOWNLOAD_CMD))
		{
			iRet = PortRecv(gucComPort, gaucRcvBuf,  0);
			if(iRet!=0) continue;
			if(gaucRcvBuf[0]== SHAKE_REQUEST)
			{
				PortSend(gucComPort, SHAKE_REPLY);
				continue;
			}
			if(gaucRcvBuf[0]!=STX)
			{
				continue;
			}
			RECV_PACK_SEQ:
			iRet = PortRecv(gucComPort, gaucRcvBuf+1, 20);
			if(iRet!=0) continue;			
			iRet = PortRecv(gucComPort, gaucRcvBuf+2, 20);
			if(iRet!=0) continue;
			if(gaucRcvBuf[2]==STX)
			{
				goto RECV_PACK_SEQ;	
			}
			else if(gaucRcvBuf[2]!=LD_CMD_TYPE)
			{
				continue;
			}
			iRet = PortRecv(gucComPort, gaucRcvBuf+3, 20);
			if(iRet!=0) continue;
			else
			{	
				*piResult = RET_UART_OK;
				break;
			}
		}

	}
	else
	{
		return -1;
	}

	if(*piResult ==RET_UART_REQ_TIMEOUT)
	{
		return  -2;
	}
	
	iRet = PortRecvs(gucComPort, gaucRcvBuf+4, 2, DOWNLOAD_TIMEOUT);
	if(iRet!=2)
	{
		*piResult =RET_UART_REQ_TIMEOUT;
		return -3;
	}
	iLen = ((uchar)gaucRcvBuf[4]<<8)+(uchar)gaucRcvBuf[5];
	iLen++;
	
	iRcvLen=0;
	
	s_TimerSetMs(TM_UART_DOWNLOAD_CMD,DOWNLOAD_RECVPACK_TIMEOUT);
	while((iLen>0)&&s_TimerCheckMs(TM_UART_DOWNLOAD_CMD))
	{
		iTmp = PortRecvs(gucComPort, gaucRcvBuf+6+iRcvLen, iLen, DOWNLOAD_RECVPACK_TIMEOUT);
		if(iTmp>0)
		{
			iRcvLen+=iTmp;
			iLen -=iTmp;
			s_TimerSetMs(TM_UART_DOWNLOAD_CMD,DOWNLOAD_RECVPACK_TIMEOUT);
		}
	}	
	if(iLen>0)
	{
		
		*piResult =RET_UART_RCV_DATA_TIMEOUT;
		return -5;
	}
	if(gaucRcvBuf[6+iRcvLen-1]!=ucGenLRC(gaucRcvBuf+1, iRcvLen+6-2))
	{
		*piResult =RET_UART_LRC_ERR;
		return -6;
	}
	
	*piResult =RET_UART_OK;
    
	return  iRcvLen+6;
}

/**
�����������ݰ���
@param[out]*piResult:������	
@retval >0���յ����ݰ�����
@retval <=0 ����
@remarks
	\li ����-----------------ʱ��--------�汾---------�޸�˵��                    
 	\li PengRongshou---07/06/28--1.0.0.0-------����
*/
static int iRecvMsgH(int *piResult)
{
	int iRet,iTmp,iLen,iRcvLen,len;
    uchar sum;
    
	*piResult =RET_UART_REQ_TIMEOUT;
	if(gucComPort!=0xff)
	{
		s_TimerSetMs(TM_UART_DOWNLOAD_CMD,DOWNLOAD_TIMEOUT);
		while(s_TimerCheckMs(TM_UART_DOWNLOAD_CMD))
		{
			iRet = PortRecv(gucComPort, gaucRcvBuf,  0);
			if(iRet!=0) continue;
			if(gaucRcvBuf[0]== SHAKE_REQUEST)
			{
				PortSend(gucComPort, SHAKE_REPLY_H);
				continue;
			}
			if(gaucRcvBuf[0]!=STX)
			{
				continue;
			}
			RECV_PACK_SEQ:
			iRet = PortRecv(gucComPort, gaucRcvBuf+1, 20);
			if(iRet!=0) continue;			
			iRet = PortRecv(gucComPort, gaucRcvBuf+2, 20);
			if(iRet!=0) continue;
			if(gaucRcvBuf[2]==STX)
			{
				goto RECV_PACK_SEQ;	
			}
			else if(gaucRcvBuf[2]!=LD_CMD_TYPE)
			{
				continue;
			}
			iRet = PortRecv(gucComPort, gaucRcvBuf+3, 20);
			if(iRet!=0) continue;
			else
			{	
				*piResult = RET_UART_OK;
				break;
			}
		}

	}
	else
	{
		return -1;
	}

	if(*piResult ==RET_UART_REQ_TIMEOUT)
	{
		return  -2;
	}
	
	iRet = PortRecvs(gucComPort, gaucRcvBuf+4, 4, DOWNLOAD_TIMEOUT);
	if(iRet!=4)
	{
		*piResult =RET_UART_REQ_TIMEOUT;
		return -3;
	}
	iLen = gaucRcvBuf[7]|(gaucRcvBuf[6]<<8)|(gaucRcvBuf[5]<<16)|(gaucRcvBuf[4]<<24); 
	iLen++;//+LRC

	sum = GetXOR(0,gaucRcvBuf+1,7);
	for(iRcvLen=0;iRcvLen<iLen;)
	{
        if((iLen-iRcvLen)>LD_MAX_RECV_DATA_LEN)len=LD_MAX_RECV_DATA_LEN;
        else len = iLen-iRcvLen;
        
		iTmp = PortRecvs(gucComPort, (uchar *)(gaucRcvBuf+8+iRcvLen), len, 3000);
        if(iTmp<=0)
        {
    		*piResult =RET_UART_RCV_DATA_TIMEOUT;
    		return -5;
    	}
        sum = GetXOR(sum, (uchar *)(gaucRcvBuf+8+iRcvLen), iTmp);            
    	iRcvLen+=iTmp;

	}	
    
	if(sum)
	{
		*piResult =RET_UART_LRC_ERR;
		return -6;
	}
	
	*piResult =RET_UART_OK;
    
	return  iRcvLen+6;
}

/**
������ȫ�ֱ���gaucSndBuf�е����ݷ���
@param[in]iSndLen���͵ĳ��ȡ�
@param[out]piResult ������	
@retval 0�ɹ�
@retval ��0--����ʧ��
@remarks
	\li ����-----------------ʱ��--------�汾---------�޸�˵��                    
 	\li PengRongshou---07/06/28--1.0.0.0-------����
*/
static int iSndMsg(int *piResult,int iSndLen)
{
	uchar ucRet;

	gaucSndBuf[iSndLen-1]=ucGenLRC(gaucSndBuf+1,iSndLen-2);
	ucRet = PortSends(gucComPort,gaucSndBuf, iSndLen);
	if(ucRet!=0)
	{
		*piResult = RET_UART_ERR;		
		return -1;
	}
	
	*piResult = RET_UART_OK;
	return 0;
}
/**

������ʾ������
@param[in]iStep ����
@param[in]iCmd ����
@retval 
@remarks
	 \li ����-----------------ʱ��--------�汾---------�޸�˵�� 				   
	 \li PengRongshou---07/06/18--1.0.0.0-------����
 */
void vUartDownloadingPromt(int iStep,int iCmdorErrCode)
{
	static char pszDisplay[]="...............";
	static uchar ucDisplayCnt=0;
	static char pszChnMsg[32];
	static char pszEngMsg[32];

    if(lastStep==0xFF)
    {
    	ScrCls();
       	SCR_PRINT(0, 0, 0x81, "    ��������    ", " UART DOWNLOAD  ");
    }

	switch(iStep)
	{
		case IUARTDOWNLOADINIT:
		{
            ScrClrLine(4, 5);
			SCR_PRINT(0, 4, 0x01,"��ʼ��...","INITIALIZING...\n");
			break;
		}
        
		case IUARTDOWNLOADSHAKE:
		{
            ScrClrLine(4, 5);
			SCR_PRINT(0, 4, 0x01,"������...","CONNECTING...\n");
			break;
		}
        
		case IUARTDOWNLOADREQ:
		{
			switch(iCmdorErrCode)
			{
				case LD_CMDCODE_SETBAUTRATE :{strcpy(pszChnMsg,"���ò�����...");strcpy(pszEngMsg,"Set Baudrate...");break;}
				case LD_CMDCODE_GETPOSINFO  :{strcpy(pszChnMsg,"��ȡ�ն���Ϣ");strcpy(pszEngMsg,"Read TermInfo");break;}
				case LD_CMDCODE_GETAPPSINFO :{strcpy(pszChnMsg,"��ȡӦ����Ϣ...");strcpy(pszEngMsg,"Read AppInfo...");break;}
				case LD_CMDCODE_GETSOINFO   :{strcpy(pszChnMsg,"��ȡSO��Ϣ...");strcpy(pszEngMsg, "Read SoInfo...");break;}
				case LD_CMDCODE_REBUILDFS   :{strcpy(pszChnMsg,"�ؽ��ļ�ϵͳ...");strcpy(pszEngMsg,"Rebuild FS...");break;}
				case LD_CMDCODE_GET_ALL_SONAME :{strcpy(pszChnMsg,"��ȡ����SO��...");strcpy(pszEngMsg, "Get All SO...");break;}
				case LD_CMDCODE_SETTIME     :{strcpy(pszChnMsg,"�����ն�ʱ��...");strcpy(pszEngMsg,"Set Time...");break;}
				case LD_CMDCODE_DLUPK       :{strcpy(pszChnMsg,"���ع�Կ...");strcpy(pszEngMsg,"Download PUK...");break;}
				case LD_CMDCODE_DLAPP       :{strcpy(pszChnMsg,"����Ӧ��...");strcpy(pszEngMsg,"Download APP...");break;}
				case LD_CMDCODE_DLFONTLIB   :{strcpy(pszChnMsg,"�����ֿ�...");strcpy(pszEngMsg,"Download FONT...");break;}
				case LD_CMDCODE_DLMAGDRIVER :{strcpy(pszChnMsg,"���ع̼�...");strcpy(pszEngMsg,"Download DRIVER...");break;}
				case LD_CMDCODE_DLPARA      :{strcpy(pszChnMsg,"���ز���...");strcpy(pszEngMsg,"Download PARAM...");break;}
				case LD_CMDCODE_DLPUBFILE :{strcpy(pszChnMsg,"���ع����ļ�...");strcpy(pszEngMsg,"Download Public...");break;}
				case LD_CMDCODE_DLFILEDATA :{break;}
				case LD_CMDCODE_DLFILEDATAC :{break;}
				case LD_CMDCODE_WRITEFILE   :{break;}//{strcpy(pszChnMsg,"�����ļ�...");strcpy(pszEngMsg,"Save File...");break;}
				case LD_CMDCODE_DELAPP      :{strcpy(pszChnMsg,"ɾ��Ӧ��...");strcpy(pszEngMsg,"Del App...");break;}
				case LD_CMDCODE_DELSO		:{strcpy(pszChnMsg,"ɾ��SO...");strcpy(pszEngMsg, "Del SO...");break;}
				case LD_CMDCODE_DELPUBFILE:{strcpy(pszChnMsg,"ɾ�������ļ�...");strcpy(pszEngMsg, "Del Public...");break;}
                case LD_CMDCODE_DLPAUSE:{strcpy(pszChnMsg,"�������,������!");strcpy(pszEngMsg,"Ok,plz reboot!");break;}				
				case LD_CMDCODE_DLCOMPLETE  :{strcpy(pszChnMsg,"�������...");strcpy(pszEngMsg,"Completed...");break;}	
				default:{strcpy(pszChnMsg,"��������...");strcpy(pszEngMsg,"Downloading...");break;}
				
			}

            if(iCmdorErrCode != gsLastCmd || lastStep!=iStep)
            {
    			pszChnMsg[16]=0;
    			pszEngMsg[16]=0;

                ScrClrLine(4, 5);
    			SCR_PRINT(0, 4, 0x01,pszChnMsg,pszEngMsg);
            }
            
			if(LD_CMDCODE_WRITEFILE==iCmdorErrCode)
			{
                ScrClrLine(6, 7);
				SCR_PRINT(0, 6, 0x01,"���ڱ���...","Save File...");
			}
			else
			{
                if(iCmdorErrCode != gsLastCmd || lastStep!=iStep)
                    ucDisplayCnt=0;
                
                if(ucDisplayCnt==0)
                    ScrClrLine(6, 7);
                else
    				ScrPrint(0, 6, 0x01,pszDisplay+(5-ucDisplayCnt)*3);
				ucDisplayCnt = (ucDisplayCnt+1)%6;
			}
			break;
		}
        
		case IUARTDOWNLOADDONE:
		{
            ScrClrLine(4, 5);
			SCR_PRINT(0, 4, 0x01,"���������...","Completed...");
			break;
		}
        
		default:
		{
			break;
		}
	}

    lastStep=iStep;

	return;
}
void vDownLoadPrtErr(int iErrCode)
{
	char pszChn[24],pszEng[24];
	strcpy(pszChn,"ʧ��");
	strcpy(pszEng,"Fail.");
	switch(iErrCode)
	{
		case LD_OK :
		{
			strcpy(pszChn,"�ɹ�");
			strcpy(pszEng,"OK");
			return;
		}
		case LDERR_GENERIC :
		{
//			strcpy(pszChn,"");
//			strcpy(pszEng,"");
			break;
		}                            
		case LDERR_HAVEMOREDATA :
		{
			return;
		}                                           
		case LDERR_UNSUPPORTEDBAUDRATE:
		{
			strcpy(pszChn,"�����ʲ�֧��");
			strcpy(pszEng,"Baudrate error.");
			break;
		}                            
		case LDERR_INVALIDTIME    :
		{
			strcpy(pszChn,"�Ƿ�ʱ��");
			strcpy(pszEng,"Timer error.");
			break;
		}                                            
		case LDERR_CLOCKHWERR:
		{
			strcpy(pszChn,"Ӳ��ʱ�Ӵ���");
			strcpy(pszEng,"RTC error.");
			break;
		}                                               
		case LDERR_INVALIDSIGNATURE :
		{
			strcpy(pszChn,"��֤ǩ��ʧ��");
			strcpy(pszEng,"Verify SIG fail.");
            if(s_GetTamperStatus()) SCR_PRINT(0,2,0x01,"POS�Ѵ���","POS TAMPERED!");
			break;
		}                                    
		case LDERR_TOOMANYAPPLICATIONS:
		{
			strcpy(pszChn,"Ӧ��̫��");
			strcpy(pszEng,"Too many app");
			break;
		}                             
		case LDERR_TOOMANYFILES :
		{
			strcpy(pszChn,"�ļ�̫��");
			strcpy(pszEng,"Too many files.");
			break;
		}                                            
		case LDERR_APPLCIATIONNOTEXIST:
		{
			strcpy(pszChn,"ָ��Ӧ�ò�����");
			strcpy(pszEng,"App not exist.");
			return;
		}                               
		case LDERR_UNKNOWNAPPTYPE:
		{
			strcpy(pszChn,"����ʶ��Ӧ������");
			strcpy(pszEng,"Unknow app type.");
			break;
		}                            		
		case LDERR_SIGTYPEERR:
		{
			strcpy(pszChn,"ǩ���ļ����ʹ���");
			strcpy(pszEng,"SIG type error.");
			break;
		}                            				
		case LDERR_SIGAPPERR:
		{
			strcpy(pszChn,"����Ӧ��������");
			strcpy(pszEng,"SIG attr error.");
			break;
		}                            				
		case LDERR_WRITEFILEFAIL:
		{
			strcpy(pszChn,"д���ļ�ʧ��");
			strcpy(pszEng,"Write file fail.");
			break;
		}                                             
		case LDERR_NOSUFFICIENTSPACE  :
		{
			strcpy(pszChn,"û���㹻�ռ�");
			strcpy(pszEng,"No space.");
			break;
		}                                 
		case LDERR_TIMEOUT   :
		{
			strcpy(pszChn,"��ʱ");
			strcpy(pszEng,"Time out");
			break;
		}                                                     
		case LDERR_UNSUPPORTEDCMD :
		{
			strcpy(pszChn,"��֧�ֵ�����");
			strcpy(pszEng,"CMD not support.");
			break;
		}                                      
		case LDERR_DECOMPRESS_ERR:
		{
			strcpy(pszChn,"��ѹʧ��");
			strcpy(pszEng,"Decompress fail.");
			break;
		}                               
		case LDERR_FWVERSION:
		{
			strcpy(pszChn,"�̼��汾��ƥ��");
			strcpy(pszEng,"FW Not Match.");
			break;
		} 
		
		default:
		{
		}
	}
	SCR_PRINT(0,6,0x01,pszChn,pszEng);
	GetKeyMs(300);
	return;
}

/*
 *type: 00-99
 *ver:00-99
 *dat:the data of module firmware
 *len:the length of module firmware.
 */
int firmware_update(unsigned char type, unsigned char ver, unsigned char *dat, int len)
{
	if(type == SM_THK88_FIRMWARE && GetSMType())
	{
		return thk88_update_firmware(dat, len);
	}
	return (-1);
}

