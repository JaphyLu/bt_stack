#ifndef _ICC_DEVICE_CONFIGURE_H
#ifdef __cplusplus
extern "C" {
#endif
#define _ICC_DEVICE_CONFIGURE_H


/* SAM */
typedef struct{
	int sam1_pwr_pin;//sam1_pwr
	int sam1_pwr_enable;//-1
	int sam1_pwr_port;

	int sam2_pwr_pin;//sam2_pwr
	int sam2_pwr_enable;//-1
	int sam2_pwr_port;

	int sam3_pwr_pin;//sam3_pwr
	int sam3_pwr_enable;//-1
	int sam3_pwr_port;

	int sam4_pwr_pin;//sam4_pwr
	int sam4_pwr_enable;//-1
	int sam4_pwr_port;

	int sam_sel0_pin;//sel0
	int sam_sel1_pin;//sel1
	int sam_sel0_port;
	int sam_sel1_port;

	int sam_sel_en_pin;//sel_en,may not exist 
	int sam_sel_en_enable;//-0
	int sam_sel_en_port;

	int sam_rst_pin;//sam_rst
	int sam_rst_enable;//-1
	int sam_rst_port;

}SCI_SAM_INTERFACE;

/* NCN8025 */
typedef struct{
	int vsel0_pin; 		//8025_sel0 
	int vsel0_port;
	
	int vsel1_pin;			//8025_sel1
	int vsel1_port;
	
	int ncmdvcc_pin;		//8025_icc_vcc
	int ncmdvcc_port;
	
	int aux1_pin;           //icc_c4
	int aux1_port;
	
	int aux2_pin;           //icc_c8
	int aux2_port;
	
	int rstin_pin;			 //icc_rst
	int rstin_port;

	int io_pin;			//icc_io
	int io_port;

	int clkin;			//icc_clk
	int clkin_port;

	int nint;			//ncn8025_intr
	int nint_port;

}SCI_NCN8025_INTERFACE;


/*typedef struct{
	SCI_NCN8025_INTERFACE ncn8025_interface;
	SCI_SAM_INTERFACE sam_interface;
}SCI_NCN8025_SAM;*/



typedef struct{
	int sdwn_pd_pin;
	int sdwn_pd_enable;//-1
	int sdwn_pd_port;

	int intpd_pin;
	int intpd_port;

	int i2c_scl_pin;
	int i2c_scl_port;

	int i2c_sda_pin;
	int i2c_sda_port;

	int i2c_base_addr;
}SCI_TDA8026_INTERFACE;

typedef struct{
	int sci_user_io;
	int sci_sam_io;
	int sci_user_clk;
	int sci_sam_clk;
	int sci_user_base;
	int sci_sam_base;
	int sci_ex_intr;
}SCI_BCM5892_CONTROLLER;

typedef struct{
	SCI_BCM5892_CONTROLLER sci_controller;//ָ��CPU�������ṹ��
	SCI_NCN8025_INTERFACE ncn8025_interface;//ָ��ӿ�оƬNCN8025�ṹ��
	SCI_SAM_INTERFACE sam_interface;//ָ��SAM�ṹ��
	SCI_TDA8026_INTERFACE tda8026_interface;//ָ��ӿ�оƬTDA8026�ṹ��

	SCI_TDA8026_INTERFACE second_tda8026_interface;//ָ��ڶ����ӿ�оƬTDA8026�ṹ��
	int user_slot_num;//���USER������
	int sam_slot_num;//���SAM������
	int icc_interfaceIC_type;
}SCI_RESOURCE;


extern SCI_RESOURCE sci_device_resource;


int sci_resource_configure(void);
void insert_card_action(void);

#ifdef __cplusplus
}
#endif
#endif
