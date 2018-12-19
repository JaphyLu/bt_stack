/* RSA.C - RSA routines for RSAREF
 */

/* Copyright (C) RSA Laboratories, a division of RSA Data Security,
     Inc., created 1991. All rights reserved.
 */

#include "global.h"
#include "rsaref.h"
#include "r_random.h"
#include "Rsa.h"
#include "xy_rsa.h"
#include "nn.h"
#include "string.h"

int RSAPublicDecrypt(unsigned char *output,         /* output block */
                         unsigned int *outputLen,       /* length of output block */
                         unsigned char *input,          /* input block */
                         unsigned int inputLen,         /* length of input block */
                         R_RSA_PUBLIC_KEY *publicKey)   /* RSA public key */
{
    int ret;
    unsigned int modulusLen;
    rsa_context rsa;
    int mpi_buf[1024*16];

    modulusLen = (publicKey->uiBitLen+ 7) / 8;
    if (inputLen > modulusLen)
    return (-1);

    mpi_buf_init(mpi_buf,sizeof(mpi_buf));
    memset( &rsa, 0, sizeof( rsa_context ) );
    rsa.len = modulusLen;
    mpi_read_binary( &rsa.N, &publicKey->aucModulus[256-modulusLen], modulusLen);
    mpi_read_binary( &rsa.E, publicKey->aucExponent, 4);
    if( (ret=rsa_pkcs1_encrypt( &rsa, RSA_PUBLIC, modulusLen,input, output )) != 0 ) return -1;
     *outputLen = modulusLen;
     
    return (0);
}


/* RSA private-key encryption, according to PKCS #1.
 */
int RSAPrivateEncrypt (unsigned char *output,         /* output block */
                          unsigned int *outputLen,        /* length of output block */
                          unsigned char *input,           /* input block */
                          unsigned int inputLen,          /* length of input block */
                          R_RSA_PRIVATE_KEY *privateKey)  /* RSA private key */
{
    unsigned int modulusLen;
    rsa_context rsa;
    int mpi_buf[1024*16];
    
    modulusLen = (privateKey->bits+ 7) / 8;
    if (inputLen > modulusLen)
    return (-1);

    mpi_buf_init(mpi_buf,sizeof(mpi_buf));
    memset( &rsa, 0, sizeof( rsa_context ) );
    rsa.len = modulusLen;
    mpi_read_binary( &rsa.N, privateKey->modulus, modulusLen);
    mpi_read_binary( &rsa.D, privateKey->exponent, modulusLen);
    mpi_read_binary( &rsa.P, privateKey->prime[0], modulusLen/2);
    mpi_read_binary( &rsa.Q, privateKey->prime[1], modulusLen/2);
    mpi_read_binary( &rsa.DP, privateKey->primeExponent[0], modulusLen/2);
    mpi_read_binary( &rsa.DQ, privateKey->primeExponent[1], modulusLen/2);
    mpi_read_binary( &rsa.QP, privateKey->coefficient, modulusLen);
    if( rsa_pkcs1_encrypt( &rsa, RSA_PRIVATE, modulusLen,input, output ) != 0 ) return -1;
     *outputLen = modulusLen;

    return 0;
}

/* Raw RSA public-key operation. Output has same length as modulus.

   Assumes inputLen < length of modulus.
   Requires input < modulus.
 */
static int RSAPublicBlock (output, outputLen, input, inputLen, publicKey)
unsigned char *output;                                      /* output block */
unsigned int *outputLen;                          /* length of output block */
unsigned char *input;                                        /* input block */
unsigned int inputLen;                             /* length of input block */
R_RSA_PUBLIC_KEY *publicKey;                              /* RSA public key */
{
  MY_NN_DIGIT c[MAX_MY_NN_DIGITS], e[MAX_MY_NN_DIGITS], m[MAX_MY_NN_DIGITS],
    n[MAX_MY_NN_DIGITS];
  unsigned int eDigits, nDigits;

  // ��ƽ�� 2007��6��22���޸�
  // �޸�Ŀ��:Ϊ����Ӧ��ͬģ����RSA��˽Կ�ԣ�����˽Կ��
  // ��ʹ��ǰ����Ҫ�������䵽256�ֽڡ�

  MY_NN_Decode (m, MAX_MY_NN_DIGITS, input, inputLen);
  MY_NN_Decode (n, MAX_MY_NN_DIGITS, publicKey->aucModulus, MAX_RSA_MODULUS_LEN);
  //MY_NN_Decode (n, MAX_MY_NN_DIGITS, publicKey->modulus, publicKey->bits/8);
  MY_NN_Decode (e, MAX_MY_NN_DIGITS, publicKey->aucExponent, 4);
  nDigits = MY_NN_Digits (n, MAX_MY_NN_DIGITS);
  eDigits = MY_NN_Digits (e, MAX_MY_NN_DIGITS);

  if (MY_NN_Cmp (m, n, nDigits) >= 0)
    return (RE_DATA);

  /* Compute c = m^e mod n.
   */
  MY_NN_ModExp (c, m, e, eDigits, n, nDigits);

  *outputLen = (publicKey->uiBitLen + 7) / 8;
  MY_NN_Encode (output, *outputLen, c, nDigits);

  /* Zeroize sensitive information.
   */
  R_memset ((POINTER)c, 0, sizeof (c));
  R_memset ((POINTER)m, 0, sizeof (m));

  return (0);
}

/* Raw RSA private-key operation. Output has same length as modulus.

   Assumes inputLen < length of modulus.
   Requires input < modulus.
 */
static int RSAPrivateBlock (output, outputLen, input, inputLen, privateKey)
unsigned char *output;                                      /* output block */
unsigned int *outputLen;                          /* length of output block */
unsigned char *input;                                        /* input block */
unsigned int inputLen;                             /* length of input block */
R_RSA_PRIVATE_KEY *privateKey;                           /* RSA private key */
{
  MY_NN_DIGIT c[MAX_MY_NN_DIGITS], cP[MAX_MY_NN_DIGITS], cQ[MAX_MY_NN_DIGITS],
    dP[MAX_MY_NN_DIGITS], dQ[MAX_MY_NN_DIGITS], mP[MAX_MY_NN_DIGITS],
    mQ[MAX_MY_NN_DIGITS], n[MAX_MY_NN_DIGITS], p[MAX_MY_NN_DIGITS], q[MAX_MY_NN_DIGITS],
    qInv[MAX_MY_NN_DIGITS], t[MAX_MY_NN_DIGITS];
  unsigned int cDigits, nDigits, pDigits;

//     MY_NN_Decode (a, digits, b, len)   Decodes character string b into a.

  MY_NN_Decode (c, MAX_MY_NN_DIGITS, input, inputLen);

  // ��ƽ�� 2007��6��22���޸�
  // �޸�Ŀ��:Ϊ����Ӧ��ͬģ����RSA��˽Կ�ԣ�����˽Կ��
  // ��ʹ��ǰ����Ҫ�������䵽256�ֽڡ�

  MY_NN_Decode (n, MAX_MY_NN_DIGITS, privateKey->modulus, MAX_RSA_MODULUS_LEN);
  MY_NN_Decode (p, MAX_MY_NN_DIGITS, privateKey->prime[0], MAX_RSA_PRIME_LEN);
  MY_NN_Decode (q, MAX_MY_NN_DIGITS, privateKey->prime[1], MAX_RSA_PRIME_LEN);
  MY_NN_Decode
    (dP, MAX_MY_NN_DIGITS, privateKey->primeExponent[0], MAX_RSA_PRIME_LEN);
  MY_NN_Decode
    (dQ, MAX_MY_NN_DIGITS, privateKey->primeExponent[1], MAX_RSA_PRIME_LEN);
  MY_NN_Decode (qInv, MAX_MY_NN_DIGITS, privateKey->coefficient, MAX_RSA_PRIME_LEN);

  /*MY_NN_Decode (n, MAX_MY_NN_DIGITS, privateKey->modulus, privateKey->bits/8);
  MY_NN_Decode (p, MAX_MY_NN_DIGITS, privateKey->prime[0], privateKey->bits/16);
  MY_NN_Decode (q, MAX_MY_NN_DIGITS, privateKey->prime[1], privateKey->bits/16);
  MY_NN_Decode
    (dP, MAX_MY_NN_DIGITS, privateKey->primeExponent[0], privateKey->bits/16);
  MY_NN_Decode
    (dQ, MAX_MY_NN_DIGITS, privateKey->primeExponent[1], privateKey->bits/16);
  MY_NN_Decode (qInv, MAX_MY_NN_DIGITS, privateKey->coefficient, privateKey->bits/16);*/

  cDigits = MY_NN_Digits (c, MAX_MY_NN_DIGITS);
  nDigits = MY_NN_Digits (n, MAX_MY_NN_DIGITS);
  pDigits = MY_NN_Digits (p, MAX_MY_NN_DIGITS);

  if (MY_NN_Cmp (c, n, nDigits) >= 0)
    return (RE_DATA);

  /* Compute mP = cP^dP mod p  and  mQ = cQ^dQ mod q. (Assumes q has
     length at most pDigits, i.e., p > q.)
   */
  MY_NN_Mod (cP, c, cDigits, p, pDigits);
  MY_NN_Mod (cQ, c, cDigits, q, pDigits);
  MY_NN_ModExp (mP, cP, dP, pDigits, p, pDigits);
  MY_NN_AssignZero (mQ, nDigits);
  MY_NN_ModExp (mQ, cQ, dQ, pDigits, q, pDigits);

  /* Chinese Remainder Theorem:
       m = ((((mP - mQ) mod p) * qInv) mod p) * q + mQ.
   */
  if (MY_NN_Cmp (mP, mQ, pDigits) >= 0)
    MY_NN_Sub (t, mP, mQ, pDigits);
  else {
    MY_NN_Sub (t, mQ, mP, pDigits);
    MY_NN_Sub (t, p, t, pDigits);
  }
  MY_NN_ModMult (t, t, qInv, p, pDigits);
  MY_NN_Mult (t, t, q, pDigits);
  MY_NN_Add (t, t, mQ, nDigits);

  *outputLen = (privateKey->bits + 7) / 8;
  MY_NN_Encode (output, *outputLen, t, nDigits);

  /* Zeroize sensitive information.
   */
  R_memset ((POINTER)c, 0, sizeof (c));
  R_memset ((POINTER)cP, 0, sizeof (cP));
  R_memset ((POINTER)cQ, 0, sizeof (cQ));
  R_memset ((POINTER)dP, 0, sizeof (dP));
  R_memset ((POINTER)dQ, 0, sizeof (dQ));
  R_memset ((POINTER)mP, 0, sizeof (mP));
  R_memset ((POINTER)mQ, 0, sizeof (mQ));
  R_memset ((POINTER)p, 0, sizeof (p));
  R_memset ((POINTER)q, 0, sizeof (q));
  R_memset ((POINTER)qInv, 0, sizeof (qInv));
  R_memset ((POINTER)t, 0, sizeof (t));

  return (0);
}
/* RSA public-key decryption, according to PKCS #1.
 */
int RSAPublicDecryptA(unsigned char *output,         /* output block */
                         unsigned int *outputLen,       /* length of output block */
                         unsigned char *input,          /* input block */
                         unsigned int inputLen,         /* length of input block */
                         R_RSA_PUBLIC_KEY *publicKey)   /* RSA public key */
{
  int status;
  unsigned char pkcsBlock[MAX_RSA_MODULUS_LEN];
  unsigned int i, modulusLen, pkcsBlockLen;

  modulusLen = (publicKey->uiBitLen+ 7) / 8;
  if (inputLen > modulusLen)
    return (RE_LEN);

  if (status = RSAPublicBlock
      (pkcsBlock, &pkcsBlockLen, input, inputLen, publicKey))
    return (status);

  if (pkcsBlockLen != modulusLen)
    return (RE_LEN);

  i = 0;
  *outputLen = modulusLen - i;

  if (*outputLen > modulusLen)
    return (RE_DATA);

  R_memcpy ((POINTER)output, (POINTER)&pkcsBlock[i], *outputLen);

  /* Zeroize potentially sensitive information.
   */
  R_memset ((POINTER)pkcsBlock, 0, sizeof (pkcsBlock));

  return (0);
}


/* RSA private-key encryption, according to PKCS #1.
 */
int RSAPrivateEncryptA (unsigned char *output,         /* output block */
                          unsigned int *outputLen,        /* length of output block */
                          unsigned char *input,           /* input block */
                          unsigned int inputLen,          /* length of input block */
                          R_RSA_PRIVATE_KEY *privateKey)  /* RSA private key */
{
  int status;
  unsigned char pkcsBlock[MAX_RSA_MODULUS_LEN];
  unsigned int i, modulusLen;

  modulusLen = (privateKey->bits + 7) / 8;

  if (inputLen  > modulusLen)
    return (RE_LEN);

  i = 0;
  R_memcpy (pkcsBlock+i, (POINTER)input, inputLen);

  status = RSAPrivateBlock
    (output, outputLen, pkcsBlock, modulusLen, privateKey);

  /* Zeroize potentially sensitive information.
   */
  R_memset ((POINTER)pkcsBlock, 0, sizeof (pkcsBlock));

  return (status);
}

/**
  Description:  	RSA�ӽ������㺯����
			�ú�������RSA���ܻ�������㣬���ܻ����ͨ��ѡ�ò�ͬ����Կʵ�֡�
			��(pbyModule,pbyExp)ѡ��˽����Կ������м��ܣ���ѡ�ù�����Կ��
			����н��ܡ������������������Ϊ��λ��ǰ����BYTE(8 bits)Ϊ��λ��
			�ú�����ʵ�ֳ��Ȳ�����2048 bits ��RSA���㡣
 @param[in]
  			BYTE *pbyModule : ���RSA�����ģ������ָ�루����n=p*q����
							����λ��ǰ����λ�ں��˳��洢
 @param[in]	uint dwModuleLen : ģ���ȣ���BYTEΪ��λ��ȡֵ��Χ1��512����
 @param[in]	BYTE *pbyExp : ���RSA�����ָ��������ָ�롣����e������λ��
                   					ǰ����λ�ں��˳��洢
 @param[in] 	uint dwExpLen : ָ�����ȣ���BYTEΪ��λ��ȡֵ��Χ��1��256��
                   				������dwExpLen<=dwModuleLen����  
 @param[in]	BYTE *pbyDataIn : �������ݻ�����ָ�룬����ΪdwModuleLen��                   				
 @param[out]  BYTE *pbyDataOut : ������ݻ�����ָ�룬����ΪdwModuleLen

 @retval		  0:�ɹ�
 @retval		  -1��ʾ��������������������ָ��Ϊ�գ�����
 @retval		  -2��ʾ�����ģ�ĳ���dwModuleLen����ָ������dwExpLenΪ0������
 @retval		  -3��ʾģ�ĳ���dwModuleLen����ָ���ĳ���dwExpLen���󣬴���
 @retval		  -4��ʾpbyDataIn���ڵ���pbyModule������
 @retval		  -5��ʾdwExpLen����dwModuleLen������
  @remarks
	 \li ����-----------------ʱ��--------�汾---------�޸�˵�� 				   
	 \li PengRongshou---07/04/23--1.0.0.0-------����
 */
int RSARecover(unsigned char *pbyModule, unsigned int uiModuleLen, unsigned char *pbyExp,
                unsigned int uiExpLen, unsigned char *pbyDataIn, unsigned char *pbyDataOut)
{
    rsa_context rsa;
    int mode;
    int mpi_buf[1024*32];

    if (   (NULL==pbyModule)
        || (NULL==pbyExp)
        || (NULL==pbyDataIn)
        || (NULL==pbyDataOut)  )
    {
        return -1;
    }
	
    if ((0==uiModuleLen) || (0==uiExpLen))
    {
        return -2;
    }
	
    if ((uiModuleLen>MAX_RSA_RECOVER_LEN) || (uiExpLen>MAX_RSA_RECOVER_LEN))
    {
        return -3;
    }

    if (memcmp(pbyDataIn,pbyModule,uiModuleLen)>=0)
    {
        return -4;
    }
    if (uiExpLen > uiModuleLen)
    {
        return -5;
    }
    mpi_buf_init(mpi_buf,sizeof(mpi_buf));
    memset( &rsa, 0, sizeof( rsa_context ) );
    rsa.len = uiModuleLen;
    mpi_read_binary(&rsa.N,pbyModule,uiModuleLen);
    if((uiExpLen == 3) || (uiExpLen == 4))  
    {
        mpi_read_binary(&rsa.E,pbyExp,uiExpLen);
        mode = RSA_PUBLIC;
    }
    else 
    {
        mpi_read_binary(&rsa.D,pbyExp,uiExpLen);
        mode = RSA_PRIVATE;
    }

    if(rsa_pkcs1_encrypt( &rsa, mode, rsa.len,pbyDataIn, pbyDataOut) != 0 ) return -1;

    return 0;
}

