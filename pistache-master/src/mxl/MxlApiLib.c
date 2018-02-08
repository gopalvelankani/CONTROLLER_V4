// MxlApiLib.cpp : définit les fonctions exportées pour l'application DLL.
//

#include <stdio.h>
#include "MxlApiLib.h"

#define MXL_DEVICE_ID 0

#define HYDRA_I2C_ADDR            0x63
#define HYDRA_OEM_I2C_DEV_NO      0

#define MXL_XTAL_CAP_VALUE 12

oem_data_t oemPtr;

//char *standard[MXL_HYDRA_DVBS2 + 1] = { ("DSS"), ("DVB-S"), ("DVB-S2") };
//char *fec[MXL_HYDRA_FEC_9_10 + 1] = { ("AUTO"), ("1/2"), ("3/5"), ("2/3"), ("3/4"), ("4/5"), ("5/6"), ("6/7"), ("7/8"), ("8/9"), ("9/10") };
//char *modulation[MXL_HYDRA_MOD_8PSK + 1] = { ("AUTO"), ("QPSK"), ("8PSK") };
//char *pilots[MXL_HYDRA_PILOTS_AUTO + 1] = { ("OFF"), ("ON"), ("AUTO") };
//char *rolloff[MXL_HYDRA_ROLLOFF_0_35 + 1] = { ("AUTO"), (".20"), (".25"), (".35") };
//char *spectrum[MXL_HYDRA_SPECTRUM_NON_INVERTED + 1] = { ("AUTO"), ("INVERTED"), ("NORMAL") };

// #ifdef __cplusplus
// extern "C" {
// #endif
// 	unsigned int(*write32bCPU)(unsigned char ucCs, unsigned char ucAddr, unsigned int uiData) = NULL;
// 	unsigned int(*read32bCPU)(unsigned char ucCs, unsigned char ucUaddress) = NULL;
// #ifdef __cplusplus
// 	}
// #endif

// int setWritePtr(void *fnctCallBackPtr) {
// 	write32bCPU = (unsigned int(__cdecl *)(unsigned char, unsigned char, unsigned int))fnctCallBackPtr;
// 	return 0;
// }

// int setReadPtr(void *fnctCallBackPtr) {
// 	read32bCPU = (unsigned int(__cdecl *)(unsigned char, unsigned char))fnctCallBackPtr;
// 	return 0;
// }

int writeI2C(unsigned char i2cAddress, unsigned short length, unsigned char *buffer) {
	oem_data_t cfgI2C;
	cfgI2C.i2cAddress = i2cAddress;
	mvdWriteI2C(&cfgI2C, length, buffer);
	return 1;
}

int readI2C(unsigned char i2cAddress, unsigned short length, unsigned char *buffer) {
	oem_data_t cfgI2C;
	cfgI2C.i2cAddress = i2cAddress;
	mvdReadI2C(&cfgI2C, length, buffer);
	return 1;
}

MXL_STATUS_E writeRegister(unsigned int Address, unsigned int Data) {
	MXL_STATUS_E mxlStatus = MXL_SUCCESS;
	mxlStatus = MxLWare_HYDRA_WriteRegister(MXL_DEVICE_ID, Address, Data);
	return mxlStatus;
}

MXL_STATUS_E readRegister(unsigned int Address, unsigned int *Data) {
	MXL_STATUS_E mxlStatus = MXL_SUCCESS;
	mxlStatus = MxLWare_HYDRA_ReadRegister(MXL_DEVICE_ID, Address, Data);
	return mxlStatus;
}

MXL_STATUS_E setI2cAddress(unsigned char i2cAddress) {
	MXL_STATUS_E mxlStatus = MXL_SUCCESS;
	oemPtr.i2cAddress = i2cAddress;
	return mxlStatus;
}

MXL_STATUS_E App_FirmwareDownload(char *fileName) {

	UINT8 *buff = NULL;
	FILE *Firmware;
	int result;
	fpos_t pos;

	UINT32 size = 0;
	MXL_STATUS_E mxlStatus = MXL_SUCCESS;

/*	result = fopen_s(&Firmware, fileName, "rb");

	if (result != 0) {
		return MXL_FAILURE;
	}

	fseek(Firmware, 0L, SEEK_END);
	fgetpos(Firmware, &pos);

	size = (UINT32) pos; // sizeof(buff);
	buff = (UINT8 *)malloc((UINT32)pos);
	fseek(Firmware, 0L, SEEK_SET);
	fread_s(buff, (size_t)pos, (size_t)pos, 1, Firmware);


	mxlStatus = MxLWare_HYDRA_API_CfgDevFWDownload(MXL_DEVICE_ID,
		size,
		buff,
		(MXL_CALLBACK_FN_T)NULL);

	fclose(Firmware);*/

	return mxlStatus;
}

MXL_STATUS_E MxL_Connect(uint16_t i2cAddress, UINT16 i2cSpeed, UINT32 i2cFreqBoard) {
	MXL_STATUS_E mxlStatus = MXL_SUCCESS;
	MXL_HYDRA_DEVICE_E hydraDevSku = MXL_HYDRA_DEVICE_584;
    UINT16 freqS = ((131072 * i2cSpeed) / i2cFreqBoard);
	MxLWare_HYDRA_OEM_DeviceReset(freqS);
	oemPtr.i2cAddress = i2cAddress;
	oemPtr.drvIndex = HYDRA_OEM_I2C_DEV_NO;
	mxlStatus = MxLWare_HYDRA_API_CfgDrvInit(MXL_DEVICE_ID, &oemPtr, hydraDevSku);

	return mxlStatus;
}

MXL_STATUS_E MxL_GetVersion(MXL_HYDRA_VER_INFO_T *versionInfo) {
	MXL_STATUS_E mxlStatus = MXL_SUCCESS;
	// Oerwrite defaults
	mxlStatus = MxLWare_HYDRA_API_CfgDevOverwriteDefault(MXL_DEVICE_ID);
	mxlStatus = MxLWare_HYDRA_API_ReqDevVersionInfo(MXL_DEVICE_ID, versionInfo);
	return mxlStatus;
}

MXL_STATUS_E MxL_HYDRA_DevInitilization(MXL_HYDRA_DEVICE_E devSku, MXL_BOOL_E forceFirmwareDownload, MXL_HYDRA_AUX_CTRL_MODE_E lnbInterface, MXL_HYDRA_TS_MUX_TYPE_E tsMuxType, MXL_HYDRA_MPEG_MODE_E tsInterfaceMode, UINT8 maxTsInterfaceClkRate) {
	// 1. Init MxlWare Driver
	// 2. Overwrite defaults
	// 3. XTAL settings
	// 4. Firmware download
	// 5. Configure Interrupts
	// 6. Config FSK/DISEQC
	// 7. Confing MPEG Interface

	MXL_STATUS_E mxlStatus = MXL_SUCCESS;
	//MXL_HYDRA_INTR_CFG_T intrCfg;
	MXL_HYDRA_MPEGOUT_PARAM_T mpegInterfaceCfg;
	MXL_HYDRA_VER_INFO_T versionInfo;
	UINT8 maxDemods;
	UINT32 intrMask = 0;
	UINT8 j = 0;

    UINT16 freqS = ((131072 * I2C_SPEED) / I2C_BRD_FREQ);
	MxLWare_HYDRA_OEM_DeviceReset(freqS);

	// Initilize MxLWare driver
	oemPtr.i2cAddress = HYDRA_I2C_ADDR;
	oemPtr.drvIndex = HYDRA_OEM_I2C_DEV_NO;
	mxlStatus = MxLWare_HYDRA_API_CfgDrvInit(MXL_DEVICE_ID, &oemPtr, devSku);

	if (mxlStatus == MXL_SUCCESS)
	{
		// Oerwrite defaults
		mxlStatus = MxLWare_HYDRA_API_CfgDevOverwriteDefault(MXL_DEVICE_ID);
		if (mxlStatus != MXL_SUCCESS)
		{
			printf("MxLWare_HYDRA_API_CfgDevOverwriteDefault - Error (%d)\n", mxlStatus);
			return mxlStatus;
		}
		else
			printf("Done! - MxLWare_HYDRA_API_CfgDevOverwriteDefault\n");

		// Download firmware
		if (forceFirmwareDownload == MXL_TRUE)
		{
			// Download firmwrae even if it is already downloaded and running
			//mxlStatus = App_FirmwareDownload(MXL_DEVICE_ID);
		}
		else
		{
			// Check if firmware is already downladed and is running
			mxlStatus = MxLWare_HYDRA_API_ReqDevVersionInfo(MXL_DEVICE_ID, &versionInfo);

			if (mxlStatus != MXL_SUCCESS)
			{
				printf("MxLWare_HYDRA_API_ReqDevVersionInfo - Error (%d)\n", mxlStatus);
				return mxlStatus;
			}
			mxlStatus = MxLWare_HYDRA_API_CfgDevPowerMode(MXL_DEVICE_ID, MXL_HYDRA_PWR_MODE_ACTIVE);
			if (mxlStatus != MXL_SUCCESS)
			{
				printf("MxLWare_HYDRA_API_CfgDevPowerMode - Error (%d)\n", mxlStatus);
				return mxlStatus;
			}

			if (versionInfo.firmwareDownloaded == MXL_FALSE)
			{
				// Download firmware only if firmware is not active/running.
			//	mxlStatus = App_FirmwareDownload(MXL_DEVICE_ID);
			}
		}

		if (mxlStatus != MXL_SUCCESS)
		{
			printf("Firmware Download - Error (%d)\n", mxlStatus);
			return mxlStatus;
		}
		else
			printf("Done! - Firmware Download \n");

		// Get device version info
		mxlStatus = MxLWare_HYDRA_API_ReqDevVersionInfo(MXL_DEVICE_ID, &versionInfo);
		if (mxlStatus != MXL_SUCCESS)
		{
			printf("MxLWare_HYDRA_API_ReqDevVersionInfo - Error (%d)\n", mxlStatus);
			return mxlStatus;
		}
		else
		{
			printf("Firmware Version  : %d.%d.%d.%d\n", versionInfo.firmwareVer[0],
				versionInfo.firmwareVer[1],
				versionInfo.firmwareVer[2],
				versionInfo.firmwareVer[3]);

			printf("MxLWare Version   : %d.%d.%d.%d\n", versionInfo.mxlWareVer[0],
				versionInfo.mxlWareVer[1],
				versionInfo.mxlWareVer[2],
				versionInfo.mxlWareVer[3]);
		}

		switch (devSku)
		{
			// Max # of demods is based on SKU,
			// for 541 & 544 there are 4 demods,
			// for 581 & 584 there are 8 demods
		default:
		case MXL_HYDRA_DEVICE_541:
		case MXL_HYDRA_DEVICE_541S:
		case MXL_HYDRA_DEVICE_541C:
		case MXL_HYDRA_DEVICE_542C:
			maxDemods = MXL_HYDRA_DEMOD_ID_4;
			break;

		case MXL_HYDRA_DEVICE_581:
		case MXL_HYDRA_DEVICE_584:
			maxDemods = MXL_HYDRA_DEMOD_MAX;
			break;

			// for 561 there are 6 demods
		case MXL_HYDRA_DEVICE_561:
			maxDemods = MXL_HYDRA_DEMOD_ID_6;
			break;
		}

#if 0 // Not use in this test
		//Select either DiSEqC or FSK - only one interface will be avilable either FSK or DiSEqC
		mxlStatus = MxLWare_HYDRA_API_CfgDevDiseqcFskOpMode(MXL_DEVICE_ID, lnbInterface);
		if (mxlStatus != MXL_SUCCESS)
		{
			printf("MxLWare_HYDRA_API_CfgDevDiseqcFskOpMode - Error (%d)\n", mxlStatus);
			return mxlStatus;
		}
		else
			printf("Done! - MxLWare_HYDRA_API_CfgDevDiseqcFskOpMode \n");

		if (lnbInterface == MXL_HYDRA_AUX_CTRL_MODE_FSK)
		{
			// Configure FSK operating mode
			mxlStatus = MxLWare_HYDRA_API_CfgFskOpMode(MXL_DEVICE_ID, MXL_HYDRA_FSK_CFG_TYPE_39KPBS);
			if (mxlStatus != MXL_SUCCESS)
			{
				printf("MxLWare_HYDRA_API_CfgFskOpMode - Error (%d)\n", mxlStatus);
				return mxlStatus;
			}
			else
				printf("Done! - MxLWare_HYDRA_API_CfgFskOpMode \n");
		}
		else
		{
			// Configure DISEQC operating mode - Only 22 KHz carrier frequency is supported.
			mxlStatus = MxLWare_HYDRA_API_CfgDiseqcOpMode(MXL_DEVICE_ID,
				MXL_HYDRA_DISEQC_ID_0,
				MXL_HYDRA_DISEQC_ENVELOPE_MODE,
				MXL_HYDRA_DISEQC_2_X,
				MXL_HYDRA_DISEQC_CARRIER_FREQ_22KHZ);
			if (mxlStatus != MXL_SUCCESS)
			{
				printf("MxLWare_HYDRA_API_CfgDevDiseqcFskOpMode - Error (%d)\n", mxlStatus);
				return mxlStatus;
			}
			else
				printf("Done! - MxLWare_HYDRA_API_CfgDevDiseqcFskOpMode \n");
		}
#endif

		// Config TS MUX mode - Disable TS MUX feature
		mxlStatus = MxLWare_HYDRA_API_CfgTSMuxMode(MXL_DEVICE_ID, tsMuxType);
		if (mxlStatus != MXL_SUCCESS)
		{
			printf("MxLWare_HYDRA_API_CfgTSMuxMode - Error (%d)\n", mxlStatus);
			return mxlStatus;
		}
		else
			printf("Done! - MxLWare_HYDRA_API_CfgTSMuxMode \n");

		// Config TS interface of the device
		mpegInterfaceCfg.enable = MXL_ENABLE;
		mpegInterfaceCfg.lsbOrMsbFirst = MXL_HYDRA_MPEG_SERIAL_LSB_1ST;
		mpegInterfaceCfg.maxMpegClkRate = maxTsInterfaceClkRate; //  supports only (0 - 104 & 139)MHz
		mpegInterfaceCfg.mpegClkPhase = MXL_HYDRA_MPEG_CLK_PHASE_SHIFT_0_DEG;
		mpegInterfaceCfg.mpegClkPol = MXL_HYDRA_MPEG_ACTIVE_HIGH;
		mpegInterfaceCfg.mpegClkType = MXL_HYDRA_MPEG_CLK_CONTINUOUS;
		mpegInterfaceCfg.mpegErrorIndication = MXL_HYDRA_MPEG_ERR_INDICATION_DISABLED;
		mpegInterfaceCfg.mpegMode = tsInterfaceMode;
		mpegInterfaceCfg.mpegSyncPol = MXL_HYDRA_MPEG_ACTIVE_HIGH;
		mpegInterfaceCfg.mpegSyncPulseWidth = MXL_HYDRA_MPEG_SYNC_WIDTH_BYTE;
		mpegInterfaceCfg.mpegValidPol = MXL_HYDRA_MPEG_ACTIVE_HIGH;

		for (j = 0; j < maxDemods; j++)
		{
			// Configure TS interface
			mxlStatus = MxLWare_HYDRA_API_CfgMpegOutParams(MXL_DEVICE_ID, (MXL_HYDRA_DEMOD_ID_E)j, &mpegInterfaceCfg);
			if (mxlStatus != MXL_SUCCESS)
			{
				printf("MxLWare_HYDRA_API_CfgMpegOutParams for (%d) - Error (%d)\n", j, mxlStatus);
				return mxlStatus;
			}
			else
				printf("Done! - MxLWare_HYDRA_API_CfgMpegOutParams (%d) \n", j);
		}
	}

	return mxlStatus;
}

MXL_STATUS_E configureLNB(unsigned char lnbId, unsigned char Enable, unsigned char Low_high) {
	MXL_HYDRA_DISEQC_ID_E diseqcId;
	MXL_HYDRA_TUNER_ID_E tunerId;
	MXL_BOOL_E continuousToneCtrl = MXL_OFF;
	MXL_BOOL_E tunerEnable = MXL_OFF;
	MXL_HYDRA_DISEQC_OPMODE_E opMode = MXL_HYDRA_DISEQC_TONE_MODE; //MXL_HYDRA_DISEQC_ENVELOPE_MODE;//
	MXL_HYDRA_DISEQC_VER_E version = MXL_HYDRA_DISEQC_DISABLE; // MXL_HYDRA_DISEQC_1_X; //
	MXL_HYDRA_DISEQC_CARRIER_FREQ_E carrierFreqInHz = MXL_HYDRA_DISEQC_CARRIER_FREQ_22KHZ;
	MXL_STATUS_E mxlStatus;

	switch (lnbId) {
		case 0: diseqcId = MXL_HYDRA_DISEQC_ID_0; break;
		case 1: diseqcId = MXL_HYDRA_DISEQC_ID_1; break;
		case 2: diseqcId = MXL_HYDRA_DISEQC_ID_2; break;
		case 3: diseqcId = MXL_HYDRA_DISEQC_ID_3; break;
		default: diseqcId = MXL_HYDRA_DISEQC_ID_0; break;
	}

	switch (lnbId) {
		case 0: tunerId = MXL_HYDRA_TUNER_ID_0; break;
		case 1: tunerId = MXL_HYDRA_TUNER_ID_1; break;
		case 2: tunerId = MXL_HYDRA_TUNER_ID_2; break;
		case 3: tunerId = MXL_HYDRA_TUNER_ID_3; break;
		default: tunerId = MXL_HYDRA_TUNER_ID_0; break;
	}

	if (Low_high == 1)continuousToneCtrl = MXL_ON;
	else continuousToneCtrl = MXL_OFF;
	if (Enable == 1)tunerEnable = MXL_ON;
	else tunerEnable = MXL_OFF;

	mxlStatus = MxLWare_HYDRA_API_CfgTunerEnable(MXL_DEVICE_ID, tunerId, tunerEnable);

	mxlStatus = MxLWare_HYDRA_API_ResetFsk(MXL_DEVICE_ID);
	mxlStatus = MxLWare_HYDRA_API_CfgDiseqcOpMode(MXL_DEVICE_ID, diseqcId, opMode, version, carrierFreqInHz);
	mxlStatus = MxLWare_HYDRA_API_CfgDiseqcContinuousToneCtrl(MXL_DEVICE_ID, diseqcId, continuousToneCtrl);

	return mxlStatus;
}

MXL_STATUS_E getLNB(unsigned char lnbId, unsigned char *Enable, unsigned char *low_high) {
	MXL_HYDRA_DISEQC_ID_E diseqcId;
	MXL_HYDRA_TUNER_ID_E tunerId;
	MXL_BOOL_E tunerEnable = MXL_OFF;
	MXL_HYDRA_DISEQC_OPMODE_E opMode = MXL_HYDRA_DISEQC_TONE_MODE; //MXL_HYDRA_DISEQC_ENVELOPE_MODE;//
	MXL_HYDRA_DISEQC_VER_E version = MXL_HYDRA_DISEQC_DISABLE; // MXL_HYDRA_DISEQC_1_X; //
	MXL_HYDRA_DISEQC_CARRIER_FREQ_E carrierFreqInHz = MXL_HYDRA_DISEQC_CARRIER_FREQ_22KHZ;
	MXL_STATUS_E mxlStatus;

	switch (lnbId) {
		case 0: diseqcId = MXL_HYDRA_DISEQC_ID_0; break;
		case 1: diseqcId = MXL_HYDRA_DISEQC_ID_1; break;
		case 2: diseqcId = MXL_HYDRA_DISEQC_ID_2; break;
		case 3: diseqcId = MXL_HYDRA_DISEQC_ID_3; break;
		default : diseqcId = MXL_HYDRA_DISEQC_ID_0; break;
	}

	switch (lnbId) {
		case 0: tunerId = MXL_HYDRA_TUNER_ID_0; break;
		case 1: tunerId = MXL_HYDRA_TUNER_ID_1; break;
		case 2: tunerId = MXL_HYDRA_TUNER_ID_2; break;
		case 3: tunerId = MXL_HYDRA_TUNER_ID_3; break;
		default: tunerId = MXL_HYDRA_TUNER_ID_0; break;
	}

	mxlStatus = MxLWare_HYDRA_API_ReqTunerEnableStatus(MXL_DEVICE_ID, tunerId, &tunerEnable);
	if (tunerEnable == MXL_ON) *Enable = 1;
	else *Enable = 0;

	*low_high = 0;

	return mxlStatus;
}

MXL_STATUS_E setTuner(unsigned char demod_id, unsigned char lnb_id, unsigned char standard,
	unsigned int freq, unsigned int rate, unsigned char modulation, unsigned char fec, unsigned char rolloff, unsigned char pilots,
	unsigned char spectrum, unsigned int scr_index, unsigned int search_range) {

	MXL_HYDRA_DEMOD_ID_E demodId;
	MXL_HYDRA_TUNER_ID_E tunerId;
	MXL_HYDRA_TUNE_PARAMS_T chanTuneParamsPtr;
	MXL_HYDRA_SPECTRUM_E spectrumInfo;
	MXL_HYDRA_BCAST_STD_E standardMask;
	MXL_HYDRA_FEC_E fecInfo;
	MXL_HYDRA_MODULATION_E modInfo;
	MXL_HYDRA_PILOTS_E pilotsInfo;
	MXL_HYDRA_ROLLOFF_E rollOffInfo;
	MXL_STATUS_E mxlStatus = MXL_SUCCESS;


	switch (demod_id) {
		case 0: demodId = MXL_HYDRA_DEMOD_ID_0; break;
		case 1: demodId = MXL_HYDRA_DEMOD_ID_1; break;
		case 2: demodId = MXL_HYDRA_DEMOD_ID_2; break;
		case 3: demodId = MXL_HYDRA_DEMOD_ID_3; break;
		case 4: demodId = MXL_HYDRA_DEMOD_ID_4; break;
		case 5: demodId = MXL_HYDRA_DEMOD_ID_5; break;
		case 6: demodId = MXL_HYDRA_DEMOD_ID_6; break;
		case 7: demodId = MXL_HYDRA_DEMOD_ID_7; break;
		default: demodId = MXL_HYDRA_DEMOD_ID_0; break;
	}

	switch (lnb_id) {
		case 0: tunerId = MXL_HYDRA_TUNER_ID_0; break;
		case 1: tunerId = MXL_HYDRA_TUNER_ID_1; break;
		case 2: tunerId = MXL_HYDRA_TUNER_ID_2; break;
		case 3: tunerId = MXL_HYDRA_TUNER_ID_3; break;
		default: tunerId = MXL_HYDRA_TUNER_ID_0; break;
	}

	switch (spectrum) {
		case 0: spectrumInfo = MXL_HYDRA_SPECTRUM_AUTO; break;
		case 1: spectrumInfo = MXL_HYDRA_SPECTRUM_INVERTED; break;
		case 2: spectrumInfo = MXL_HYDRA_SPECTRUM_NON_INVERTED; break;
		default: spectrumInfo = MXL_HYDRA_SPECTRUM_AUTO; break;
	}

	switch (fec) {
		case 0: fecInfo = MXL_HYDRA_FEC_AUTO; break;
		case 1: fecInfo = MXL_HYDRA_FEC_1_2; break;
		case 2: fecInfo = MXL_HYDRA_FEC_3_5; break;
		case 3: fecInfo = MXL_HYDRA_FEC_2_3; break;
		case 4: fecInfo = MXL_HYDRA_FEC_3_4; break;
		case 5: fecInfo = MXL_HYDRA_FEC_4_5; break;
		case 6: fecInfo = MXL_HYDRA_FEC_5_6; break;
		case 7: fecInfo = MXL_HYDRA_FEC_6_7; break;
		case 8: fecInfo = MXL_HYDRA_FEC_7_8; break;
		case 9: fecInfo = MXL_HYDRA_FEC_8_9; break;
		case 10: fecInfo = MXL_HYDRA_FEC_9_10; break;
		default: fecInfo = MXL_HYDRA_FEC_AUTO; break;
	}

	switch (modulation) {
		case 0: modInfo = MXL_HYDRA_MOD_AUTO; break;
		case 1: modInfo = MXL_HYDRA_MOD_QPSK; break;
		case 2: modInfo = MXL_HYDRA_MOD_8PSK; break;
		default: modInfo = MXL_HYDRA_MOD_AUTO; break;
	}

	switch (pilots) {
		case 0: pilotsInfo = MXL_HYDRA_PILOTS_OFF; break;
		case 1: pilotsInfo = MXL_HYDRA_PILOTS_ON; break;
		case 2: pilotsInfo = MXL_HYDRA_PILOTS_AUTO; break;
		default: pilotsInfo = MXL_HYDRA_PILOTS_AUTO; break;
	}

	switch (rolloff) {
		case 0: rollOffInfo = MXL_HYDRA_ROLLOFF_AUTO; break;
		case 1: rollOffInfo = MXL_HYDRA_ROLLOFF_0_20; break;
		case 2: rollOffInfo = MXL_HYDRA_ROLLOFF_0_25; break;
		case 3: rollOffInfo = MXL_HYDRA_ROLLOFF_0_35; break;
		default: rollOffInfo = MXL_HYDRA_ROLLOFF_AUTO; break;
	}


	switch (standard) {
		case 0: standardMask = MXL_HYDRA_DSS;
				chanTuneParamsPtr.params.paramsDSS.fec = fecInfo;
				chanTuneParamsPtr.params.paramsDSS.rollOff = rollOffInfo;
				break;
		case 1: standardMask = MXL_HYDRA_DVBS;
				chanTuneParamsPtr.params.paramsS.fec = fecInfo;
				chanTuneParamsPtr.params.paramsS.modulation = modInfo;
				chanTuneParamsPtr.params.paramsS.rollOff = rollOffInfo;
				break;
		case 2: standardMask = MXL_HYDRA_DVBS2;
				chanTuneParamsPtr.params.paramsS2.fec = fecInfo;
				chanTuneParamsPtr.params.paramsS2.modulation = modInfo;
				chanTuneParamsPtr.params.paramsS2.pilots = pilotsInfo;
				chanTuneParamsPtr.params.paramsS2.rollOff = rollOffInfo;
				break;
		default: standardMask = MXL_HYDRA_DSS;
				chanTuneParamsPtr.params.paramsDSS.fec = fecInfo;
				chanTuneParamsPtr.params.paramsDSS.rollOff = rollOffInfo;
				break;
	}

	chanTuneParamsPtr.frequencyInHz = freq;
	chanTuneParamsPtr.symbolRateKSps = rate / 1000;
	chanTuneParamsPtr.freqSearchRangeKHz = search_range / 1000;
	chanTuneParamsPtr.spectrumInfo = spectrumInfo;
	chanTuneParamsPtr.standardMask = standardMask;

	mxlStatus = MxLWare_HYDRA_API_CfgDemodChanTune(MXL_DEVICE_ID, tunerId, demodId, &chanTuneParamsPtr);

	return mxlStatus;
}

MXL_STATUS_E getTuneInfo(unsigned char demod_id, unsigned char *locked, unsigned char *standard,
	unsigned int *freq, unsigned int *rate, unsigned char *modulation, unsigned char *fec, unsigned char *rolloff, unsigned char *pilots,
	unsigned char *spectrum, unsigned int *RxPwr, unsigned int *SNR) {
	MXL_HYDRA_TUNE_PARAMS_T demodTuneParamsPtr;
	MXL_HYDRA_DEMOD_ID_E demodId;
	MXL_BOOL_E tunerEnable = MXL_OFF;
	MXL_STATUS_E mxlStatus;

	MXL_HYDRA_DEMOD_LOCK_T demodLockPtr;
	SINT32 inputPwrLevelPtr;
	MXL_HYDRA_DEMOD_SIG_OFFSET_INFO_T demodSigOffsetInfoPtr;
	SINT16 snrPtr;
	MXL_HYDRA_DEMOD_SCALED_BER_T scaledBer;
	MXL_HYDRA_DEMOD_STATUS_T demodStatusPtr;
	MXL_HYDRA_FEC_E fecInfo;
	MXL_HYDRA_ROLLOFF_E rollOffInfo;
	MXL_HYDRA_MODULATION_E modInfo;
	MXL_HYDRA_PILOTS_E pilotsInfo;

	switch (demod_id) {
		case 0: demodId = MXL_HYDRA_DEMOD_ID_0; break;
		case 1: demodId = MXL_HYDRA_DEMOD_ID_1; break;
		case 2: demodId = MXL_HYDRA_DEMOD_ID_2; break;
		case 3: demodId = MXL_HYDRA_DEMOD_ID_3; break;
		case 4: demodId = MXL_HYDRA_DEMOD_ID_4; break;
		case 5: demodId = MXL_HYDRA_DEMOD_ID_5; break;
		case 6: demodId = MXL_HYDRA_DEMOD_ID_6; break;
		case 7: demodId = MXL_HYDRA_DEMOD_ID_7; break;
		default: demodId = MXL_HYDRA_DEMOD_ID_0; break;
	}

	MxLWare_HYDRA_API_ReqDemodLockStatus(MXL_DEVICE_ID, demodId, &demodLockPtr);
	if (demodLockPtr.fecLock == MXL_ENABLE) *locked = 1;
	else *locked = 0;

	mxlStatus = MxLWare_HYDRA_API_ReqDemodChanParams(MXL_DEVICE_ID, demodId, &demodTuneParamsPtr);


	MxLWare_HYDRA_API_ReqDemodSignalOffsetInfo(MXL_DEVICE_ID, demodId, &demodSigOffsetInfoPtr);


	MxLWare_HYDRA_API_ReqDemodRxPowerLevel(MXL_DEVICE_ID, demodId, &inputPwrLevelPtr);
	*RxPwr = (unsigned int)inputPwrLevelPtr;

	MxLWare_HYDRA_API_ReqDemodSNR(MXL_DEVICE_ID, demodId, &snrPtr);
	*SNR = (unsigned int)snrPtr;

	MxLWare_HYDRA_API_ReqDemodScaledBER(MXL_DEVICE_ID, demodId, &scaledBer);
	MxLWare_HYDRA_API_ReqDemodErrorCounters(MXL_DEVICE_ID, demodId, &demodStatusPtr);

	*freq = demodTuneParamsPtr.frequencyInHz - demodSigOffsetInfoPtr.carrierOffsetInHz;
	*rate = demodTuneParamsPtr.symbolRateKSps * 1000 - demodSigOffsetInfoPtr.symbolOffsetInSymbol;

	switch (demodTuneParamsPtr.spectrumInfo){
		case MXL_HYDRA_SPECTRUM_INVERTED : *spectrum = 1; break;
		case MXL_HYDRA_SPECTRUM_NON_INVERTED : *spectrum = 2; break;
		default: *spectrum = 0; break;
	}

	switch (demodStatusPtr.standardMask) {
		case MXL_HYDRA_DVBS:
			*standard = 1;
			fecInfo = demodTuneParamsPtr.params.paramsS.fec;
			rollOffInfo = demodTuneParamsPtr.params.paramsS.rollOff;
			modInfo = demodTuneParamsPtr.params.paramsS.modulation;
			pilotsInfo = MXL_HYDRA_PILOTS_OFF;
			break;
		case MXL_HYDRA_DVBS2:
			*standard = 2;
			fecInfo = demodTuneParamsPtr.params.paramsS2.fec;
			rollOffInfo = demodTuneParamsPtr.params.paramsS2.rollOff;
			modInfo = demodTuneParamsPtr.params.paramsS2.modulation;
			pilotsInfo = demodTuneParamsPtr.params.paramsS2.pilots;
			break;
		default :
			*standard = 0;
			fecInfo = demodTuneParamsPtr.params.paramsDSS.fec;
			rollOffInfo = demodTuneParamsPtr.params.paramsDSS.rollOff;
			modInfo = MXL_HYDRA_MOD_AUTO;
			pilotsInfo = MXL_HYDRA_PILOTS_OFF;
			break;
	}

	switch (fecInfo) {
		case MXL_HYDRA_FEC_AUTO : *fec = 0; break;
		case MXL_HYDRA_FEC_1_2 :  *fec = 1; break;
		case MXL_HYDRA_FEC_3_5 :  *fec = 2; break;
		case MXL_HYDRA_FEC_2_3 :  *fec = 3; break;
		case MXL_HYDRA_FEC_3_4 :  *fec = 4; break;
		case MXL_HYDRA_FEC_4_5 :  *fec = 5; break;
		case MXL_HYDRA_FEC_5_6 :  *fec = 6; break;
		case MXL_HYDRA_FEC_6_7 :  *fec = 7; break;
		case MXL_HYDRA_FEC_7_8 :  *fec = 8; break;
		case MXL_HYDRA_FEC_8_9 :  *fec = 9; break;
		case MXL_HYDRA_FEC_9_10:  *fec = 10; break;
		default: *fec = 0; break;
	}

	switch (rollOffInfo) {
		case MXL_HYDRA_ROLLOFF_AUTO: *rolloff = 0; break;
		case MXL_HYDRA_ROLLOFF_0_20: *rolloff = 1; break;
		case MXL_HYDRA_ROLLOFF_0_25: *rolloff = 2; break;
		case MXL_HYDRA_ROLLOFF_0_35: *rolloff = 3; break;
		default: *rolloff = 0; break;
	}

	switch (modInfo) {
		case MXL_HYDRA_MOD_AUTO:  *modulation = 0; break;
		case MXL_HYDRA_MOD_QPSK:   *modulation = 1; break;
		case MXL_HYDRA_MOD_8PSK: *modulation = 2; break;
		default: *modulation = 0; break;
	}

	switch (pilotsInfo) {
		case MXL_HYDRA_PILOTS_OFF:  *pilots = 0; break;
		case MXL_HYDRA_PILOTS_ON:   *pilots = 1; break;
		case MXL_HYDRA_PILOTS_AUTO: *pilots = 2; break;
		default: *pilots = 0; break;
	}

	return mxlStatus;
}

MXL_STATUS_E setTuneOff(unsigned char demod_id) {
	MXL_HYDRA_DEMOD_ID_E demodId;
	MXL_STATUS_E mxlStatus;

	switch (demod_id) {
	case 0: demodId = MXL_HYDRA_DEMOD_ID_0; break;
	case 1: demodId = MXL_HYDRA_DEMOD_ID_1; break;
	case 2: demodId = MXL_HYDRA_DEMOD_ID_2; break;
	case 3: demodId = MXL_HYDRA_DEMOD_ID_3; break;
	case 4: demodId = MXL_HYDRA_DEMOD_ID_4; break;
	case 5: demodId = MXL_HYDRA_DEMOD_ID_5; break;
	case 6: demodId = MXL_HYDRA_DEMOD_ID_6; break;
	case 7: demodId = MXL_HYDRA_DEMOD_ID_7; break;
	default: demodId = MXL_HYDRA_DEMOD_ID_0; break;
	}

	mxlStatus = MxLWare_HYDRA_API_CfgDemodDisable(MXL_DEVICE_ID , demodId);

	return mxlStatus;
}

MXL_STATUS_E setMpegMode(unsigned char demod_id, unsigned char enable,
	unsigned char clk_type,	unsigned char clock_pol, unsigned char clk_rate, unsigned char clk_phase,
	unsigned char msb_first, unsigned char sync_byte_pulse, unsigned char valid_pol, unsigned char sync_pol,
	unsigned char mode, unsigned char error_indic) {
	MXL_HYDRA_DEMOD_ID_E demodId;
	MXL_STATUS_E mxlStatus;
	MXL_HYDRA_MPEGOUT_PARAM_T mpegOutParamPtr;

	switch (demod_id) {
		case 0: demodId = MXL_HYDRA_DEMOD_ID_0; break;
		case 1: demodId = MXL_HYDRA_DEMOD_ID_1; break;
		case 2: demodId = MXL_HYDRA_DEMOD_ID_2; break;
		case 3: demodId = MXL_HYDRA_DEMOD_ID_3; break;
		case 4: demodId = MXL_HYDRA_DEMOD_ID_4; break;
		case 5: demodId = MXL_HYDRA_DEMOD_ID_5; break;
		case 6: demodId = MXL_HYDRA_DEMOD_ID_6; break;
		case 7: demodId = MXL_HYDRA_DEMOD_ID_7; break;
		default: demodId = MXL_HYDRA_DEMOD_ID_0; break;
	}

	if (enable) mpegOutParamPtr.enable = MXL_ENABLE;
	else mpegOutParamPtr.enable = MXL_DISABLE;

	switch (clk_type) {
		case 0:  mpegOutParamPtr.mpegClkType = MXL_HYDRA_MPEG_CLK_CONTINUOUS; break;
		case 1:  mpegOutParamPtr.mpegClkType = MXL_HYDRA_MPEG_CLK_GAPPED; break;
		default: mpegOutParamPtr.mpegClkType = MXL_HYDRA_MPEG_CLK_CONTINUOUS; break;
	}

	switch (clock_pol){
		case 0: mpegOutParamPtr.mpegClkPol = MXL_HYDRA_MPEG_CLK_IN_PHASE; break;
		case 1: mpegOutParamPtr.mpegClkPol = MXL_HYDRA_MPEG_CLK_INVERTED; break;
		default: mpegOutParamPtr.mpegClkPol = MXL_HYDRA_MPEG_CLK_IN_PHASE; break;
	}

	mpegOutParamPtr.maxMpegClkRate = clk_rate;

	switch (clk_phase) {
		case 0: mpegOutParamPtr.mpegClkPhase = MXL_HYDRA_MPEG_CLK_PHASE_SHIFT_0_DEG; break;
		case 1: mpegOutParamPtr.mpegClkPhase = MXL_HYDRA_MPEG_CLK_PHASE_SHIFT_90_DEG; break;
		case 2: mpegOutParamPtr.mpegClkPhase = MXL_HYDRA_MPEG_CLK_PHASE_SHIFT_180_DEG; break;
		case 3: mpegOutParamPtr.mpegClkPhase = MXL_HYDRA_MPEG_CLK_PHASE_SHIFT_270_DEG; break;
		default : mpegOutParamPtr.mpegClkPhase = MXL_HYDRA_MPEG_CLK_PHASE_SHIFT_0_DEG; break;
	}

	switch (valid_pol){
		case 0: mpegOutParamPtr.mpegValidPol = MXL_HYDRA_MPEG_ACTIVE_LOW; break;
		case 1: mpegOutParamPtr.mpegValidPol = MXL_HYDRA_MPEG_ACTIVE_HIGH; break;
		default: mpegOutParamPtr.mpegValidPol = MXL_HYDRA_MPEG_ACTIVE_LOW; break;
	}

	switch (sync_pol){
		case 0: mpegOutParamPtr.mpegSyncPol = MXL_HYDRA_MPEG_ACTIVE_LOW; break;
		case 1: mpegOutParamPtr.mpegSyncPol = MXL_HYDRA_MPEG_ACTIVE_HIGH; break;
		default: mpegOutParamPtr.mpegSyncPol = MXL_HYDRA_MPEG_ACTIVE_LOW; break;
	}

	if (msb_first == 1) {
		mpegOutParamPtr.lsbOrMsbFirst = MXL_HYDRA_MPEG_SERIAL_MSB_1ST;
	} else {
		mpegOutParamPtr.lsbOrMsbFirst = MXL_HYDRA_MPEG_SERIAL_LSB_1ST;
	}

	if (sync_byte_pulse == 1) {
		mpegOutParamPtr.mpegSyncPulseWidth = MXL_HYDRA_MPEG_SYNC_WIDTH_BYTE;
	} else {
		mpegOutParamPtr.mpegSyncPulseWidth = MXL_HYDRA_MPEG_SYNC_WIDTH_BIT;
	}

	switch (mode) {
		case 0: mpegOutParamPtr.mpegMode = MXL_HYDRA_MPEG_MODE_SERIAL_4_WIRE; break;
		case 1: mpegOutParamPtr.mpegMode = MXL_HYDRA_MPEG_MODE_SERIAL_3_WIRE; break;
		case 2: mpegOutParamPtr.mpegMode = MXL_HYDRA_MPEG_MODE_SERIAL_2_WIRE; break;
		case 3: mpegOutParamPtr.mpegMode = MXL_HYDRA_MPEG_MODE_PARALLEL; break;
		default: mpegOutParamPtr.mpegMode = MXL_HYDRA_MPEG_MODE_SERIAL_3_WIRE; break;
	}

	switch (error_indic) {
		case 0: mpegOutParamPtr.mpegErrorIndication = MXL_HYDRA_MPEG_ERR_REPLACE_SYNC; break;
		case 1: mpegOutParamPtr.mpegErrorIndication = MXL_HYDRA_MPEG_ERR_REPLACE_VALID; break;
		case 2: mpegOutParamPtr.mpegErrorIndication = MXL_HYDRA_MPEG_ERR_INDICATION_DISABLED; break;
		default: mpegOutParamPtr.mpegErrorIndication = MXL_HYDRA_MPEG_ERR_INDICATION_DISABLED; break;
	}

	mxlStatus = MxLWare_HYDRA_API_CfgTSMuxMode(MXL_DEVICE_ID, MXL_HYDRA_TS_MUX_DISABLE);

	mxlStatus = MxLWare_HYDRA_API_CfgMpegOutParams(MXL_DEVICE_ID, demodId, &mpegOutParamPtr);

	return mxlStatus;
}

