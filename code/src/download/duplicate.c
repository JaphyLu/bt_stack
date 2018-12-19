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
*  FileName: Duplicate.c
*  Author:	PengRongshou	 
*  Version : 1.0		
*  Date:2007/7/12
*  Description: �Կ�
	

*  Function List:	

*  History: 		// ��ʷ�޸ļ�¼
   <author>  <time>   <version >   <desc>
   PengRongshou	07/09/10	 1.0	 build this moudle
2010.10.26 
�޸ĶԿ��ļ�ʱ���ȴ�д���ļ��ĳ�ʱ����ԭ��1M�ļ���25�����ӵ�85��
�޸ĸ������£�����flash�ĵ���������д�����ֽں�ʱ100us,1MB���ʱ50�룬���ϲ�����ʱ8�루һ������0.5�룩�����Եȴ�1MBд�볬ʱ����60��
---------------------------------------------------------------*/
 /** 
 * @addtogroup 
 * 
 * @{
 */
#include <stdio.h>
#include "base.h"
#include "../file/filedef.h"
#include "localdownload.h"

#include "posapi.h"
#include "../encrypt/rsa.h"
#include "../puk/puk.h"
#include "../comm/comm.h"

#ifndef NULL
#define NULL 0
#endif

#define	SHAKE_HAND_TIMEOUT_DEFAULT	300
#define	SHAKE_HAND_TIMEOUT_BACKGROUND	2000
#define	SHAKE_HAND_TIMEOUT	2000
#define	DOWNLOAD_TIMEOUT	5000// 1S
#define	DOWNLOAD_RECVPACK_TIMEOUT	500  // ms

//define RET CODE 
#define 	RET_DUP_OK	0
#define 	RET_DUP_ERR	-1
#define		RET_DUP_SHAKE_TIMEOUT -2
#define		RET_DUP_RECV_TIMEOUT	-3
#define		RET_DUP_RCV_DATA_TIMEOUT -4
#define		RET_DUP_LRC_ERR 	-5
#define		RET_DUP_RCV_ERR	-6
#define		RET_DUP_RCVBUFLEN_ERR	-7
#define		RET_DUP_CHANGEBAUD_ERR -8
#define 	RET_DUP_MSG_ERROR	-9
#define		RET_DUP_TERMINAL_NOT_MATCH	-10
#define		RET_DUP_CHECKTERMINAL_FAIL	-11
#define		RET_DUP_CHECKCONFTAB_FAIL	-12
#define		RET_DUP_CONFITTAB_NOT_MATCH	-13
#define		RET_DUP_COPY_PUK_FAIL	-14
#define		RET_DUP_COPY_FONT_FAIL	-16
#define		RET_DUP_COPY_FILE_FAIL	-17
#define		RET_DUP_DELALL_APP_FAIL	-18
#define		RET_DUP_DEL_PUB_FAIL	-19
#define		RET_DUP_COPY_PUB_FAIL	-20

enum {
	IDUPDOWNLOADINIT=0 ,
	IDUPDOWNLOADSHAKE,
	IDUPDOWNLOADCHECKTERMINFO,
	IDUPDOWNLOADSETBAUD,
	IDUPDOWNLOADCHECKCONFTAB,
	IDUPDOWNLOADDELALLAPP,
	IDUPDOWNLOADPUK,
	IDUPDOWNLOADFONTLIB,
	IDUPDOWNLOADAPPFILE,
    IDUPDOWNLOADDELPUBFILE,
	IDUPDOWNLOADPUBFILE,
	IDUPDOWNLOADFINISH,
	IDUPDOWNLOADDONE,
	IDUPDOWNLOADEXIT
};
typedef struct tagStatusTable{
	int iStatusNumber;
	int (*pFunction)( int * );
}STATUSTABLE;

static uchar gucComPort=0xff;
static const uchar gaucComPortArr[4]={COM1,COM2,PINPAD,P_USB_HOST};
static uchar gaucComPortList[sizeof(gaucComPortArr)];

#define RCV_BUF_LEN 20480
static uchar *gaucRcvBuf=(uchar *)MAPP_RUN_ADDR;
extern uchar gaucSndBuf[20480];

static short gsLastMsgNo=0;//�ϴΰ����
static short gsLastCmd = -1;
static uchar gucCompressSuport=0;
static uint gusMaxDataLen=1024;
static uint guiMaxBaudRate=115200;
static uchar gucDownloadNeedPrompt = 0;
static int giShakeTimeOut=SHAKE_HAND_TIMEOUT_DEFAULT;
// Refer to uartDownload
//��¼����ʱ����ʾ�������Ƶ��ˢ����Ļ������˸
static int lastStep = 0xff;

static volatile uint isLargePackets=0;

static void vDupDownloadingPromt(int iStep,int iCmd);
static int iSndMsg(int *piResult,int iSndLen);
static int iRecvMsg(int *piResult);
static int iDupDownLoadFile(int iFileType,char* pszFileName,uchar* pucFileAttr,int iAttrAppNo,int*piResult);

#define	TM_DUP_DOWNLOAD_CMD		&tm_dup0

static T_SOFTTIMER tm_dup0;
/**
״̬����ʼ������

 @param[in,out]  *piResult,��һ״̬�����룬�˳�ʱ���ر�״̬������
  
 @retval ��һ״̬
 \li #IDUPDOWNLOADDONE 
\li  #IDUPDOWNLOADSHAKE
 @remarks
	 \li ����-----------------ʱ��--------�汾---------�޸�˵�� 				   
	 \li PengRongshou---07/06/28--1.0.0.0-------����
 */
int iDupDownloadInit( int *piResult)
{
	int	iRet,iLoop,iFailCount=0;
	uchar aucComAttr[32];

	// Refer to uartDownloadInit
	lastStep = 0xff;//ˢ�²˵�����
	if(gucDownloadNeedPrompt)
		vDupDownloadingPromt(IDUPDOWNLOADINIT,0);
	gsLastMsgNo=-1;//�ϴΰ����
	gsLastCmd=-1;//�ϴ�����
	isLargePackets = 0;

	memcpy(gaucComPortList, gaucComPortArr, sizeof(gaucComPortArr));
	snprintf(aucComAttr,sizeof(aucComAttr),"%d%s",DEFAULT_BAUDRATE,COMATTR);
	gucComPort = 0xff;
	//open all comport in gaucComPortList
	for(iLoop=0;iLoop<sizeof(gaucComPortList);iLoop++)
	{
		if(gaucComPortList[iLoop]==0xff) continue;
		PortClose(gaucComPortList[iLoop]);
		if(gaucComPortList[iLoop] == P_USB_HOST)
			iRet = PortOpen(gaucComPortList[iLoop], "PAXDEV");
		else
			iRet = PortOpen(gaucComPortList[iLoop], aucComAttr);
		if(iRet!=0)
		{
			gaucComPortList[iLoop]=0xff;
			iFailCount++;
		}
	}
	if(iFailCount<sizeof(gaucComPortList))
	{
		*piResult = RET_DUP_OK;
		
		return	IDUPDOWNLOADSHAKE;
	}
	else
	{
	
		//all comport open err, 
		*piResult = RET_DUP_ERR;
		return IDUPDOWNLOADDONE;
	}
}


/**
 ���֣�������ع��̵�����

 @param[in,out]  *piResult,��һ״̬�����룬�˳�ʱ���ر�״̬������
  
 @retval #IDUPDOWNLOADDONE ��һ״̬�����ؽ���
  @retval #IDUPDOWNLOADREQ	 ��һ״̬�Ǽ�����������
 @remarks
	 \li ����-----------------ʱ��--------�汾---------�޸�˵�� 				   
	 \li PengRongshou---07/06/28--1.0.0.0-------����
 */
int iDupDownloadShake( int *piResult)
{
	#undef		NEXT_STATUS
	#define 	NEXT_STATUS 	IDUPDOWNLOADSHAKE+1
	uchar ucTmp;
	int	iLoop,iFindLoop,iTmp;
	if(gucDownloadNeedPrompt)
		vDupDownloadingPromt(IDUPDOWNLOADSHAKE,0);
	s_TimerSetMs(TM_DUP_DOWNLOAD_CMD,giShakeTimeOut);
	iFindLoop=-1;
	while(s_TimerCheckMs(TM_DUP_DOWNLOAD_CMD))
	{	
		iFindLoop=(iFindLoop+1)%sizeof(gaucComPortList);
		//iDPrintf("iFindLoop[%d]\n",iFindLoop);
		if(gaucComPortList[iFindLoop]==0xff) continue;

		if(0!=PortSend(gaucComPortList[iFindLoop], SHAKE_REQUEST))
		{
			continue;
		}
		if(0==PortRecv(gaucComPortList[iFindLoop], &ucTmp,50))
		{		
			if(ucTmp==SHAKE_REPLY||ucTmp==SHAKE_REPLY_H)
			{
				gucComPort = gaucComPortList[iFindLoop];
				for(iTmp=0;iTmp<sizeof(gaucComPortList);iTmp++)
				{
					if(iTmp==iFindLoop) continue;
					if(gaucComPortList[iTmp]==0xff) continue;
					PortClose(gaucComPortList[iTmp]);
				}
				*piResult = RET_DUP_OK;
                if(ucTmp==SHAKE_REPLY_H)isLargePackets=1;
				return  NEXT_STATUS;
			}
			
		}
		
	}

	for(iTmp=0;iTmp<sizeof(gaucComPortList);iTmp++)
	{
		if(gaucComPortList[iTmp]==0xff) continue;
		PortClose(gaucComPortList[iTmp]);
	}
	
	*piResult = RET_DUP_SHAKE_TIMEOUT;
	return IDUPDOWNLOADDONE;
	
	/****************************************************************/
	

}
/**
��ȡ���շ��ն���Ϣ�����Э��汾����ذ汾���ն������Ƿ�һ��
��ȡ���շ�֧�ֵ�������ʡ����ݳ��ȵ�ֵ
 @param[in,out]  *piResult,��һ״̬������
 @retval # ��һ״̬
 @remarks
	 \li ����-----------------ʱ��--------�汾---------�޸�˵�� 				   
	 \li PengRongshou---07/06/28--1.0.0.0-------����
 */
int iDupDownloadCheckTermInfo(int*piResult)
{
	#undef		NEXT_STATUS
	#define 	NEXT_STATUS	 IDUPDOWNLOADCHECKTERMINFO+1
	int iRcvLen;
	uchar aucTmp[16],sTermName[16];
	vDupDownloadingPromt(IDUPDOWNLOADCHECKTERMINFO,LD_CMDCODE_GETPOSINFO);
	gaucSndBuf[3]=LD_CMDCODE_GETPOSINFO;
	gaucSndBuf[4]=0;
	gaucSndBuf[5]=0;
    if(isLargePackets)
    {
        gaucSndBuf[6]=0;
        gaucSndBuf[7]=0;
    	iSndMsg(piResult,9);
    }
    else
    {
    	iSndMsg(piResult,7);
    }
	iRcvLen = iRecvMsg(piResult);
	if(iRcvLen<7)
	{
		return IDUPDOWNLOADDONE;
	}
	if(gaucRcvBuf[6]!=LD_OK)
	{
		*piResult = RET_DUP_CHECKTERMINAL_FAIL;
		return IDUPDOWNLOADDONE;
	}
	get_term_name(sTermName);
	if((memcmp(gaucRcvBuf+7,sTermName,strlen(sTermName))!=0)//�ն�����
    	||(gaucRcvBuf[23]!=PROTOCOL_VERSION))//Э��汾
	{
		*piResult = RET_DUP_TERMINAL_NOT_MATCH;
		return IDUPDOWNLOADDONE;
	}
	gucCompressSuport = gaucRcvBuf[40];//ѹ���Ƿ�֧�ֱ�־

    if(isLargePackets)
    {
    	//��������󳤶�
    	gusMaxDataLen = (gaucRcvBuf[41]<<24)|
                    	(gaucRcvBuf[42]<<16)|
                    	(gaucRcvBuf[43]<<8)|
                        (gaucRcvBuf[44]);
    	if(gusMaxDataLen>LD_MAX_RECV_DATA_LEN)
    	    gusMaxDataLen = LD_MAX_RECV_DATA_LEN;

    	//ȡ���֧�ֲ�����
    	guiMaxBaudRate = (gaucRcvBuf[77]<<24)|
    					 (gaucRcvBuf[78]<<16)|
    					 (gaucRcvBuf[79]<<8)|
    					 (gaucRcvBuf[80]);
    }
    else
    {
    	//��������󳤶�
    	gusMaxDataLen = (gaucRcvBuf[41]<<8)|(gaucRcvBuf[42]);
    	if(gusMaxDataLen>LD_MAX_RECV_DATA_LEN)
    	    gusMaxDataLen = LD_MAX_RECV_DATA_LEN;

    	//ȡ���֧�ֲ�����
    	guiMaxBaudRate = (gaucRcvBuf[75]<<24)|
    					 (gaucRcvBuf[76]<<16)|
    					 (gaucRcvBuf[77]<<8)|
    					 (gaucRcvBuf[78]);

    }
	return NEXT_STATUS;

}
/**
�����µĲ�����
 @param[in,out]  *piResult,��һ״̬������
 @retval # ��һ״̬
 @remarks
	 \li ����-----------------ʱ��--------�汾---------�޸�˵�� 				   
	 \li PengRongshou---07/06/28--1.0.0.0-------����
 */
int iDupDownloadSetBaud( int *piResult)
{
	#undef		NEXT_STATUS
	#define 	NEXT_STATUS	 IDUPDOWNLOADSETBAUD+1
	uchar aucComAttr[32];
	int iRcvLen,iRet;
	vDupDownloadingPromt(IDUPDOWNLOADSETBAUD, LD_CMDCODE_SETBAUTRATE);
	if(guiMaxBaudRate>LD_MAX_BAUDRATE) guiMaxBaudRate = LD_MAX_BAUDRATE;
	else if(guiMaxBaudRate==DEFAULT_BAUDRATE) return NEXT_STATUS;
	snprintf(aucComAttr,sizeof(aucComAttr),"%d%s",guiMaxBaudRate,COMATTR);
	gaucSndBuf[3]=LD_CMDCODE_SETBAUTRATE;
    if(isLargePackets)
    {
    	gaucSndBuf[4]=0;
    	gaucSndBuf[5]=0;
        gaucSndBuf[6]=0;
        gaucSndBuf[7]=4;
    	
    	gaucSndBuf[8] = (uchar)((guiMaxBaudRate>>24)&0xff);
    	gaucSndBuf[9] = (uchar)((guiMaxBaudRate>>16)&0xff);
    	gaucSndBuf[10] = (uchar)((guiMaxBaudRate>>8)&0xff);
    	gaucSndBuf[11] = (uchar)((guiMaxBaudRate>>0)&0xff);
    	iSndMsg(piResult,13);
    }
    else
    {
    	gaucSndBuf[4]=0;
    	gaucSndBuf[5]=4;
    	
    	gaucSndBuf[6] = (uchar)((guiMaxBaudRate>>24)&0xff);
    	gaucSndBuf[7] = (uchar)((guiMaxBaudRate>>16)&0xff);
    	gaucSndBuf[8] = (uchar)((guiMaxBaudRate>>8)&0xff);
    	gaucSndBuf[9] = (uchar)((guiMaxBaudRate>>0)&0xff);
    	iSndMsg(piResult,11);
    }
	iRcvLen = iRecvMsg(piResult);
	if(iRcvLen<7)
	{
		return IDUPDOWNLOADDONE;
	}
	if(gaucRcvBuf[6]!=LD_OK)
	{
		return NEXT_STATUS;
	}
	if(P_USB_HOST!=gucComPort)
	{	
		//�����µĲ�����
		PortClose(gucComPort);
		iRet = PortOpen(gucComPort, aucComAttr);
		if(iRet !=0)
		{
			*piResult = RET_DUP_CHANGEBAUD_ERR;
			return IDUPDOWNLOADDONE;
		}
	}
	//�ӳ�100ms
	DelayMs(100);//delay 100ms;
	return NEXT_STATUS;
}
/**
	�ӽ��շ���ȡ���ñ���Ϣ�����Ƚ����ñ��Ƿ���ȣ�
���ñ������ܶԿ�
 @param[in,out]  *piResult,��һ״̬������
 @retval # ��һ״̬
 @remarks
	 \li ����-----------------ʱ��--------�汾---------�޸�˵�� 				   
	 \li PengRongshou---07/06/28--1.0.0.0-------����
 */
int iDupDownloadCheckConfTab(int*piResult)
{

	#undef		NEXT_STATUS
	#define 	NEXT_STATUS	 IDUPDOWNLOADCHECKCONFTAB+1
	int iRcvLen,iRet,iLen,iCfgTabLen;
	uchar aucTmp[CONFIG_TAB_LEN+8];
	gaucSndBuf[3]=LD_CMDCODE_GETCONFTABLE;

    if(isLargePackets)
    {
    	gaucSndBuf[4]=0;
    	gaucSndBuf[5]=0;
        gaucSndBuf[6]=0;
        gaucSndBuf[7]=0;
    	iSndMsg(piResult,9);
    }
    else
    {
    	gaucSndBuf[4]=0;
    	gaucSndBuf[5]=0;
    	iSndMsg(piResult,7);
    }
	iRcvLen = iRecvMsg(piResult);
	if(iRcvLen<7)
	{
		return IDUPDOWNLOADDONE;
	}
	if(gaucRcvBuf[6]!=LD_OK)
	{
		*piResult = RET_DUP_CHECKTERMINAL_FAIL;
		return IDUPDOWNLOADDONE;
	}
	iCfgTabLen = ReadSecuConfTab(aucTmp);
	if(0>=iCfgTabLen)
	{
		*piResult = RET_DUP_ERR;
		return IDUPDOWNLOADDONE;
	}
	
	if((memcmp(aucTmp,gaucRcvBuf+11,1)!=0))
	{     		
		*piResult = RET_DUP_CONFITTAB_NOT_MATCH;
		return IDUPDOWNLOADDONE;
	}

	return NEXT_STATUS;
}
/**
ɾ�����շ�����Ӧ�ó��򡢲����ļ����ֿ⡢��dmr
 @param[in,out]  *piResult,��һ״̬������
 @retval # ��һ״̬
 @remarks
	 \li ����-----------------ʱ��--------�汾---------�޸�˵�� 				   
	 \li PengRongshou---07/06/28--1.0.0.0-------����
 */
int iDupDownloadDelAllApp(int*piResult)
{
	#undef		NEXT_STATUS
	#define 	NEXT_STATUS	 IDUPDOWNLOADDELALLAPP+1
	int iRcvLen,iRet,iLen;
	vDupDownloadingPromt(IDUPDOWNLOADDELALLAPP,LD_CMDCODE_DELALLAPP);
	gaucSndBuf[3]=LD_CMDCODE_DELALLAPP;
    if(isLargePackets)
    {
    	gaucSndBuf[4]=0;
    	gaucSndBuf[5]=0;
        gaucSndBuf[6]=0;
        gaucSndBuf[7]=0;
    	iSndMsg(piResult,9);
    }
    else
    {
    	gaucSndBuf[4]=0;
    	gaucSndBuf[5]=0;
    	iSndMsg(piResult,7);
    }
    
	iRcvLen = iRecvMsg(piResult);
	if(iRcvLen<7)
	{
		return IDUPDOWNLOADDONE;
	}
	if(gaucRcvBuf[6]!=LD_OK)
	{
		*piResult = RET_DUP_DELALL_APP_FAIL;
		return IDUPDOWNLOADDONE;
	}
	return NEXT_STATUS;
			
}
/**
 ���ع�Կ
 @param[in,out]  *piResult,��һ״̬������
 @retval # ��һ״̬
 @remarks
	 \li ����-----------------ʱ��--------�汾---------�޸�˵�� 				   
	 \li PengRongshou---07/06/28--1.0.0.0-------����
 */
int iDupDownloadPuk(int*piResult)
{
	#undef		NEXT_STATUS
	#define 	NEXT_STATUS	  IDUPDOWNLOADPUK+1

	return NEXT_STATUS;
}
/**
 �����ֿ�
 @param[in,out]  *piResult,��һ״̬������
 @retval # ��һ״̬
 @remarks
	 \li ����-----------------ʱ��--------�汾---------�޸�˵�� 				   
	 \li PengRongshou---07/06/28--1.0.0.0-------����
 */
int iDupDownloadFontLib(int*piResult)
{
	#undef		NEXT_STATUS
	#define 	NEXT_STATUS	 IDUPDOWNLOADFONTLIB+1 
	int iRet;
	
	iRet = iDupDownLoadFile(FILE_TYPE_FONT, FONT_FILE_NAME, "\xff\x02",0xff, piResult);
	if(iRet==0)
	{
		return NEXT_STATUS;
	}
	else
	{
		*piResult = RET_DUP_COPY_FONT_FAIL;
		return IDUPDOWNLOADDONE;
	}
}
/**
����Ӧ�ó����û��ļ��������ļ���˽�п⵽���շ�
 @param[in,out]  *piResult,��һ״̬������
 @retval # ��һ״̬�����ؽ���
 @remarks
	 \li ����-----------------ʱ��--------�汾---------�޸�˵�� 				   
	 \li PengRongshou---07/06/28--1.0.0.0-------����
 */
int iDupDownloadAppFile(int*piResult)
{
	#undef		NEXT_STATUS
	#define 	NEXT_STATUS	 IDUPDOWNLOADAPPFILE+1
	int iLoop,iRet;
	int iFileType,iAttrAppNo;
	char pszFileName[33];
	uchar aucFileAttr[8];
	int iAllFileNum,iSearch;
	FILE_INFO astFileInfo[MAX_FILES];
	iAllFileNum=GetFileInfo(astFileInfo);

	for(iLoop=0;iLoop<MAX_APP_NUM;iLoop++)
	{
		//����Ӧ�ó���
		snprintf(pszFileName,sizeof(pszFileName),"%s%d",APP_FILE_NAME_PREFIX,iLoop);
		if(iLoop==0)
		{
			aucFileAttr[0]=0xff;
			aucFileAttr[1]=FILE_TYPE_MAPP;
			iFileType = FILE_TYPE_MAPP;
		}
		else
		{	
			aucFileAttr[0]=0xff;
			aucFileAttr[1]=FILE_TYPE_APP;
			iFileType = FILE_TYPE_APP;			
		}
		if(s_SearchFile(pszFileName, aucFileAttr)<0)continue;
		
		iAttrAppNo=iDupDownLoadFile(iFileType,pszFileName,aucFileAttr,0xff,piResult);
		if(iAttrAppNo<0)
		{
			return IDUPDOWNLOADDONE;
		}

		//����Ӧ�ø������û��ļ��Ͳ����ļ�
		for(iSearch=0;iSearch<iAllFileNum;iSearch++)
		{
			if(astFileInfo[iSearch].attr!=iLoop) continue;

			//modify by skx ����Ӧ����˽�ж�̬�ļ��Կ�	//080520���۲��Կ��û��ļ�
			if((astFileInfo[iSearch].type!=FILE_TYPE_APP_PARA) &&
				(astFileInfo[iSearch].type!=FILE_TYPE_USERSO) && 
				(astFileInfo[iSearch].type!=FILE_TYPE_SYSTEMSO))
			continue;

			if( astFileInfo[iSearch].type == FILE_TYPE_USERSO ||
				astFileInfo[iSearch].type == FILE_TYPE_SYSTEMSO)
				iFileType = LD_PARA_SO;
			else
				iFileType = astFileInfo[iSearch].type;
			aucFileAttr[0]=iLoop;
			aucFileAttr[1]=astFileInfo[iSearch].type;
			memcpy(pszFileName,astFileInfo[iSearch].name,sizeof(astFileInfo[iSearch].name));
			iRet = iDupDownLoadFile(iFileType,pszFileName,aucFileAttr,iAttrAppNo,piResult);
			if(iRet!=0)
			{
				return IDUPDOWNLOADDONE;
			}
		}
	}
		
	return  NEXT_STATUS;
}



/**
���ع����ļ�������̬��(��paxbase)
 @param[in,out]  *piResult,��һ״̬������
 @retval # ��һ״̬�����ؽ���
 @remarks
	 \li ����-----------------ʱ��--------�汾---------�޸�˵�� 				   
	 \li songkx-----------------13/12/04-------1.0.0.0----------����
 */	
int iDupDownloadPubFile(int*piResult)
{
	#undef		NEXT_STATUS
	#define 	NEXT_STATUS	 IDUPDOWNLOADPUBFILE+1
	int iLoop,iRet;
	int iFileType,iAttrAppNo;
	char pszFileName[33];
	uchar aucFileAttr[8];
	int iAllFileNum,iSearch;
	FILE_INFO astFileInfo[MAX_FILES];
	iAllFileNum=GetFileInfo(astFileInfo);//�ҳ��ļ�ϵͳ�������ļ�

	*piResult = RET_DUP_OK;
	
	if( !iAllFileNum ) 
	{
		*piResult = RET_DUP_COPY_PUB_FAIL;
		return IDUPDOWNLOADDONE;
	}
	

	for(iSearch =0 ; iSearch< iAllFileNum ;iSearch++)
	{
		if(astFileInfo[iSearch].attr == FILE_ATTR_PUBSO ||
			astFileInfo[iSearch].attr == FILE_ATTR_PUBFILE)//�����ļ����߹���̬���ļ�
		{
            //���Կ�baseso�Լ�WIFI_MPATCH
			if(astFileInfo[iSearch].attr == FILE_ATTR_PUBSO && 
				strstr(astFileInfo[iSearch].name,FIRMWARE_SO_NAME))continue;
			if( astFileInfo[iSearch].attr == FILE_ATTR_PUBSO && is_wifi_mpatch(astFileInfo[iSearch].name))continue;
			iFileType = astFileInfo[iSearch].type;
			strcpy(pszFileName , astFileInfo[iSearch].name);
			aucFileAttr[0] = astFileInfo[iSearch].attr;
			aucFileAttr[1] = astFileInfo[iSearch].type;
			iRet=iDupDownLoadFile(iFileType,pszFileName,aucFileAttr,0xff,piResult);
			if( iRet ) 
			{
				*piResult = RET_DUP_COPY_PUB_FAIL;
				return IDUPDOWNLOADDONE;
			}
		}
	}

	return NEXT_STATUS;
	
}
/**
ɾ�����й����ļ������ж�̬��(��PAXBASE)
 @param[in,out]  *piResult,��һ״̬������
 @retval # ��һ״̬�����ؽ���
 @remarks
	 \li ����-----------------ʱ��--------�汾---------�޸�˵�� 				   
	 \li songkx-----------------13/12/05-------1.0.0.0----------����
 */	
int iDupDownloadDelPubFile(int*piResult)
{
	#undef		NEXT_STATUS
	#define 	NEXT_STATUS	 IDUPDOWNLOADDELPUBFILE+1
	int iRcvLen,iRet,iLen;
	vDupDownloadingPromt(IDUPDOWNLOADDELALLAPP,LD_CMDCODE_DELPUBFILE);
	gaucSndBuf[3]=LD_CMDCODE_DELPUBFILE;
	gaucSndBuf[4]=0;
	gaucSndBuf[5]=0;
    if(isLargePackets)
    {
        gaucSndBuf[6]=0;
        gaucSndBuf[7]=0;
    	iSndMsg(piResult,9);
    }
    else
        iSndMsg(piResult,7);
	iRcvLen = iRecvMsg(piResult);

	if(iRcvLen<7)return IDUPDOWNLOADDONE;
    
	if(gaucRcvBuf[6]!=LD_OK)
	{
		*piResult = RET_DUP_DEL_PUB_FAIL;
		return IDUPDOWNLOADDONE;
	}
	return NEXT_STATUS;
	
}


/**
�������������������շ�
 @param[in,out]  *piResult,��һ״̬������
 @retval #IDUPDOWNLOADEXIT ��һ״̬�����ؽ���
 @remarks
	 \li ����-----------------ʱ��--------�汾---------�޸�˵�� 				   
	 \li PengRongshou---07/06/28--1.0.0.0-------����
 */
int iDupDownloadFinish(int *piResult)
{
	#undef		NEXT_STATUS
	#define NEXT_STATUS IDUPDOWNLOADFINISH+1
	int iRcvLen;

	vDupDownloadingPromt(IDUPDOWNLOADFINISH, 0);
	gucDownloadNeedPrompt=0;	

	gaucSndBuf[3]=LD_CMDCODE_DLCOMPLETE;
	gaucSndBuf[4]=0;
	gaucSndBuf[5]=0;
    if(isLargePackets)
    {
        gaucSndBuf[6]=0;
        gaucSndBuf[7]=0;
    	iSndMsg(piResult,9);
    }
    else
    {
    	iSndMsg(piResult,7);
    }
	iRcvLen = iRecvMsg(piResult);

	if(iRcvLen<7)
	{
			
	}
	*piResult = RET_OK;
	return NEXT_STATUS;
}
/**
 ״̬������״̬��״̬���˳�ʱ������뱾״̬��
 �ڱ�״̬������piResultֵ��������Ӧ����
 @param[in,out]  *piResult,��һ״̬������
 @retval #IDUPDOWNLOADEXIT ��һ״̬�����ؽ���
 @remarks
	 \li ����-----------------ʱ��--------�汾---------�޸�˵�� 				   
	 \li PengRongshou---07/06/28--1.0.0.0-------����
 */
int iDupDownloadDone( int *piResult)
{	
	int iTmp;
	giShakeTimeOut=SHAKE_HAND_TIMEOUT_DEFAULT;
	if(gucComPort!=0xFF)
	{
		if(P_USB_HOST == gucComPort)
		{
			PortClose(P_USB_HOST);
		}
		gucComPort = 0xff;
	}
	vDupDownloadingPromt(IDUPDOWNLOADDONE,*piResult);
	switch(*piResult)
	{
		case RET_DUP_OK:
		{
		}
		default:
		{
			break;
		}
	}
	gucDownloadNeedPrompt=0;	
	return IDUPDOWNLOADEXIT;
}


/**
 ״̬��������������״̬�����
 @param[in]iMode
	\li #FRONT_MODE 1 :ǰ̨ģʽ
	\li #BACKGOUND_MODE  0 :��̨ģʽ��û�н��յ�����ǰ������lcd������ʾ
@retval #RET_DUP_RECV_TIMEOUT �������ʱ
@remarks
	\li ����-----------------ʱ��--------�汾---------�޸�˵��                    
 	\li PengRongshou---07/06/28--1.0.0.0-------����
*/
int iDupDownloadMain(int iMode)
{
	STATUSTABLE aDupDStatus[] =
	{
		{ IDUPDOWNLOADINIT, iDupDownloadInit},
		{ IDUPDOWNLOADSHAKE, iDupDownloadShake},
		{IDUPDOWNLOADCHECKTERMINFO,iDupDownloadCheckTermInfo},
		{IDUPDOWNLOADSETBAUD,iDupDownloadSetBaud},
		{IDUPDOWNLOADCHECKCONFTAB,iDupDownloadCheckConfTab},
		{IDUPDOWNLOADDELALLAPP,iDupDownloadDelAllApp},
		
		{IDUPDOWNLOADPUK,iDupDownloadPuk},
		{IDUPDOWNLOADFONTLIB,iDupDownloadFontLib},
		{IDUPDOWNLOADAPPFILE,iDupDownloadAppFile},
		{IDUPDOWNLOADDELPUBFILE,iDupDownloadDelPubFile},
		{IDUPDOWNLOADPUBFILE,iDupDownloadPubFile},
		{IDUPDOWNLOADFINISH,iDupDownloadFinish},
		{IDUPDOWNLOADDONE, iDupDownloadDone},
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
	
	iStatusNumber = IDUPDOWNLOADINIT;

	while ( iStatusNumber != IDUPDOWNLOADEXIT )
	{
		for ( p = ( STATUSTABLE *)aDupDStatus ; p->pFunction != 0 ; p++ )
		{
			if ( iStatusNumber == p->iStatusNumber )
			{
				iStatusNumber = ( *p->pFunction)( &iResult );
				break;
			}
		}
		
		if ( p->iStatusNumber == -1 || p->pFunction == NULL )
		{
			iStatusNumber = IDUPDOWNLOADINIT;
		}

	}

	return iResult;
}

/**
����Ӧ�����ݰ���
@param[out]*piResult:������	
@retval >0���յ����ݰ�����
@retval <=0 ����
@remarks
	\li ����-----------------ʱ��--------�汾---------�޸�˵��                    
 	\li PengRongshou---07/09/18--1.0.0.0-------����
*/
static int iRecvMsg(int *piResult)
{
	int iRet,iLoop,iTmp,iLen,iRcvLen;

	*piResult =RET_DUP_RECV_TIMEOUT;
	if(gucComPort!=0xff)
	{
		s_TimerSetMs(TM_DUP_DOWNLOAD_CMD,DOWNLOAD_TIMEOUT);
		while(s_TimerCheckMs(TM_DUP_DOWNLOAD_CMD))
		{
			iRet = PortRecv(gucComPort, gaucRcvBuf,  0);
			//iDPrintf("[%s %d]recv stx[%d]\n",FILELINE,iRet);
			if(iRet!=0) continue;
			if(gaucRcvBuf[0]== SHAKE_REQUEST)
			{
                if(isLargePackets)
                    PortSend(gucComPort, SHAKE_REPLY_H);
                else
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
				*piResult = RET_DUP_OK;
				break;
			}
		}

	}
	else
	{
		return -1;
	}

	if(*piResult ==RET_DUP_RECV_TIMEOUT)
	{
		return  -2;
	}

	//recv msg length
	
	iRet = PortRecvs(gucComPort, gaucRcvBuf+4, 2, DOWNLOAD_TIMEOUT);

	if(iRet!=2)
	{
		*piResult =RET_DUP_RECV_TIMEOUT;
		return -3;
	}
	iLen = ((uchar)gaucRcvBuf[4]<<8)+(uchar)gaucRcvBuf[5];
	iLen++;
	if(iLen+6>RCV_BUF_LEN)
	{
		
		*piResult =RET_DUP_RCVBUFLEN_ERR;
		return -4;
	}
	iRcvLen=0;
	
	s_TimerSetMs(TM_DUP_DOWNLOAD_CMD,DOWNLOAD_RECVPACK_TIMEOUT);
	while((iLen>0)&&s_TimerCheckMs(TM_DUP_DOWNLOAD_CMD))
	{
		iTmp = PortRecvs(gucComPort, gaucRcvBuf+6+iRcvLen, iLen, DOWNLOAD_RECVPACK_TIMEOUT);
		if(iTmp>0)
		{
			iRcvLen+=iTmp;
			iLen -=iTmp;
			s_TimerSetMs(TM_DUP_DOWNLOAD_CMD,DOWNLOAD_RECVPACK_TIMEOUT);

		}
	}	
	if(iLen>0)
	{
		*piResult =RET_DUP_RCV_DATA_TIMEOUT;
		return -5;
	}
	if(gaucRcvBuf[6+iRcvLen-1]!=ucGenLRC(gaucRcvBuf+1, iRcvLen+6-2))
	{
		*piResult =RET_DUP_LRC_ERR;
		return -6;
	}
	
	*piResult =RET_DUP_OK	;
	if((gaucRcvBuf[1]!=(gsLastMsgNo%0x100))||(gaucRcvBuf[3]!=gsLastCmd))
	{
		*piResult =RET_DUP_MSG_ERROR;
		return -7;
	}
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
	int iLen;
	gsLastMsgNo++;
	gaucSndBuf[0]=STX;
	gaucSndBuf[1]=gsLastMsgNo;
	gaucSndBuf[2]=LD_CMD_TYPE;
	gaucSndBuf[iSndLen-1]=ucGenLRC(gaucSndBuf+1,iSndLen-2);
	gsLastCmd = gaucSndBuf[3];

	if(iSndLen>8192)
	{
		iLen = 4096;
		ucRet = PortSends(gucComPort,gaucSndBuf, iLen);
		if(ucRet!=0)
		{			
			*piResult = RET_DUP_ERR;		
			return -1;
		}
		
	}
	else
	{
		iLen =0;
	}

    s_TimerSetMs(TM_DUP_DOWNLOAD_CMD,DOWNLOAD_TIMEOUT);
	while(PortTxPoolCheck(gucComPort))
	{
		if(!s_TimerCheckMs(TM_DUP_DOWNLOAD_CMD))
		{
            *piResult = RET_DUP_ERR;		
			return -1;
        }
	}
	ucRet = PortSends(gucComPort,gaucSndBuf+iLen, iSndLen-iLen);
	if(ucRet!=0)
	{
		*piResult = RET_DUP_ERR;		
		return -1;
	}
	
	*piResult = RET_DUP_OK;
	return 0;
}
/**

�����ļ���
 
 @param[in]pucFileAttr �ļ�����"\xAA\xBB",AA:�ļ�������BB:�ļ�����
 @param[in]pszFileName�ļ��� 
 @param[out]  ���صĴ�����
  
 @retval 
 @remarks
	 \li ����-----------------ʱ��--------�汾---------�޸�˵�� 				   
	 \li PengRongshou---07/09/18--1.0.0.0-------����
 */
static int iSndFile(uchar * pucFileAttr,char * pszFileName,int *piResult)
{
	int iFileID,iPercent,iFileSize,iHaveSnd=0;
	int iLen,iRcvLen,iRet=0;
    
	iFileID = s_open(pszFileName,O_RDWR,pucFileAttr);
	if(iFileID<0) return -1;
	iFileSize = s_filesize(pszFileName, pucFileAttr);
	gaucSndBuf[3]=LD_CMDCODE_DLFILEDATA;
	
	for(iHaveSnd=0;iHaveSnd<iFileSize;)
	{
        if(isLargePackets)
        {
    		iLen = read(iFileID,gaucSndBuf+8,gusMaxDataLen);
    		if(iLen<=0) 
    		{
    			iRet = -2;
    			break;
    		}

            gaucSndBuf[4]=0;
            gaucSndBuf[5]=0;
    		gaucSndBuf[6]=(iLen>>8)&0xff;
    		gaucSndBuf[7]=iLen&0xff;
    		//vDupDownloadingPromt(IDUPDOWNLOADAPPFILE,LD_CMDCODE_DLFILEDATA);
    		iPercent = 100*iHaveSnd/iFileSize;
    		if(iPercent>100) iPercent =100;
    		vDupDownloadingPromt(-1,iPercent);
    		iHaveSnd+=iLen;
    		iSndMsg( piResult, iLen+9);
        }
        else
        {
    		iLen = read(iFileID,gaucSndBuf+6,gusMaxDataLen);
    		if(iLen<=0) 
    		{
    			iRet = -2;
    			break;
    		}

    		gaucSndBuf[4]=(iLen>>8)&0xff;
    		gaucSndBuf[5]=iLen&0xff;
    		//vDupDownloadingPromt(IDUPDOWNLOADAPPFILE,LD_CMDCODE_DLFILEDATA);
    		iPercent = 100*iHaveSnd/iFileSize;
    		if(iPercent>100) iPercent =100;
    		vDupDownloadingPromt(-1,iPercent);
    		iHaveSnd+=iLen;
    		iSndMsg( piResult, iLen+7);
        }

        s_TimerSetMs(TM_DUP_DOWNLOAD_CMD,DOWNLOAD_TIMEOUT);
		while(PortTxPoolCheck(gucComPort))
		{
			if(!s_TimerCheckMs(TM_DUP_DOWNLOAD_CMD))
			{
                iRet = -3;
                goto exit;
            }
		}
        
		iRcvLen = iRecvMsg(piResult);
		if(iRcvLen<7)
		{
			 iRet = -4;
			 break;
		}
		if(gaucRcvBuf[6]!=LD_OK)
		{
			*piResult = gaucRcvBuf[6];
			iRet =  -5;
			break;
		}
	}

exit:    
	close(iFileID);
	return iRet;
}

/**
�����ļ�
 @param[in] iFileType �ļ�����
 @param[in] pszFileName �ļ���
 @param[in] pucFileAttr�ļ�����
 @param[in] iAttrAppNo,�ļ�����Ӧ�ú�,�Է���Ӧ�ú�,������Ӧ�ó����ļ�ʱ���� 
 			ֻ���ļ�����Ϊ�����ļ����û��ļ�,dmr�ļ�ʱ��Ч
 @param[out] piResult ���ؽ��
 @retval 
 @remarks
	 \li ����-----------------ʱ��--------�汾---------�޸�˵�� 				   
	 \li PengRongshou---07/09/18--1.0.0.0-------����
 */
int iDupDownLoadFile(int iFileType,char* pszFileName,uchar* pucFileAttr,int iAttrAppNo,int*piResult)
{

	int iLen,iTmp,iLoop;
	int iRcvLen,iRet;
	uchar ucCmdCode;
	//���������ļ�����
	iLen = s_filesize(pszFileName,pucFileAttr);

	if(iLen<=0)
	{
		return 0;
	}

	iTmp = 4;//�������㲻ͬ����ĵ������򳤶�,
	//add by skx for public file 
	if(pucFileAttr[0]==FILE_ATTR_PUBFILE || pucFileAttr[0]==FILE_ATTR_PUBSO )
	{
		ucCmdCode = LD_CMDCODE_DLPUBFILE;
        if(isLargePackets)
        {
    		gaucSndBuf[12]=0x00;//appno
    		if( pucFileAttr[0] == FILE_ATTR_PUBSO )
    			gaucSndBuf[13]=LD_PUB_SO;//SO�ļ�ר����������
    		else
    			gaucSndBuf[13]=iFileType;
    		
    		iTmp+=2;
    		memset(gaucSndBuf+14,0x00,16);
    		strcpy(gaucSndBuf+14,pszFileName);
        }
        else
        {
    		gaucSndBuf[10]=0x00;//appno
    		if( pucFileAttr[0] == FILE_ATTR_PUBSO )
    			gaucSndBuf[11]=LD_PUB_SO;//SO�ļ�ר����������
    		else
    			gaucSndBuf[11]=iFileType;
    		
    		iTmp+=2;
    		memset(gaucSndBuf+12,0x00,16);
    		strcpy(gaucSndBuf+12,pszFileName);
        }
		iTmp+=16;
	}
	else
	{
		switch(iFileType)
		{
			case FILE_TYPE_FONT:
				ucCmdCode = LD_CMDCODE_DLFONTLIB;
				break;
			case FILE_TYPE_APP:
			case FILE_TYPE_MAPP:
				ucCmdCode = LD_CMDCODE_DLAPP;
				break;
			case FILE_TYPE_USER_FILE:
			case FILE_TYPE_APP_PARA:
			//�����û�˽�ж�̬�ļ��Կ�
			case LD_PARA_SO://�ļ������ļ�����ȡ
				ucCmdCode = LD_CMDCODE_DLPARA;
                if(isLargePackets)
                {
    				gaucSndBuf[12]=iAttrAppNo;
    				gaucSndBuf[13]=iFileType;
    				iTmp+=2;
    				if(iFileType==FILE_TYPE_USER_FILE)
    				{	
    					memset(gaucSndBuf+14,0x00,16);
    					strcpy(gaucSndBuf+14,pszFileName);
    					iTmp+=16;
    				}
                }
                else
                {
    				gaucSndBuf[10]=iAttrAppNo;
    				gaucSndBuf[11]=iFileType;
    				iTmp+=2;
    				if(iFileType==FILE_TYPE_USER_FILE)
    				{	
    					memset(gaucSndBuf+12,0x00,16);
    					strcpy(gaucSndBuf+12,pszFileName);
    					iTmp+=16;
    				}
                }
				break;
			default:
				return -1;
		}
		
	}

	
	vDupDownloadingPromt(IDUPDOWNLOADAPPFILE,ucCmdCode);
	gaucSndBuf[3]=ucCmdCode;
    if(isLargePackets)
    {
        gaucSndBuf[4]=0;
        gaucSndBuf[5]=0;
    	gaucSndBuf[6]=(iTmp>>8)&0xFF;
    	gaucSndBuf[7]=iTmp&0xFF;
    	gaucSndBuf[8]=(iLen>>24)&0xFF;
    	gaucSndBuf[9]=(iLen>>16)&0xFF;
    	gaucSndBuf[10]=(iLen>>8)&0xFF;
    	gaucSndBuf[11]=(iLen>>0)&0xFF;
    	iTmp+=9;//�����ܳ���
    }
    else
    {
    	gaucSndBuf[4]=(iTmp>>8)&0xFF;
    	gaucSndBuf[5]=iTmp&0xFF;
    	gaucSndBuf[6]=(iLen>>24)&0xFF;
    	gaucSndBuf[7]=(iLen>>16)&0xFF;
    	gaucSndBuf[8]=(iLen>>8)&0xFF;
    	gaucSndBuf[9]=(iLen>>0)&0xFF;
    	iTmp+=7;//�����ܳ���
    }
	
	iSndMsg( piResult, iTmp);
	iRcvLen = iRecvMsg(piResult);

	if(iRcvLen<7)
	{
		return -2;
	}
	if(gaucRcvBuf[6]!=LD_OK)
	{
		*piResult = gaucRcvBuf[6];
		return -3;
	}

	//�����ļ�����
	if(0!=iSndFile(pucFileAttr,pszFileName,piResult))
	{
		*piResult = RET_DUP_COPY_FILE_FAIL;
		return -4;
	}

	// FontLib updating, update prompt then.
	if (ucCmdCode == LD_CMDCODE_DLFONTLIB)
		lastStep = 0xff;
	vDupDownloadingPromt(IDUPDOWNLOADAPPFILE,LD_CMDCODE_WRITEFILE);
	
	//д���ļ�
	gaucSndBuf[3]=LD_CMDCODE_WRITEFILE;
	gaucSndBuf[4]=0;
	gaucSndBuf[5]=0;
    if(isLargePackets)
    {
        gaucSndBuf[6]=0;
        gaucSndBuf[7]=0;
    	iSndMsg(piResult,9);
    }
    else
    {
    	iSndMsg(piResult,7);
    }
	for(iLoop=0;iLoop<1+iLen/(64*1024);iLoop++)//���ļ�д��ʱ��Ƚϳ�
	{
		iRcvLen = iRecvMsg(piResult);
		if(*piResult==RET_DUP_RECV_TIMEOUT)
		{
			continue;
		}
		if(iRcvLen>0)
		{
			break;
		}
	}
	
	if(iRcvLen<7)
	{
		return -5;
	}
	
	if(gaucRcvBuf[6]!=LD_OK)
	{
		*piResult = gaucRcvBuf[6];
		return -5;
	}

	if(ucCmdCode==LD_CMDCODE_DLAPP)
	{
		return gaucRcvBuf[7];
	}
	else
	{
		return 0;
	}
}

/**
�Կ����̽�����ʾ������
@param[in]iStep ����
@param[in]iCmd ����
@retval 
@remarks
	 \li ����-----------------ʱ��--------�汾---------�޸�˵�� 				   
	 \li PengRongshou---07/09/18--1.0.0.0-------����
 */
void vDupDownloadingPromt(int iStep,int iCmd)
{
	static char pszDisplay[]="...............";
	static uchar ucDisplayCnt=0;
	static char pszChnMsg[32];
	static char pszEngMsg[32];
	uchar aucTitle[17];

	if (lastStep == 0xff) 
    {
		ScrCls();
		SCR_PRINT(0, 0, 0xC1, "      �Կ�      ", "   DUPLICATE    ");
	}
	
	switch(iStep)
	{
		case IDUPDOWNLOADINIT:
		{
            ScrClrLine(4, 5);
			SCR_PRINT(0, 4, 0x01,"��ʼ��...","INITIALIZING...\n");
			break;
		}
        
		case IDUPDOWNLOADSHAKE:
		{
            ScrClrLine(4, 5);
			SCR_PRINT(0, 4, 0x01,"������...","CONNECTING...\n");
			break;
		}
        
		case IDUPDOWNLOADCHECKTERMINFO:
		case IDUPDOWNLOADSETBAUD:
		case IDUPDOWNLOADCHECKCONFTAB:
		case IDUPDOWNLOADPUK:
		case IDUPDOWNLOADFONTLIB:
		case IDUPDOWNLOADAPPFILE:
		{
			switch(iCmd)
			{
				case LD_CMDCODE_SETBAUTRATE :{strcpy(pszChnMsg,"���ò�����...");strcpy(pszEngMsg,"Set Baudrate...");break;}
				case LD_CMDCODE_GETPOSINFO  :{strcpy(pszChnMsg,"��ȡ�ն���Ϣ");strcpy(pszEngMsg,"Read TermInfo");break;}
				case LD_CMDCODE_GETAPPSINFO :{strcpy(pszChnMsg,"��ȡӦ����Ϣ...");strcpy(pszEngMsg,"Read AppInfo...");break;}
				case LD_CMDCODE_REBUILDFS   :{strcpy(pszChnMsg,"�ؽ��ļ�ϵͳ...");strcpy(pszEngMsg,"Rebuild FS...");break;}
				case LD_CMDCODE_SETTIME     :{strcpy(pszChnMsg,"�����ն�ʱ��...");strcpy(pszEngMsg,"Set Time...");break;}
				case LD_CMDCODE_DLUPK       :{strcpy(pszChnMsg,"���ƹ�Կ...");strcpy(pszEngMsg,"Copying PUK...");break;}
				case LD_CMDCODE_DLAPP       :{strcpy(pszChnMsg,"����Ӧ��...");strcpy(pszEngMsg,"Copying APP...");break;}
				case LD_CMDCODE_DLFONTLIB   :{strcpy(pszChnMsg,"�����ֿ�...");strcpy(pszEngMsg,"Copying FONT...");break;}
				case LD_CMDCODE_DLPARA      :{strcpy(pszChnMsg,"���Ʋ���...");strcpy(pszEngMsg,"Copying PARAM...");break;}
				case LD_CMDCODE_DLPUBFILE	:{strcpy(pszChnMsg,"���ƹ����ļ�...");strcpy(pszEngMsg,"Copying Public...");break;}
				case LD_CMDCODE_DELPUBFILE	:{strcpy(pszChnMsg,"ɾ�������ļ�...");strcpy(pszEngMsg,"Del Public...");break;}
				case LD_CMDCODE_DLFILEDATA :{break;}
				case LD_CMDCODE_DLFILEDATAC :{break;}
				case LD_CMDCODE_WRITEFILE   :{break;}//{strcpy(pszChnMsg,"�����ļ�...");strcpy(pszEngMsg,"Save File...");break;}
				case LD_CMDCODE_DELAPP      :
				case LD_CMDCODE_DELALLAPP:{strcpy(pszChnMsg,"ɾ��Ӧ��...");strcpy(pszEngMsg,"Del App...");break;}
				case LD_CMDCODE_GETCONFTABLE:
				default:{strcpy(pszChnMsg,"���ڸ���...");strcpy(pszEngMsg,"Copying...");break;}
			}

            if(iCmd != gsLastCmd || lastStep!=iStep)
            {
    			pszChnMsg[16]=0;
    			pszEngMsg[16]=0;
                ScrClrLine(4, 5);
    			SCR_PRINT(0, 4, 0x01,pszChnMsg,pszEngMsg);
            }
            
			if(LD_CMDCODE_WRITEFILE==iCmd)
			{
                ScrClrLine(6, 7);
				SCR_PRINT(0, 6, 0x01,"���ڱ���...","Save File...");
			}
			else
			{
                if(iCmd != gsLastCmd || lastStep!=iStep)
                    ucDisplayCnt=0;
                
                if(ucDisplayCnt==0)
                    ScrClrLine(6, 7);
                else
    				ScrPrint(0, 6, 0x01,pszDisplay+(5-ucDisplayCnt)*3);
				ucDisplayCnt = (ucDisplayCnt+1)%6;
			}
			break;
		}

		case IDUPDOWNLOADFINISH:
		{
            ScrClrLine(4, 5);
			SCR_PRINT(0, 4, 0x01,"�Կ��ɹ�!","Duplicate OK!");
			DelayMs(1000);//Delay 1s to display the success msg 

			break;
		}

		case IDUPDOWNLOADDONE:
		{
			strcpy(pszChnMsg,"�Կ�δ���!");
			strcpy(pszEngMsg,"Duplicate Fail!");

			switch(iCmd)
			{
				case RET_DUP_OK	                 :{return;}
				case RET_DUP_ERR	               :{break;}
				case RET_DUP_SHAKE_TIMEOUT       :{break;}
				case RET_DUP_RECV_TIMEOUT	       :
				case RET_DUP_RCV_DATA_TIMEOUT    :{strcpy(pszChnMsg,"������Ӧ��ʱ");strcpy(pszEngMsg,"Rcv msg timeout");break;}
				case RET_DUP_LRC_ERR 	           :{strcpy(pszChnMsg,"���ձ���LRC��");strcpy(pszEngMsg,"Rcv Lrc error.");break;}
				case RET_DUP_RCV_ERR	           :{break;}
				case RET_DUP_RCVBUFLEN_ERR	     :{break;}
				case RET_DUP_CHANGEBAUD_ERR      :{strcpy(pszChnMsg,"�����²�����ʧ��");strcpy(pszEngMsg,"Set new baud fail.");break;}
				case RET_DUP_MSG_ERROR	         :{break;}
				case RET_DUP_TERMINAL_NOT_MATCH :{strcpy(pszChnMsg,"�ն���Ϣ��ƥ��");strcpy(pszEngMsg,"Term not match");break;}
				case RET_DUP_CHECKTERMINAL_FAIL	 :{strcpy(pszChnMsg,"���ն���Ϣʧ��");strcpy(pszEngMsg,"Get terminfo fail");break;}
				case RET_DUP_CHECKCONFTAB_FAIL	 :{strcpy(pszChnMsg,"ȡ���ñ�ʧ��");strcpy(pszEngMsg,"Get cfgtab fail");break;}
				case RET_DUP_CONFITTAB_NOT_MATCH :	{strcpy(pszChnMsg,"���ñ�һ��");strcpy(pszEngMsg,"Cfgtab not match");break;}
				case RET_DUP_COPY_PUK_FAIL	     :{strcpy(pszChnMsg,"����PUKʧ��");strcpy(pszEngMsg,"Copy PUK fail");break;}
				case RET_DUP_COPY_FONT_FAIL	 :{strcpy(pszChnMsg,"�����ֿ�ʧ��");strcpy(pszEngMsg,"Copy font fail");break;}
				case RET_DUP_COPY_FILE_FAIL	 :{strcpy(pszChnMsg,"����Ӧ���ļ�ʧ��");strcpy(pszEngMsg,"copy app fail");break;}
				case RET_DUP_DELALL_APP_FAIL	   :{strcpy(pszChnMsg,"ɾ��Ӧ��ʧ��");strcpy(pszEngMsg,"del app fail");break;}
				case RET_DUP_COPY_PUB_FAIL	 :{strcpy(pszChnMsg,"���ƹ����ļ�ʧ��");strcpy(pszEngMsg,"copy Pub fail");break;}
			}
			ScrClrLine(4, 5);
			SCR_PRINT(0, 4, 0x01,pszChnMsg,pszEngMsg);
			DelayMs(1000);
			break;
		}

		case -1://display percent
		{
			char aucPercent[17];
			sprintf(aucPercent,"      %d%%",iCmd);
            if(lastStep!=iStep)
                ScrClrLine(6, 7);
			SCR_PRINT(0, 4, 0x01,pszChnMsg,pszEngMsg);
			SCR_PRINT(0, 6, 0x01,aucPercent,aucPercent);
			break;
		}
        
		default:
		{
			break;
		}
	}

	lastStep = iStep;
	return;
}

/**


 
 @param[in]
	 \li
	 \li
	 \li
 @param[out] 
 @param[in,out]  
  
 @retval 
 @remarks
	 \li ����-----------------ʱ��--------�汾---------�޸�˵�� 				   
	 \li PengRongshou---07/08/18--1.0.0.0-------����
 */



/**  @} */
