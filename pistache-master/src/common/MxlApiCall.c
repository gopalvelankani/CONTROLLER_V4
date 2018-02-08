#include <stdio.h>
#include "MxlApiCall.h"
#include "MxlApiLib.h"
void getStatus(){
	MXL_STATUS_E mxlStatus = MXL_SUCCESS;

	MXL_HYDRA_VER_INFO_T versionInfo;
	mxlStatus = MxL_Connect(0X60, 200, 125000);
	printf("Status ==== %d",&mxlStatus);
	if(mxlStatus == MXL_SUCCESS){
	printf("Status Connected");
	}
}

int main()
{
    MXL_STATUS_E mxlStatus = MXL_SUCCESS;

	MXL_HYDRA_VER_INFO_T versionInfo;
	mxlStatus = MxL_Connect(0X60, 200, 125000);
	printf("Status ==== %d",&mxlStatus);
	if(mxlStatus == MXL_SUCCESS){
	printf("Status Connected");
	}
    return 0;
}
