/*****************************************************************
2007-11-13
�޸���:�����
�޸���־:
�޸�MSR_TrackInfo�ṹ��
*****************************************************************/
#ifndef _MAGCARD_H_
#define	_MAGCARD_H_



#ifndef     ON
#define     ON                  1
#endif

#ifndef     OFF
#define     OFF                 0
#endif

#ifndef     IO_OUT
#define     IO_OUT              0
#endif

#ifndef     IO_IN
#define     IO_IN               1
#endif

#ifndef     IO_INT_FALLING
#define     IO_INT_FALLING      2
#endif

#ifndef     IO_INT_RISING
#define     IO_INT_RISING       3
#endif


#define PARI_ERR        0x01
#define LRC_ERR         0x10

#define TRACK_LEN       256
#define TRACK_BIT_LEN   2100



// Modified by yangrb 2007-11-13
typedef struct
{
    uchar   buf[150];           //  ���������ݻ���,δ����,����ʼ����������У���
    uchar   start;              //  ����ʱ��ʼ���Ƿ���ȷ. 1=�ǣ�0=��
    uchar   stop;               //  ����ʱ�������Ƿ���ȷ. 1=�ǣ�0=��
    uchar   lrc;                //  ����ʱУ����Ƿ���ȷ. 1=�ǣ�0=��
    uchar   odd;                //  ����ʱĳ�ֽ�У���Ƿ���ȷ.1=�� 0=��
    int     len;                //  ����õ������ݳ���(������ʼ������������У���)
    int     error;              //  ������״̬��0=�ɹ���1=����
    //int     OddErr;             //  n=��У���д���ĸ���
}MSR_TrackInfo;

typedef struct
{
    uchar   buf[750];           //  �Ӵ�ͷ���ص�λ����
    int     BitOneNum;          //  �Ӵ�ͷ���ص�λ����Ϊ1�ĸ���
}MSR_BitInfo;

void   s_MagInit(void);

void   MagOpen(void);
void   MagClose(void);
void   MagReset(void);
uchar  MagSwiped(void);
uchar  MagRead(uchar *Track1, uchar *Track2, uchar *Track3);
void	s_SetMagBitBuf(MSR_BitInfo msr_bt[3]);
#endif
