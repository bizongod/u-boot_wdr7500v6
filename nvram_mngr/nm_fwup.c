/*! Copyright(c) 1996-2009 Shenzhen TP-LINK Technologies Co. Ltd.
 * \file    nm_fwup.c
 * \brief   Implements for upgrade firmware to NVRAM.
 * \author  Meng Qing
 * \version 1.0
 * \date    21/05/2009
 */


/**************************************************************************************************/
/*                                      CONFIGURATIONS                                            */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                      INCLUDE_FILES                                             */
/**************************************************************************************************/
#include <common.h>
#include "nm_lib.h"
#include "nm_fwup.h"
#include "nm_api.h"

#include "sysProductInfo.h"

#include "md5.h"

/**************************************************************************************************/
/*                                      DEFINES                                                   */
/**************************************************************************************************/
/* Porting memory managing utils. */
extern void *malloc(unsigned int size);
extern void free(void *src);
#define fflush(stdout) 

/**************************************************************************************************/
/*                                      TYPES                                                     */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                      EXTERN_PROTOTYPES                                         */
/**************************************************************************************************/
STATUS nm_initFwupPtnStruct(void);
STATUS nm_getDataFromFwupFile(NM_PTN_STRUCT *ptnStruct, char *fwupPtnIndex, char *fwupFileBase);
STATUS nm_getDataFromNvram(NM_PTN_STRUCT *ptnStruct, NM_PTN_STRUCT *runtimePtnStruct);
STATUS nm_updateDataToNvram(NM_PTN_STRUCT *ptnStruct);
STATUS nm_updateRuntimePtnTable(NM_PTN_STRUCT *ptnStruct, NM_PTN_STRUCT *runtimePtnStruct);
static int nm_checkSupportList(char *support_list, int len);
STATUS nm_checkUpdateContent(NM_PTN_STRUCT *ptnStruct, char *pAppBuf, int nFileBytes, int *errorCode);
STATUS nm_cleanupPtnContentCache(void);
int nm_buildUpgradeStruct(char *pAppBuf, int nFileBytes);
STATUS nm_upgradeFwupFile(char *pAppBuf, int nFileBytes);


/**************************************************************************************************/
/*                                      LOCAL_PROTOTYPES                                          */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                      VARIABLES                                                 */
/**************************************************************************************************/

NM_STR_MAP nm_fwupPtnIndexFileParaStrMap[] =
{
    {NM_FWUP_PTN_INDEX_PARA_ID_NAME,    "fwup-ptn"},
    {NM_FWUP_PTN_INDEX_PARA_ID_BASE,    "base"},
    {NM_FWUP_PTN_INDEX_PARA_ID_SIZE,    "size"},

    {-1,                                NULL}
};

static unsigned char md5Key[IMAGE_SIZE_MD5] = 
{
	0x7a, 0x2b, 0x15, 0xed,  0x9b, 0x98, 0x59, 0x6d,
	0xe5, 0x04, 0xab, 0x44,  0xac, 0x2a, 0x9f, 0x4e
};

NM_PTN_STRUCT *g_nmFwupPtnStruct;
NM_PTN_STRUCT g_nmFwupPtnStructEntity;
int g_nmCountFwupCurrWriteBytes;
int g_nmCountFwupAllWriteBytes;

STATUS g_nmUpgradeResult;


char *ptnContentCache[NM_PTN_NUM_MAX];

/**************************************************************************************************/
/*                                      LOCAL_FUNCTIONS                                           */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                      PUBLIC_FUNCTIONS                                          */
/**************************************************************************************************/

/*******************************************************************
 * Name		: nm_tpFirmwareMd5Check
 * Abstract	: firmware md5 value check.
 * Input	: 
 * Output	: 
 * Return	: OK/ERROR
 */
STATUS nm_tpFirmwareMd5Check(unsigned char *ptr,int bufsize)
{
	unsigned char fileMd5Checksum[IMAGE_SIZE_MD5];
	unsigned char digst[IMAGE_SIZE_MD5];
	MD5_CTX ctx;
	
	memcpy(fileMd5Checksum, ptr + IMAGE_SIZE_LEN, IMAGE_SIZE_MD5);
	memcpy(ptr + IMAGE_SIZE_LEN, md5Key, IMAGE_SIZE_MD5);

	MD5Init(&ctx);
	MD5Update(&ctx, ptr + IMAGE_SIZE_LEN, bufsize - IMAGE_SIZE_LEN);
	MD5Final(digst, &ctx);

	if (0 != memcmp(digst, fileMd5Checksum, IMAGE_SIZE_MD5))
	{
		NM_ERROR("Check md5 error.\n");
		return -1;
	}

	memcpy(ptr + IMAGE_SIZE_LEN, fileMd5Checksum, IMAGE_SIZE_MD5);

	return 0;
}


/*******************************************************************
 * Name		: nm_initFwupPtnStruct
 * Abstract	: Initialize partition-struct.
 * Input	: 
 * Output	: 
 * Return	: OK/ERROR
 */
STATUS nm_initFwupPtnStruct()
{
    int index = 0;
    
    memset(&g_nmFwupPtnStructEntity, 0, sizeof(g_nmFwupPtnStructEntity));
    g_nmFwupPtnStruct = &g_nmFwupPtnStructEntity;


    for (index=0; index<NM_PTN_NUM_MAX; index++)
    {           
        if (ptnContentCache[index] != NULL)
        {
            ptnContentCache[index] = NULL;
        }   
    }
    
    return OK;
}


/*******************************************************************
 * Name		: nm_getDataFromFwupFile
 * Abstract	: 
 * Input	: fwupFileBase: start addr of FwupPtnTable
 * Output	: 
 * Return	: OK/ERROR.
 */
STATUS nm_getDataFromFwupFile(NM_PTN_STRUCT *ptnStruct, char *fwupPtnIndex, char *fwupFileBase)
{   
    int index = 0;
    int paraId = -1;
    int argc;
    char *argv[NM_FWUP_PTN_INDEX_ARG_NUM_MAX];
    NM_PTN_ENTRY *currPtnEntry = NULL;

    argc = nm_lib_makeArgs(fwupPtnIndex, argv, NM_FWUP_PTN_INDEX_ARG_NUM_MAX);
    
    while (index < argc)
    {
        if ((paraId = nm_lib_strToKey(nm_fwupPtnIndexFileParaStrMap, argv[index])) < 0)
        {
            NM_ERROR("invalid partition-index-file para id.\r\n");
            goto error;
        }

        index++;

        switch (paraId)
        {
        case NM_FWUP_PTN_INDEX_PARA_ID_NAME:
            /* we only update upgrade-info to partitions exist in partition-table */
            currPtnEntry = nm_lib_ptnNameToEntry(ptnStruct, argv[index]);

            if (currPtnEntry == NULL)
            {
                NM_DEBUG("partition name not found.");
                continue;           
            }

            if (currPtnEntry->upgradeInfo.dataType == NM_FWUP_UPGRADE_DATA_TYPE_BLANK)
            {
                currPtnEntry->upgradeInfo.dataType = NM_FWUP_UPGRADE_DATA_FROM_FWUP_FILE;
            }
            index++;
            break;
            
        case NM_FWUP_PTN_INDEX_PARA_ID_BASE:
            /* get data-offset in fwupFile */
            if (nm_lib_parseU32((NM_UINT32 *)&currPtnEntry->upgradeInfo.dataStart, argv[index]) < 0)
            {
                NM_ERROR("parse upgradeInfo start value failed.");
                goto error;
            }
            
            currPtnEntry->upgradeInfo.dataStart += (unsigned int)fwupFileBase;
            index++;
            break;

        case NM_FWUP_PTN_INDEX_PARA_ID_SIZE:
            if (nm_lib_parseU32((NM_UINT32 *)&currPtnEntry->upgradeInfo.dataLen, argv[index]) < 0)
            {
                NM_ERROR("parse upgradeInfo len value failed.");
                goto error;
            }
            index++;
            break;

        default:
            NM_ERROR("invalid para id.");
            goto error;
            break;
        }
        
    }

    /* force get partition-table from fwup-file */
    currPtnEntry = nm_lib_ptnNameToEntry(ptnStruct, NM_PTN_NAME_PTN_TABLE); 
    if (currPtnEntry == NULL)
    {
        NM_ERROR("no partition-table in fwup-file.\r\n");
        goto error; 
    }

    currPtnEntry->upgradeInfo.dataType = NM_FWUP_UPGRADE_DATA_FROM_FWUP_FILE;
    currPtnEntry->upgradeInfo.dataStart = (unsigned int)fwupFileBase + NM_FWUP_PTN_INDEX_SIZE;
    /* length of partition-table is "probe to os-image"(4 bytes) and ptn-index-file(string) */
    currPtnEntry->upgradeInfo.dataLen = sizeof(int) + strlen((char*)(currPtnEntry->upgradeInfo.dataStart + sizeof(int)));
    
    return OK;
error:
    return ERROR;
}



/*******************************************************************
 * Name		: nm_getDataFromNvram
 * Abstract	: 
 * Input	: 
 * Output	: 
 * Return	: OK/ERROR.
 */
STATUS nm_getDataFromNvram(NM_PTN_STRUCT *ptnStruct, NM_PTN_STRUCT *runtimePtnStruct)
{   
    int index = 0;
    NM_PTN_ENTRY *currPtnEntry = NULL;
    NM_PTN_ENTRY *tmpPtnEntry = NULL;
    NM_UINT32 readSize = 0;
    
	if (ptnStruct == NULL)
	{
        NM_ERROR("invalid input ptnStruct.");
        goto error;		
	}

    nm_cleanupPtnContentCache();   

    for (index=0; index<NM_PTN_NUM_MAX; index++)
    {       
#if 0		
        currPtnEntry = nm_lib_ptnNameToEntry(ptnStruct, runtimePtnStruct->entries[index].name);

        if (currPtnEntry == NULL)
        {
            continue;           
        }

        if (currPtnEntry->upgradeInfo.dataType == NM_FWUP_UPGRADE_DATA_TYPE_BLANK)
        {
			/* if base not changed, do nothing */
			if (currPtnEntry->base == runtimePtnStruct->entries[index].base)
			{
				currPtnEntry->upgradeInfo.dataType = NM_FWUP_UPGRADE_DATA_TYPE_NO_CHANGE;
				continue;
			}
            /* read content from NVRAM to a memory cache */
            readSize = 0;
            if(currPtnEntry->size <= runtimePtnStruct->entries[index].size)
            {
                readSize = currPtnEntry->size;
            }
            else
            {
                readSize = runtimePtnStruct->entries[index].size;
            }
            //ptnContentCache[index] = malloc(runtimePtnStruct->entries[index].size);
            ptnContentCache[index] = malloc(readSize);

            if (ptnContentCache[index] == NULL)
            {
                NM_ERROR("memory malloc failed.");
                goto error;
            }
            
            //memset(ptnContentCache[index], 0, runtimePtnStruct->entries[index].size);
            memset(ptnContentCache[index], 0, readSize);

            if (nm_lib_readHeadlessPtnFromNvram((char *)runtimePtnStruct->entries[index].base, 
                                                ptnContentCache[index], readSize) < 0)
            {               
                NM_ERROR("get data from NVRAM failed.");
                goto error;
            }

            currPtnEntry->upgradeInfo.dataStart = (unsigned int)ptnContentCache[index];
            currPtnEntry->upgradeInfo.dataLen = readSize;
            currPtnEntry->upgradeInfo.dataType = NM_FWUP_UPGRADE_DATA_FROM_NVRAM;
        }
#else
		tmpPtnEntry = (NM_PTN_ENTRY *)&(ptnStruct->entries[index]);
		if (tmpPtnEntry->upgradeInfo.dataType == NM_FWUP_UPGRADE_DATA_TYPE_BLANK)
		{
			/* if not in nvram */
			currPtnEntry = nm_lib_ptnNameToEntry(runtimePtnStruct, tmpPtnEntry->name);
			if (currPtnEntry == NULL)
			{
				continue;			
			}
		
			/* if base not changed, do nothing */
			if (currPtnEntry->base == tmpPtnEntry->base)
			{
				tmpPtnEntry->upgradeInfo.dataType = NM_FWUP_UPGRADE_DATA_TYPE_NO_CHANGE;
				continue;
			}
			/* read content from NVRAM to a memory cache */
			readSize = 0;
			if(currPtnEntry->size <= tmpPtnEntry->size)
			{
				readSize = currPtnEntry->size;
			}
			else
			{
				readSize = tmpPtnEntry->size;
			}

			ptnContentCache[index] = malloc(readSize);
		
			if (ptnContentCache[index] == NULL)
			{
				NM_ERROR("memory malloc failed.");
				goto error;
			}
			
			memset(ptnContentCache[index], 0, readSize);
		
			if (nm_lib_readHeadlessPtnFromNvram((char *)currPtnEntry->base, 
												ptnContentCache[index], readSize) < 0)
			{				
				NM_ERROR("get data from NVRAM failed.");
				goto error;
			}
		
			tmpPtnEntry->upgradeInfo.dataStart = (unsigned int)ptnContentCache[index];
			tmpPtnEntry->upgradeInfo.dataLen = readSize;
			tmpPtnEntry->upgradeInfo.dataType = NM_FWUP_UPGRADE_DATA_FROM_NVRAM;
		}
#endif
    }

    return OK;
error:
    return ERROR;
}
    


/*******************************************************************
 * Name		: nm_updateDataToNvram
 * Abstract	: write to NARAM
 * Input	: 
 * Output	: 
 * Return	: OK/ERROR.
 */
STATUS nm_updateDataToNvram(NM_PTN_STRUCT *ptnStruct)
{   
    int index = 0;
	int numBlookUpdate = 0;
    NM_PTN_ENTRY *currPtnEntry = NULL;
    int  firstFragmentSize = 0;
    int firstFragment = TRUE;
    unsigned long int fragmentBase = 0;
    int  fragmentDataStart = 0;
    int  fwupDataLen = 0;

    /* clear write bytes counter first */
    g_nmCountFwupAllWriteBytes = 0;

    for (index=0; index<NM_PTN_NUM_MAX; index++)
    {
        currPtnEntry = (NM_PTN_ENTRY *)&(ptnStruct->entries[index]);

        switch (currPtnEntry->upgradeInfo.dataType)
        {
        case NM_FWUP_UPGRADE_DATA_TYPE_BLANK:
            /* if a partition is "blank", means it's a new partition
             * without content, we set content of this partition to all zero */
            if (ptnContentCache[index] != NULL)
            {
                free(ptnContentCache[index]);
                ptnContentCache[index] = NULL;
            }

            ptnContentCache[index] = malloc(currPtnEntry->size);            

            if (ptnContentCache[index] == NULL)
            {
                NM_ERROR("memory malloc failed.");
                goto error;
            }
            
            memset(ptnContentCache[index], 0, currPtnEntry->size);

            currPtnEntry->upgradeInfo.dataStart = (unsigned int)ptnContentCache[index];
            currPtnEntry->upgradeInfo.dataLen = currPtnEntry->size;
            break;
		case NM_FWUP_UPGRADE_DATA_TYPE_NO_CHANGE:
			NM_DEBUG("PTN %s no need to update.", currPtnEntry->name);
			break;

        case NM_FWUP_UPGRADE_DATA_FROM_FWUP_FILE:
        case NM_FWUP_UPGRADE_DATA_FROM_NVRAM:
            /* Do Nothing */
            break;

        default:
            NM_ERROR("invalid upgradeInfo dataType found.");
            goto error;
            break;  
        }
        
    }


    for (index=0; index<NM_PTN_NUM_MAX; index++)
    {
        currPtnEntry = (NM_PTN_ENTRY *)&(ptnStruct->entries[index]);

        if (currPtnEntry->usedFlag != TRUE)
        {
            continue;
        }

        g_nmCountFwupAllWriteBytes += currPtnEntry->upgradeInfo.dataLen;
        
        NM_DEBUG("PTN %02d: dataLen = %08x, g_nmCountFwupAllWriteBytes = %08x", 
                        index+1, currPtnEntry->upgradeInfo.dataLen, g_nmCountFwupAllWriteBytes);
    }

    for (index=0; index<NM_PTN_NUM_MAX; index++)
    {
        currPtnEntry = (NM_PTN_ENTRY *)&(ptnStruct->entries[index]);

        switch (currPtnEntry->upgradeInfo.dataType)
        {
		case NM_FWUP_UPGRADE_DATA_TYPE_NO_CHANGE:
			NM_DEBUG("PTN %s no need to update.\r\n", currPtnEntry->name);
			break;
        case NM_FWUP_UPGRADE_DATA_TYPE_BLANK:       
        case NM_FWUP_UPGRADE_DATA_FROM_FWUP_FILE:   
        case NM_FWUP_UPGRADE_DATA_FROM_NVRAM:
            if (currPtnEntry->usedFlag != TRUE)
            {
                NM_DEBUG("PTN %02d: usedFlag = FALSE", index+1);
                continue;
            }

            NM_DEBUG("PTN %02d: name = %-16s, base = 0x%08x, size = 0x%08x Bytes, upDataType = %d, upDataStart = %08x, upDataLen = %08x",
                index+1, 
                currPtnEntry->name, 
                currPtnEntry->base,
                currPtnEntry->size,
                currPtnEntry->upgradeInfo.dataType,
                currPtnEntry->upgradeInfo.dataStart,
                currPtnEntry->upgradeInfo.dataLen);

			/* 升级过程中将升级文件分成多个分片进行升级, 这样做是为了在升级完
			 * 一个分片后可以统计一次当前已经升级文件长度, 从而使上层模块知道
			 * 当前的升级进度.切分分片时需要注意升级过程中flash的block对齐问题 */
            if (currPtnEntry->upgradeInfo.dataLen > NM_FWUP_FRAGMENT_SIZE)
            {
                fwupDataLen = currPtnEntry->upgradeInfo.dataLen;
                firstFragment = TRUE;
                
                firstFragmentSize = NM_FWUP_FRAGMENT_SIZE - (currPtnEntry->base % NM_FWUP_FRAGMENT_SIZE);
                fragmentBase = 0;
                fragmentDataStart = 0;
                
                while (fwupDataLen > 0)
                {
                    if (firstFragment)
                    {
                        fragmentBase = currPtnEntry->base;
                        fragmentDataStart = currPtnEntry->upgradeInfo.dataStart;
                        
                        NM_DEBUG("PTN f %02d: fragmentBase = %08x, FragmentStart = %08x, FragmentLen = %08x, datalen = %08x", 
                            index+1, fragmentBase, fragmentDataStart, firstFragmentSize, fwupDataLen);

                        if (nm_lib_writeHeadlessPtnToNvram((char *)fragmentBase, 
                                                            (char *)fragmentDataStart,
                                                            firstFragmentSize) < 0)
                        {
                            NM_ERROR("WRITE TO NVRAM FAILED!!!!!!!!.");
                            goto error;
                        }

                        fragmentBase += firstFragmentSize;
                        fragmentDataStart += firstFragmentSize;
                        g_nmCountFwupCurrWriteBytes += firstFragmentSize;
                        fwupDataLen -= firstFragmentSize;
                        NM_DEBUG("PTN f %02d: write bytes = %08x", index+1, g_nmCountFwupCurrWriteBytes);
                        firstFragment = FALSE;
                    }
                    /* last block */
                    else if (fwupDataLen < NM_FWUP_FRAGMENT_SIZE)
                    {
                        NM_DEBUG("PTN l %02d: fragmentBase = %08x, FragmentStart = %08x, FragmentLen = %08x, datalen = %08x", 
                            index+1, fragmentBase, fragmentDataStart, fwupDataLen, fwupDataLen);

                        if (nm_lib_writeHeadlessPtnToNvram((char *)fragmentBase, 
                                                            (char *)fragmentDataStart,
                                                            fwupDataLen) < 0)
                        {
                            NM_ERROR("WRITE TO NVRAM FAILED!!!!!!!!.");
                            goto error;
                        }
						
                        fragmentBase += fwupDataLen;
                        fragmentDataStart += fwupDataLen;
                        g_nmCountFwupCurrWriteBytes += fwupDataLen;
                        fwupDataLen -= fwupDataLen;
                        NM_DEBUG("PTN l %02d: write bytes = %08x", index+1, g_nmCountFwupCurrWriteBytes);
                    }
                    else
                    {
                        NM_DEBUG("PTN n %02d: fragmentBase = %08x, FragmentStart = %08x, FragmentLen = %08x, datalen = %08x", 
                            index+1, fragmentBase, fragmentDataStart, NM_FWUP_FRAGMENT_SIZE, fwupDataLen);
                        
                        if (nm_lib_writeHeadlessPtnToNvram((char *)fragmentBase, 
                                                            (char *)fragmentDataStart,
                                                            NM_FWUP_FRAGMENT_SIZE) < 0)
                        {
                            NM_ERROR("WRITE TO NVRAM FAILED!!!!!!!!.");
                            goto error;
                        }
                   
                        fragmentBase += NM_FWUP_FRAGMENT_SIZE;
                        fragmentDataStart += NM_FWUP_FRAGMENT_SIZE;
                        g_nmCountFwupCurrWriteBytes += NM_FWUP_FRAGMENT_SIZE;
                        fwupDataLen -= NM_FWUP_FRAGMENT_SIZE;
                        NM_DEBUG("PTN n %02d: write bytes = %08x", index+1, g_nmCountFwupCurrWriteBytes);
                    }
					/* add by mengqing, 18Sep09, 由于对单个的FLASH读写操作使用tasklock加锁，升级软件时
					 * 需要在各个文件块之间增加taskdelay人为释放CPU，否则CPU可能被升级模块长期占据导致
					 * web页面无法获得当前的升级进度 */

					if(numBlookUpdate >= 70)
					{
						numBlookUpdate = 0;
						printf("\r\n");
					}
					numBlookUpdate ++;
                    printf("#");
					fflush(stdout);
                }
            }
            else
            {           
                /* we should add head to ptn-table partition */
                if (memcmp(currPtnEntry->name, NM_PTN_NAME_PTN_TABLE, NM_PTN_NAME_LEN) == 0)
                {
                    if (nm_lib_writePtnToNvram((char *)currPtnEntry->base, 
                                                    (char *)currPtnEntry->upgradeInfo.dataStart,
                                                    currPtnEntry->upgradeInfo.dataLen) < 0)

                    {
                        NM_ERROR("WRITE TO NVRAM FAILED!!!!!!!!.");
                        goto error;
                    }
                }
                /* head of other partitions can be found in fwup-file or NVRAM */
                else
                {
                	if (nm_lib_writeHeadlessPtnToNvram((char *)currPtnEntry->base, 
                                                    (char *)currPtnEntry->upgradeInfo.dataStart,
                                                    currPtnEntry->upgradeInfo.dataLen) < 0)                                 
                	{
                   	 	NM_ERROR("WRITE TO NVRAM FAILED!!!!!!!!.");
                    	goto error;
                	}
				}
             
                g_nmCountFwupCurrWriteBytes += currPtnEntry->upgradeInfo.dataLen;
                NM_DEBUG("PTN %02d: write bytes = %08x", index+1, g_nmCountFwupCurrWriteBytes);

				if(numBlookUpdate >= 70)
				{
					numBlookUpdate = 0;
					printf("\r\n");
				}
				numBlookUpdate ++;
				printf("#");
				fflush(stdout);
            }
            break;

        default:
            NM_ERROR("invalid upgradeInfo dataType found.");
            goto error;
            break;  
        }       
    }
	printf("\r\nDone.\r\n");
    return OK;
error:
    return ERROR;
}


/*******************************************************************
 * Name		: nm_updateRuntimePtnTable
 * Abstract	: update the runtimePtnTable.
 * Input	: 
 * Output	: 
 * Return	: OK/ERROR
 */
STATUS nm_updateRuntimePtnTable(NM_PTN_STRUCT *ptnStruct, NM_PTN_STRUCT *runtimePtnStruct)
{   
    int index = 0;
    NM_PTN_ENTRY *currPtnEntry = NULL;
    NM_PTN_ENTRY *currRuntimePtnEntry = NULL;

    for (index=0; index<NM_PTN_NUM_MAX; index++)
    {
        currPtnEntry = (NM_PTN_ENTRY *)&(ptnStruct->entries[index]);
        currRuntimePtnEntry = (NM_PTN_ENTRY *)&(runtimePtnStruct->entries[index]);

        memcpy(currRuntimePtnEntry->name, currPtnEntry->name, NM_PTN_NAME_LEN);
        currRuntimePtnEntry->base = currPtnEntry->base;
        currRuntimePtnEntry->tail = currPtnEntry->tail;
        currRuntimePtnEntry->size = currPtnEntry->size;
        currRuntimePtnEntry->usedFlag = currPtnEntry->usedFlag;
    }   

    return OK;
}

/*******************************************************************
 * Name		: nm_checkSupportList
 * Abstract	: check the supportlist.
 * Input	: 
 * Output	: 
 * Return	: OK/ERROR
 */
STATUS nm_checkSupportList(char *support_list, int len)
{
    int ret = 0;
    
    PRODUCT_INFO_STRUCT *pProductInfo = NULL;

    /* skip partition header */
    len -= 8;
    support_list += 8;
 
    /* check list prefix string */
    if (len < 12 || strncmp(support_list, "SupportList:", 12) != 0)
        return ERROR;

    len -= 12;
    support_list += 12;

    pProductInfo = sysmgr_getProductInfo();
    ret = sysmgr_cfg_checkSupportList(pProductInfo, support_list, len);
    if (0 == ret)
    {
        NM_INFO("Firmwave supports, check OK.\r\n");
        return OK;
    }
    
    NM_INFO("Firmwave not supports, check failed.\r\n");
    return ERROR;
}


/*******************************************************************
 * Name		: nm_checkUpdateContent
 * Abstract	: check the updata content.
 * Input	: 
 * Output	: 
 * Return	: OK/ERROR
 */
STATUS nm_checkUpdateContent(NM_PTN_STRUCT *ptnStruct, char *pAppBuf, int nFileBytes, int *errorCode)
{   
    int index = 0;
    NM_PTN_ENTRY *currPtnEntry = NULL;
    int ptnFound = FALSE;
	int suppportListIndex = 0;
	int softVersionIndex = 0;

    /* check update content */
    for (index=0; index<NM_PTN_NUM_MAX; index++)
    {
        currPtnEntry = (NM_PTN_ENTRY *)&(ptnStruct->entries[index]);

        if (currPtnEntry->upgradeInfo.dataType == NM_FWUP_UPGRADE_DATA_FROM_FWUP_FILE)
        {       
            if ((currPtnEntry->upgradeInfo.dataStart + currPtnEntry->upgradeInfo.dataLen)
                > (unsigned int)(pAppBuf + nFileBytes))
            {
                NM_ERROR("ptn \"%s\": update data end out of fwup-file.", currPtnEntry->name);
                *errorCode = NM_FWUP_ERROR_BAD_FILE;
                goto error;
            }
        }
    }

    /* check important partitions */
    ptnFound = FALSE;
    for (index=0; index<NM_PTN_NUM_MAX; index++)
    {
		currPtnEntry = (NM_PTN_ENTRY *)&(ptnStruct->entries[index]);

       	if (strncmp(currPtnEntry->name, NM_PTN_NAME_FS_UBOOT, NM_PTN_NAME_LEN) == 0)
        {
            ptnFound = TRUE;
            break;
        }
    }
    if (ptnFound == FALSE)
    {               
        NM_ERROR("ptn \"%s\" not found whether in fwup-file or NVRAM.", NM_PTN_NAME_FS_UBOOT);
        *errorCode = NM_FWUP_ERROR_BAD_FILE;
        goto error;
    }


	ptnFound = FALSE;
    for (index=0; index<NM_PTN_NUM_MAX; index++)
    {
        currPtnEntry = (NM_PTN_ENTRY *)&(ptnStruct->entries[index]);
        
        if (strncmp(currPtnEntry->name, NM_PTN_NAME_PTN_TABLE, NM_PTN_NAME_LEN) == 0)
        {
            ptnFound = TRUE;
            break;
        }
    }
    if (ptnFound == FALSE)
    {               
        NM_ERROR("ptn \"%s\" not found whether in fwup-file or NVRAM.", NM_PTN_NAME_PTN_TABLE);
        *errorCode = NM_FWUP_ERROR_BAD_FILE;
        goto error;
    }

    ptnFound = FALSE;
    for (index=0; index<NM_PTN_NUM_MAX; index++)
    {
        currPtnEntry = (NM_PTN_ENTRY *)&(ptnStruct->entries[index]);
        
        if (strncmp(currPtnEntry->name, NM_PTN_NAME_DEFAULT_MAC, NM_PTN_NAME_LEN) == 0)
        {
            ptnFound = TRUE;
            break;
        }
    }
    if (ptnFound == FALSE)
    {               
        NM_ERROR("ptn \"%s\" not found whether in fwup-file or NVRAM.", NM_PTN_NAME_DEFAULT_MAC);
        *errorCode = NM_FWUP_ERROR_BAD_FILE;
        goto error;
    }


	ptnFound = FALSE;
	for (index=0; index<NM_PTN_NUM_MAX; index++)
	{
		currPtnEntry = (NM_PTN_ENTRY *)&(ptnStruct->entries[index]);
		
		if (strncmp(currPtnEntry->name, NM_PTN_NAME_PRODUCT_INFO, NM_PTN_NAME_LEN) == 0)
		{
			ptnFound = TRUE;
			break;
		}
	}
	if (ptnFound == FALSE)
	{				
		NM_ERROR("ptn \"%s\" not found whether in fwup-file or NVRAM.", NM_PTN_NAME_PRODUCT_INFO);
		*errorCode = NM_FWUP_ERROR_BAD_FILE;
		goto error;
	}

	ptnFound = FALSE;
    for (index=0; index<NM_PTN_NUM_MAX; index++)
    {
        currPtnEntry = (NM_PTN_ENTRY *)&(ptnStruct->entries[index]);
        
        if (strncmp(currPtnEntry->name, NM_PTN_NAME_SUPPORT_LIST, NM_PTN_NAME_LEN) == 0)
        {
			suppportListIndex = index;
            ptnFound = TRUE;
            break;
        }
    }
    if (ptnFound == FALSE)
    {               
        NM_ERROR("ptn \"%s\" not found whether in fwup-file or NVRAM.", NM_PTN_NAME_SUPPORT_LIST);
        *errorCode = NM_FWUP_ERROR_BAD_FILE;
        goto error;
    }

	
	ptnFound = FALSE;
    for (index=0; index<NM_PTN_NUM_MAX; index++)
    {
        currPtnEntry = (NM_PTN_ENTRY *)&(ptnStruct->entries[index]);
        
        if (strncmp(currPtnEntry->name, NM_PTN_NAME_SOFT_VERSION, NM_PTN_NAME_LEN) == 0)
        {
			softVersionIndex = index;
            ptnFound = TRUE;
            break;
        }
    }
    if (ptnFound == FALSE)
    {               
        NM_ERROR("ptn \"%s\" not found whether in fwup-file or NVRAM.", NM_PTN_NAME_SOFT_VERSION);
        *errorCode = NM_FWUP_ERROR_BAD_FILE;
        goto error;
    }
	

    ptnFound = FALSE;
    for (index=0; index<NM_PTN_NUM_MAX; index++)
    {
        currPtnEntry = (NM_PTN_ENTRY *)&(ptnStruct->entries[index]);
        
        if (strncmp(currPtnEntry->name, NM_PTN_NAME_OS_IMAGE, NM_PTN_NAME_LEN) == 0)
        {
            ptnFound = TRUE;
            break;
        }
    }
    if (ptnFound == FALSE)
    {               
        NM_ERROR("ptn \"%s\" not found whether in fwup-file or NVRAM.", NM_PTN_NAME_OS_IMAGE);
        *errorCode = NM_FWUP_ERROR_BAD_FILE;
        goto error;
    }


	ptnFound = FALSE;
	for (index=0; index<NM_PTN_NUM_MAX; index++)
	{
		currPtnEntry = (NM_PTN_ENTRY *)&(ptnStruct->entries[index]);
		
		if (strncmp(currPtnEntry->name, NM_PTN_NAME_FILE_SYSTEM, NM_PTN_NAME_LEN) == 0)
		{
			ptnFound = TRUE;
			break;
		}
	}
	if (ptnFound == FALSE)
	{				
		NM_ERROR("ptn \"%s\" not found whether in fwup-file or NVRAM.", NM_PTN_NAME_FILE_SYSTEM);
		*errorCode = NM_FWUP_ERROR_BAD_FILE;
		goto error;
	}

	
	//check the hardware version support list
	currPtnEntry = (NM_PTN_ENTRY *)&(ptnStruct->entries[suppportListIndex]);
	if (OK != nm_checkSupportList((char*)currPtnEntry->upgradeInfo.dataStart, currPtnEntry->upgradeInfo.dataLen))
	{
		NM_ERROR("the firmware is not for this model");
		*errorCode = NM_FWUP_ERROR_INCORRECT_MODEL;
		goto error;
	}

    return OK;
error:
    return ERROR;
}



/*******************************************************************
 * Name		: nm_cleanupPtnContentCache
 * Abstract	: free the memmory of ptnContentCache.
 * Input	: 
 * Output	: 
 * Return	: OK/ERROR.
 */

STATUS nm_cleanupPtnContentCache()
{   
    int index = 0;


    for (index=0; index<NM_PTN_NUM_MAX; index++)
    {           
        if (ptnContentCache[index] != NULL)
        {
            free(ptnContentCache[index]);
            ptnContentCache[index] = NULL;
        }   
    }
    
    return OK;
}


/*******************************************************************
 * Name		: nm_buildUpgradeStruct
 * Abstract	: Generate an upgrade file from NVRAM and firmware file.
 * Input	: 
 * Output	: 
 * Return	: OK/ERROR.
 */
int nm_buildUpgradeStruct(char *pAppBuf, int nFileBytes)
{
    char fwupPtnIndex[NM_FWUP_PTN_INDEX_SIZE+1] = {0};
    char *fwupFileBase = NULL;
    int index;
    int ret = 0;

    memset(g_nmFwupPtnStruct, 0, sizeof(NM_PTN_STRUCT));
    for (index=0; index<NM_PTN_NUM_MAX; index++)
    {
        g_nmFwupPtnStruct->entries[index].usedFlag = FALSE;
    }
    g_nmCountFwupAllWriteBytes = 0;
    g_nmCountFwupCurrWriteBytes = 0;
    nm_cleanupPtnContentCache();

    /* backup "fwup-partition-index" */
    fwupFileBase = pAppBuf;
    strncpy(fwupPtnIndex, pAppBuf, NM_FWUP_PTN_INDEX_SIZE+1); 
    pAppBuf += NM_FWUP_PTN_INDEX_SIZE;
    pAppBuf += sizeof(int);

    NM_DEBUG("nFileBytes = %d",  nFileBytes);
    if (nm_lib_parsePtnIndexFile(g_nmFwupPtnStruct, pAppBuf) != OK)
    {
        NM_ERROR("parse new ptn-index failed.");
        ret = NM_FWUP_ERROR_BAD_FILE;
        goto cleanup;
    }

    if (nm_getDataFromFwupFile(g_nmFwupPtnStruct, (char *)&fwupPtnIndex, fwupFileBase) != OK)
    {
        NM_ERROR("getDataFromFwupFile failed.");
        ret = NM_FWUP_ERROR_BAD_FILE;
        goto cleanup;
    }

    if (nm_getDataFromNvram(g_nmFwupPtnStruct, g_nmPtnStruct) != OK)
    {
        NM_ERROR("getDataFromNvram failed.");
        ret = NM_FWUP_ERROR_BAD_FILE;
        goto cleanup;
    }

    if (nm_checkUpdateContent(g_nmFwupPtnStruct, fwupFileBase, nFileBytes, &ret) != OK)
    {
        NM_ERROR("checkUpdateContent failed.");
        goto cleanup;
    }

    return 0;
    
cleanup:
    memset(g_nmFwupPtnStruct, 0, sizeof(NM_PTN_STRUCT));
    g_nmCountFwupAllWriteBytes = 0;
    g_nmCountFwupCurrWriteBytes = 0;
    nm_cleanupPtnContentCache();
    g_nmUpgradeResult = FALSE;
    return ret;
}


/*******************************************************************
 * Name		: nm_upgradeFwupFile
 * Abstract	: upgrade the FwupFile to NVRAM
 * Input	: 
 * Output	: 
 * Return	: OK/ERROR.
 */
STATUS nm_upgradeFwupFile(char *pAppBuf, int nFileBytes)
{   
    g_nmUpgradeResult = FALSE;

    if (nm_updateDataToNvram(g_nmFwupPtnStruct) != OK)
    {
        NM_ERROR("updateDataToNvram failed.");
        goto cleanup;
    }

    /* update run-time partition-table, active new partition-table without restart */
    if (nm_updateRuntimePtnTable(g_nmFwupPtnStruct, g_nmPtnStruct) != OK)
    {
        NM_ERROR("updateDataToNvram failed.");
        goto cleanup;
    }

    memset(g_nmFwupPtnStruct, 0, sizeof(NM_PTN_STRUCT));
    g_nmCountFwupAllWriteBytes = 0;
    g_nmCountFwupCurrWriteBytes = 0;
    nm_cleanupPtnContentCache();
    g_nmUpgradeResult = TRUE;
    return OK;
    
cleanup:
    memset(g_nmFwupPtnStruct, 0, sizeof(NM_PTN_STRUCT));
    g_nmCountFwupAllWriteBytes = 0;
    g_nmCountFwupCurrWriteBytes = 0;
    nm_cleanupPtnContentCache();
    g_nmUpgradeResult = FALSE;
    return ERROR;
}

/**************************************************************************************************/
/*                                      GLOBAL_FUNCTIONS                                          */
/**************************************************************************************************/

