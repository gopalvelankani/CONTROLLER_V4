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

#include "MxLWare_HYDRA_OEM_Defines.h"
#include "MxLWare_HYDRA_OEM_Drv.h"

#ifdef __MXL_OEM_DRIVER__ 1
#include "MxL_HYDRA_I2cWrappers.h"
#endif

FILE *SpyFile;
FILE *SpyFile2;
unsigned int CMPT_READ = 0;
unsigned int CMPT_WRITE = 0;
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define SOCKET_INVALID(s) (s<0)
#define SOCKET_TYPE int


#define MMSB_32_8(x)    (x&0xFF000000)>>24
#define MLSB_32_8(x)    0xFF&((x&0x00FF0000)>>16)
#define LMSB_32_8(x)    0xFF&((x&0x0000FF00)>>8)
#define LLSB_32_8(x)    0xFF&((x&0x000000FF))
/**
 ************************************************************************
 *
 * @brief MxLWare_HYDRA_OEM_DeviceReset
 *
 * @param[in]   devId           Device ID
 *
 * @author Mahee
 *
 * @date 06/12/2012 Initial release
 *
 * This API performs a hardware reset on Hydra SOC identified by devId.
 *
 * @retval MXL_SUCCESS            - OK
 * @retval MXL_FAILURE            - Failure
 * @retval MXL_INVALID_PARAMETER  - Invalid parameter is passed
 *
 ************************************************************************/
MXL_STATUS_E MxLWare_HYDRA_OEM_DeviceReset(UINT16 freqI2C)
{
  MXL_STATUS_E mxlStatus = MXL_SUCCESS;
  write32bCPU(I2C_CS, 12, freqI2C);
  //fopen_s(&SpyFile, "c:/spy_i2c.txt", "w+");
  //fopen_s(&SpyFile2, "c:/spy_i2c_2.txt", "w+");
  return mxlStatus;
}

/**
 ************************************************************************
 *
 * @brief MxLWare_HYDRA_OEM_SleepInMs
 *
 * @param[in]   delayTimeInMs
 *
 * @author Mahee
 *
 * @date 06/12/2012 Initial release
 *
 * his API will implement delay in milliseconds specified by delayTimeInMs.
 *
 * @retval MXL_SUCCESS            - OK
 * @retval MXL_FAILURE            - Failure
 * @retval MXL_INVALID_PARAMETER  - Invalid parameter is passed
 *
 ************************************************************************/
void MxLWare_HYDRA_OEM_SleepInMs(UINT16 delayTimeInMs)
{
 // delayTimeInMs=delayTimeInMs;
  // _sleep(delayTimeInMs);
    usleep(1000 * delayTimeInMs);
#ifdef __MXL_OEM_DRIVER__
  Sleep(delayTimeInMs);
#endif

}

/**
 ************************************************************************
 *
 * @brief MxLWare_HYDRA_OEM_GetCurrTimeInMs
 *
 * @param[out]   msecsPtr
 *
 * @author Mahee
 *
 * @date 06/12/2012 Initial release
 *
 * This API should return system time milliseconds.
 *
 * @retval MXL_SUCCESS            - OK
 * @retval MXL_FAILURE            - Failure
 * @retval MXL_INVALID_PARAMETER  - Invalid parameter is passed
 *
 ************************************************************************/
void MxLWare_HYDRA_OEM_GetCurrTimeInMs(UINT64 *msecsPtr)
{
  msecsPtr = msecsPtr;

#ifdef __MXL_OEM_DRIVER__
  *msecsPtr = GetTickCount();
#endif

}

/**
 ************************************************************************
 *
 * @brief MxLWare_HYDRA_OEM_I2cWrite
 *
 * @param[in]   devId           Device ID
 * @param[in]   dataLen
 * @param[in]   buffPtr
 *
 * @author Mahee
 *
 * @date 06/12/2012 Initial release
 *
 * This API performs an I2C block write by writing data payload to Hydra device.
 *
 * @retval MXL_SUCCESS            - OK
 * @retval MXL_FAILURE            - Failure
 * @retval MXL_INVALID_PARAMETER  - Invalid parameter is passed
 *
 ************************************************************************/

MXL_STATUS_E MxLWare_HYDRA_OEM_I2cWrite(UINT8 devId, UINT16 dataLen, UINT8 *buffPtr)
{
  MXL_STATUS_E mxlStatus = MXL_SUCCESS;
  oem_data_t *oemDataPtr = (oem_data_t *)0;
 int i;
  oemDataPtr = (oem_data_t *)MxL_HYDRA_OEM_DataPtr[devId];
  buffPtr = buffPtr;
  dataLen = dataLen;

 // fprintf_s(SpyFile, "START/WRITE @¯C0/");
  //for (i = 0; i < dataLen; i++) {
	// fprintf_s(SpyFile, "+/%02X/", buffPtr[i]);
  //}
  //fprintf_s(SpyFile, "+/STOP\n");

  // fprintf_s(SpyFile2, "START/WRITE ");
  mvdWriteI2C(oemDataPtr, dataLen, buffPtr);
  // fprintf_s(SpyFile2, "END/WRITE\n");

  if (oemDataPtr)
  {
#ifdef __MXL_OEM_DRIVER__
    mxlStatus = MxL_HYDRA_I2C_WriteData(oemDataPtr->drvIndex, devId, oemDataPtr->i2cAddress, oemDataPtr->channel, dataLen, buffPtr);
#endif
  }
  return mxlStatus;
}

/**
 ************************************************************************
 *
 * @brief MxLWare_HYDRA_OEM_I2cRead
 *
 * @param[in]   devId           Device ID
 * @param[in]   dataLen
 * @param[out]  buffPtr
 *
 * @author Mahee
 *
 * @date 06/12/2012 Initial release
 *
 * This API shall be used to perform I2C block read transaction.
 *
 * @retval MXL_SUCCESS            - OK
 * @retval MXL_FAILURE            - Failure
 * @retval MXL_INVALID_PARAMETER  - Invalid parameter is passed
 *
 ************************************************************************/
MXL_STATUS_E MxLWare_HYDRA_OEM_I2cRead(UINT8 devId, UINT16 dataLen, UINT8 *buffPtr)
{
  MXL_STATUS_E mxlStatus = MXL_SUCCESS;
  oem_data_t *oemDataPtr = (oem_data_t *)0;
  int i;

  oemDataPtr = (oem_data_t *)MxL_HYDRA_OEM_DataPtr[devId];
  buffPtr = buffPtr;
  dataLen = dataLen;
//  fprintf_s(SpyFile, "START/READ  @¯C1/");

  // fprintf_s(SpyFile2, "START/READ ");
  mvdReadI2C(oemDataPtr, dataLen, buffPtr);
  // fprintf_s(SpyFile2, "END/READ\n");

  for (i = 0; i < dataLen; i++) {
	 //fprintf_s(SpyFile, "+/%02X/", buffPtr[i]);
  }
  //fprintf_s(SpyFile, "-/STOP\n");
  if (oemDataPtr)
  {
#ifdef __MXL_OEM_DRIVER__
    mxlStatus = MxL_HYDRA_I2C_ReadData(oemDataPtr->drvIndex, devId, oemDataPtr->i2cAddress, oemDataPtr->channel, dataLen, buffPtr);
#endif
  }
  return mxlStatus;
}

/**
 ************************************************************************
 *
 * @brief MxLWare_HYDRA_OEM_InitI2cAccessLock
 *
 * @param[in]   devId           Device ID
 *
 * @author Mahee
 *
 * @date 04/11/2013 Initial release
 *
 * This API will initilize locking mechanism used to serialize access for
 * I2C operations.
 *
 * @retval MXL_SUCCESS            - OK
 * @retval MXL_FAILURE            - Failure
 * @retval MXL_INVALID_PARAMETER  - Invalid parameter is passed
 *
 ************************************************************************/
MXL_STATUS_E MxLWare_HYDRA_OEM_InitI2cAccessLock(UINT8 devId)
{
  MXL_STATUS_E mxlStatus = MXL_SUCCESS;
  oem_data_t *oemDataPtr = (oem_data_t *)0;

  oemDataPtr = (oem_data_t *)MxL_HYDRA_OEM_DataPtr[devId];

  if (oemDataPtr)
  {

  }

  return mxlStatus;
}

/**
 ************************************************************************
 *
 * @brief MxLWare_HYDRA_OEM_DeleteI2cAccessLock
 *
 * @param[in]   devId           Device ID
 *
 * @author Mahee
 *
 * @date 04/11/2013 Initial release
 *
 * This API will release locking mechanism and associated resources.
 *
 * @retval MXL_SUCCESS            - OK
 * @retval MXL_FAILURE            - Failure
 * @retval MXL_INVALID_PARAMETER  - Invalid parameter is passed
 *
 ************************************************************************/
MXL_STATUS_E MxLWare_HYDRA_OEM_DeleteI2cAccessLock(UINT8 devId)
{
  MXL_STATUS_E mxlStatus = MXL_SUCCESS;
  oem_data_t *oemDataPtr = (oem_data_t *)0;

  oemDataPtr = (oem_data_t *)MxL_HYDRA_OEM_DataPtr[devId];
  if (oemDataPtr)
  {

  }

  return mxlStatus;
}

/**
 ************************************************************************
 *
 * @brief MxLWare_HYDRA_OEM_Lock
 *
 * @param[in]   devId           Device ID
 *
 * @author Mahee
 *
 * @date 04/11/2013 Initial release
 *
 * This API will should be used to lock access to device i2c access
 *
 * @retval MXL_SUCCESS            - OK
 * @retval MXL_FAILURE            - Failure
 * @retval MXL_INVALID_PARAMETER  - Invalid parameter is passed
 *
 ************************************************************************/
MXL_STATUS_E MxLWare_HYDRA_OEM_Lock(UINT8 devId)
{
  MXL_STATUS_E mxlStatus = MXL_SUCCESS;
  oem_data_t *oemDataPtr = (oem_data_t *)0;

  oemDataPtr = (oem_data_t *)MxL_HYDRA_OEM_DataPtr[devId];
  if (oemDataPtr)
  {

  }

  return mxlStatus;
}

/**
 ************************************************************************
 *
 * @brief MxLWare_HYDRA_OEM_Unlock
 *
 * @param[in]   devId           Device ID
 *
 * @author Mahee
 *
 * @date 04/11/2013 Initial release
 *
 * This API will should be used to unlock access to device i2c access
 *
 * @retval MXL_SUCCESS            - OK
 * @retval MXL_FAILURE            - Failure
 * @retval MXL_INVALID_PARAMETER  - Invalid parameter is passed
 *
 ************************************************************************/
MXL_STATUS_E MxLWare_HYDRA_OEM_Unlock(UINT8 devId)
{
  MXL_STATUS_E mxlStatus = MXL_SUCCESS;
  oem_data_t *oemDataPtr = (oem_data_t *)0;

  oemDataPtr = (oem_data_t *)MxL_HYDRA_OEM_DataPtr[devId];

  if (oemDataPtr)
  {

  }

  return mxlStatus;
}

/**
 ************************************************************************
 *
 * @brief MxLWare_HYDRA_OEM_MemAlloc
 *
 * @param[in]   sizeInBytes
 *
 * @author Sateesh
 *
 * @date 04/23/2015 Initial release
 *
 * This API shall be used to allocate memory.
 *
 * @retval memPtr                 - Memory Pointer
 *
 ************************************************************************/
void* MxLWare_HYDRA_OEM_MemAlloc(UINT32 sizeInBytes)
{
  void *memPtr = NULL;
  sizeInBytes = sizeInBytes;

#ifdef __MXL_OEM_DRIVER__
  memPtr = malloc(sizeInBytes * sizeof(UINT8));
#endif
  return memPtr;
}

/**
 ************************************************************************
 *
 * @brief MxLWare_HYDRA_OEM_MemFree
 *
 * @param[in]   memPtr
 *
 * @author Sateesh
 *
 * @date 04/23/2015 Initial release
 *
 * This API shall be used to free memory.
 *
 *
 ************************************************************************/
void MxLWare_HYDRA_OEM_MemFree(void *memPtr)
{
  memPtr = memPtr;
#ifdef __MXL_OEM_DRIVER__
  free(memPtr);
#endif
}


// MVD I2C Read/Write functions
void mvdWriteI2C(oem_data_t *oemDataPtr, UINT16 dataLen, UINT8 *buffPtr) {
	//mxlStatus = MxL_HYDRA_I2C_ReadData(oemDataPtr->drvIndex, devId, oemDataPtr->i2cAddress, oemDataPtr->channel, dataLen, buffPtr);
	int i = 0;
//	unsigned int value = read32bCPU(I2C_CS, 8);
	UINT16 size = dataLen;
	unsigned char index_out = 1;
	unsigned int word0 = (I2C_DELAY | size);
	unsigned int status;
	unsigned int dataOut = (unsigned int)(oemDataPtr->i2cAddress * 2);

	// Check I2C core is not busy
	do {
		status = read32bCPU(I2C_CS, 8);
    usleep(100);
		i++;
	} while (status & (1 << 19));

	if (i > 1){
		i = i;
	}

	write32bCPU(I2C_CS, 8, word0);
  usleep(100);
	for (i = 0; i < dataLen; i++) {
		dataOut <<= 8;
		dataOut |= buffPtr[i];
		if (index_out == 3) {
			write32bCPU(I2C_CS, 4, dataOut);
      usleep(100);
			index_out = 0;
		} else {
			index_out++;
		}
	}

	if ( index_out == 1 ) {
		write32bCPU(I2C_CS, 4, (dataOut << 24));
    usleep(100);
	} else if ( index_out == 2 ) {
		write32bCPU(I2C_CS, 4, (dataOut << 16));
    usleep(100);
	} else if ( index_out == 3 ) {
		write32bCPU(I2C_CS, 4, (dataOut << 8));
    usleep(100);
	}

	word0 |= (I2C_DELAY|(1 << 18));
	write32bCPU(I2C_CS, 8, word0);
  usleep(100);
	do {
      status = read32bCPU(I2C_CS, 8);
      usleep(100);
      i++;
   } while (status & (1 << 19));
}
int write32bCPU(unsigned char ucCs, unsigned char ucUaddress, unsigned int uiData32)
{
    struct sockaddr_in serverAddress,
        clientAddress;
    unsigned short port = 62000;
    unsigned char msgBuf[20];
    unsigned char RxBuffer[105]={0};
    unsigned short len=0;
    struct timeval tv;

    tv.tv_sec = 0;  /* 0.5 Secs Timeout */
    tv.tv_usec = 500000;
    int socketHandle = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP/* or 0? */);
    setsockopt(socketHandle, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv,sizeof(struct timeval));
    int reuse_sockt=1;
    if(setsockopt(socketHandle, SOL_SOCKET, SO_REUSEADDR, &reuse_sockt,sizeof(reuse_sockt)) == -1){
      printf("Set socket option failed\n");
          return 0; 
    }
    if (socketHandle<0)
    {
        printf("could not make socket\n");
        return 0;
    }
    memset(&serverAddress, 0, sizeof(serverAddress));
    memset(&clientAddress, 0, sizeof(clientAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);
    serverAddress.sin_addr.s_addr = inet_addr("192.168.1.20");

    clientAddress.sin_family = AF_INET;
    clientAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    clientAddress.sin_port = htons(port);

    if(bind(socketHandle,(struct sockaddr*)&clientAddress,sizeof(clientAddress))<0){
      printf("bind failed\n");
    }

    len = 0;
    msgBuf[0] = 0x4D;
    msgBuf[1] = 0x56;
    msgBuf[2] = 0x44;
    msgBuf[3] = 0x01;
    msgBuf[4] = (0xFF00&ucCs)>>8;
    msgBuf[5] = (0x00FF&ucCs)>>0;
    unsigned short uProg;
    msgBuf[6] = 0xF0;
    msgBuf[7] = ucUaddress;
    msgBuf[8] = MMSB_32_8(uiData32);
    msgBuf[9] = MLSB_32_8(uiData32);
    msgBuf[10]= LMSB_32_8(uiData32);
    msgBuf[11]= LLSB_32_8(uiData32);
    len += 12;
    int x=0;
    unsigned int count;
    while (x == 0)
    {
          int n = sendto(socketHandle,msgBuf, len, 0,(struct sockaddr*)&serverAddress, sizeof(serverAddress));

          int serv_addr_size = sizeof(clientAddress);
          count=recvfrom(socketHandle,RxBuffer,32, 0, (struct sockaddr*)&clientAddress,(socklen_t*) &serv_addr_size);
          x=1;
    }
      close(socketHandle);
      return count;
  }
unsigned int read32bCPU(unsigned char ucCs, unsigned char ucUaddress)
{
    struct sockaddr_in serverAddress,
        clientAddress;
    unsigned short port = 62000;
    unsigned char msgBuf[20];
    unsigned char RxBuffer[105]={0};
    unsigned short len=0;
    struct timeval tv;
    unsigned int status;
    tv.tv_sec = 0;  /* 0.5 Secs Timeout */
    tv.tv_usec = 500000;
    int socketHandle = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP/* or 0? */);
    setsockopt(socketHandle, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv,sizeof(struct timeval));
    int reuse_sockt=1;
    if(setsockopt(socketHandle, SOL_SOCKET, SO_REUSEADDR, &reuse_sockt,sizeof(reuse_sockt)) == -1){
      printf("Set socket option failed\n");
          return 0; 
    }
    if (socketHandle<0)
    {
        printf("could not make socket\n");
        return 0;
    }
    memset(&serverAddress, 0, sizeof(serverAddress));
    memset(&clientAddress, 0, sizeof(clientAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);
    serverAddress.sin_addr.s_addr = inet_addr("192.168.1.20");
    clientAddress.sin_family = AF_INET;
    clientAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    clientAddress.sin_port = htons(port);

    if(bind(socketHandle,(struct sockaddr*)&clientAddress,sizeof(clientAddress))<0){
      printf("bind failed\n");
    }
    len = 0;
    msgBuf[0] = 0x4D;
    msgBuf[1] = 0x56;
    msgBuf[2] = 0x44;
    msgBuf[3] = 0x01;
    msgBuf[4] = (0xFF00&ucCs)>>8;
    msgBuf[5] = (0x00FF&ucCs)>>0;
    unsigned short uProg;
    msgBuf[6] = 0x01;
    msgBuf[7] =ucUaddress;
    msgBuf[8] = 0x00;//dont care
    msgBuf[9] = 0x00;//dont care
    msgBuf[10]= 0x00;//dont care
    msgBuf[11]= 0x00;//dont care
    len += 12;
    int x=0;
    int count;
    while (x == 0)
    {
          int n = sendto(socketHandle,msgBuf, len, 0,(struct sockaddr*)&serverAddress, sizeof(serverAddress));
          int serv_addr_size = sizeof(clientAddress);
          count=recvfrom(socketHandle,RxBuffer,32, 0, (struct sockaddr*)&clientAddress,(socklen_t*) &serv_addr_size);
          int i=0;
          /*for(i=0;i<sizeof(RxBuffer) / sizeof(RxBuffer[0]);i++)
          {
            printf("%d",RxBuffer[i]);
          }*/
          status = RxBuffer[8]<<24 | RxBuffer[9]<<16 | RxBuffer[10]<<8 | RxBuffer[11];
          // printf("Read data %d\n",status);
          x=1;
    }
      close(socketHandle);
      return status;
  }
void mvdReadI2C(oem_data_t *oemDataPtr, UINT16 dataLen, UINT8 *buffPtr) {
	int i = 0, j;
	unsigned int status;
	unsigned int DataIn;
	//mvdWriteI2C(oemDataPtr, 0, NULL);
	write32bCPU(I2C_CS, 8, I2C_DELAY | (I2C_RMODE << 16) | dataLen);
  usleep(100);
	write32bCPU(I2C_CS, 4, (((oemDataPtr->i2cAddress*2)|1)<<24));
  usleep(100);
	write32bCPU(I2C_CS, 8, (I2C_DELAY | (1 << 18) | (1 << 17) | (I2C_RMODE << 16) | dataLen));
  usleep(100);
	// Check I2C core is not busy
	do {
		status = read32bCPU(I2C_CS, 8);
    usleep(100);
		i++;
	} while (status & (1 << 19));

	for (i = 0; i < dataLen; ) {
		DataIn = read32bCPU(I2C_CS, 4);
    usleep(100);
		for (j = 0; (j < 4) && (i < dataLen); j++) {
			buffPtr[i] = ((DataIn & 0xFF000000) >> 24);
			DataIn <<= 8;
			//buffPtr[i] = ((DataIn & 0x000000FF) >> 0);
			//DataIn >>= 8;
			i++;
		}
	}
}
