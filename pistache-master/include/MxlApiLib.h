// Le bloc ifdef suivant est la façon standard de créer des macros qui facilitent l'exportation
// à partir d'une DLL. Tous les fichiers contenus dans cette DLL sont compilés avec le symbole MXLAPILIB_EXPORTS
// défini sur la ligne de commande. Ce symbole ne doit pas être défini pour un projet
// qui utilisent cette DLL. De cette manière, les autres projets dont les fichiers sources comprennent ce fichier considèrent les fonctions
// MXLAPILIB_API comme étant importées à partir d'une DLL, tandis que cette DLL considère les symboles
// définis avec cette macro comme étant exportés.
#include "MxLWare_HYDRA_OEM_Drv.h"
#include "MxLWare_HYDRA_DemodTunerApi.h"
#include "MxLWare_HYDRA_CommonApi.h"
#include "MxLWare_HYDRA_DeviceApi.h"
#include "MxLWare_HYDRA_PhyCtrl.h"
#include "MxLWare_HYDRA_Registers.h"
#include "MxLWare_HYDRA_TsMuxCtrlApi.h"

// Pointer on external functions
int confAllegro8297(unsigned char i2cAddr,unsigned short enable1,unsigned short voltage1,unsigned short enable2,unsigned short voltage2);
int readAllegro8297(unsigned char i2cAddr,unsigned int reg);
int setWritePtr(void *fnctCallBackPtr);
int setReadPtr(void *fnctCallBackPtr);
MXL_STATUS_E writeRegister(unsigned int Address, unsigned int Data);
MXL_STATUS_E readRegister(unsigned int Address, unsigned int *Data);
MXL_STATUS_E setI2cAddress(unsigned char i2cAddress);
int writeI2C(unsigned char i2cAddress, unsigned short length, unsigned char *buffer);
int readI2C(unsigned char i2cAddress, unsigned short length, unsigned char *buffer);
// Mxl API functions
MXL_STATUS_E MxL_Connect(UINT16 i2cAddress, UINT16 i2cSpeed, UINT32 i2cFreqBoard);
MXL_STATUS_E MxL_GetVersion(MXL_HYDRA_VER_INFO_T *versionInfo);
MXL_STATUS_E App_FirmwareDownload(char *fileName);
MXL_STATUS_E MxL_HYDRA_DevInitilization(MXL_HYDRA_DEVICE_E devSku, MXL_BOOL_E forceFirmwareDownload, MXL_HYDRA_AUX_CTRL_MODE_E lnbInterface, MXL_HYDRA_TS_MUX_TYPE_E tsMuxType, MXL_HYDRA_MPEG_MODE_E tsInterfaceMode, UINT8 maxTsInterfaceClkRate);
MXL_STATUS_E configureLNB(unsigned char lnbId, unsigned char Enable, unsigned char Low_high);
MXL_STATUS_E getLNB(unsigned char lnbId, unsigned char *Enable, unsigned char *low_high);

MXL_STATUS_E setTuner(unsigned char demod_id, unsigned char lnb_id, unsigned char standard,
	unsigned int freq, unsigned int rate, unsigned char modulation, unsigned char fec, unsigned char rolloff, unsigned char pilots,
	unsigned char spectrum, unsigned int scr_index, unsigned int search_range);
MXL_STATUS_E getTuneInfo(unsigned char demod_id, unsigned char *lnb_id, unsigned char *standard,
	unsigned int *freq, unsigned int *rate, unsigned char *modulation, unsigned char *fec, unsigned char *rolloff, unsigned char *pilots,
	unsigned char *spectrum, unsigned int *RxPwr, unsigned int *SNR);
MXL_STATUS_E setTuneOff(unsigned char demod_id);
MXL_STATUS_E setMpegMode(unsigned char demod_id, unsigned char enable,
	unsigned char clk_type, unsigned char clock_pol, unsigned char clk_rate, unsigned char clk_phase,
	unsigned char msb_first, unsigned char sync_pulse, unsigned char valid_pol, unsigned char sync_pol,
	unsigned char mode, unsigned char error_indic);
int write32bI2C(unsigned char cCs, unsigned char addr, unsigned int data);
int setEthernet_MULT(unsigned int cCs);
int write32(unsigned char cCs, unsigned char addr, unsigned int *sData,int sDataLen);
int read32bI2C(unsigned char cCs, unsigned char addr);