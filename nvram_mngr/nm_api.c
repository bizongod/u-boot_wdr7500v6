
/*! Copyright(c) 1996-2009 Shenzhen TP-LINK Technologies Co. Ltd.
 * \file	nm_lib.c
 * \brief	api functions for NVRAM manager.
 * \author	Meng Qing
 * \version	1.0
 * \date	24/04/2009
 */


/**************************************************************************************************/
/*                                      CONFIGURATIONS                                            */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                      INCLUDE_FILES                                             */
/**************************************************************************************************/
#include <common.h>

#include "nm_lib.h"
#include "nm_api.h"


/**************************************************************************************************/
/*                                      DEFINES                                                   */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                      TYPES                                                     */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                      EXTERN_PROTOTYPES                                         */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                      LOCAL_PROTOTYPES                                          */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                      VARIABLES                                                 */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                      LOCAL_FUNCTIONS                                           */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                      PUBLIC_FUNCTIONS                                          */
/**************************************************************************************************/

/*******************************************************************
 * Name		: nm_api_writePtnToNvram
 * Abstract	: write the value of a partition to NVRAM.
 * Input	: 
 * Output	: 
 * Return	: OK/ERROR
 */
int nm_api_writePtnToNvram(char *name, char *buf, int len)
{
	int ret;
	char *ptr = buf;
	NM_PTN_ENTRY *ptnEntry;

	NM_SEM_TAKE(g_nmReadWriteLock, WAIT_FOREVER);
	
	/* get runtime-partition-entry by name */
	if ((ptnEntry = nm_lib_ptnNameToEntry(g_nmPtnStruct, name)) == NULL)
	{
		NM_ERROR("partition name not found.\r\n");
		goto error;
	}

	/* some partitions don't have partition head(partition-length & checksum) */
	if ((strncmp(NM_PTN_NAME_FS_UBOOT, ptnEntry->name, NM_PTN_NAME_LEN) == 0)
		|| (strncmp(NM_PTN_NAME_OS_IMAGE, ptnEntry->name, NM_PTN_NAME_LEN) == 0)
		|| (strncmp(NM_PTN_NAME_FILE_SYSTEM, ptnEntry->name, NM_PTN_NAME_LEN) == 0)
		|| (strncmp(NM_PTN_NAME_RADIO, ptnEntry->name, NM_PTN_NAME_LEN) == 0)
		|| (strncmp(NM_PTN_NAME_LOG, ptnEntry->name, NM_PTN_NAME_LEN) == 0))
	{
		if (len > ptnEntry->size)
		{
			NM_ERROR("no enough space in this partition.");
			goto error;
		}

		ret = nm_lib_writeHeadlessPtnToNvram((char *)ptnEntry->base, ptr, len);
	}
	else
	{
		if (len > (ptnEntry->size - sizeof(int) - sizeof(int)))
		{
			NM_ERROR("no enough space in this partition.");
			goto error;
		}
		
		ret = nm_lib_writePtnToNvram((char *)ptnEntry->base, ptr, len);
	}

	NM_SEM_GIVE(g_nmReadWriteLock);
	return ret;
	
error:
	NM_SEM_GIVE(g_nmReadWriteLock);
	return -1;
}



/*******************************************************************
 * Name		: nm_api_readPtnFromNvram
 * Abstract	: read value of a partition from NVRAM.
 * Input	: 
 * Output	: 
 * Return	: OK/ERROR
 */
int nm_api_readPtnFromNvram(char *name, char *buf, int len)
{
	int ret;
	char *ptr = buf;
	NM_PTN_ENTRY *ptnEntry;

	NM_SEM_TAKE(g_nmReadWriteLock, WAIT_FOREVER);
		
	/* get runtime-partition-entry by name */
	if ((ptnEntry = nm_lib_ptnNameToEntry(g_nmPtnStruct, name)) == NULL)
	{
		NM_ERROR("partition name not found.\r\n");
		goto error;
	}


	/* some partitions don't have partition head(partition-length & checksum) */
	if ((strncmp(NM_PTN_NAME_FS_UBOOT, ptnEntry->name, NM_PTN_NAME_LEN) == 0)
		|| (strncmp(NM_PTN_NAME_OS_IMAGE, ptnEntry->name, NM_PTN_NAME_LEN) == 0)
		|| (strncmp(NM_PTN_NAME_FILE_SYSTEM, ptnEntry->name, NM_PTN_NAME_LEN) == 0)
		|| (strncmp(NM_PTN_NAME_RADIO, ptnEntry->name, NM_PTN_NAME_LEN) == 0)
		|| (strncmp(NM_PTN_NAME_LOG, ptnEntry->name, NM_PTN_NAME_LEN) == 0))
	{
		ret = nm_lib_readHeadlessPtnFromNvram((char *)ptnEntry->base, ptr, len);
	}
	else
	{
		ret = nm_lib_readPtnFromNvram((char *)ptnEntry->base, ptr, len);
	}

	NM_SEM_GIVE(g_nmReadWriteLock);
	return ret;
	
error:
	NM_SEM_GIVE(g_nmReadWriteLock);
	return -1;
}




/*******************************************************************
 * Name		: nm_api_writeToNvram
 * Abstract	: write value from a buffer to NVRAM.
 * Input	: 
 * Output	: 
 * Return	: OK/ERROR
 */
int nm_api_writeToNvram(char *base, char *buf, int len)
{
	int ret;
	
	NM_SEM_TAKE(g_nmReadWriteLock, WAIT_FOREVER);
	ret = nm_lib_writeHeadlessPtnToNvram(base, buf, len);
	NM_SEM_GIVE(g_nmReadWriteLock);

	return ret;
}



/*******************************************************************
 * Name		: nm_api_readFromNvram
 * Abstract	: read value from NVRAM to a buffer.
 * Input	: 
 * Output	: 
 * Return	: OK/ERROR
 */
int nm_api_readFromNvram(char *base, char *buf, int len)
{
	int ret;
	
	NM_SEM_TAKE(g_nmReadWriteLock, WAIT_FOREVER);
	ret = nm_lib_readHeadlessPtnFromNvram(base, buf, len);
	NM_SEM_GIVE(g_nmReadWriteLock);

	return ret;
}

/**************************************************************************************************/
/*                                      GLOBAL_FUNCTIONS                                          */
/**************************************************************************************************/


