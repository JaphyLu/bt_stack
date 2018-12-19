#ifndef _FATAPI_H_
#define _FATAPI_H_

/*
** ������
**/
#define FS_OK                0/* no error, everything OK! */
#define FS_ERR_NOINIT        -1/*host not initialized,added on 091118*/
#define FS_ERR_DEVPORT_OCCUPIED  -2/*device port is attached to a host,added on 091118*/
#define FS_ERR_PORT_NOTOPEN   -3/* T_UDISK or T_UDISK_FAST port failed to open*/
#define FS_ERR_DEV_REMOVED   -4/* device is removed during communication*/
#define FS_ERR_NODEV         -5 /* ���������豸������*/
#define FS_ERR_BUSY          -6 /* �ڵ㲻��ͬʱ������*/
#define FS_ERR_ARG           -7/* �������*/
#define FS_ERR_EOF           -8 /* �ѵ�β��*/
#define FS_ERR_NAME_SIZE     -9/* ����̫��*/
#define FS_ERR_NAME_EXIST    -10/* �����Ѵ���*/
#define FS_ERR_CHAR          -11/* ���ڷǷ��ַ�*/
#define FS_ERR_NAME_SPACE    -12/* short name NUM is full*/
#define FS_ERR_DISK_FULL     -13/* DISK SPACE is full */
#define FS_ERR_ROOT_FULL     -14/* root space is full */
#define FS_ERR_CACHE_FULL    -15/* cache full, please submit */
#define FS_ERR_PATH          -16/* path depth */
#define FS_ERR_NOENT         -17/* no such inode */
#define FS_ERR_NOTDIR        -18/* not dir */
#define FS_ERR_ISDIR         -19/* is dir */
#define FS_ERR_NOMEM         -20/* NO MEMORY */
#define FS_ERR_BADFD         -21/* BAD file descriptor */
#define FS_ERR_NOTNULL       -22/* dir not null */
#define FS_ERR_WO            -23/* write only */
#define FS_ERR_RO            -24/* read only */
#define FS_ERR_SYS           -25/* sys not support */
#define FS_ERR_CLUSTER       -26/* cluster chain destroy */
#define FS_ERR_TIMEOUT       -27/* timeout */
#define FS_ERR_HW            -28/* hardware error */
#define FAT_ERROR_MAX        -60

/*
** ·����󳤶�
**/
#define PATH_NAME_MAX (2*1024)
/*
** �����ַ�������궨��
**/
#define NAME_FMT_UNICODE 0x1
#define NAME_FMT_ASCII 0x2

/*
** �����Ͷ�������󳤶�
**/
#define LONG_NAME_MAX (200*2)
#define SHORT_NAME_MAX (11+1)

#define ATTR_RO 0x01  /* read only */ 
#define ATTR_HID 0x02 /* hidden */
#define ATTR_SYS 0x04 /* system file */
#define ATTR_VOL 0x08 /* volume ID */
#define ATTR_DIR 0x10 /* directory */
#define ATTR_ARC 0x20 /* archive */
#define ATTR_LONG_NAME 0x0F

typedef struct fs_date_time_s
{
	long seconds;
	long minute;
	long hour;
	long day;
	long month;
	long year;
}FS_DATE_TIME;

typedef struct
{
	FS_DATE_TIME    wrtime;/* lastly write time */
	FS_DATE_TIME    crtime;/* create time */
	int             size;/* file size */
	int             space;/* space size */
	int             name_fmt;
	int             attr;/* ����*/
	int             name_size;/* name����Ч���� */
	char            name[LONG_NAME_MAX+4];
}FS_INODE_INFO;

/*
** ���ַ����ṹ
**/
typedef struct
{
	char *str;
	int  size;
	int  fmt;
}FS_W_STR;

/*
** Seek flag�궨��
**/
typedef enum {
	FS_SEEK_SET=1,
	FS_SEEK_CUR,
	FS_SEEK_END,
}FS_SEEK_FLAG;

/*
** fs operation attr
**/
#define FS_ATTR_C 0x1 /* Create if not exist*/
#define FS_ATTR_D 0x2 /* inode is DIR */
#define FS_ATTR_E 0x4/* exclusive */
#define FS_ATTR_R 0x8 /* Read */
#define FS_ATTR_W 0x10 /* Write */

#define FS_ATTR_CRWD (FS_ATTR_C|FS_ATTR_R|FS_ATTR_W|FS_ATTR_D)
#define FS_ATTR_RW   (FS_ATTR_R|FS_ATTR_W)
#define FS_ATTR_RWD  (FS_ATTR_R|FS_ATTR_W|FS_ATTR_D)
#define FS_ATTR_CRW (FS_ATTR_C|FS_ATTR_R|FS_ATTR_W)

/*************************************************
**
**                    File System API
**
**************************************************/
int FsOpen(FS_W_STR *name, int attr);
int FsClose(int fd);
int FsDelete(FS_W_STR *name);
int FsGetInfo(int fd, FS_INODE_INFO *fs_inode);
int FsRename(FS_W_STR *name, FS_W_STR *new_name);

int FsDirSeek(int fd, int num, int flag);
int FsDirRead(int fd, FS_INODE_INFO *fs_inode, int num);

int FsFileRead(int fd, char *buf, int len);
int FsFileWrite(int fd, char *buf, int len);
int FsFileSeek(int fd, int offset, int flag);
int FsFileTell(int fd);

/*
** FsFileTruncate: �ض��ļ����ͷſ��пռ�
** fd: file decriptor
** size: �ض̺��ļ���С;
** reserve_space: �������пռ��С;
** 
**/
int FsFileTruncate(int fd, int size, int reserve_space);

/*
** FsSetWorkDir: Set Work Directory
** FsGetWorkDir: Get Work Directory
**/
int FsSetWorkDir(FS_W_STR *name);
int FsGetWorkDir(FS_W_STR *name);

/*
** �洢���ʺ궨��
**/
#define FS_UDISK 0x0 /* usb disk */
#define FS_FLASH 0x1 /* nand flash */
#define FS_SDCARD 0x2 /* sd card */
#define FS_UDISK_A 0x3//udisk A
#define FS_MAX_NUMS 4

/*
** disk_num == 0 is udisk
** disk_num == 1 is nand flash
** disk_num == 2 is sd card
** diak_num == 3 is udiska
**/
int FsFormat(int disk_num, void (*cb)(int rate));

//u���Ƿ����
int FsUdiskIn(void);

/*
** �洢������Ϣ
**/
#define FS_VER_FAT16 0x0
#define FS_VER_FAT32 0x1
typedef struct
{
	int ver;/* �汾��:FS_VER_FAT16 or FS_VER_FAT32 */
	int free_space;/* ���пռ�,��λ�ֽ�*/
	int used_space;/* ���ÿռ�,��λ�ֽ�*/
}FS_DISK_INFO;

int FsGetDiskInfo_std(int disk_num, FS_DISK_INFO *disk_info);

#endif/* _FATAPI_H_ */
