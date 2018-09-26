# Install script for directory: /home/user/Videos/CONTROLLER_V2_5/pistache-master

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "1")
endif()

if(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/pistache" TYPE FILE FILES
    "/home/user/Videos/CONTROLLER_V2_5/pistache-master/include/prototype.h"
    "/home/user/Videos/CONTROLLER_V2_5/pistache-master/include/client.h"
    "/home/user/Videos/CONTROLLER_V2_5/pistache-master/include/common.h"
    "/home/user/Videos/CONTROLLER_V2_5/pistache-master/include/typeid.h"
    "/home/user/Videos/CONTROLLER_V2_5/pistache-master/include/MxLWare_HYDRA_OEM_Drv.h"
    "/home/user/Videos/CONTROLLER_V2_5/pistache-master/include/optional.h"
    "/home/user/Videos/CONTROLLER_V2_5/pistache-master/include/MxLWare_HYDRA_Registers.h"
    "/home/user/Videos/CONTROLLER_V2_5/pistache-master/include/MaxLinearDataTypes.h"
    "/home/user/Videos/CONTROLLER_V2_5/pistache-master/include/stream.h"
    "/home/user/Videos/CONTROLLER_V2_5/pistache-master/include/MxLWare_HYDRA_DiseqcFskApi.h"
    "/home/user/Videos/CONTROLLER_V2_5/pistache-master/include/MxLWare_HYDRA_CommonApi.h"
    "/home/user/Videos/CONTROLLER_V2_5/pistache-master/include/MxLWare_HYDRA_DemodTunerApi.h"
    "/home/user/Videos/CONTROLLER_V2_5/pistache-master/include/http_headers.h"
    "/home/user/Videos/CONTROLLER_V2_5/pistache-master/include/tcp.h"
    "/home/user/Videos/CONTROLLER_V2_5/pistache-master/include/net.h"
    "/home/user/Videos/CONTROLLER_V2_5/pistache-master/include/mime.h"
    "/home/user/Videos/CONTROLLER_V2_5/pistache-master/include/MxLWare_HYDRA_Commands.h"
    "/home/user/Videos/CONTROLLER_V2_5/pistache-master/include/flags.h"
    "/home/user/Videos/CONTROLLER_V2_5/pistache-master/include/Itoc.h"
    "/home/user/Videos/CONTROLLER_V2_5/pistache-master/include/async.h"
    "/home/user/Videos/CONTROLLER_V2_5/pistache-master/include/MxL_HYDRA_MxLWare.h"
    "/home/user/Videos/CONTROLLER_V2_5/pistache-master/include/iterator_adapter.h"
    "/home/user/Videos/CONTROLLER_V2_5/pistache-master/include/reactor.h"
    "/home/user/Videos/CONTROLLER_V2_5/pistache-master/include/MxLWare_HYDRA_ProductId.h"
    "/home/user/Videos/CONTROLLER_V2_5/pistache-master/include/MxLWare_HYDRA_BERTApi.h"
    "/home/user/Videos/CONTROLLER_V2_5/pistache-master/include/MxLWare_FW_Download.h"
    "/home/user/Videos/CONTROLLER_V2_5/pistache-master/include/http_header.h"
    "/home/user/Videos/CONTROLLER_V2_5/pistache-master/include/callcommand.h"
    "/home/user/Videos/CONTROLLER_V2_5/pistache-master/include/MxLWare_HYDRA_SkuFeatures.h"
    "/home/user/Videos/CONTROLLER_V2_5/pistache-master/include/peer.h"
    "/home/user/Videos/CONTROLLER_V2_5/pistache-master/include/MaxLinearDebug.h"
    "/home/user/Videos/CONTROLLER_V2_5/pistache-master/include/http_defs.h"
    "/home/user/Videos/CONTROLLER_V2_5/pistache-master/include/MxlApiLib.h"
    "/home/user/Videos/CONTROLLER_V2_5/pistache-master/include/upsampler.h"
    "/home/user/Videos/CONTROLLER_V2_5/pistache-master/include/MxL_HYDRA_Diag_Commands.h"
    "/home/user/Videos/CONTROLLER_V2_5/pistache-master/include/http.h"
    "/home/user/Videos/CONTROLLER_V2_5/pistache-master/include/config.h"
    "/home/user/Videos/CONTROLLER_V2_5/pistache-master/include/MxLWare_HYDRA_OEM_Defines.h"
    "/home/user/Videos/CONTROLLER_V2_5/pistache-master/include/endpoint.h"
    "/home/user/Videos/CONTROLLER_V2_5/pistache-master/include/view.h"
    "/home/user/Videos/CONTROLLER_V2_5/pistache-master/include/MxLWare_HYDRA_PhyCtrl.h"
    "/home/user/Videos/CONTROLLER_V2_5/pistache-master/include/timer_pool.h"
    "/home/user/Videos/CONTROLLER_V2_5/pistache-master/include/transport.h"
    "/home/user/Videos/CONTROLLER_V2_5/pistache-master/include/listener.h"
    "/home/user/Videos/CONTROLLER_V2_5/pistache-master/include/dbHandler.h"
    "/home/user/Videos/CONTROLLER_V2_5/pistache-master/include/MxLWare_HYDRA_ChanBondApi.h"
    "/home/user/Videos/CONTROLLER_V2_5/pistache-master/include/MxLWare_HYDRA_TsMuxCtrlApi.h"
    "/home/user/Videos/CONTROLLER_V2_5/pistache-master/include/MxlApiCall.h"
    "/home/user/Videos/CONTROLLER_V2_5/pistache-master/include/description.h"
    "/home/user/Videos/CONTROLLER_V2_5/pistache-master/include/udpipstack.h"
    "/home/user/Videos/CONTROLLER_V2_5/pistache-master/include/cookie.h"
    "/home/user/Videos/CONTROLLER_V2_5/pistache-master/include/router.h"
    "/home/user/Videos/CONTROLLER_V2_5/pistache-master/include/mailbox.h"
    "/home/user/Videos/CONTROLLER_V2_5/pistache-master/include/os.h"
    "/home/user/Videos/CONTROLLER_V2_5/pistache-master/include/MxLWare_HYDRA_DeviceApi.h"
    )
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("/home/user/Videos/CONTROLLER_V2_5/pistache-master/build/src/cmake_install.cmake")
  include("/home/user/Videos/CONTROLLER_V2_5/pistache-master/build/examples/cmake_install.cmake")
  include("/home/user/Videos/CONTROLLER_V2_5/pistache-master/build/googletest-release-1.7.0/cmake_install.cmake")
  include("/home/user/Videos/CONTROLLER_V2_5/pistache-master/build/tests/cmake_install.cmake")

endif()

if(CMAKE_INSTALL_COMPONENT)
  set(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INSTALL_COMPONENT}.txt")
else()
  set(CMAKE_INSTALL_MANIFEST "install_manifest.txt")
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
file(WRITE "/home/user/Videos/CONTROLLER_V2_5/pistache-master/build/${CMAKE_INSTALL_MANIFEST}"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
