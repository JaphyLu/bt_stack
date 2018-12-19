#ifndef  _LOCALDL_H
#define  _LOCALDL_H

#define     LOCAL_DL_VER            10

#define     LOCAL_DL_PACK_LEN       8192
#define     LOCAL_DL_MAX_BAUD       230400


#define     HANDSHAKE_RECV          'Q'
#define     HANDSHAKE_SEND          'S'

#define     LDL_RET_SUCCESS         0
#define     LDL_RET_FINISH          1
#define     LDL_RET_ERROR           2
#define     LDL_RET_TIMEOUT         3
#define     LDL_RET_REHANDSHAKE     4

#define     LDL_TIMEOUT             15000
#define     LDL_STX                 0x02

#define     LDL_COMMAND             0x80

#define     LDLCMD_BAUD             0xA1        //  ����ͨѶ������
#define     LDLCMD_TERMINFO         0xA2        //  ��ѯPOS�ն���Ϣ
#define     LDLCMD_APPINFO          0xA3        //  ��ѯӦ����Ϣ
#define     LDLCMD_REBUILD          0xA4        //  �ؽ�POS�ն��ļ�ϵͳ
#define     LDLCMD_SETTIME          0xA5        //  ����POS�ն�ʱ��
#define     LDLCMD_DLPUK            0xA6        //  �����û���Կ
#define     LDLCMD_DLAPP            0xA7        //  ����Ӧ�ó����ļ�����
#define     LDLCMD_DLFONT           0xA8        //  �����ֿ��ļ�����
#define     LDLCMD_DLPARA           0xA9        //  ���ز����ļ�����
#define     LDLCMD_DLDMR            0xAA        //  ����LCDע���ļ�����
#define     LDLCMD_DATA             0xAB        //  �����ļ�����
#define     LDLCMD_COMPRESSDATA     0xAC        //  ��ѹ����ʽ�����ļ�����
#define     LDLCMD_WRITEDATA        0xAD        //  д���ļ�
#define     LDLCMD_DELAPP           0xAE        //  ɾ��Ӧ�ó���
#define     LDLCMD_QUIT             0xBF        //  �������
#define     LDLCMD_OPTIONCONF       0xC0        //  �����ն����ñ�


#define     LD_OK                   0x00        //  �ɹ�
#define     LDERR_GENERIC           0x01        //  ʧ��
#define     LDERR_HAVEMOREDATA      0x02        //  ������һ����,���������δ��һ��ȫ������
#define     LDERR_BAUDERR           0x03        //  �����ʲ�֧��
#define     LDERR_INVALIDTIME       0x04        //  �Ƿ�ʱ��
#define     LDERR_CLOCKHWERR        0x05        //  ʱ��Ӳ������
#define     LDERR_SIGERR            0x06        //  ��֤ǩ��ʧ��
#define     LDERR_TOOMANYAPP        0x07        //  Ӧ��̫�࣬�������ظ���Ӧ��
#define     LDERR_TOOMANYFILES      0x08        //  �ļ�̫�࣬�������ظ����ļ�
#define     LDERR_NOAPP             0x09        //  ָ��Ӧ�ò�����
#define     LDERR_UNKNOWNAPP        0x0A        //  ����ʶ���Ӧ������
#define     LDERR_SIGTYPEERR        0x0B        //  ǩ�����������ͺ������������Ͳ�һ��
#define     LDERR_SIGAPPERR         0x0C        //  ǩ��������������Ӧ��������������Ӧ������һ��
#define     LDERR_WRITEFILEFAIL     0x10        //  �ļ�д��ʧ��
#define     LDERR_NOSPACE           0x11        //  û���㹻�Ŀռ�
#define     LDERR_UNSUPPORTEDCMD    0xFF        //  ��֧�ָ�����
#define 	LDERR_FWVERSION			0x14		// ���صĹ̼��汾����ȷ

#endif

