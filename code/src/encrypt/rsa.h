#ifndef _RSA_H_
#define _RSA_H_

/* RSA.H - header file for RSA.C
 */

/* Copyright (C) RSA Laboratories, a division of RSA Data Security,
     Inc., created 1991. All rights reserved.
 */

 /* RSA key lengths.
 */
#define MIN_RSA_MODULUS_BITS 508
/* #define MAX_RSA_MODULUS_BITS 1024 ** linq modify ->>2048 ***/
#define MAX_RSA_MODULUS_BITS 2048
#define MAX_RSA_MODULUS_LEN ((MAX_RSA_MODULUS_BITS + 7) / 8)
#define MAX_RSA_PRIME_BITS ((MAX_RSA_MODULUS_BITS + 1) / 2)
#define MAX_RSA_PRIME_LEN ((MAX_RSA_PRIME_BITS + 7) / 8)

#define MAX_RSA_RECOVER_LEN 512				// RSARecover֧��512�ֽ�����
/* Random structure.
 */
typedef struct {
  unsigned int bytesNeeded;
  unsigned char state[16];
  unsigned int outputAvailable;
  unsigned char output[16];
} R_RANDOM_STRUCT;

/* RSA public and private key.
 */
#ifndef R_RSA_PUBLIC_KEY
typedef struct
{
    /*length in bits of modulus, 0��ʾ���ṹ������������Ϣ*/	
    unsigned int    uiBitLen;    
    /*��Կģ��ģ��С��#MAX_RSA_MODULUS_LENʱ��λΪ0*/
    unsigned char   aucModulus[MAX_RSA_MODULUS_LEN];           
    /*public exponent*/
    unsigned char   aucExponent[4];                             
} R_RSA_PUBLIC_KEY;
#endif

typedef struct {
  unsigned int bits;                           /* length in bits of modulus */
  unsigned char modulus[MAX_RSA_MODULUS_LEN];                    /* modulus */
  unsigned char publicExponent[4];     /* public exponent */
  unsigned char exponent[MAX_RSA_MODULUS_LEN];          /* private exponent */
  unsigned char prime[2][MAX_RSA_PRIME_LEN];               /* prime factors */
  unsigned char primeExponent[2][MAX_RSA_PRIME_LEN];   /* exponents for CRT */
  unsigned char coefficient[MAX_RSA_PRIME_LEN];          /* CRT coefficient */
  unsigned short usCRC;                             /*CRC У����*/
} R_RSA_PRIVATE_KEY;

#ifndef ST_RSA_KEY
typedef struct 
{
    int iModulusLen;// the length of modulus bits.
    unsigned char aucModulus[512]; //Modulus, if the  length of the Modulus is less than 512bytes, It is padded with 0x00 on the left.
    int iExponentLen; //the length of exponent bits
    unsigned char aucExponent[512];// Modulus, if the  length of the Exponent is less than 512bytes, It is padded with 0x00 on the left.
    unsigned char aucKeyInfo[128];// RSA key info, defined by application.
}ST_RSA_KEY;
#endif

//typedef struct
//{
//     unsigned int  modlen;          //PIN���ܹ�Կģ����
//     unsigned char mod[256];        //PIN���ܹ�Կģ��,����λǰ��0x00
//     unsigned char exp[4];          //PIN���ܹ�Կָ��,����λǰ��0x00
//     unsigned char iccrandomlen;    //�ӿ���ȡ�õ��������
//     unsigned char iccrandom[8];    //�ӿ���ȡ�õ������
// }RSA_PINKEY;

// Public-Key ���ܲ���
int RSAPublicEncrypt(unsigned char *output,		  /* output block */
						 unsigned int *outputLen,		  /* length of output block */
						 unsigned char *input, 		  /* input block */
						 unsigned int inputLen,		  /* length of input block */
						 R_RSA_PUBLIC_KEY *publicKey,	  /* RSA public key */
						 R_RANDOM_STRUCT *randomStruct);  /* random structure */
// Public-Key���ܲ���
int RSAPublicDecrypt(unsigned char *output,         /* output block */
                         unsigned int *outputLen,       /* length of output block */
                         unsigned char *input,          /* input block */
                         unsigned int inputLen,         /* length of input block */
                         R_RSA_PUBLIC_KEY *publicKey);   /* RSA public key */

// Private-Key���ܲ���
int RSAPrivateEncrypt (unsigned char *output,         /* output block */
                           unsigned int *outputLen,        /* length of output block */
                           unsigned char *input,           /* input block */
                           unsigned int inputLen,          /* length of input block */
                           R_RSA_PRIVATE_KEY *privateKey);  /* RSA private key */

// Private-key���ܲ���
int RSAPrivateDecrypt (unsigned char *output,       /* output block */
                           unsigned int *outputLen,     /* length of output block */
                           unsigned char *input,        /* input block */
                           unsigned int inputLen,        /* length of input block */
                           R_RSA_PRIVATE_KEY *privateKey);  /* RSA private key */

#endif


