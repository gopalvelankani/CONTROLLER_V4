//MxL_HYDRA_Diag_Commands.h

#ifndef __MXL_HDR_DIAG_CMDS_H__
#define __MXL_HDR_DIAG_CMDS_H__

#include "MaxLinearDataTypes.h"
#include "MxLWare_HYDRA_Commands.h"

#ifdef __cplusplus
extern "C" {
#endif


/************************************************************************************
       MXL500 Macros for LNBs/DiSEqC, Tuners, SWM/FSK
 ************************************************************************************/
#define MXL58x_FFT_NUM_OF_BINS  256

//FSK Message Maximum Length
#define MXL58x_FSK_MSG_MAX_LENGTH   (64)

//DiSEqC MAX Packet size
#define MXL58x_DISEQC_MAX_PKT_SIZE 32

#define MXL58x_VERSION_SIZE                     4

// SPUR and NF settings
#define MAX_NUM_NF_PER_DEMOD                        (3)
/************************************************************************************
       Macros for Demods: Tensilica 106 and Moto M68HC05
 ************************************************************************************/
#define MXL58x_CUSTOM_GOLD_DIM   5

#define MXL58x_MAX_NUM_OF_ACQ_TRACES         256 // how many traces to print via UART

#define MXL58x_HOST_SMALL_NUM_OF_ACQ_TRACES  29  // how many traces can be sent to HOST

#define MXL58x_MAX_TYPE_SELECT_NUMBER        24  // how many diff kinds of strings are allowed to appear in acq trace

#define MXL58x_M68HC05_MAX_READ_BYTE_NUMBER  12  // max number of UBISO mailbox read bytes

#define MXL58x_M68HC05_MAX_WRITE_BYTE_NUMBER  13 // max number of UBISO mailbox write bytes

// MXL58x_DEMOD_SET_BREAKPOINT_T configure how to print M68HC05 registers.
// Host sends MXL58x_DEMOD_SET_BREAKPOINT_T to set 106 on what to print, and what not to print.
// 106 will send MXL58x_DEMOD_GET_DEBUG_T from 05, to 106, finally to UART.
#define MXL58x_M68HC05_SHORT_BREAKPOINT_NUM            4   // M68HC05 supports four single printings.
#define MXL58x_M68HC05_REG_NUM_OF_SHORT_BREAKPOINT     3   // M68HC05 prints 3 registers in each printing
#define MXL58x_M68HC05_REG_NUM_OF_LONG_BREAKPOINT      128 // M68HC05 supports one block printing (
                                                           // many registers are printed in one shot).

#define MXL58x_M68HC05_TOT_BREAKPOINT_NUM  (MXL58x_M68HC05_SHORT_BREAKPOINT_NUM+1) // M68HC05 tot number of breakpoints, i.e. 5

// The following bits force 106 on what to print, and what not to print
#define MXL58x_PRINT_UART_ACQ_TRACE_EN_BIT         (0x00000001) // N/A for host. Self Cleared Bit by FW
#define MXL58x_SEND_HOST_ACQ_TRACE_EN_BIT          (0x00000002) // N/A for host. Self Cleared Bit by FW

#define MXL58x_ECHO_HOST_CMD_EN_BIT                (0x00000004) // Enable Bit of monPrint

#define MXL58x_M68HC05_BREAKPOINT0_EN_BIT          (0x00000008) // Enable Breakpoint 0 (three registers)
#define MXL58x_M68HC05_BREAKPOINT1_EN_BIT          (0x00000010) // Enable Breakpoint 1 (three registers)
#define MXL58x_M68HC05_BREAKPOINT2_EN_BIT          (0x00000020) // Enable Breakpoint 2 (three registers)
#define MXL58x_M68HC05_BREAKPOINT3_EN_BIT          (0x00000040) // Enable Breakpoint 3 (three registers)
#define MXL58x_M68HC05_BREAKPOINT4_EN_BIT          (0x00000080) // Enable Breakpoint 4 (a block of registers)

#define MXL58x_HOST_CFG_BREAKPOINT0_EN_BIT         (0x00000100) // If this bit = 1, Host
                                                                // configures three register addresses
                                                                // of Breakpoint 0. If not, HC05 itself
                                                                // configures Breakpoint 0.
#define MXL58x_HOST_CFG_BREAKPOINT1_EN_BIT         (0x00000200) // If this bit = 1, Host
                                                                // configures three register addresses
                                                                // of Breakpoint 1. If not, HC05 itself
                                                                // configures Breakpoint 1.
#define MXL58x_HOST_CFG_BREAKPOINT2_EN_BIT         (0x00000400) // This bit = 1 or 0, Host
                                                                // configures three register addresses
                                                                // of Breakpoint 0, or HC05 itself
                                                                // configures Breakpoint 0.
#define MXL58x_HOST_CFG_BREAKPOINT3_EN_BIT         (0x00000800) // This bit = 1 or 0, Host
                                                                // configures three register addresses
                                                                // of Breakpoint 0, or HC05 itself
                                                                // configures Breakpoint 0.
#define MXL58x_HOST_CFG_BREAKPOINT4_EN_BIT         (0x00001000) // This bit = 1 or 0, Host
                                                                // configures base address (and num of bytes)
                                                                // of Breakpoint 4, or HC05 itself
                                                                // configures Breakpoint 4.

#define MXL58x_BREAKPOINT_UART_PRINT_EN_BIT        (0x00002000) // Print all breakpoints to UART
#define MXL58x_BREAKPOINT_ONLY_FIRST_TIME_EN_BIT   (0x00004000) // Print ONLY when
                                                                // a breakpoint gets hit for
                                                                // the first-time.
                                                                // If this bit is cleared,
                                                                // print for unlimited times

#define MXL58x_SEND_CONSTANT_PARAM_TO_HOST_EN_BIT  (0x00008000) // Enable to send constants to Host

#define MXL58x_ACQ_TRACE_TO_UART_EN_BIT            (0x00010000) // Enable Bit for printing ACQ Trace.
                                                                // If this bit is disabled, NO acq trace
                                                                // will be printed to UART

#define MXL58x_ACQ_TRACE_TO_HOST_EN_BIT            (0x00020000) // Enable Bit for GUI to get ACQ Trace.
                                                                // If this bit is disabled, NO acq trace
                                                                // will be sent to host.
                                                                // If both MXL58x_ACQ_TRACE_TO_UART_EN_BIT and
                                                                // MXL58x_ACQ_TRACE_TO_HOST_EN_BIT are disabled,
                                                                // T106 will not read acq trace from HC05.

#define MXL58x_FTL_INTR_SERVICE_EN_BIT             (0x00040000) // Bit 5 of "firmware.asm\reg_intr_106_enable; memory allocation label"
                                                                // Enable Bit for 106 to serive a FTL interrupt from HC05.
                                                                // If enabled, Mark's FTL algorithm is run by T106. If disabled, Mark's algorithm is run by HC05.

#define MXL58x_REAL_TIME_LOG_EN_BIT                (0x00080000) // Enable Bit for real-time logging.

#define MXL58x_BYPASS_FTL_EN_BIT                   (0x00100000) // Bit 6 of "firmware.asm\reg_intr_106_enable; memory allocation label"
                                                                // Enable Bit to bypass FTL module. If set, neither 106 nor HC05 will run FTL.

#define MXL58x_BKPT_HALT_M68HC05_EN_BIT            (0x00200000) // Bit 7 of "firmware.asm\reg_intr_106_enable; memory allocation label"
                                                                // Enable Bit to halt HC05 at breakpoint. If set, HC05 will stop at any breakpoint forever.

#define MXL58x_BYPASS_LLR_TAB_EN_BIT               (0x00400000) // Bit 5 of "firmware.asm\reg_intr_106_enable+1"
                                                                // Enable Bit to bypass LLR module. If set, HC05 will bypass LLR, and the T106 will load LLR table.

#define MXL58x_RE_ACQ_DISABLE_BIT                  (0x00800000) // This bit, if set, will turn off Re-Acq in HC05 and T106
                                                                  // Bit 6 of "firmware.asm\reg_intr_106_enable+1"

#define MXL58x_LEGACY_DEMOD_AAF_SETTING_BIT        (0x01000000) // This bit, if set, will use UBISO Legacy AAF setting in HC05.
                                                                // If clear, will use new CommSys AAF settings in DmdGlobalAafTable of DMD_AafCoef.c
                                                                // Bit 7 of "firmware.asm\reg_intr_106_enable+1"

#define MXL58x_LEGACY_NBCSYNC_FREQ_EST_DISABLE_BIT (0x02000000) // Legacy frequency offset estimation using NBC sync detector correlator output. TRUE, new freq est is used. FALSE, legacy freq est is used.

#define MXL58x_NEW_NBCSYNC_FREQ_EST_APPROACH_1_BIT (0x04000000) // New frequency offset estimation using NBC sync detector correlator output. TRUE, approach 1. FALSE, approach 2.

#define MXL58x_DFE_CIC_FIR1_BYPASS_EN_BIT          (0x80000000) // Enable Bit for DFE CIC-FIR1 MODULE BYPASS

// M68HC05 CTL2 mode register
#define MXL58x_DEMOD_M68HC05_CTL2_MODE_ADDR        (0x00F8)     // Its value is one of enum MXL58x_DEMOD_CCI_CTL2_E

// M68HC05 version register addresses
#define MXL58x_DEMOD_M68HC05_MAJOR_VERSION_ADDR    (0x00FC)
#define MXL58x_DEMOD_M68HC05_MINOR_VERSION_ADDR    (0x00FD)


#define DMD_SCRAMBLE_CODE_DATA_SIZE                 12


/************************************************************************************
       Enums
 ************************************************************************************/

  // ID enum
typedef enum
{
  MXL58x_RF_INPUT_ID_0 = 0,
  MXL58x_RF_INPUT_ID_1,
  MXL58x_RF_INPUT_MAX
} MXL58x_RF_INPUT_ID_E;

typedef enum
{
  MXL58x_DISEQC_ID_0 = 0,
  MXL58x_DISEQC_ID_1,
  MXL58x_DISEQC_ID_2,
  MXL58x_DISEQC_ID_3,
  MXL58x_DISEQC_ID_MAX
} MXL58x_DISEQC_ID_E;

#if 0
typedef enum
{
  MXL58x_DISEQC_TONE_NONE = 0,
  MXL58x_DISEQC_TONE_SA,
  MXL58x_DISEQC_TONE_SB
} MXL58x_DISEQC_TONE_CTRL_E;
#endif

typedef  enum
{
  MXL58x_TUNER_ID_0 = 0,
  MXL58x_TUNER_ID_1,
  MXL58x_TUNER_ID_2,
  MXL58x_TUNER_ID_3,
  MXL58x_TUNER_ID_MAX
} MXL58x_TUNER_ID_E;

typedef  enum
{
  MXL58x_DEMOD_ID_0 = 0,
  MXL58x_DEMOD_ID_1,
  MXL58x_DEMOD_ID_2,
  MXL58x_DEMOD_ID_3,
  MXL58x_DEMOD_ID_4,
  MXL58x_DEMOD_ID_5,
  MXL58x_DEMOD_ID_6,
  MXL58x_DEMOD_ID_7,
  MXL58x_DEMOD_ID_MAX,
} MXL58x_DEMOD_ID_E;

// Standard Definition
typedef enum
{
  MXL58x_STANDARD_DIRECTV_LEGACY = 0, // Host diplays N/A if SOC fails to detect standard in blind scan
  MXL58x_STANDARD_DVB_S,              // Host diplays DVB-S
  MXL58x_STANDARD_DVB_S2,             // Host diplays DVB-S2
  MXL58x_STANDARD_NOT_AVAIL,          // Host diplays DirecTV Legacy
  MXL58x_STANDARD_AUTO                // Host can config SOC to auto-detect standard in blind scan
} MXL58x_SAT_STANDARD_E;

// Demod Modulations
typedef enum
{
  MXL58x_DEMOD_MODULATION_NOT_AVAIL = 0,  // Host diplays N/A if SOC fails to detect modulation in blind scan
  MXL58x_DEMOD_MODULATION_QPSK,           // Host diplays QPSK
  MXL58x_DEMOD_MODULATION_8PSK,           // Host diplays 8PSK
  MXL58x_DEMOD_MODULATION_AUTO            // Host configs SOC to auto-detect modulation in blind scan
} MXL58x_DEMOD_MODULATION_E;

// Demod Forward Error Correction
typedef enum
{
  MXL58x_DEMOD_CODE_RATE_NOT_AVAIL = 0, // Host diplays N/A
  MXL58x_DEMOD_CODE_RATE_1_4,           // Host diplays 1/4
  MXL58x_DEMOD_CODE_RATE_1_3,           // Host diplays 1/3
  MXL58x_DEMOD_CODE_RATE_1_2,           // ...
  MXL58x_DEMOD_CODE_RATE_3_5,
  MXL58x_DEMOD_CODE_RATE_2_3,
  MXL58x_DEMOD_CODE_RATE_3_4,
  MXL58x_DEMOD_CODE_RATE_4_5,
  MXL58x_DEMOD_CODE_RATE_5_6,
  MXL58x_DEMOD_CODE_RATE_6_7,
  MXL58x_DEMOD_CODE_RATE_7_8,
  MXL58x_DEMOD_CODE_RATE_8_9,           // ...
  MXL58x_DEMOD_CODE_RATE_9_10,          // Host diplays 9/10
  MXL58x_DEMOD_CODE_RATE_AUTO           // Host configs SOC to auto-scan code rate during blind scan
} MXL58x_DEMOD_CODE_RATE_E;

#if 1


typedef enum
{
  // Message Write/Read Loop-back Testing
  MXL58x_DEV_MSG_LOOP_BACK_TEST_CMD = MXL_HYDRA_LAST_HOST_CMD+1,

  MXL58x_TUNER_SCAN_TUNE_CMD,

  MXL58x_DEMOD_SET_SCAN_PARAM_CMD,

  MXL58x_DEMOD_READ_REG_CMD, // GUI's button "apply" to read reg
  MXL58x_DEMOD_WRITE_REG_CMD, // GUI's button "apply" to write reg

  MXL58x_DEMOD_READ_FTL_CMD, // GUI's button of "FTL read" on demod window
  MXL58x_DEMOD_WRITE_FTL_CMD, // GUI's button of "FTL write" on demod window

  MXL58x_DEMOD_GET_BLOCK_MEM_CMD,  // Customer reads a block of 12 registers to host

  MXL58x_DEMOD_SET_MONINTOR_CMD, // Configure what to print out via UART and what not.

  MXL58x_DEMOD_GET_TRACE_CMD, // GUI's dropdown button of "Get Trace" on demod window
  MXL58x_DEMOD_CLR_TRACE_CMD, // GUI's dropdown button of "Clear Trace" on demod window

  MXL58x_DEMOD_SET_BREAKPOINT_CMD, // Host sends this to configure what 106 and M68HC05 shall print out
  MXL58x_DEMOD_GET_BREAKPOINT_CMD, // 106 sends to host about M68HC05's breakpoint info

  MXL58x_DEMOD_MAILBOX_OP_CMD, // M68HC05 Mailbox operation command

  MXL58x_DEMOD_CCI_CTL2_CMD, // Set up CCI vs. non-CCI CTL2

  //--- SPUR command IDs--
  MXL58x_DBG_CFG_NF_SETTINGS_CMD,
  MXL58x_DBG_REQ_NF_SETTINGS_CMD,
  MXL58x_DBG_RSP_NF_SETTINGS_CMD,

  //--- FFT Debug Command IDs--
  MXL58x_DBG_REQ_FFT_SPECTRUM_CMD,

  MXL58x_DBG_CFG_CHAN_TUNE_CMD,

  MXL58x_DBG_TUNER_TUNE_CMD,

  // Set interrupt type and duration
  MXL58x_DEMOD_INTERRUPT_TYPE_CMD,

  MXL58x_HYDRA_INTR_CLEAR_CMD,

  MXL_HYDRA_DIG_RF_CAPTURE_DATA_CMD,

  MXL_HYDRA_DIG_RF_CFG_CAPTURE_CMD,

  MXL_HYDRA_DIG_RF_INIT_CMD,

  MXL58x_DIG_RF_TEST_CMD,

  MXL58x_SPECTRUM_REQ_CMD,

  MXL58x_DBG_CFG_EQ_SPACE_CMD, // config EQ to be T-spaced or T/2-spaced

  MXL58x_TUNER_ACTIVATE_CMD,
  MXL58x_CFG_POWER_MODE_CMD,
  MXL58x_XTAL_CAP_CMD,

  MXL58x_XPT_FAILSAFE_CMD = 75,

  //---For host to know how many commands in total---
  MXL58x_LAST_FW_CMD

  
} MXL58x_HOST_DBG_CMD_ID_E;





#else

typedef enum
{
  // Message Write/Read Loop-back Testing
  MXL58x_DEV_MSG_LOOP_BACK_TEST_CMD = 0xA0,

  MXL58x_TUNER_SCAN_TUNE_CMD,

  MXL58x_DEMOD_SET_SCAN_PARAM_CMD,

  MXL58x_DEMOD_READ_REG_CMD, // GUI's button "apply" to read reg
  MXL58x_DEMOD_WRITE_REG_CMD, // GUI's button "apply" to write reg

  MXL58x_DEMOD_READ_FTL_CMD, // GUI's button of "FTL read" on demod window
  MXL58x_DEMOD_WRITE_FTL_CMD, // GUI's button of "FTL write" on demod window

  MXL58x_DEMOD_GET_BLOCK_MEM_CMD,  // Customer reads a block of 12 registers to host

  MXL58x_DEMOD_SET_MONINTOR_CMD, // Configure what to print out via UART and what not.

  MXL58x_DEMOD_GET_TRACE_CMD, // GUI's dropdown button of "Get Trace" on demod window
  MXL58x_DEMOD_CLR_TRACE_CMD, // GUI's dropdown button of "Clear Trace" on demod window

  MXL58x_DEMOD_SET_BREAKPOINT_CMD, // Host sends this to configure what 106 and M68HC05 shall print out
  MXL58x_DEMOD_GET_BREAKPOINT_CMD, // 106 sends to host about M68HC05's breakpoint info

  MXL58x_DEMOD_MAILBOX_OP_CMD, // M68HC05 Mailbox operation command

  MXL58x_DEMOD_CCI_CTL2_CMD, // Set up CCI vs. non-CCI CTL2

  //--- SPUR command IDs--
  MXL58x_DBG_CFG_NF_SETTINGS_CMD,
  MXL58x_DBG_REQ_NF_SETTINGS_CMD,
  MXL58x_DBG_RSP_NF_SETTINGS_CMD,

  //--- FFT Debug Command IDs--
  MXL58x_DBG_REQ_FFT_SPECTRUM_CMD,

  MXL58x_DBG_CFG_CHAN_TUNE_CMD,

  MXL58x_DBG_TUNER_TUNE_CMD,

  MXL_HYDRA_DIG_RF_CAPTURE_DATA_CMD,

  MXL_HYDRA_DIG_RF_CFG_CAPTURE_CMD,

  MXL58x_DIG_RF_TEST_CMD,

  //---For host to know how many commands in total---
  MXL58x_LAST_FW_CMD

} MXL58x_HOST_DBG_CMD_ID_E;

#endif


// For GUI manual sampling freq
typedef struct
{
  UINT32 demodId;
  UINT32 manualSfInHz;  /* Manual set sampling freq for test only */

} MXL58x_DBG_CHAN_TUNE_PARAMS_T; // Debug only

// FSK enum
typedef enum
{
  MXL58x_PWR_MODE_STANDBY_ON   = 0,
  MXL58x_PWR_MODE_SLEEP_ON,
  MXL58x_PWR_MODE_STANDBY_OFF,
  MXL58x_PWR_MODE_SLEEP_OFF,
} MXL58x_PWR_MODE_E;

typedef enum
{
  MXL58x_FSK_CONFIGURE_TYPE_DIRECTV = 0,
  MXL58x_FSK_CONFIGURE_TYPE_SKYITALIA_39, // SkyItalia 39.017kbps
  MXL58x_FSK_CONFIGURE_TYPE_SKYITALIA_115 // SkyItalia 115.2kbps
} MXL58x_FSK_OP_MODE_E;

//Tuner enum
typedef struct
{
  UINT32 tunerId;

  UINT32 centerFrequencyInKHz; // Center Frequency in kHz within L-band.
  UINT32 freqOffsetRangeInKHz; // Tuner will tune to a range of [Center Freq – freqOffsetRangeInKHz, Center Freq + freqOffsetRangeInKHz]
} MXL58x_TUNER_TUNE_T;

// Demod enum
typedef enum
{
  MXL58x_BLIND_SCAN = 0,
  MXL58x_COLD_SCAN,
  MXL58x_WARM_SCAN
} MXL58x_CH_SCAN_MODE_E;

typedef enum
{
  MXL58x_DEMOD_UNLOCKED = 0,        /* Signal acquisition is unlocked */
  MXL58x_DEMOD_LOCKED,              /* Signal acquisition is locked   */
  MXL58x_DEMOD_ACQUIRING,           /* Signal acquisition is on-going */
  MXL58x_DEMOD_ACQ_FAIL             // Fail acq
} MXL58x_DEMOD_LOCK_STATUS_E;

typedef enum // ACQ states for a lookup table of strings
{
  MXL58x_DEMOD_ACQ_IS_LOCKED     = 0,      /* 0x00    ; => Successful acquisition            */
  MXL58x_DEMOD_ACQ_INIT_FEC_DONE = 3,            /* 0x03    ; => Start setup for locked mode      */
  MXL58x_DEMOD_ACQ_INIT_CTL_DONE,                  /* 0x04    ; => Start Init FEC                        */
  MXL58x_DEMOD_ACQ_8PSK_CTL,                        /* 0x05    ; => Start Init CTL                        */
  MXL58x_DEMOD_ACQ_QPSK_CTL,                        /* 0x06    ; => Start Init CTL                        */
  MXL58x_DEMOD_ACQ_8PSK_CTL_EST,                  /* 0x07    ; => Start Init CTL                        */
  MXL58x_DEMOD_ACQ_QPSK_CTL_EST,                  /* 0x08    ; => Start Init CTL                        */
  MXL58x_DEMOD_ACQ_INIT_EQU_DONE,                  /* 0x09    ; => Start Init CTL                        */
  MXL58x_DEMOD_ACQ_INIT_STR_DONE,                  /* 0x0A    ; => Start Init Equaliser            */
  MXL58x_DEMOD_ACQ_ALIAS_DONE,                        /* 0x0B    ; => Start Init STR                        */
  MXL58x_DEMOD_ACQ_FTL_LOOP_DONE,                  /* 0x0C    ; => Start Anti alias                  */
  MXL58x_DEMOD_ACQ_SPECTRUM_ANALYSER,      /* 0x0D    ; only when search ON                  */
  MXL58x_DEMOD_ACQ_TUNER_AGC_DONE,                  /* 0x0E    ; => Start FTL                              */
  MXL58x_DEMOD_ACQ_NEW_BEGIN,              /* 0x0F    ; => Process acq params                  */
  // Acq failed states (ACQ_FAIL_BIT = 1 )
  MXL58x_DEMOD_ACQ_RETRY         =16,            /* 0x10 */
  MXL58x_DEMOD_ACQ_INIT_FEC_FAIL =24,            /* 0x18 */
  MXL58x_DEMOD_ACQ_STR_LOOP_FAIL,                  /* 0x19 */
  MXL58x_DEMOD_ACQ_FAILED,                              /* 0x1A */
  MXL58x_DEMOD_ACQ_LEG_FEC_FAIL,           /* 0x1B */
  MXL58x_DEMOD_ACQ_DVBS2_FEC_FAIL,         /* 0x1C */
  MXL58x_DEMOD_ACQ_DVBS2_STR_FAIL,         /* 0x1D */
  MXL58x_DEMOD_ACQ_INIT_CTL_FAIL,          /* 0x1E */
  MXL58x_DEMOD_ACQ_TOO_MUCH_ERRORS,        /* 0x1F    ; MONITOR count >8 errors in 9 blocks */
  // Acq not started states (ACQ_PENDING_BIT = 1)
  MXL58x_DEMOD_ACQ_RESET_CPU         =32,  /* 0x20 */
  MXL58x_DEMOD_ACQ_NEW_TUNE,               /* 0x21    ; New tuner loaded waiting for new acq... */
} MXL58x_DEMOD_ACQ_STATE_TYPE_E;

// Enum of T106 states of handling HC05 breakpoints
typedef enum
{
  MXL58x_READY_TO_SERVICE_M68HC05_BREAKPOINT = 0, // When READY, T106 accepts new HC05 INTR
  MXL58x_BUSY_TO_SERVICE_M68HC05_BREAKPOINT       // When BUSY, T106 drops HC05 INTR
} MXL58x_SERVICE_M68HC05_BREAKPOINT_STATE_E;

// Enum of priority levels of HC05 breakpoints
typedef enum
{
  MXL58x_POST_DEBUG_LEVEL_1 = 1,  // minimum
  MXL58x_POST_DEBUG_LEVEL_2,
  MXL58x_POST_DEBUG_LEVEL_3,
  MXL58x_POST_DEBUG_LEVEL_4,
  MXL58x_POST_NORMAL_LEVEL_1 = 2,
  MXL58x_POST_NORMAL_LEVEL_2,
  MXL58x_POST_NORMAL_LEVEL_3,
  MXL58x_POST_NORMAL_LEVEL_4      // maximum
} MXL58x_MSG_POST_QUEUE_PRIORITY_E;

// The following enum is tied with a command MXL58x_DEMOD_SET_ERROR_COUNTER_MODE_CMD
// to config the mode of the output of RS error counter. The output is to be shown on GUI.
typedef enum
{
  MXL58x_DVBS2_CRC_ERROR_COUNTER = 0, // DVB-S2 has only one mode of error counter

  MXL58x_DVBS_RS1_CORRECTED_COUNTER = 0, // DVB-S corrected error counter after 1st-iteration RS decoding.
  MXL58x_DVBS_RS1_UNCORRECTED_COUNTER, // DVB-S uncorrected error counter after 1st-iteration RS decoding.
  MXL58x_DVBS_RS2_CORRECTED_COUNTER, // DVB-S corrected errors after 2nd-iteration RS decoding.
  MXL58x_DVBS_RS2_UNCORRECTED_COUNTER // DVB-S uncorrected errors after 2nd-iteration RS decoding.
} MXL58x_ERROR_COUNTER_OUTPUT_E;

// MXL58x_DVBS_ITER_RS_OUT_SEL_E defines the output to mpeg TS: whether 1st RS or 2nd RS
typedef enum
{
  MXL58x_DVBS_ITER_RS_OUT_1ST = 0, // 1st RS out connects Mpeg TS
  MXL58x_DVBS_ITER_RS_OUT_2ND      // 2nd RS
} MXL58x_DVBS_ITER_RS_OUT_SEL_E;

// MXL58x_DEMOD_CCI_CTL2_CMD command will config "CCI CTL2"
typedef enum
{
  MXL58x_DEMOD_CTL2_DISABLED = 0,  // GUI displays "CTL2 DISABLED": all modes are same
  MXL58x_DEMOD_CTL2_NO_CCI_1, // Display "No CCI 1": special mode 11, different from modes 12-14. This option should be default value on GUI dropdown menu.
  MXL58x_DEMOD_CTL2_NO_CCI_2, // Display "No CCI 2": modes 11-14 are same.
  MXL58x_DEMOD_CTL2_CCI, // Display "CCI"
  MXL58x_DEMOD_CTL2_AUTO_CCI_CONT_DETECT, // Display "AUTO_CONT_DETECT": PHY automatically detects CCI vs non-CCI forever.
  MXL58x_DEMOD_CTL2_AUTO_CCI_1ST_TIME_DETECT, // Display "AUTO_1ST_TIME_DETECT": PHY automatically detects CCI vs non-CCI for one time.
} MXL58x_DEMOD_CCI_CTL2_E;

typedef enum
{
  MXL_5XX_SKU_TYPE_MIN = 0x00,
  MXL_5XX_SKU_TYPE_581 = 0x00,
  MXL_5XX_SKU_TYPE_584 = 0x01,
  MXL_5XX_SKU_TYPE_585 = 0x02,
  MXL_5XX_SKU_TYPE_544 = 0x03,
  MXL_5XX_SKU_TYPE_561 = 0x04,
  MXL_5XX_SKU_TYPE_5xx = 0x05,
  MXL_5XX_SKU_TYPE_5yy = 0x06,
  MXL_5XX_SKU_TYPE_511 = 0x07,
  MXL_5XX_SKU_TYPE_MAX = 0x08,

} MXL_5XX_SKU_TYPE_E;

/************************************************************************************
       Structs
 ************************************************************************************/
// DiSEqC common definitions for MxLware and Firmware to use
typedef struct
{
  UINT32 diseqcId;
  UINT32 nbyte;
  UINT8 bufMsg[8];
} MXL58x_DISEQC_MSG_T;

typedef enum
{
  DSQ_CFG_TX_POWER = 0,
  DSQ_CFG_TONE_FREQ,
  DSQ_CFG_TONE_BURST,
  DSQ_CFG_TONE,
  DSQ_CFG_MODE
} MXL58x_DISEQC_PARAM_E;

typedef enum
{
  DSQ_CONTINUOUS_TONE = 0,
  DSQ_NON_CONTINUOUS_TONE0
} MXL58x_DISEQC_TONE_E;

typedef enum
{
  DSQ_NORMAL_MODE = 0,
  DSQ_TEST_MODE
} MXL58x_DISEQC_MOD_E;

typedef enum
{
  DSQ_13V = 0,
  DSQ_18V
} MXL58x_DISEQC_PWR_E;

typedef enum
{
  DSQ_INTERNAL_TONE_22KHz = 0,
  DSQ_INTERNAL_TONE_33KHz,
  DSQ_INTERNAL_TONE_44KHz,
  DSQ_EXTERNAL_TONE_22KHz
} MXL58x_DSQ_CFG_TONE_FREQ_E;

typedef enum
{
  DSQ_TONE_BURST_MODULATED = 0,
  DSQ_TONE_BURST_UNMODULATED
} MXL58x_DSQ_CFG_TONE_BURST_E;


typedef struct
{
  UINT32 diseqcId;
  UINT32 paramId;
  UINT32 value;
} MXL58x_DISEQC_CFGREQ_T;



// FSK common definitions for MxLware and Firmware to use
typedef struct
{
  UINT32 msgLength;
  UINT8 msgContent[MXL58x_FSK_MSG_MAX_LENGTH];
} MXL58x_FSK_MSG_T;

// Xtal
typedef struct
{
  UINT32 pll_mode;            /* PLL mode */
  UINT32 pll_freq;            /* PLL frequency */
  UINT32 xtal_freq;           /* Crystal frequency */
  UINT32 sample_freq;         /* Sample frequency */
  UINT32 clk_tree;            /* 1 = workaround enabled */
} MXL58x_DEV_XTAL_T;

// Demod Para for Channel Tune
typedef struct
{
  UINT32 demodId;

  MXL58x_DEV_XTAL_T pll_params;

  UINT32 frequency;         // Frequency
  UINT32 modulation;        // Input : mode 0-23

  UINT32 spectrumInversion; // Input : Spectrum inversion.
  UINT32 rolloff;           /* rolloff (alpha) factor */
  UINT32 symbol_rate;       /* Symbol rate */
  UINT32 pilots;            /* TRUE = pilots enabled */
  UINT32 modulationScheme;  // Input : Modulation Scheme
  UINT32 fecCodeRate;       // Input : Forward error correction rate.

  UINT32 acquisition_range; /* Acquistion range (active only if frq_search = true) */
  // UINT32 rsBypass;          //RS code bypass or not
  UINT32 freq_search;       /* TRUE = freq_search active */

  UINT32 channelScanMode;   // one of MXL58x_CH_SCAN_MODE_E: Blind, cold, warm
  //UINT32 ctltracking;     /* TRUE = ctl tracking active */
  //UINT32 goldcodeindex;   /* Gold code index[0->16]. Value 17 forces the use of custom sequence */
  //UINT32 customgold[MXL58x_CUSTOM_GOLD_DIM]; /* Custom sequence */

  // Debug use
  UINT32 manualSfInHz;      /* Manual set sampling freq for test only */

  UINT32 maxCarrierOffsetInMHz; // Maximum carrier freq offset in MHz

  UINT32 guardBandInMHz; // Guardband in MHz
} MXL58x_DEMOD_PARAM_T;

// Demod Status
typedef struct
{
  UINT32 lock_status;       /* one of MXL58x_DEMOD_LOCK_STATUS_E: global lock status */
  UINT32 acq_status;
} MXL58x_DEMOD_SIGNAL_INFO_T;

typedef struct
{
  SINT32 AGC_level;                   /* AGC level */
  SINT32 AGC_dig_level;               /* AGC digital level */
  UINT32 clip_count;                 /* Clip count MK - Dont need  */
} MXL58x_DEMOD_AGC_INFO_T;

typedef struct // DVB-S exclusive status
{
  //REAL32 bitErrorRate; // Bit Error Rate for DVB-S

  // 1st iteration
  UINT32 corrRsErrors; // corrected RS Errors: 1st iteration
  UINT32 unCorrRsErrors; // uncorrected RS Errors: 1st iteration

  UINT32 berWindow;  // total packet number: 1st iteration. BER1 = berCount/berWindow
  UINT32 berCount;   // error packet number: 1st iteration

  // 2nd iteration
  UINT32 corrRsErrors2; // corrected RS Errors: 2nd iteration
  UINT32 unCorrRsErrors2; // uncorrected RS Errors: 2nd iteration

  UINT32 berWindow2; // total packet number: 2nd iteration. BER2 = berCount2/berWindow2
  UINT32 berCount2;  // error packet number: 2nd iteration
} MXL58x_DVBS_FEC_STATUS_T;

typedef struct // DVB-S2 exclusive status
{
  // REAL32 packetErrorRate; // Packet Error Rate for DVB-S2

  UINT32 crcErrors; // CRC Errors for DVB-S2

  UINT32 perWindow; // PER2 = perCount/perWindow

  UINT32 perCount;

  UINT8  iValue;            // DMD0_STATUS_DVBS2_IQ_LDPC_ITER_ERROR_ADDR      (0x3FFFC6D8)  // <iValue, qValue, ldpcIterations, ldpcError> is placed in 32-bit as MSB->LSB
  UINT8  qValue;

  UINT8  ldpcIterations;
  UINT8  ldpcError;

  UINT32 vnr;                  // DMD0_STATUS_DVBS2_VNR_ADDR
  UINT32 correctedBlocksBCH;
  UINT32 erroredBlockBCH;
  UINT32 packetCountBCH;
  UINT32 crcErrorCount;
  UINT32 crcPacketCount;
  UINT16 ifIntegrator;
  UINT16 matType;
  UINT16 upl;
  UINT16 dfl;
  UINT16 currentSyncedValue;
  UINT16  currentSyncValue;
} MXL58x_DVBS2_FEC_STATUS_T;

typedef struct
{
  UINT32 demodId;                 // one of MXL58x_DEMOD_ID_E

  UINT32 centerFrequencyInKHz;    // Current tuned L-band frequency in kHz (0 if not tuned).
  SINT32 inputPower;              // Output: Current Signal Strength, shall be signed: +/1 in dBm @ inputPower

  MXL58x_DEMOD_AGC_INFO_T agcInfo; // Output: shall be signed: +/- @ MXL58x_DEMOD_AGC_INFO_T

  MXL58x_DEMOD_SIGNAL_INFO_T  signalInfo;

  UINT32 standard;                // one of MXL58x_SAT_STANDARD_E: DVB-S, DVB-S2, or DirecTV legacy DSS
  UINT32 spectrumInversion;
  UINT32 spectrumRollOff;
  UINT32 symbolRate;
  UINT32 modulationScheme;
  UINT32 fecCodeRate; // one of MXL58x_DEMOD_CODE_RATE_E

  SINT32 strSnr; // Instant SNR in dB -  DMD0_STATUS_INSTANT_SNR_ADDR
  SINT32 strAverageSnr; // Average SNR in dB

  SINT32 frequencyOffset;   // Output: shall be signed: +/1 in KHz @ frequencyOffset
                            // Carrier Frequency offset (KHz) = CTL frequency offset + FTL frequency offset

  SINT32 ctlFreqOffset;     // Output: As above, floating point. At least, it shall be signed SINT32: +/- in KHz @ ctlFreqOffset
  SINT32 strFreqOffset;     // Output: For best performance, it is floating point. At least, it shall be signed integer SINT32: +/- in Hz
  SINT32 ftlFreqOffset;     // Output: As above, floating point. At least, it shall be signed SINT32: +/- in KHz @ ftlFreqOffset

  UINT32 strNbcSyncLock;    // STR NBC Sync Lock, 0/1 = No Lock/Lock
  UINT32 cycleSlipCount;    // Cycle Slip Count
  SINT32 ctlLoopFilterOut;  // CTL Loop Filter Output
  //MK-  DMD0_STATUS_CTL_LOOP_FILTER_OUT_ADDR

  SINT16 displayI;
  SINT16 displayQ;
  UINT32 reAcqCnt;  //DMD0_STATUS_RE_ACQ_CNT_ADDR

  UINT32 demodLockTimeinmsecs;

  union
  {
    MXL58x_DVBS_FEC_STATUS_T  dvbs;
    MXL58x_DVBS2_FEC_STATUS_T dvbs2;
  } u;

} MXL58x_DEMOD_STATUS_T;



typedef struct
{
  UINT32 demodIndex;
} MXL58x_ABORT_TUNE_T;


// Command structure for reading FFT spectrum
typedef struct
{
  UINT32 demodIndex;

  UINT32 readHwMemOrFwBuffer; // If MXL_TRUE, host reads spectrum from HW memory.  If MXL_FALSE, host reads spectrum from T106 FW buffer.
                              //
                              // PHY path: HW memory --> FW buffer --> Host
                              //
                              // Use cases: (1) power spectrum reporting, should use MXL_FALSE.
                              //            (2) triggering to run a new FFT through GUI writing a sequencer of regiters, then reading back the consequent spectrum, should use MXL_TRUE.
                              //            (3) reading a snapshot spectrum taken at an intermediate FTL sub-state, should use MXL_FALSE.
                              //            (4) reading an existing spectrum inside HW memory, should use MXL_TRUE.
} MXL58x_REQ_FFT_SPEC_T;

/*----------------------------------------------------------------------------------------
   The following structs are required by MXL500 SOC in accordance to Debug MXL User Guide.
----------------------------------------------------------------------------------------*/
// To read register:
// Host sends MXL58x_DEMOD_REG_TISTER to 106 in order to read data.
// Then 106 will report the same structure to Host after filling it out.
// To write:
// Host sends MXL58x_DEMOD_REG_TISTER to 106 in order to write data.
// Then 106 will report the same structure to Host as a confirmation.
typedef struct
{
  UINT32 demodIndex; // DEMOD Ctrl: one of MXL58x_DEMOD_ID_E
  UINT32 regAddr;
  UINT32 regData;
} MXL58x_DEMOD_REG_T;

// Host sends MXL58x_DEMOD_PKT_NUM_T to 106 in order to set how many packets to calculate PER.
// Then 106 will report the same structure to Host as a confirmation.
typedef struct
{
  UINT32 demodIndex; // DEMOD ID: one of MXL58x_DEMOD_ID_E
  UINT32 numPackets;   // It is configured from Host to 106
                                   // 0 shall mean continuous packets to calculate PER
                                   // 2048 packets
                                   // 4096
                                   // 8192
                                   // 16384
                                   // 32768
                                   // 65536
} MXL58x_DEMOD_PKT_NUM_T;

// Host sends MXL58x_DEMOD_IQ_SOURCE_T to 106 in order to set what HW I/Q to display on GUI.
// Then 106 will report the same structure to Host as a confirmation.
typedef struct
{
  UINT32 demodIndex;// DEMOD ID
  UINT32 sourceOfIQ; // ==0, it means I/Q comes from Formatter
                                      // ==1, Legacy FEC
                                      // ==2, Frequency Recovery
                                      // ==3, NBC
                                      // ==4, CTL
                                      // ==5, EQ
                                      // ==6, FPGA
} MXL58x_DEMOD_IQ_SOURCE_T;

// Host reads back I/Q values from 106 to display on GUI.
// Then 106 will report the same structure to Host as a confirmation.
typedef struct
{
  UINT32 demodIndex; // DEMOD ID
  SINT16 displayI;  // I value passes from 106 to host
  SINT16 displayQ;  // Q value passes from 106 to host
} MXL58x_DEMOD_IQ_DATA_T;

// To read
// Host sends MXL58x_DEMOD_READ_FTL_CMD to 106 in order to read back FTL's freq offset.
// Then 106 will report the same structure to Host as a confirmation.
// To write
// Host sends MXL58x_DEMOD_WRITE_FTL_CMD to 106 in order to over-write FTL freq offset.
// Then 106 will report the same structure to Host as a confirmation.
typedef struct
{
  UINT32 demodIndex; // DEMOD ID
  SINT32 regData; // If host reads from 106:
                  // A value passes from 106 to host
                  // (1) Bits 0-7 gets displayed as "lsb" of demod window
                  // (2) Bits 15-8 gets displayed as "msb" of demod window
                  // (3) Together, Bits 15-0 are a SINT16 integer to be displayed
                  //     as "xxxx KHz". The place to display is on the right to the
                  //     "FTL offset" "lsb" of demod window.
                  //     Ignores other bits.
                  //
                  // If host writes to 106:
                  // A value passes from host to 106
                  // Bits 0-7 comes from "lsb" of demod window
                  // Bits 15-8 comes from "msb" of demod window
                  // Ignores other bits.
} MXL58x_DEMOD_FTL_T;

// Host sends to set 106 to get 12 bytes from M68HC05.
// Then 106 will report the same structure to Host as a confirmation.
typedef struct
{
  UINT32 demodIndex; // DEMOD ID
  UINT32 blockBaseAddr;
  UINT32 blockData[MXL58x_M68HC05_MAX_READ_BYTE_NUMBER]; // 12
} MXL58x_DEMOD_MEM_BLOCK_T;

// First, Host sends a request to request a trace of acq times.
// Then, 106 confirms by sending MXL58x_DEMOD_TRACE_T back to Host.
typedef struct
{
  UINT32 state1ByteTime2Bytes; // state is bits 23-16, while time is bits 15-0.
} MXL58x_ACQ_TRACE_T; // Acq Trace in HC05

typedef struct
{
  UINT32 demodIndex; // DEMOD ID

  UINT32 fwAcqIndex; // How many useful traces are stored.
  MXL58x_ACQ_TRACE_T fwAcqList[MXL58x_MAX_NUM_OF_ACQ_TRACES];
} MXL58x_DEMOD_TRACE_T; // It confirms MXL58x_DEMOD_CTRL_TRACE_T

// This struct is only used for sending a small nubmer of traces to host.
typedef struct
{
  UINT32 demodIndex; // DEMOD ID

  UINT32 fwAcqIndex; // How many useful traces are stored.
  MXL58x_ACQ_TRACE_T fwAcqList[MXL58x_HOST_SMALL_NUM_OF_ACQ_TRACES];
} MXL58x_DEMOD_SHORT_TRACE_T; // It confirms MXL58x_DEMOD_CTRL_TRACE_T

// Lookup table to display acq strings: this struct is never transmitted to Host
// So it's OK for this struct to use enum instead of UINT32
typedef struct
{
  char stateString[20]; // char array
  MXL58x_DEMOD_ACQ_STATE_TYPE_E stateEnum;
} MXL58x_DEMOD_ACQ_STATE_SELECTION_T;

// This struct is displayed on host to show the version of M68HC05.
// Works with a command MXL58x_DEMOD_GET_M68HC05_VER_CMD
typedef struct
{
  UINT32 checkSum; // Check sum of the binary image file downloaded to M68HC05
  UINT32 ver16Bits[MXL58x_DEMOD_ID_MAX]; // For multi demods, there are multi version
                    // numbers. Note that for lyra, there are 2 demods.
                      // For Hydra it is 8 demods.This includes two versions.
                    // Host will check whether the two versions are the same or not.
                    // Version is 16-bit stored in Bit 15~0 of ver16Bits.
                    // Bit 15~8 is a major revision (MSB), while Bit 7~0 is a minor
                    // revision (LSB).
                    // If the two versions are the same, Host should display as “MSB.LSB.checkSum”
} MXL58x_DEMOD_M68HC05_VER_T;

// This struct is programmed by host to control delays for acq trace
typedef struct
{
  UINT32 pollStatInMsec; // Time interval that T106 polls SoC statistics.

  UINT32 readTraceTime; // readTraceTime*pollStatInMsec is the time between tune and read-M68HC05-trace.
  UINT32 sendTraceToHostTime; // sendTraceToHostTime*pollStatInMsec is the time between read-M68HC05-trace and send-to-host.
  UINT32 printTraceToUartTime; // printTraceToUartTime*pollStatInMsec is the time between send-to-host and print-to-UART.

} MXL58x_DELAY_PARAM_T;

typedef struct
{
  UINT32 demodIndex; // DEMOD ID
  UINT32 monPrint; // 32 enable bits for turning on/off Soc features
  MXL58x_DELAY_PARAM_T initDelays;  // A set of delay parameters that control T106 behaviors
} MXL58x_DEMOD_MON_T;

// Host sends MXL58x_DEMOD_SET_BREAKPOINT_T to set up breakpoints in HC05.
typedef struct
{
  UINT32 demodIndex; // DEMOD ID

  UINT32 priorityLevel[MXL58x_M68HC05_TOT_BREAKPOINT_NUM]; // Can only be assigned by one of the four enum values:
                    // MXL58x_POST_DEBUG_LEVEL_1, MXL58x_POST_DEBUG_LEVEL_2, MXL58x_POST_DEBUG_LEVEL_3, or
                    // MXL58x_POST_DEBUG_LEVEL_4.
                    // For Breakpoint 0, priorityLevel[0] determines priority level.
                    // For Breakpoint 1, priorityLevel[1] determines priority level.
                    // For Breakpoint 2, priorityLevel[2] determines priority level.
                    // For Breakpoint 3, priorityLevel[3] determines priority level.
                    // For Breakpoint 4, priorityLevel[4] determines priority level.
                    // A high-priority breakpoint will be sent to host faster with no dropping.

  UINT32 regAddr[MXL58x_M68HC05_SHORT_BREAKPOINT_NUM][MXL58x_M68HC05_REG_NUM_OF_SHORT_BREAKPOINT];
                    // Bit 15-0 is reg address. Each breakpoint needs three reg addresses.
                    // regAddr[0][0..2] are three reg addresses for Breakpoint 0
                    // regAddr[1][0..2] are for Breakpoint 1.
                    // regAddr[2][0..2] are for Breakpoint 2.
                    // regAddr[3][0..2] are for Breakpoint 3.

  UINT32 blockBaseAddr; // Bit 15-0 is base address for Breakpoint 4.
  UINT32 blockSize; // Bit 15-0 is num of registers for Breakpoint 4.
} MXL58x_DEMOD_SET_BREAKPOINT_T;

// MXL58x_DEMOD_SHORT_BREAKPOINT_T and MXL58x_DEMOD_LONG_BREAKPOINT_T
typedef struct
{
  UINT32 demodIndex; // DEMOD ID

  UINT32 header1; // Bit 31-16 is "Number of Breakpoint Interrupts" received by 106,
                  // Bit 15-8 is "Service Status: Ready/Busy" for 106 own interrupt handling.
                  //          If Ready, should be enum MXL58x_READY_TO_SERVICE_M68HC05_BREAKPOINT.
                  //          If Busy, should be enum MXL58x_BUSY_TO_SERVICE_M68HC05_BREAKPOINT.
                  // Bit 7-0 is "Breakpoint Id" (from 0-3).

  UINT32 header2; // Bit 31-16 is "Breakpoint Hit Counter" (how many times HC05 hits this breakpoint).
                  // Bit 15-0 is "Number of Breakpoint Registers" of this breakpoint.
                  // Right after tune gets started, this value is zeroed out.

  UINT32 reg[MXL58x_M68HC05_REG_NUM_OF_SHORT_BREAKPOINT];
                  // In reg[0], Bit 23-8 represents a 16-bit register address, while
                  // Bit 7-0 represents one-byte register value.
                  // reg[1] and reg[2] are the same format.

} MXL58x_DEMOD_SHORT_BREAKPOINT_T;

typedef struct
{
  UINT32 demodIndex; // DEMOD ID

  UINT32 header1; // Bit 31-16 is "Number of Breakpoint Interrupts" received by 106,
                  // Bit 15-8 is "Service Status: Ready/Busy" for 106 own interrupt handling.
                  //          If Ready, should be enum MXL58x_READY_TO_SERVICE_M68HC05_BREAKPOINT.
                  //          If Busy, should be enum MXL58x_BUSY_TO_SERVICE_M68HC05_BREAKPOINT.
                  // Bit 7-0 is Breakpoint Id, fixed as 4.

  UINT32 header2; // Bit 31-16 is "Breakpoint Hit Counter" (how many times HC05 hits this breakpoint).
                  // Bit 15-0 is "Number of Breakpoint Registers" of this breakpoint.
                  //          It is flexible, but no more than
                  //          MXL58x_M68HC05_REG_NUM_OF_LONG_BREAKPOINT.
                  //          It shows how many bytes within blockData[] are valid data.
                  //          Right after tune gets started, this value is zeroed out.

  UINT32 blockBaseAddr; // Bit 15-0 is a base address for a block of registers

  UINT32 blockData[MXL58x_M68HC05_REG_NUM_OF_LONG_BREAKPOINT]; // All register data
                        // are tightly packed, e.g.
                        // reg data 0 is in Bit 31-24, reg data 1 is in Bit 23-16
                        // reg data 2 is in Bit 15-8, reg data 3 is in Bit 7-0
} MXL58x_DEMOD_LONG_BREAKPOINT_T;

// This struct is set from GUI to T106 for the mode of the output of the error counter
typedef struct
{
  UINT32 demodIndex; // DEMOD ID

  UINT32 errorOut; // one of MXL58x_ERROR_COUNTER_OUTPUT_E

  UINT32 rsOutSel; // one of MXL58x_DVBS_ITER_RS_OUT_SEL_E

  UINT32 firstRsBypass; // MXL_TRUE or MXL_FALSE. If true, 1st iteration RS is bypassed.
  UINT32 secondRsBypass; // MXL_TRUE or MXL_FALSE. If true, 2nd iteration RS is bypassed.

} MXL58x_DEMOD_ERROR_COUNTER_MODE_T;

// This struct is for MXL58x_DEMOD_CCI_CTL2_CMD command
typedef struct
{
  UINT32 demodIndex; // DEMOD ID
  UINT32 cciCtl2Mode; // should be one of enum MXL58x_DEMOD_CCI_CTL2_E
} MXL58x_DEMOD_CCI_CTL2_T;

// SKU
typedef struct
{

    MXL_BOOL_E enable[MXL58x_DEMOD_ID_MAX];  // indicates whether this demond is enabled for this sku or not

} MXL_5xx_SKU_DEMOD_T;

typedef struct
{
    MXL_BOOL_E enable[MXL58x_TUNER_ID_MAX];  // indicates whether this demond is enabled for this sku or not

} MXL_5xx_SKU_TUNER_T;


typedef struct
{
  MXL_5XX_SKU_TYPE_E  skuType;
  MXL_5xx_SKU_TUNER_T tuner;
  MXL_5xx_SKU_DEMOD_T demod;

} MXL_5xx_SKU_FEATURES_DEFINE_T;

typedef struct
{
  UINT32  demodId;
  SINT32  cosValue [MAX_NUM_NF_PER_DEMOD];
  SINT32  sinValue [MAX_NUM_NF_PER_DEMOD];
  UINT8   nfStatus [MAX_NUM_NF_PER_DEMOD   + 1 ]; /*4th entry is don’t care. Enable(1),disable (0)*/
  UINT8   notchFilterGain [MAX_NUM_NF_PER_DEMOD   + 1 ]; /*4th entry is don’t care.*/
  UINT8   nfOverWriteStatus[MAX_NUM_NF_PER_DEMOD + 1] ; /* Overwrite or not, 1: Overwrite, 0: Don't overwrite */
  SINT32  notchLocInHz [MAX_NUM_NF_PER_DEMOD ];

} MXL58x_CFG_NF_MSG_T;

/* Debug strucutures for handling SPURS */
typedef struct
{
  UINT32 demodIndex;
} MXL58x_REQ_NF_MSG_T;

typedef struct
{
  UINT32 demodIndex;
  UINT8   nfStatus [MAX_NUM_NF_PER_DEMOD   + 1 ]; /*4th entry is don’t care. Enable(1),disable (0)*/
  UINT8   notchFilterGain [MAX_NUM_NF_PER_DEMOD   + 1 ]; /*4th entry is don’t care.*/
  UINT32  notchLocInHz [MAX_NUM_NF_PER_DEMOD ];
} MXL58x_RSP_NF_MSG_T;

typedef enum
{
  HYDRA_UART_INTERFACE = 0,
  HYDRA_JTAG_INTERFACE

} MXL_HYDRA_DBG_INTERFACE_E;

typedef enum
{
  // Error bit numbers (not the actual value).
  ERR_CODE_INVALID_DEMOD        = 0x1,
  FW_DYN_MEM_FULL               = 0x2,
  FW_HOST_RETQ_FULL             = 0x3,
  HWD_INVALID_HOST_CMD          = 0x4,
  HC05_COMMUNICATION_FAILURE    = 0x5,

}MXL58x_ERROR_CODES_E;


#ifdef __cplusplus
}
#endif

#endif //__MXL_HDR_CMDS_H__


