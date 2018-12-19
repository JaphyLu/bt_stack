#include "cfgmanage.h"
#include "base.h"
#include "..\flash\nand.h"

static uchar config_store_buff[BLOCK_SIZE];
#define CONFIG_STORE_ADDR	config_store_buff

#define KEYWORD_LEN  16   //ÿ���ؼ��ֵĳ���
const unsigned char KeywordTable[][KEYWORD_LEN+1]={
"$(CFG_SN)       ",
"$(EXSN)         ",
"$(MACADDR)      ",
"$(CFG_FILE)     ",
"$(LCDGRAY)      ",
"$(UA_PUK)       ",
"$(US_PUK)       ",
"$(BEEP_VOL)	 ",
"$(KEY_TONE)	 ",
"$(RF_PARA) 	 ",
"$(US_PUK1)      ",
"$(US_PUK2)      ",
"$(CUSTOMER_SN)  ",
"$(TUSN)         ",
};

extern uint* cal_ecc(uint *datain);
extern int ecc_correct(uint *datain,uchar *dataout);
extern void sha1(unsigned char *src,unsigned int len,unsigned char *out);

static int search(uint block_num, uint *index);
static int cfg_nand_read(uint page_addr, uchar* dst_addr);
static uint cfg_nand_write(uint page_addr, uchar* src_addr);
static uint cfg_nand_erase(uint block_num);

/************************************************************************
����ԭ�ͣ�int ReadTerminalInfo(int id,uchar*context, int len)
���ܣ���index��Ӧ������д��FLASH�����һ��д����ʱ�������´���
������id(input)   �ؼ��ֵ���������
	  context(input) �ؼ�������Ļ����������С����С��len+1(����������\x00)
	  len            context�ĳ���
����ֵ��
	   >0  ��ȡ�ĳ���
	   -1 ���кŴ���
	   -2 û�и�������
	   -3 �����Ŷ�Ӧ������Ϊ��
************************************************************************/
int ReadTerminalInfo(int id,uchar*context, int len)
{
	int len1=-1;
	int iret;
	int total_vaild_key=0;
	int Key_num;
	unsigned int index[MAX_INDEX];
	unsigned char tmp_buf[PAGE_SIZE];

	memset(index, 0xff, MAX_INDEX*4);	
	Key_num=sizeof(KeywordTable)/sizeof(KeywordTable[0]);  //�����ҵ�Keyword������

	if((id>=Key_num) || (len <1)) return -1;  //��֧�ִ�INDEX��ѯ		
	iret=Search_Keyword(index);
	if(!iret) return -2; //û������
	if(index[id] == 0xffffffff) return -3; //��������û����

	memcpy(tmp_buf, (uchar*)(CONFIG_STORE_ADDR+index[id]*PAGE_SIZE), PAGE_SIZE);
	
	len1=tmp_buf[KEYWORD_LEN];
	len1+=tmp_buf[KEYWORD_LEN+1]*256;
	if (len1 > len) len1 = len;
	memcpy(context,tmp_buf+KEYWORD_LEN+2,len1);
	context[len1]=0;
	return len1;
}


/************************************************************************
����ԭ�ͣ�int Search_Keyword(unsigned int *index)   
���ܣ����ҹؼ��֣�����Ч�ؼ��������Ϣ�ŵ�SDRAM
	          (��Ч�ؼ���:�ùؼ���������������������������������һ����)
������index(output)  ��Źؼ�����SDRAM�е�λ�ã�
        		indexӦ��Ϊһ�����飬�����±�ΪMAX_INDEX,
����ֵ��
	   0 ��ʾû����Ч�Ĺؼ���
	   ��0��ʾ��Ч�ؼ��ֵĸ���
************************************************************************/
int Search_Keyword(uint *index)
{
    int i,j,indexi,Key_num,vaild_key_num = 0;
    int len[CONFIG_BLOCK_NUM]={-1};
    uint offset, index_tmp[CONFIG_BLOCK_NUM][MAX_INDEX];
    uchar tmpbuf[CONFIG_BLOCK_NUM][PAGE_SIZE];

    memset(index_tmp, 0xff, CONFIG_BLOCK_NUM*MAX_INDEX*4);
    memset(tmpbuf, 0x00, CONFIG_BLOCK_NUM*PAGE_SIZE);
    
    Key_num=sizeof(KeywordTable)/sizeof(KeywordTable[0]);  //�����ҵ�Keyword������

    //���ҹؼ������ڸ����е�ҳ��
    for (i = 0; i < CONFIG_BLOCK_NUM; i++)
    {
        j = search((i + CONFIG_BASE_ADDR/BLOCK_SIZE), index_tmp[i]);
    }

    //���ؼ���KeywordTable[indexi]��ֵ�����ݿ������ڴ���
    for (indexi = 0; indexi < Key_num; indexi++)
    {
        j = 0;
        for (i = 0; i < CONFIG_BLOCK_NUM; i++)
        {           
            if(index_tmp[i][indexi] == 0xffffffff) continue;            
            offset = CONFIG_BASE_ADDR+i*BLOCK_SIZE+index_tmp[i][indexi]*PAGE_SIZE;
            if (cfg_nand_read(offset, tmpbuf[i])) continue;         
            j++;
        }
        if (j < 2) continue;//�ؼ���KeywordTable[indexi]��������
        
        /*������һ���Ĺؼ�����Ϣд��DDR��*/
        for (i = 0; i < PAGE_SIZE; i++)
        {
            if (tmpbuf[0][i] == tmpbuf[1][i]) continue;
            if (tmpbuf[0][i] == tmpbuf[2][i]) continue;
            if (tmpbuf[1][i] == tmpbuf[2][i])
            {
                tmpbuf[0][i] = tmpbuf[1][i];
                continue;
            }
            tmpbuf[0][i] |= tmpbuf[1][i];
        }
        memcpy((uchar*)(CONFIG_STORE_ADDR+vaild_key_num*PAGE_SIZE), tmpbuf[0], PAGE_SIZE);
        index[indexi] = vaild_key_num;
        vaild_key_num++;        
    }

    return vaild_key_num; 
}


/************************************************************************
����ԭ�ͣ�int search(uint block_num, uint *index)   
���ܣ����ҿ�block_num�и����ؼ��ֶ�Ӧ����Ч���ݵ�ҳ��(0~63)
������index(output)  ��Źؼ�����flash ���е�ҳ��(0~63)��
        		indexӦ��Ϊһ�����飬�����±�ΪMAX_INDEX,
        		���KeywordTable�ж�Ӧ�±�Ĺؼ�������ŵ�ҳ��(0~63)
����ֵ��
	   0 ��ʾû����Ч�Ĺؼ���
	   ��0��ʾ��Ч�ؼ��ֵĸ���
************************************************************************/
static int search(uint block_num, uint *index)
{
	int i,j,len,indexi;
	unsigned char*pt;
	unsigned char lrc;
	int Key_num, ret;
	int total_vaild_key=0;
	unsigned char tmp_buf[PAGE_SIZE];
	unsigned int offset = 0;
	
	pt=tmp_buf;
	Key_num=sizeof(KeywordTable)/sizeof(KeywordTable[0]);  //�����ҵ�Keyword������

	offset = block_num*BLOCK_SIZE;
	for(i=CONFIG_PAGE_NUM-1; i>=0; i--)
	{
		memset(pt, 0x00, PAGE_SIZE);
		ret = cfg_nand_read((unsigned int)(offset+i*PAGE_SIZE), pt);
		if (ret) continue;
		if('$'!=pt[0]) continue;
		for (indexi=0;indexi<Key_num;indexi++)
		{		
			if(index[indexi] != 0xffffffff) 
				continue;	 //�Ѿ��ҵ��˼�¼�ģ��Ͳ���Ѱ��
			if(!memcmp((const unsigned char* )pt,KeywordTable[indexi],KEYWORD_LEN)) 
				break;	//�ҵ��˹ؼ��֣��˳�ѭ��
		}
		if(indexi==Key_num) continue;
		
		len=pt[KEYWORD_LEN];
		len+=pt[KEYWORD_LEN+1]*256;
		if(len>PAGE_SIZE) continue;
		for(j=0,lrc=0;j<len;j++)
		{
			lrc^=pt[KEYWORD_LEN+2+j];
		}
		if(lrc==pt[KEYWORD_LEN+2+len])
		{
			index[indexi]=(unsigned int)(i);  //�ҵ��˶�Ӧ�������ţ�����ҳ������index��
			total_vaild_key++;
		}		
	}

	return total_vaild_key;
}

/************************************************************************
����ԭ�ͣ�uint search_blank_page() 
���ܣ��ӵ�ǰ���ÿ����ҵ��հ�ҳ������WriteToFlash��������
����ֵ��
			0~CONFIG_PAGE_NUM-1:FLASH�Ŀհ�ҳ��ҳ��
			CONFIG_PAGE_NUM:	 ��ǰ���ÿ���û�пհ�ҳ
************************************************************************/
static unsigned int search_blank_page(uint block_num)
{
	int i;
	int ret;
	unsigned char tmp_buf[PAGE_SIZE];
	uint offset;

	offset = BLOCK_SIZE*block_num;
	for(i=CONFIG_PAGE_NUM-1; i>=0; i--) //��FLASH�����page ���� 
	{
		ret = cfg_nand_read((unsigned int)(offset+i*PAGE_SIZE), tmp_buf);
		if (ret) continue;

		if ('$'==tmp_buf[0]) return (unsigned int)(i+1); 
	}
	
	return 0; 
}

/************************************************************************
����ԭ�ͣ�int WriteToFlash(int index,unsigned char *context,int len)   
���ܣ���index��Ӧ������д��FLASH������WriteTerminalInfo��������
������index(input)   �ؼ��ֵ������ţ�
	  context(input) �ؼ��ֶ�Ӧ������
	  len            context�ĳ���
����ֵ��
	   0  ��ʾ�ɹ�
	   -1 ���кŴ���
	   -2 ���ȳ�����Ч����
	   -3 FLASHû���㹻�Ŀռ�
************************************************************************/
int WriteToFlash(int index,unsigned char *context,int len)
{
	int i, j;
	int ret, Key_num, failed_flag = 0;
	unsigned int templen;
	unsigned int block_addr, blank_page;
	unsigned char temp_buf[PAGE_SIZE+64];
	unsigned char lrc;	
	unsigned char *puc_buf;
	
	puc_buf=temp_buf;
	Key_num=sizeof(KeywordTable)/sizeof(KeywordTable[0]);  //�����ҵ�Keyword������

	if(index >= Key_num)
		return -1;				//���кŴ���

	templen=KEYWORD_LEN+2+len+1;   //�����¼����Ҫ��FLASH���ȡ�
	if(templen>PAGE_SIZE)//ÿ��������ܳ���ÿҳ�Ĵ�С��
		return -2;  //д�볤�ȴ���
			
	memcpy(puc_buf,KeywordTable[index],KEYWORD_LEN);
	memcpy(puc_buf+KEYWORD_LEN,&len,2);
	memcpy(puc_buf+KEYWORD_LEN+2,context,len);
	for(i=0,lrc=0;i<len;i++)
	{
		lrc^=context[i];
	}
	puc_buf[KEYWORD_LEN+2+len]=lrc;  

	for (i = 0; i < CONFIG_BLOCK_NUM; i++)
	{		
		block_addr = CONFIG_BASE_ADDR + i*BLOCK_SIZE;

		blank_page = search_blank_page(CONFIG_BASE_ADDR/BLOCK_SIZE + i);
//printk("*********%s__%d****index:%dblank:%d,page:%d******\r\n", __FUNCTION__, __LINE__, index,i, blank_page);//lanwq		
		if(blank_page == CONFIG_PAGE_NUM)	return -3;	//�ռ䲻��

		for (j = blank_page; j < CONFIG_PAGE_NUM; j++)
		{
			//��ǰҳд����ȷ����д������һҳ
			if (!cfg_nand_write((block_addr + PAGE_SIZE*j), puc_buf))
				break;
		}
		if (j == CONFIG_PAGE_NUM) failed_flag++;
//printk("*********%s__%d****index:%dblank:%d,page:%d,failed_flag:%d******\r\n", __FUNCTION__, __LINE__, index,i, blank_page,failed_flag);//lanwq		
	}	

	if (failed_flag > 1) return -3;
	
	return 0;
}

/************************************************************************
����ԭ�ͣ�int WriteTerminalInfo(int index,unsigned char *context,int len)  
���ܣ���index��Ӧ������д��FLASH�����һ��д����ʱ�������´���
������index(input)   �ؼ��ֵ���������
	  context(input) �ؼ��ֶ�Ӧ������
	  len            context�ĳ���
����ֵ��
	   0  ��ʾ�ɹ�
	   -1 ���кŴ���
	   -2 ���ȳ�����Ч����
	   -4 дFLASHʧ��
************************************************************************/
int WriteTerminalInfo(int index,unsigned char *context,int len)
{
	int iret,i,j,z;
	int vaild_key_num = 0, failed_flag = 0;
	int block_addr;
	unsigned int index_buf[MAX_INDEX];
	unsigned char tmp_buf[PAGE_SIZE+64];
	unsigned char lrc;
	uchar *pt;

	memset(index_buf, 0xff, MAX_INDEX*4);
	memset(tmp_buf, 0x00, PAGE_SIZE+64);
	iret=Search_Keyword(index_buf);	
	if (!iret)
	{	
		//���޼�¼��Ȳ���
		for (j = 0; j < CONFIG_BLOCK_NUM; j++)
		{
			cfg_nand_erase(CONFIG_BASE_ADDR/BLOCK_SIZE + j);
		}
	}
	
	//���д��������µļ�¼��ͬ������д��	
	if(index_buf[index]!= 0xffffffff)
	{
		memcpy(tmp_buf,(uchar*)(CONFIG_STORE_ADDR+index_buf[index]*PAGE_SIZE), PAGE_SIZE);
		if ((len==(tmp_buf[KEYWORD_LEN+1]*256+tmp_buf[KEYWORD_LEN])) 
			&& (!memcmp((unsigned char*)(tmp_buf+KEYWORD_LEN+2), context, len)))
			return 0;
	}
	

	iret=WriteToFlash(index,context,len);	
	if(iret!=-3) return iret; 	
	//printk("*********%s__%d**flash full********\r\n", __FUNCTION__, __LINE__);//lanwq
	//��д��ʱ��������ʱ����Ҫ����������д		
	memset(index_buf, 0xff, MAX_INDEX*4);
	iret=Search_Keyword(index_buf);

	for (z = 0; z < CONFIG_BLOCK_NUM; z++)
	{			
		cfg_nand_erase(CONFIG_BASE_ADDR/BLOCK_SIZE + z);
		
		j = 0;
		pt = (uchar*)(CONFIG_STORE_ADDR);
		block_addr = CONFIG_BASE_ADDR + z*BLOCK_SIZE;
		
		for(i=0;i<MAX_INDEX;i++)  
		{
			if((index_buf[i] == 0xffffffff) || (i==index)) continue; //�����������ڵļ�¼�ͱ�����Ҫ���µļ�¼
		
			//���ڴ��п���KeywordTable[i] ����ʱ����
			memset(tmp_buf, 0x00, PAGE_SIZE);
			memcpy(tmp_buf, pt+index_buf[i]*PAGE_SIZE, PAGE_SIZE);
			
			while (j < CONFIG_PAGE_NUM)
			{
				//��ǰҳд����ȷ����д������һҳ
				iret = cfg_nand_write((block_addr+j*PAGE_SIZE), tmp_buf);
				j++;
				if (!iret) break;
			}		
		}
		
		
		//����д������ݷ��뻺����
		memset(tmp_buf, 0x00, PAGE_SIZE);
		memcpy(tmp_buf,KeywordTable[index],KEYWORD_LEN);
		memcpy(tmp_buf+KEYWORD_LEN,&len,2);
		memcpy(tmp_buf+KEYWORD_LEN+2,context,len);		
		for(i=0,lrc=0;i<len;i++)
		{
			lrc^=context[i];
		}
		tmp_buf[KEYWORD_LEN+2+len]=lrc;
		
		//д���µļ�¼
		while (j < CONFIG_PAGE_NUM)
		{
			if (!cfg_nand_write((block_addr+j*PAGE_SIZE), tmp_buf))
				break;
			j++;
		}
		
		if (j == CONFIG_PAGE_NUM) failed_flag++;
	
	}
	
	if (failed_flag > 1)
	{
		return -4;//�¼�¼д��FLASHʧ��			
	}

	return 0;	
}

static int cfg_nand_read(uint page_addr, uchar* dst_addr)
{
	uint i,j;
	uchar rbuf[(PAGE_SIZE+64)];
	int ret;
	if(page_addr<CONFIG_BASE_ADDR || page_addr > CONFIG_END_ADDR) return 0;

	umc_nand_page_rd(page_addr*2, rbuf,PAGE_SIZE +64);
	for(i=PAGE_SIZE;i<(PAGE_SIZE+64);i++) 
	{
		if(rbuf[i] !=0xFF) break;
	}

	if(i ==PAGE_SIZE+64) /*�տ鲻����ecc����*/
	{
		memcpy(dst_addr,rbuf,PAGE_SIZE);
		return 0;
	}
	
	return ecc_correct((uint *)rbuf, dst_addr);
}

static uint cfg_nand_write(uint page_addr, uchar* src_addr)
{
	uint i,j;
	uchar hash_buf[32];
	uint *ecc_buf;
	uchar rbuf[(PAGE_SIZE+64)];

	if(page_addr<CONFIG_BASE_ADDR || page_addr > CONFIG_END_ADDR) return 0;

	ecc_buf = cal_ecc((uint *)src_addr);
	sha1(src_addr,PAGE_SIZE,hash_buf);
	memcpy(src_addr+PAGE_SIZE,ecc_buf,32);
	memcpy(&src_addr[(PAGE_SIZE+32)],hash_buf,HASH_SIZE);
	memcpy(&src_addr[(PAGE_SIZE+32+HASH_SIZE)],hash_buf,HASH_SIZE);

	umc_nand_page_prg( page_addr*2, src_addr, PAGE_SIZE + 64);

	if (cfg_nand_read(page_addr, rbuf) || memcmp(src_addr, rbuf, PAGE_SIZE))
		return -1;
	
	return 0;
}

static uint cfg_nand_erase(uint block_num)
{
    if(block_num<(CONFIG_BASE_ADDR /BLOCK_SIZE) || 
        block_num >(CONFIG_END_ADDR /BLOCK_SIZE -1)) return 0;
    
    nand_phy_erase(block_num);
	return 0;
}

void s_GetCfgPageNo(int id, uint PageNo[])
{
    uint i,index_tmp[CONFIG_BLOCK_NUM][MAX_INDEX];

    memset(index_tmp, 0xff, CONFIG_BLOCK_NUM*MAX_INDEX*4);
    
    //���ҹؼ������ڸ����е�ҳ��
    for (i = 0; i < CONFIG_BLOCK_NUM; i++)
    {
        search((i + CONFIG_BASE_ADDR/BLOCK_SIZE), index_tmp[i]);
        PageNo[i] = index_tmp[i][id];
    }
}
#if 0//lanwq
void test_cfg(void)
{
	int i,j,len;
	char tmp_buf[2048]= {0};
	uchar ch;
	static int cnt = 0;
	ScrCls();
	ScrPrint(0,1,0,"1--write\r\n");	
	ScrPrint(0,2,0,"2--read\r\n");	
	ScrPrint(0,3,0,"3--earse\r\n");		
	
	i = 0;
	while(1)
	{
		ch = 0x00;
		ch = getkey();
		printk("%02x ", ch);
		switch(ch)
		{
			case 0x31:
				for (i = 0; i < 20; i++)
				{
					for (j = 0; j < 2048; j++)
						tmp_buf[j] = i+cnt;
				
					len = WriteTerminalInfo(i%10, tmp_buf, 30);
					printk("%d, \r\n", len);			
				}
				cnt++;
			break;
			case 0x32:
				for (i = 0; i < 20; i++)
				{
					memset(tmp_buf, 0x00, 2048);
					len = ReadTerminalInfo(i, tmp_buf, 2048);
				
					printk("\r\nID:%d, LEN:%d\r\n", i, len);
					j = 0;
					while(j < len) 
					{
						printk("%d, ", tmp_buf[j++]);			
					}
				}
			break;
			case 0x33:
				for (i = 0; i < 3; i++)
				{
					cfg_nand_erase(i+1);
				}
			break;

			default:
			break;
		}
	}
}
#endif

