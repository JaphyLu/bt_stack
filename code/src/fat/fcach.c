/*
** ʵ��cache����
** 1.����ʱ���ȴ�cache�ж�
** 2.��дʱ���Ȼ�����cache�У�������commit�ӿ�
** 3.cache�����ܱ���һ��transaction����Ϣ���Ӷ��ܹ�rollback
*************************************************************
** 2008-01-17 sunjh
�޸���cache���ƣ�
1.ֻcache FAT�����ݣ�
2.дsector����д��disk��
**/
#include "fat.h"
#include "fcach.h"

#define FAT_CACHE 1

int fat_cache_hit = 0, fat_cache_not_hit = 0;/* ͳ����Ϣ*/
static u8 fat_cache[FAT_DISK_NUM][4096];
static FAT_SECTOR_CACHE fat_cache_arr[FAT_DISK_NUM]=
{
	{0,0,0,fat_cache[0]},
	{0,0,0,fat_cache[1]},
	{0,0,0,fat_cache[2]},
    {0,0,0,fat_cache[3]}
};
static FAT_SECTOR_CACHE *fat_cache_swap[FAT_DISK_NUM]=/* ������*/
{
	fat_cache_arr+0,
	fat_cache_arr+1,
	fat_cache_arr+2,
	fat_cache_arr+3
};

#if FAT_CACHE
#define UDISK_FAT_CACHE_SIZE 0///2*128*1024/* UDISK FAT��cache buffer size*/
#define NAND_FAT_CACHE_SIZE  0/* nand flash FAT��cache buffer size*/
#define SDCARD_FAT_CACHE_SIZE 0///2*128*1024/* UDISK FAT��cache buffer size*/

static u8 udisk_fat_cache_buff[UDISK_FAT_CACHE_SIZE];
static u8 udisk_a_fat_cache_buff[UDISK_FAT_CACHE_SIZE];
static u8 nand_fat_cache_buff[NAND_FAT_CACHE_SIZE+1];
static u8 sdcard_fat_cache_buff[SDCARD_FAT_CACHE_SIZE];
static FAT_SECTOR_CACHE udisk_fat_cache_slot[UDISK_FAT_CACHE_SIZE/512+1];
static FAT_SECTOR_CACHE udisk_a_fat_cache_slot[UDISK_FAT_CACHE_SIZE/512+1];
static FAT_SECTOR_CACHE nand_fat_cache_slot[NAND_FAT_CACHE_SIZE/512+1];
static FAT_SECTOR_CACHE sdcard_fat_cache_slot[SDCARD_FAT_CACHE_SIZE/512+1];
static FAT_SECTOR_CACHE *fat_cache_head[FAT_DISK_NUM];
static int fat_cache_num[FAT_DISK_NUM];/* ������*/
static int fat_cache_init_flag[FAT_DISK_NUM]={-1,-1,-1,-1};/* -1 not init, 0 ok, 1 can not cache */
static u32 fat_cache_update[FAT_DISK_NUM];/* cache update time lastly*/

static void fat_cache_init(DRIVE_INFO *drv, u8 *buf, int buf_size,
				FAT_SECTOR_CACHE *slot, int slot_max_num)
{
	int slot_num, i;
	if(fat_cache_init_flag[drv->bDevice]!=-1)
	{
		return ;
	}
	slot_num = buf_size/drv->wSectorSize;
	if(slot_num<=0||buf_size<=0)
	{
		fat_cache_init_flag[drv->bDevice] = 1;
		return;
	}
	memset(slot, 0, sizeof(FAT_SECTOR_CACHE)*slot_max_num);
	fat_cache_head[drv->bDevice] = slot;
	for(i=0; i<slot_num; i++)
	{
		slot->sector = FAT_BAD_NO;
		slot->cache = buf;
		buf += drv->wSectorSize;
		slot++;
	}
	fat_cache_num[drv->bDevice] = slot_num;
	fat_cache_init_flag[drv->bDevice] = 0;
	fat_cache_update[drv->bDevice] = s_Get10MsTimerCount();//inet_jiffier;
	return ;
}

int fat_cache_lookup(DRIVE_INFO *drv, u32 sector, 
		FAT_SECTOR_CACHE **result
		)
{
	int i, found_idx, free_idx, ret, clear;
	u32 counter = (u32)-1;
	int num, free;
	FAT_SECTOR_CACHE *slot;


	if(fat_cache_init_flag[drv->bDevice] == -1)
	{
		if(drv->bDevice==FS_UDISK)
		{
			fat_cache_init(drv, udisk_fat_cache_buff, UDISK_FAT_CACHE_SIZE,
				udisk_fat_cache_slot, UDISK_FAT_CACHE_SIZE/512);
		}
		else if(drv->bDevice==FS_UDISK_A)
		{
			fat_cache_init(drv, udisk_a_fat_cache_buff, UDISK_FAT_CACHE_SIZE,
				udisk_a_fat_cache_slot, UDISK_FAT_CACHE_SIZE/512);
		}
        else if(drv->bDevice == FS_FLASH)
		{
			fat_cache_init(drv, nand_fat_cache_buff, NAND_FAT_CACHE_SIZE,
				nand_fat_cache_slot, NAND_FAT_CACHE_SIZE/512);
		}
        else if(drv->bDevice == FS_SDCARD)
		{
			fat_cache_init(drv, sdcard_fat_cache_buff, SDCARD_FAT_CACHE_SIZE,
				sdcard_fat_cache_slot, SDCARD_FAT_CACHE_SIZE/512);			
		}else
		{
			assert(0);
		}
	}

	if(fat_cache_init_flag[drv->bDevice] != 0||
		drv->bCacheEnable==0||/* ���жϱ�����������ж�֮ǰ*/
		(sector<drv->dwFAT1StartSector||sector>=drv->dwFAT2StartSector)
	)
	{
		*result = NULL;
		return FS_OK;
	}
	clear = 0;
	if(/*inet_jiffier*/s_Get10MsTimerCount()-fat_cache_update[drv->bDevice]>3000)
	{
		clear = 1;
		fat_cache_update[drv->bDevice] = s_Get10MsTimerCount();//inet_jiffier;
	}

	num = fat_cache_num[drv->bDevice];
	free = 0;
	found_idx = -1;
	free_idx = -1;
	slot = fat_cache_head[drv->bDevice];
	for(i=0; i<num; i++)
	{
		if(slot[i].sector == sector)
		{
			found_idx = i;
		}if(free==0)
		{
			if(slot[i].sector == FAT_BAD_NO)
			{
				free_idx = i;
				counter = 0;
				free = 1;
			}else if(slot[i].counter < counter)
			{
				free_idx = i;
				counter = slot[i].counter;
			}
		}
		if(clear == 1)
		{
			slot[i].counter = 0;//clear counter
		}
	}
	if(found_idx>=0)
	{
		fat_cache_hit++;
		*result = slot+found_idx;
		return FS_OK;
	}
	fat_cache_not_hit++;
	assert(free_idx>=0);
	slot[free_idx].sector = FAT_BAD_NO;
	slot[free_idx].counter = 0;
	*result = slot+free_idx;
	return FS_OK;
}
#else
int fat_cache_lookup(DRIVE_INFO *drv, u32 sector, 
		FAT_SECTOR_CACHE **result
		)
{
	*result = NULL;
	return FS_OK;
}
#endif

int fat_read(DRIVE_INFO *drv, u32 start_sector, int sector_num, FAT_SECTOR_CACHE **p)
{
	int ret;	
	FAT_SECTOR_CACHE *slot, *cache;

	assert(drv->bDevice<FS_MAX_NUMS);
	slot = fat_buf_get(drv);
	if(start_sector>=drv->dwSectorTotal)
	{
		return FS_ERR_ARG;
	}
	cache = NULL;
	ret = fat_cache_lookup(drv, start_sector, &cache);
	if(ret != FS_OK)
		return ret;
	if(cache == NULL)
	{
		ret = drv->ops->read(drv, start_sector, 1, slot->cache,
				drv->wSectorSize);
		if(ret != FS_OK)
		{
			return ret;
		}
		*p = slot;
		return FS_OK;
	}
	if(cache->sector != start_sector)
	{
		ret = drv->ops->read(drv, start_sector, 1, cache->cache,
				drv->wSectorSize);
		if(ret != FS_OK)
		{
			return ret;
		}
		cache->sector = start_sector;
	}
	cache->counter++;
	memcpy(slot->cache, cache->cache, drv->wSectorSize);
	*p = slot;
	return FS_OK;
}

int fat_write(DRIVE_INFO *drv, int start_sector, int sector_num, FAT_SECTOR_CACHE *fat_buf)
{
	int err;
	FAT_SECTOR_CACHE *cache;

	assert(drv->bDevice<FS_MAX_NUMS);
	if((u32)start_sector>=drv->dwSectorTotal)
	{
		return FS_ERR_ARG;
	}
	err = fat_cache_lookup(drv, start_sector, &cache);
	if(err)
		return err;	
	err = drv->ops->write(drv, start_sector, 1, 
		fat_buf->cache, drv->wSectorSize);
	if(err)
	{
		if(cache)
		{
			cache->sector = FAT_BAD_NO;//invalidate data
		}
	}else
	{
		if(cache)
		{
			memcpy(cache->cache, fat_buf->cache, drv->wSectorSize);
			cache->counter++;
			cache->sector = start_sector;
		}		
	}
	return err;
}

int fat_write_submit(DRIVE_INFO *drv, int err)
{
	if(drv->ops->w_submit)
		drv->ops->w_submit(drv);	

	return FS_OK;
}

FAT_SECTOR_CACHE *fat_buf_get(DRIVE_INFO *drv)
{
	return fat_cache_swap[drv->bDevice];
}

void fat_cache_destroy(DRIVE_INFO *drv)
{
	#if FAT_CACHE
	fat_cache_init_flag[drv->bDevice] = -1;
	#endif
}

