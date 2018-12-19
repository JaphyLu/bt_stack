#ifndef ISO14443_HW_HAL_H
#define ISO14443_HW_HAL_H

#ifdef __cplusplus
extern "C" {
#endif

#define PCD_MAX_BUFFER_SIZE                 ( 512 )

/*according to the definitions with ISO14443-3 Type A*/
#define ISO14443_TYPEA_SHORT_FRAME      ( 0x07 ) /*short frame, consist of 7 bits*/
#define ISO14443_TYPEA_STANDARD_FRAME   ( 0x09 ) /*standard frame, consist of 9 bits( 8 data bits + odd parity bit )*/

/*the protocol types*/
#define ISO14443_TYPEA_STANDARD         ( 0x01 )
#define ISO14443_TYPEB_STANDARD         ( 0x02 )
#define JISX6319_4_STANDARD             ( 0x03 )
#define MIFARE_STANDARD                 ( 0x04 )

/*the baudrate in communication*/
#define BAUDRATE_106000                 ( 106000 )
#define BAUDRATE_212000                 ( 212000 )
#define BAUDRATE_424000                 ( 424000 )
#define BAUDRATE_848000                 ( 848000 )

/*High layer communication protocol*/
#define ISO14443_4_STANDARD_COMPLIANT_A ( 0x00 ) /*compliant with iso14443-4 protocol*/
#define ISO14443_4_STANDARD_COMPLIANT_B ( 0x0A )
#define ISO18092_STANDARD_COMPLIANT     ( 0x07 ) /*NFC*/
#define PICC_SUPPORT_STANDARD_UNKOWN    ( 0xFF ) /*unkown protocol*/

/*the timing definitions*/
#define ISO14443_PICC_FDT_MIN         ( 70 )/*6780 clock cycles*/
#define ISO14443_POLLING_FDT_MAX      ( 1060 )/*10ms*/
#define ISO14443_POLLING_FDT_MIN      ( 541 )/*5.1ms*/

/*during emv polling, identify card type*/
#define TYPEA_INDICATOR   ( 1 << 0 )
#define TYPEB_INDICATOR   ( 1 << 1 )
#define TYPEC_INDICATOR   ( 1 << 2 )
#define TYPEM_INDICATOR   ( 1 << 31 )

/*PCD and PICC status*/
#define PCD_BE_CLOSED      ( 0 )/*rf module already closed*/
#define PCD_BE_OPENED      ( 1 )/*rf module already opened*/
#define PCD_CARRIER_ON     ( 2 )/*rf carrier has been enabled*/
#define PCD_CARRIER_OFF    ( 3 )/*rf carrier has been disabled*/
#define PCD_WAKEUP_PICC    ( 4 )/*already waked up picc*/
#define PCD_ACTIVE_PICC    ( 5 )/*already actived picc*/
#define PCD_REMOVE_PICC    ( 6 )/*enter removal procedure*/
#define PCD_CHIP_ABNORMAL  ( 7 )

/* add by wanls 20150921*/
#define TX_CRC_MASK        (0x01)
#define RX_CRC_MASK        (0x02)
/* add end */

struct ST_PCDINFO;

typedef struct
{
	unsigned char  drv_ver[5];  //��������İ汾��Ϣ���磺��1.01A����ֻ�ܶ�ȡ��д����Ч
	unsigned char drv_date[12];  // ���������������Ϣ���磺��2006.08.25���� ֻ�ܶ�ȡ
	unsigned char a_conduct_w;  //A�Ϳ�����絼д������1--��������ֵ��������
	unsigned char a_conduct_val;  // A�Ϳ�����絼���Ʊ�������Ч��Χ0~63,����ʱ��Ϊ63
	//a_conduct_w=1ʱ����Ч�������ֵ�������ڲ������ı�//���ڵ�������A�Ϳ���������ʣ��ɴ��ܵ���������Ӧ����
	unsigned char m_conduct_w;  //M1������絼д������1--��������ֵ��������
	unsigned char m_conduct_val;  // M1������絼���Ʊ�������Ч��Χ0~63,����ʱ��Ϊ63
	//m_conduct_w=1ʱ����Ч�������ֵ�������ڲ������ı� //���ڵ�������M1����������ʣ��ɴ��ܵ���������Ӧ����
	unsigned char b_modulate_w;  // B�Ϳ�����ָ��д������1--��������ֵ��������
	unsigned char b_modulate_val;  // B�Ϳ�����ָ�����Ʊ�������Ч��Χ0~63,����ʱ��Ϊ63
	// b_modulate_w=1ʱ����Ч�������ֵ�������ڲ������ı�,���ڵ�������B�Ϳ��ĵ���ָ�����ɴ��ܵ���������Ӧ����
	unsigned char  card_buffer_w;  //��Ƭ���ջ�������Сд������1--��������ֵ��������
	unsigned short card_buffer_val; //��Ƭ���ջ�������С��������λ���ֽڣ�����Чֵ1~256��
	//����256ʱ������256д�룻��Ϊ0ʱ��������д�롣
	//��Ƭ���ջ�������Сֱ�Ӿ������ն���Ƭ����һ�����ʱ�Ƿ��������ְ����͡��Լ������ְ���������С���������͵�������ڿ�Ƭ���ջ�������С�����轫���и��С����������η���
	//��PiccDetect( )����ִ�й����У���Ƭ���ջ�������С֮�����ɿ�Ƭ������նˣ�һ��������Ĵ�ֵ�������ڷǱ�׼����������Ҫ����˲���ֵ���Ա�֤������Ч����
	unsigned char  wait_retry_limit_w;// S(WTX)��Ӧ���ʹ���д������1--��������ֵ�������� (�ݲ�����)
	unsigned short wait_retry_limit_val;//S(WTX)��Ӧ������Դ���, Ĭ��ֵΪ3(�ݲ�����)
	// 20080617 
	unsigned char card_type_check_w; // ��Ƭ���ͼ��д������1--��������ֵ--��������Ҫ���ڱ�����Ƭ���淶���������
	unsigned char card_type_check_val; // 0-��鿨Ƭ���ͣ�����������鿨Ƭ����(Ĭ��Ϊ��鿨Ƭ����)
	//2009-10-30
	unsigned char card_RxThreshold_w; //���������ȼ��д������1--��������ֵ--��������Ҫ���ڱ�����Ƭ���淶���������
	unsigned char card_RxThreshold_val;//���߲���Ϊ5���ֽ�ʱ��ΪA����B���Ľ��������ȣ����߲���Ϊ7���ֽ�ʱΪB������������
	//2009-11-20
	unsigned char f_modulate_w; // felica����ָ��д������
	unsigned char f_modulate_val;//felica����ָ��

	//add by wls 2011.05.17
	unsigned char a_modulate_w; // A������ָ��д������1--��������ֵ��������
	unsigned char a_modulate_val; // A������ָ�����Ʊ�������Ч��Χ0~63,����ʱ��Ϊ63
	
    //add by wls 2011.05.17
	unsigned char a_card_RxThreshold_w; //���������ȼ��д������1--��������ֵ--������
	unsigned char a_card_RxThreshold_val;//A������������
	
	//add by liubo 2011.10.25, ���A,B��C������������
	unsigned char a_card_antenna_gain_w;
	unsigned char a_card_antenna_gain_val;
	
	unsigned char b_card_antenna_gain_w;
	unsigned char b_card_antenna_gain_val;
	
	unsigned char f_card_antenna_gain_w;
	unsigned char f_card_antenna_gain_val;
	
	//added by liubo 2011.10.25�����Felica�Ľ���������
	unsigned char f_card_RxThreshold_w;
	unsigned char f_card_RxThreshold_val;

	/* add by wanls 2012.08.14*/
	unsigned char f_conduct_w;  
	unsigned char f_conduct_val; 
	
	/* add by nt for paypass 3.0 test 2013/03/11 */
	unsigned char user_control_w;
	unsigned char user_control_key_val;

	/* add by wanls 20141106*/
	unsigned char protocol_layer_fwt_set_w;
	unsigned int  protocol_layer_fwt_set_val;
	/* add end */

	/* add by wanls 20150921*/
	unsigned char picc_cmd_exchange_set_w;
	/*Bit0 = 0 Disable TX CRC, Bit0 = 1 Enable TX CRC*/
	/*Bit1 = 0 Disable RX CRC, Bit1 = 1 Enable RX CRC*/
	unsigned char picc_cmd_exchange_set_val;
	/* add end */
	
	unsigned char reserved[60]; //modify by nt 2013/03/11 original 74.�����ֽڣ����ڽ�����չ��д��ʱӦȫ����


}PICC_PARA;/*size of PICC_PARA is 124 Bytes*/



/*the abstract interface for hardware board*/
struct ST_PCDBSP
{
    int (*RfBspInit)( void );
    
    int (*RfBspPowerOn)( void );
    int (*RfBspPowerOff)( void );
    
    int (*RfBspRead)( unsigned char, unsigned char*, int );
    int (*RfBspWrite)( unsigned char, unsigned char*, int );
    
    int (*RfBspIntInit)( void(*)(void) );
    int (*RfBspIntEnable)( void );
    int (*RfBspIntDisable)( void );
    
};

/*the abstract interfaces PCD interface integrity chip*/
struct ST_PCDOPS
{
    int (*PcdChipInit)( struct ST_PCDINFO* ) ;/*the chip control initialization*/
    
    int (*PcdChipOpenCarrier)( struct ST_PCDINFO* );/*output carrier, 13.56MHz*/
    int (*PcdChipCloseCarrier)( struct ST_PCDINFO* );/*stop carrier, 13.56MHz*/
    
    int (*PcdChipConf)( struct ST_PCDINFO* );/*configuration of PCD*/
    int (*PcdChipWaiter)( struct ST_PCDINFO*, unsigned int );
    
    int (*PcdChipTrans)( struct ST_PCDINFO* );
    int (*PcdChipTransCeive)( struct ST_PCDINFO* );/*send datas to PICC and receive datas from PICC*/
    
    int (*PcdChipMifareAuthen)( struct ST_PCDINFO* );/*pcd process the mifare one standard authentication*/
    
    void (*PcdChipISR)( void );/*the interruption service of chip*/

	
	int (*GetParamTagValue)(PICC_PARA *, unsigned char *);

	int (*SetParamValue)(PICC_PARA *);
};

/*the descriptor of PCD*/
struct ST_PCDINFO
{
    /*PCD's operation status, 0 - CLOSE; 1 - OPEN; 2 - OUTPUT CARRIER, 
      3 - CLOSE CARRIER; 4 - WAKEUP; 5 - ACTIVE; 6 - REMOVAL, 7 - CHIP ABNORMAL*/
    unsigned int      uiPcdState;
    
    /*paramters of communication with PICC*/
    unsigned char     aucUid[10]; /*the unique identification of card*/
    unsigned char     ucUidLen;/*the length of the unique identification of card*/

    unsigned char     ucNad;
    unsigned char     ucNadEn;/*the picc support NAD*/
    unsigned char     ucCid;/*the identification channel*/
    unsigned char     ucCidEn;/*the picc support CID*/
    unsigned char     ucAdc;/*the application code*/
    unsigned char     ucPPSEn;/*support PPS*/
    
//    unsigned char     ucApn;/*the application number of type B*/
    
    unsigned char     ucEmdEn;/*anti-electric-magnetic disturb*/

    unsigned int      uiPollTypeFlag;/*the type bit map in polling procedure, with bit0=A,bit1=B,bit2=C... sequence*/

    unsigned char     ucPiccType;/*card type, mifare, compliant with iso14443-4 or unkown etc.*/
    unsigned char     aucSAK[4];/*SAK[0] is length for Type A SAK(n) n = 1, 2, 3 or ATTRIB response*/
    unsigned char     aucATQx[14];/*ATQx[0] is Length for ATQA or ATQB*/    
    unsigned char     aucATS[254];/*ATS[0] is length for Type A ATS*/

    unsigned int      uiFsc;/*the maximum size of PICC frame*/
    unsigned int      uiMaxBuf;/*the maxium buffer size of PICC*/
    unsigned int      uiFwt;/*the maximum frame waiting time*/
    unsigned int      uiSfgt;/*the frame guard time*/

    /*the parameters of protocol*/
    unsigned char     ucProtocol;/*the type of protocol, Mifare, TypeA or TypeB etc.*/
    unsigned char     ucTxCrcEnable;/*enable/disable tx crc*/
    unsigned char     ucRxCrcEnable;/*enable/disable rx crc*/
    unsigned char     ucModInvert;/*inverted modulation*/

    unsigned char     ucFrameMode;/*the mode of frame, eg. short frame, standard frame etc.*/
    unsigned char     ucM1CryptoEn;
	unsigned char     ucM1AuthorityBlkNo;/*add by wanls 2012.06.04*/

    unsigned int      uiPcdBaudrate;/*the transmitted speed with PCD*/
    unsigned int      uiPiccBaudrate;/*the transmitted speed with PICC*/

    unsigned int      uiPcdSupportBaudrate;
    unsigned int      uiPiccSupportBaudrate;

    /*the PCD and PICC maintain last i block PCB*/
    unsigned char     ucPcdPcb;
    unsigned char     ucPiccPcb;

    /*rf data buffer*/
    unsigned char     aucPcdTxRBuffer[PCD_MAX_BUFFER_SIZE];
    unsigned int      uiPcdTxRNum;
    unsigned int      uiPcdTxRLastBits;
    
    struct ST_PCDBSP *ptBspOps;/*the interfaces of board*/
    struct ST_PCDOPS *ptPcdOps;/*the interfaces of driver chip*/
};

/**
 * get the max frame size according to index
 */
unsigned short GetMaxFrameSize( int index );

/**
 * initialising PCD device
 * 
 * parameter:
 *            ptPcdInfo :  device information structure pointer
 *            ptChipOps :  the operations structure pointer with chip
 *            ptBsp     :  the operation structure pointer with hardware board
 * retval:
 *            0 - successfully
 *            others, error
 */
int PcdDefaultInit( struct       ST_PCDINFO *ptPcdInfo, 
             const struct ST_PCDOPS  *ptChipOps,
             const struct ST_PCDBSP  *ptBspOps );
             
/**
 * open module power
 *
 * parameter:
 *            ptPcdInfo  :  device information structure pointer
 * retval:
 *            0 - successfully
 *            others, error
 */
int PcdPowerUp( struct ST_PCDINFO* ptPcdInfo );

/**
 * close module power
 *
 * parameter:
 *            ptPcdInfo :  device information structure pointer
 * retval:
 *            0 - successfully
 *            others, error
 */
int PcdPowerDown( struct ST_PCDINFO* ptPcdInfo );

/**
 * open carrier for radio, 13.56MHz
 *
 * parameter:
 *            ptPcdInfo :  device information structure pointer
 * retval:
 *            0 - successfully
 *            others, error
 */
int PcdCarrierOn( struct ST_PCDINFO* ptPcdInfo );

/**
 * close carrier for radio, 13.56MHz
 *
 * parameter:
 *            ptPcdInfo :  device information structure pointer
 * retval:
 *            0 - successfully
 *            others, error
 */
int PcdCarrierOff( struct ST_PCDINFO* ptPcdInfo );

/**
 * using chip timer delay n etu
 *
 * parameter:
 *            ptPcdInfo :  device information structure pointer
 *            iEtuCount :  the time with etu unit
 * retval:
 *            non-return
 */
void PcdWaiter( struct ST_PCDINFO* ptPcdInfo, unsigned int iEtuCount );

/**
 * read/write internal register in chip
 * 
 * parameter:
 *            ptPcdInfo :  device information structure pointer
 *            ucDir     :  0 - write / 1 - read
 *            uiRegAddr :  register address
 *            pucIOBuf  :  i/o buffer  
 *            iTRxN     :  buffer number
 * retval:
 *            non-return
 */
int PcdDeviceIO( struct ST_PCDINFO* ptPcdInfo, 
                 unsigned char     ucDir,
                 unsigned int      uiRegAddr,
                 unsigned char    *pucIOBuf, 
                 int               iTRxN );
              
/**
 * start mifare authentication between PCD and PICC
 *
 * parameter:
 *            ptPcdInfo :  device information structure pointer
 *            iType     :   the type of key group
 *            iBlkNo    :   the number of block
 *            pucPwd    :   the sector password.(6 bybtes) 
 * retval:
 *            0 - successfully
 *            others, error
 */
int PcdMifareAuthenticate( struct ST_PCDINFO* ptPcdInfo, int iType, int iBlkNo, unsigned char *pucPwd );

/**
 * start transfer in PCD device 
 *
 * parameter:
 *            ptPcd        :   device information structure pointer
 *            pucWriteBuf  :   the datas buffer will be transmitted
 *            iTxN         :   the number will be transmitted
 * retval:
 *            0 - successfully
 *            others, error
 */
int PcdTrans( struct ST_PCDINFO* ptPcdInfo, unsigned char* pucWriteBuf, int iTxN );
                  
/**
 * start transfer in PCD device and receiving the response from PICC
 *
 * parameter:
 *            ptPcd        :   device information structure pointer
 *            pucWriteBuf  :   the datas buffer will be transmitted
 *            iTxN         :   the number will be transmitted
 *            pucReadBuf   :   the datas buffer will be received
 *            iExN         :   the expected received number 
 * retval:
 *            0 - successfully
 *            others, error
 */
int PcdTransCeive( struct ST_PCDINFO* ptPcdInfo, 
                   unsigned char* pucWriteBuf, 
                   int iTxN, 
                   unsigned char* pucReadBuf, 
                   int iExN );


/*error code definitions*/
#define ISO14443_HW_ERR_COMM_TIMEOUT ( -1 )
#define ISO14443_HW_ERR_COMM_FIFO    ( -2 )
#define ISO14443_HW_ERR_COMM_COLL    ( -3 )
#define ISO14443_HW_ERR_COMM_PARITY  ( -4 )
#define ISO14443_HW_ERR_COMM_FRAME   ( -5 )
#define ISO14443_HW_ERR_COMM_PROT    ( -6 )
#define ISO14443_HW_ERR_COMM_CODE    ( -7 )
#define ISO14443_HW_ERR_RC663_SPI    ( -8 )
#define ISO14443_HW_ERR_COMM_CRC     ( -9 )
#define ISO14443_PCD_ERR_USER_CANCEL ( -10 ) /* add by nt for paypass 3.0 2013/03/11 */
#ifdef __cplusplus
}
#endif

#endif
