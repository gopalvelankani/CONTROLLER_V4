/*
* Copyright (c) 2011-2013 MaxLinear, Inc. All rights reserved
*
* License type: GPLv2
*
* This program is free software; you can redistribute it and/or modify it under
* the terms of the GNU General Public License as published by the Free Software
* Foundation.
*
* This program is distributed in the hope that it will be useful, but WITHOUT
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
* FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License along with
* this program; if not, write to the Free Software Foundation, Inc.,
* 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*
* This program may alternatively be licensed under a proprietary license from
* MaxLinear, Inc.
*
* See terms and conditions defined in file 'LICENSE.txt', which is part of this
* source code package.
*/

#ifndef __MxLWARE_HYDRA_OEM_DRV_H__
#define __MxLWARE_HYDRA_OEM_DRV_H__

/*****************************************************************************************
    Include Header Files
*****************************************************************************************/

#ifdef __KERNEL__
  #include <linux/kernel.h>
  #include <linux/module.h>
#else
  #include <stdlib.h>
  #include <string.h>
#endif

//#include "API_SerialInterface.h"
#include "MxLWare_HYDRA_OEM_Defines.h"
#include "MaxLinearDataTypes.h"

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************************************
    Macros
*****************************************************************************************/
#define MXLWARE_OSAL_MALLOC     malloc
#define MXLWARE_OSAL_MEMCPY     memcpy
#define MXLWARE_OSAL_MEMSET     memset

/*****************************************************************************************
    User-Defined Types (Typedefs)
*****************************************************************************************/
#define CMD_CS	0x00
#define CMD_IOS	0x40
#define CMD_WRITE	0x80
#define CMD_READ	0xC0
#define I2C_CS 5		// 15 pour le remux ; 0 poour le serial_intf (numero du bit)
#define I2C_DELAY 7<<24
#define I2C_RMODE 0		// 0 = Direct read ; 1 = RESTART mode
#define I2C_SPEED 200	// I2C speed link in kHz
#define I2C_BRD_FREQ 125000 // Host I2C board frequency in kHz

void setCpuCS(unsigned char ucCs);
int writeCPU(unsigned char ucUaddress, unsigned char ucByte3, unsigned char ucByte2, unsigned char ucByte1, unsigned char ucByte0);
int write32bCPU(unsigned char ucCs, unsigned char ucUaddress, unsigned int uiData32);
unsigned int read32bCPU(unsigned char ucCs, unsigned char ucUaddress);
int readCPU(unsigned char ucUaddress, unsigned char *pucByte3, unsigned char *pucByte2, unsigned char *pucByte1, unsigned char *pucByte0);
/*****************************************************************************************
    Global Variable Declarations
*****************************************************************************************/
extern void * MxL_HYDRA_OEM_DataPtr[];

/*****************************************************************************************
    Prototypes
*****************************************************************************************/

MXL_STATUS_E MxLWare_HYDRA_OEM_DeviceReset(UINT16 freqI2C);

MXL_STATUS_E MxLWare_HYDRA_OEM_I2cWrite(UINT8 devId, UINT16 dataLen, UINT8 *buffPtr);

MXL_STATUS_E MxLWare_HYDRA_OEM_I2cRead(UINT8 devId, UINT16 dataLen, /*@out@*/UINT8 *buffPtr);

MXL_STATUS_E MxLWare_HYDRA_OEM_InitI2cAccessLock(UINT8 devId);

MXL_STATUS_E MxLWare_HYDRA_OEM_Lock(UINT8 devId);

MXL_STATUS_E MxLWare_HYDRA_OEM_Unlock(UINT8 devId);

MXL_STATUS_E MxLWare_HYDRA_OEM_DeleteI2cAccessLock(UINT8 devId);

void MxLWare_HYDRA_OEM_SleepInMs(UINT16 delayTimeInMs);

void MxLWare_HYDRA_OEM_GetCurrTimeInMs(UINT64 *msecsPtr);

void *MxLWare_HYDRA_OEM_MemAlloc(UINT32 sizeInBytes);

void MxLWare_HYDRA_OEM_MemFree(void *memPtr);

void mvdWriteI2C(oem_data_t *oemDataPtr, UINT16 dataLen, UINT8 *buffPtr);
void mvdReadI2C(oem_data_t *oemDataPtr, UINT16 dataLen, UINT8 *buffPtr);

#ifdef __cplusplus
}
#endif

#endif // __MxLWARE_HYDRA_OEM_DRV_H__

