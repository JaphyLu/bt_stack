/*****************************************************/
/* userdef.h                                         */
/* For file system base on flash chip                */
/* For all PAX POS terminals     		             */
/* Created by ZengYun at 20/11/2003                  */
/*
2005��3��18�գ�
Ϊ�������س���ռ��304K����432K���뽫�ļ�ϵͳ�����һ��128K�ռ䣬Ҳ���ļ�ϵͳ
�����һ��128K�ռ䡣
ͬʱ���޸�FILEDEF��C�е�DATA��SECTORֵ������ֵ��2��

*/

/*****************************************************/

#ifndef _FILEDEF_H
#define _FILEDEF_H

#include "posapi.h"

//For debug
//#define _DEBUG_F

extern unsigned long   DATA_SECTORS;
extern unsigned long BLOCK_PAGES	;			  //ÿ�������������������ݿ���
extern unsigned long LOG_SIZE 	;				  //��־��С

extern unsigned long FAT_SECTOR_SIZE	;			      //FAT�����������Ĵ�С
extern unsigned long LOG_SECTOR_SIZE   ;                 //��־�����������Ĵ�С
extern unsigned long	DATA_SECTOR_SIZE    ;               //���������Ĵ�С

extern unsigned long	FAT1_ADDR;	  //FAT1��ĵ�ַ
extern unsigned long	FAT2_ADDR;	  //FAT2��ĵ�ַ
extern unsigned long	LOG_ADDR;     //��־��ĵ�ַ
extern unsigned long	DATA_ADDR;     //������������ʼ��ַ
extern ushort g_LogSector[0x10000]; /*������������*/


#define     LOG_END_ADDR   (LOG_ADDR+LOG_SIZE-0x40)  /*��ֹдԽ��*/
#define     FAT1_LOG_ADDR       (LOG_ADDR+2)  // 2Bytes
#define     FAT2_LOG_ADDR       (LOG_ADDR+4)  // 2Bytes
#define     INIT_LOG_ADDR       (LOG_ADDR+6)   // 8Bytes

#define     FAT_VERSION_FLAG    "FFAT_V20"
#define     LOG_VERSION_FLAG    "FLOG_V20"



#define MAX_FILES        256                      //����ļ���

#define MAX_APP_LEN		2048*1024 //2M
#define MAX_PARAM_LEN	512*3*1024//1.5M
//ע�ͣ�
//���FAT����̫С,�ռ䲻���������,MAX_FILES���ܻᱻϵͳ�ض���,
//����ʵ�ʵ�����ļ�������С����Ķ���ֵ

//#define for file attr
#define FILE_ATTR_MON       0XFF
/*attr 0...23 : APP0...APP23*/
#define FILE_ATTR_PUBSO   0xE0
#define FILE_ATTR_PUBFILE 0xE1


//define for file type
#define	FILE_TYPE_MAPP	    0//�ó��������
#define	FILE_TYPE_APP	    1//Ӧ�ó���
#define	FILE_TYPE_FONT	    2//�ֿ�
#define	FILE_TYPE_LIBFUN	3//�⺯��
#define	FILE_TYPE_USER_FILE	4//�û��ļ�
#define	FILE_TYPE_SYS_PARA	5//ϵͳ�����ļ�
#define	FILE_TYPE_APP_PARA	6//Ӧ�ò����ļ�
#define	FILE_TYPE_PED		8//PED�ļ�
#define	FILE_TYPE_PUK		9//�û���Կ�ļ�
#define FILE_TYPE_USERSO	10 //�û���̬��
#define FILE_TYPE_SYSTEMSO  11 //ϵͳ��̬��
//----define file convert return value

#define FTO_RET_OK					0
#define FTO_RET_APPINFO_ERR			-1
#define FTO_RET_FILE_NO_EXIST		-2
#define FTO_RET_SIG_ERR				-3
#define FTO_RET_TOOMANY_APP			-4
#define FTO_RET_NAME_DEUPT			-5
#define FTO_RET_APP_TYPE_ERR		-6
#define FTO_RET_WRITE_FILE_ERR		-7
#define FTO_RET_READ_FILE_ERR		-8
#define FTO_RET_WITHOUT_APP_NAME	-9
#define FTO_RET_TOOMANY_FILE		-10
#define FTO_RET_NO_APP				-11
#define FTO_RET_PARAM_ERR			-12
#define FTO_RET_FONT_ERR			-13
#define FTO_RET_FILE_TOOBIG			-14
#define FTO_RET_NO_SPACE			-15
#define FTO_RET_NO_BASE				-16			// û������
#define FTO_RET_OFF_BASE			-17			// ��������λ

// -- FOR SO & Public file
#define FTO_RET_SOINFO_ERR			-20
#define	FTO_RET_PARAMETER_INVALID	-21
#define FTO_RET_PARAMETER_NULL		-22
#define FTO_RET_FILE_HAS_EXISTED	-23
#define FTO_RET_TOO_MANY_HANDLES	-24

//--------------

#endif
