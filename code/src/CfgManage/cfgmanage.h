#ifndef CFG_MANAGE_H
#define CFG_MANAGE_H

#include <stdarg.h>
#include <string.h>

#define ITEM_SIZE  (20)

typedef struct 
{
	char item[ITEM_SIZE+1];
}S_ITEMS_VALUE;

typedef struct 
{
	int type;
	S_ITEMS_VALUE * items;
	int num_per_item;

}S_VERSION_CONFIG;

typedef struct 
{
	int type;  
	char keyword[ITEM_SIZE+1];
	char context[ITEM_SIZE+1];
}S_HARD_INFO;

#define FILE_FORMAT_ERR		-1 //�ļ���ʽ����
#define KEYWORD_ERR			-2 //�ؼ��ֲ����� 
#define CONTEXT_ERR			-3 //�ؼ��ֶ�Ӧ������Ϸ�
#define FILL_ERR				-4 //�������ô���
#define NO_CONTEXT_ERR		-5 //�йؼ�����û������ 
#define ONLY_ONE_QUOTE_ERR	-6 //ֻ��һ��������
#define NO_CFG_FILE                      -7 //û�������ļ����������ļ����ز����� 
#define CFGFILE_SIG_ERR		-8	//�����ļ�ǩ������
#define FILL_ITEM(flag,x) {flag,x,sizeof(x)/sizeof(S_ITEMS_VALUE)}

//��������
#define CFG_TYPE 		1
#define BOARD_TYPE 	2
#define PARA_TYPE   		3
#define OTHER_TYPE    	4

/************************************************************************
����ԭ�ͣ�void CfgInfo_Init()           
������
	        ��
����ֵ��
	<0      ��ʼ������
	����    ��ʼ���ɹ�
************************************************************************/
void CfgInfo_Init();
/************************************************************************
����ԭ�ͣ�int ReadCfgInfo(char *keyword,char *context)           
������
	keyword(input): ģ�����ڲ�ѯӲ��������Ϣ�Ĺؼ��֣��ùؼ��ּ�����ĵ�				 
	context(ouput): ����ؼ��ֶ�Ӧ��������Ϣ���ַ���
����ֵ��
	<0      ʧ��
	����    �ɹ�
************************************************************************/
int ReadCfgInfo(char *keyword,char *context);

//�汾��Ϣ���ܸ���
int CfgTotalNum();

//ͨ�����ȡ�ùؼ��ּ��ؼ��ֶ�Ӧ������
int CfgGet(int num,int type,char *keyword,char *context);

/**************************��ȡָ����Ϣ***************/
#define INDEX_SN        		0
#define INDEX_EXSN      		1
#define INDEX_MACADDR   		2
#define INDEX_CFGFILE   		3 
#define INDEX_LCDGRAY   		4
#define INDEX_UAPUK     		5
#define INDEX_USPUK     		6 
#define INDEX_BEEPVOL     		7 
#define INDEX_KEYTONE     		8 
#define INDEX_RFPARA     		9
#define INDEX_USPUK1     		10
#define INDEX_USPUK2     		11
#define INDEX_CSN     			12
#define INDEX_TUSN     			13
#define MAX_INDEX                 20

#define MAX_BASE_CFGITEM 			(16)			// ����������Ϣ����


#endif
