/*
�޸���ʷ��
20080605
1.ppp_md5_digestĿǰֻ����s80���ã�����ƽ̨Ŀǰ������
20080917 sunJH
ȥ��s80����,��������ƽ̨��������
*/
#include "md5.h"

void ppp_md5_digest(unsigned char *digest, const unsigned char *data, unsigned int len)
{
	MD5_CTX ctx;
    MD5Init(&ctx);
    MD5Update(&ctx, (unsigned char *)data, len);
    MD5Final(digest, &ctx);
}
