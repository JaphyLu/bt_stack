/*****************************************************************
2007-11-13
�޸���:�����
�޸���־:
�޸�MSR_TrackInfo�ṹ��
*****************************************************************/
#ifndef _ENMAGCARD_H_
#define	_ENMAGCARD_H_

void  encryptmag_init(void);

// API�ӿ�
void  encryptmag_reset(void);
void  encryptmag_open(void);
void  encryptmag_close(void);
void  encryptmag_read_rawdata(void);
void  encryptmag_swiped(void);

#endif
