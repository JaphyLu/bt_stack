#ifndef _MODULECHECK_H
#define _MODULECHECK_H
#define MODULEMAX   9 //���ģ����
enum{MC_ICC = 0,MC_MAG,MC_RF,MC_KEYBOARD,MC_LCD,MC_MODEM,MC_ETHNET,MC_WNET};
/**
����:��ʾģ����˵�
iModule [in] ��ʾ����ʾ��ģ�飬�����ڵ�Ԫ����ʹ������enum�ṹ�е�Ԫ�أ�
			 ��������Щģ����Ҫ����ģ���⣬���������м����ģ�顣
			 �����и�ģ���˳������˲˵��и����˳��
			 ����ĳЩģ�鲻�̶��Ļ��ͣ���Ҫ����һ���㹻������飬��̬���������ģ�顣
iModuleNum [in] ��ʾiModule�е�ǰiModuleNum�����ʾ������
*/
int ModuleCheckMenu(int iModule[],int iModuleNum);
/**
����:��ȡ��汾��Ϣ
*/
void ModuleCheckGetVer(char Version[30]);
#endif

