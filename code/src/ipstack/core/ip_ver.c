/*
	��Ű汾��Ϣ
	author: sunJH
	create: 2008-10-10
History:
081010 sunJH
net_version��ȡ�İ汾��Ϣֻ��һ���ֽ�
Ŀǰ�Ѿ�������������,
�������һ���ӿ�NetGetVersion��ȡ����
�İ汾��Ϣ
�ַ�����: 116-081010-D
����Ϊ: 116           �汾��,����һ���ۼ�
                  081010      ��������
                  D/R          DΪDebug, RΪRelease
*
*/
#include "inet/inet.h"

#ifdef NET_DEBUG
#define LAST_VER "D"
#else
#define LAST_VER "R"
#endif
 
static char net_ver_str[30]="165-120605-"LAST_VER; 


void NetGetVersion_std(char ip_ver[30])
{
	strcpy(ip_ver, net_ver_str);
}

