/*! Copyright(c) 2015-2018 Shenzhen TP-LINK Technologies Co. Ltd.
 * \file	cmd_recovery.c
 * \brief	firmware recovery by nvrammanager
 * \author	zengwei
 * \version	1.0
 * \date	11/11/2015
 */
#include <common.h>
#include <command.h>

#include "nm_lib.h"
#include "nm_fwup.h"

int do_fwrecovery( cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    int ret = 0;
    int fileLen = 0;
    int bufLen  = 0;
    uint8_t *pBuf = NULL;
    unsigned char *addr = NULL;

	if (argc < 2) {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return -1;
	}

    addr   = simple_strtoul(argv[1], NULL, 0);
    bufLen = simple_strtoul(argv[2], NULL, 0);

    if (!addr || !bufLen)
    {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return -2;
    }

    ret = nm_init();
    if (OK != ret)
    {
        printf("Partition table initiating failed.\n");
        return -3;
    }

    memcpy(&fileLen, addr, sizeof(int));
    fileLen = ntohl(fileLen);

    if(fileLen < IMAGE_SIZE_MIN || fileLen > IMAGE_SIZE_MAX || bufLen < fileLen)
    {
        printf("Bad file length(Buffer Length:%d File Length:%d)\n", bufLen, fileLen);
        return -1;
    }    
    printf("File Length:%d\n", fileLen);

	//md5 verify
	if (0 != nm_tpFirmwareMd5Check(addr, fileLen))
	{
		printf("Firmware md5 verify error!\n");
		return -1;
	}
    printf("Firmware md5 verify OK!\n");

    pBuf = addr + IMAGE_SIZE_BASE;
    ret = nm_buildUpgradeStruct((char *)pBuf, bufLen - IMAGE_SIZE_BASE);
    if (0 != ret)
    {
        printf("Firmware Invalid!\n");
        return -1;
    }
    printf("Firmware Checking Passed.\n");
    
    ret = nm_upgradeFwupFile((char *)addr + IMAGE_SIZE_BASE, bufLen - IMAGE_SIZE_BASE);
    if (ret != OK)
    {
        printf("Firmware Upgrading Failed!\n");
        return -1;
    }

    printf("All Done!\n");
    return 0;
}

/**************************************************/
U_BOOT_CMD(
	fwrecov,     3,     1,      do_fwrecovery,
	"fwrecov - TP-Link Firmware Recovery Tools\n",
	"address filelen\n"
);

