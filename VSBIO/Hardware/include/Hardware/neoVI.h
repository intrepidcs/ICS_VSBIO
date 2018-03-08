//
// neoVI header file for hardware version 4
// 2/5/01
// Copyright 2000-2001 Intrepid Control Systems, Inc.
//

#ifndef NEOVI_MAIN_HEADER_FILE
#define NEOVI_MAIN_HEADER_FILE

// neoVI Blue/ ValueCAN 2 Memory Addresses
#define NEOVI_MEMADDR_NUM_PERIODIC 0x188
#define NEOVI_MEMADDR_TXMSG0 0x530
#define NEOVI_MEMADDR_PERIOD_CONTROL 0x510
#define NEOVI_MEMADDR_TXREPORT 0x1A1
#define NEOVI_MEMADDR_SWCANASWITCH 0x1A6
#define NEOVI_MEMADDR_CANSTOPFILTER 0x1A7
#define NEOVI_MEMADDR_CANFILT_EN 0x199
#define NEOVI_MEMADDR_CANFILT_STRT 0x191
#define NEOVI_MEMADDR_CANFILT_BLCKSZ 0x19D

// Configuration Array addresses
#define NEO_CFG_MPIC_GENIO_SETUP

// EEPROM ADDRESSES
#define NEOVI_MAINPICEE_FORDSCP_ADDR 63
#define NEOVI_MAINPICEE_LBCC_NUM_LU1_ENTERIES 66
#define NEOVI_MAINPICEE_LBCC_NUM_LU1_START 67
#define NEOVI_MAINPICEE_LBCC_NUM_LU1_STOP 97
#define NEOVI_ISOJEE_ISOMODE_BITFIELD 155

// CAN baud rate registers...
// HSCAN
#define NEOVI_MAINPICEE_HSCAN_CNF3 8
#define NEOVI_MAINPICEE_HSCAN_CNF2 9
#define NEOVI_MAINPICEE_HSCAN_CNF1 10
// MSCAN
#define NEOVI_MAINPICEE_MSCAN_CNF3 20
#define NEOVI_MAINPICEE_MSCAN_CNF2 21
#define NEOVI_MAINPICEE_MSCAN_CNF1 22
// SWCAN
#define NEOVI_MAINPICEE_SWCAN_CNF3 32
#define NEOVI_MAINPICEE_SWCAN_CNF2 33
#define NEOVI_MAINPICEE_SWCAN_CNF1 34
// LSFTCAN
#define NEOVI_MAINPICEE_LSFTCAN_CNF3 44
#define NEOVI_MAINPICEE_LSFTCAN_CNF2 45
#define NEOVI_MAINPICEE_LSFTCAN_CNF1 46
// SWCAN HS
#define NEOVI_MAINPICEE_SWCAN_HS_CNF3 58
#define NEOVI_MAINPICEE_SWCAN_HS_CNF2 59
#define NEOVI_MAINPICEE_SWCAN_HS_CNF1 60

//EE_DATA_FIRM_VALID	equ	d'0'	;// indicates
//EE_DATA_NETS_ENABLED	equ	d'1'
//EE_DATA_UPDATE_CNT_b0	equ	d'2'	;// update count of firmware - 2 byte value
//EE_DATA_UPDATE_CNT_b1	equ	d'3'
//EE_DATA_GENIO_SETUP 	equ	d'5'
//
//EE_DATA_HS_CAN_SETUP 		equ	d'6'
//EE_DATA_MS_CAN_SETUP 		equ	d'18'
//EE_DATA_SW_CAN_SETUP 		equ	d'30'
//EE_DATA_LSFT_CAN_SETUP 		equ	d'42'
//EE_DATA_HS_CAN_MODE		equ	d'54'
//EE_DATA_MS_CAN_MODE		equ	d'55'
//EE_DATA_SW_CAN_MODE		equ	d'56'
//EE_DATA_LSFT_CAN_MODE		equ	d'57'
//EE_DATA_SWCAN_HS_CNF3		equ	d'58'
//EE_DATA_SWCAN_HS_CNF2		equ	d'59'
//EE_DATA_SWCAN_HS_CNF1		equ	d'60'
//
//EE_DATA_LBCC_SETUP_BITFIELD	equ	d'61'
//EE_DATA_LBCC_NDRC		equ	d'62'
//EE_DATA_LBCC_NODE_ADDRESS	equ	d'63'
//EE_DATA_LBCC_FREAD_D1		equ	d'64'
//EE_DATA_LBCC_FREAD_D2		equ	d'65'
//EE_DATA_LBCC_NUM_LU1_ENTERIES	equ	d'66'
//EE_DATA_LBCC_NUM_LU1_START	equ	d'67'
//EE_DATA_LBCC_NUM_LU1_STOP	equ	d'97'
//
//
//EE_DATA_NUMBYTES_IN_TRIG	equ	d'54'
//EE_DATA_TRIG_1_NUM_BYTES	equ	d'100'
//EE_DATA_TRIG_ENABLE_1		equ	d'98'
//EE_DATA_TRIG_ENABLE_2		equ	d'99'
//
//EE_DATA_FIRST_TASK		equ	d'155'
//EE_DATA_LED_SETTTING		equ	d'187'
//EE_DATA_J1708_RXIFS		equ	d'188'

#define NEOVI_RED_TIMESTAMP_2_10US 42949.67296
#define NEOVI_RED_TIMESTAMP_1_10US 0.00001

#define NEOVIPRO_VCAN_TIMESTAMP_2_US 65536.0
#define NEOVIPRO_VCAN_TIMESTAMP_1_US 1.0

typedef struct
{
	unsigned short CommandType;// 4
	unsigned short CommandByteLength;
	unsigned char Data[1024];// 14
} icsneoVICommandLong;

typedef struct
{
	bool IsEmpty;
	icsSpyMessage stTxMsg;
} icsneoVITxMessageStore;

// ERROR CONSTANTS for j1850
#define JERR_RX_BIT_MAX 0x01// Rx Error Bit > Max (Break!)
#define JERR_TX_BIT_MAX 0x02
#define JERR_RX_SHORT_MIN 0x03
#define JERR_TX_SHORT_MIN 0x04
#define JERR_TX_MSG_TOO_LONG 0x05// Message Too Long Error
#define JERR_RX_SOF_MIN 0x06// Rx SOF Min Time Error
#define JERR_TX_SOF_MIN 0x07// define also as a break
#define JERR_RX_SOF_MAX 0x08// Rx SOF Min Time Error
#define JERR_TX_SOF_MAX 0x09// define also as a break
#define JERR_SOF_ONLY 0x0A
#define JERR_BYTE_BOUNDARY 0x0B
#define JERR_FALSE_TRIG 0x0C
#define JERR_LOST_ARB 0x0D

// ERROR CONSTANTS FOR ISO/LIN MODE
#define ISO_LIN_SYNC_BRK_ERR 0x01
#define ISO_LIN_SYNC_WAV_ERR 0x02
#define ISO_LIN_MSG_ID_PRTY 0x03
#define ISO_LIN_CHKSUM_ERR 0x04
#define ISO_LIN_TFMAX_ERR 0x05
#define ISO_LIN_BIT_ERROR 0x06
#define ISO_LIN_SYNC_LEN_ERR 0x07

// commands from the 51
#define MAIN_51_RX_BUFF_OVERFLOW 0x1
#define MAIN_51_START_CMD 0x2
#define MAIN_51_TX_FIFO_OVERFLOW 0x3
#define MAIN_51_BULKIN_NODATA 0x4
#define MAIN_51_SETMODE_COMPLETE 0x5
#define MAIN_51_READ_EEPROM 0x6
#define MAIN_51_WRITE_EEPROM_DONE 0x7
#define MAIN_51_DEV_FIFO_OVERFLOW 0x8
#define MAIN_51_TX_REPORT 0x9
#define MAIN_51_CMD_DONE 0xA
#define MAIN_51_ERR_STATUS 0xB
#define MAIN_51_READ_SECTOR_BUFF 0xC
#define MAIN_51_WRITE_SECTOR_BUFF 0xD
#define MAIN_51_MMC_PROCESS_DONE 0xE
#define MAIN_51_REINIT_DONE 0xF

// Rx Operating Modes
#define NEOVI_MODE_RS232 0x0
#define NEOVI_MODE_USB_BULK 0x1
#define NEOVI_MODE_USB_ISO_NO_USE 0x2

// MAIN51 ERROR STATUS BYTYE
#define MAIN51_ERR_STATUS_TXFIFO_IN_ERR 1
#define MAIN51_ERR_STATUS_HOST_CHKSUM_ERR 2
#define MAIN51_ERR_STATUS_HOST_MISS_BYTE 4
#define MAIN51_ERR_STATUS_HOST_OVERRUN 8


//appended red commands
#define RED_CMD_SET_RTC (0x50)
#define RED_CMD_GET_RTC (0x49)

#define RED_CMD_BOOTLOADER_REQ (0xa0)
#define RED_CMD_SERIAL_NUM_REQ (0xa1)
#define RED_CMD_AUTHENTICATE_REQ (0xa2)
#define RED_CMD_APP_VERSION_REQ (0xa3)
#define RED_CMD_SET_BAUD_REQ (0xa4)
#define RED_CMD_READ_BAUD_REQ (0xa5)
#define RED_CMD_SAVE_TO_EEPROM (0xa6)
#define RED_CMD_ENABLE_WAVEFORM_SOURCE (0xa7)
#define RED_CMD_SET_DEFAULT_BAUD_REQ (0xa8)
#define RED_CMD_PERIPHERALS_APP_VERSION_REQ (0xa9)
#define RED_CMD_SET_ISO15765_2_CM_TX_MSG_DATA (0xaa)
#define RED_CMD_GET_ISO15765_2_CM_TX_MSG_DATA (0xab)
#define RED_CMD_READ_ISO15765_2_CM_TX_MSG (0xac)
#define RED_CMD_WRITE_ISO15765_2_CM_TX_MSG (0xad)
#define RED_CMD_WRITE_RTC_RAM (0xae)
#define RED_CMD_READ_RTC_RAM (0xaf)
#define RED_CMD_START_CM_FBLOCK (0xb0)
#define RED_CMD_STOP_CM_FBLOCK (0xb1)
#define RED_CMD_IS_CM_FBLOCK_RUNNING (0xb2)
#define RED_CMD_CLEAR_MAIN_LOOP_TIME (0xb3)
#define RED_CMD_SET_LICENSE_REQ (0xb4)
#define RED_CMD_GET_NETWORK_STATE (0xb5)
#define RED_CMD_GET_LICENSE_REQ (0xb6)
#define RED_CMD_HARDWARE_VERSION_REQ (0xb7)
#define RED_CMD_GET_TIMESTAMP (0xb8)
#define RED_CMD_SET_MISC_DIO (0xb9)
#define RED_CMD_GET_MISC_DIO (0xba)
#define RED_CMD_GET_SDCARD_INFO (0xbb)
#define RED_CMD_REQ_RUN_STATUS (0xbc)
#define RED_CMD_READ_APP_SIGNAL (0xbd)
#define RED_CMD_WRITE_APP_SIGNAL (0xbe)
#define RED_CMD_READ_DATALINK_CM_TX_MSG (0xbf)
#define RED_CMD_WRITE_DATALINK_CM_TX_MSG (0xc0)
#define RED_CMD_READ_DATALINK_CM_RX_MSG (0xc1)
#define RED_CMD_WRITE_DATALINK_CM_RX_MSG (0xc2)
#define RED_CMD_CLEAR_LIN_TABLES (0xc3)
#define RED_CMD_ENABLE_DWCAN_TRANSCEIVERS (0xc4)
#define RED_CMD_SET_FREQ_PWM_OUT (0xc5)
#define RED_CMD_SET_DUTY_PWM_OUT (0xc6)
#define RED_CMD_READ_SETTINGS_EX (0xc7)
#define RED_CMD_SEND_DLL_VERSION (0xc8)
#define RED_CMD_MODIFY_SETTINGS_REQ (0xc9)
#define RED_CMD_CLEAR_NVM (0xca)
#define RED_CMD_TEXTAPI (0xcb)
#if 0 /* As of CM19, we don't directly control coremini objects
			since they are likely to change. */
// [
#define RED_CMD_READ_ISO15765_2_CM_RX_MSG (0xcc)
#define RED_CMD_WRITE_ISO15765_2_CM_RX_MSG (0xcd)
#define RED_CMD_ReadCoreMiniFilter (0xce)
#define RED_CMD_ReadCoreMiniFiltersTxFlowControl (0xcf)
#define RED_CMD_WriteCoreMiniFilter (0xd0)
#define RED_CMD_WriteCoreMiniFiltersTxFlowControl (0xd1)
#define RED_CMD_ReadCoreMiniRunTxMsg (0xd2)
#define RED_CMD_ReadCoreMiniRunTxMsgsFlowCntrlFilter (0xd3)
#define RED_CMD_WriteCoreMiniRunTxMsg (0xd4)
#define RED_CMD_WriteCoreMiniRunTxMsgsFlowCntrlFilter (0xd5)
// ]
#endif

#define RED_CMD_PREERASE_SDCARD_COREMINI (0xd6)
#define RED_CMD_J2534_EXTENSION (0xd7)
#define RED_CMD_RXTXPAIRS_EXTENSION (0xd8)
#define RED_CMD_ANDROID_FRAME (0xd9)

#define RED_CMD_CM_FILTERS_USB (0xda)
#define RED_CMD_CM_VBATT_MONITOR (0xdb)
#define RED_CMD_CM_BITSMASH (0xdc)
// WN MISC signals removed in bug 11145
#define RED_CMD_WIVI_COMM (0xdd)
#define RED_CMD_VNET_INFO_REQ (0xde)
#define RED_CMD_VBATT_REQUEST (0xdf)
/**
 * Reqeusts CoreMini status info, runing, fingerpreint, file system info, etc...
 * @see CoreMiniMsgScriptStatus
 */
#define RED_CMD_SCRIPT_STATUS (0xe0)
/**
 * Sets bitrate settings for a particular network.
 */
#define RED_CMD_INDIV_NETWORK_SETTINGS (0xe1)
/**
 * Signals neoVI to start the power down sleep sequence.
 */
#define RED_CMD_SLEEP_REQUEST (0xe2)
//#define RED_CMD_SET_BAUD_REQ_SLAVE_VNET_SLOTA		(0xe3) /* Turns out I didn't need these since explorer can connect direct over USB */
//#define RED_CMD_READ_BAUD_REQ_SLAVE_VNET_SLOTA	(0xe4)
/* Plasma Reserialisation */
#define RED_CMD_SET_ACTIVE_SERIAL_NUM (0xe5)

#define RED_CMD_EXTRACTOR (0xe6)
#define RED_CMD_FIRE2_MISC_CONTROL (0xe7)
#define RED_CMD_LOADER_CMD (0xe8)
#define RED_CMD_HW_COM_LATENCY_TEST (0xe9)
#define RED_CMD_ERASE_MEMORY (0xea)
#define RED_CMD_ETH_PHY_REG_SETTINGS	(0xef)

#define J1534_NUM_PERIOD_TX_MSGS 128
#define J1534_NUM_PERIOD_RX_MSGS 128

#define J2534NVCMD_SetISJ2534 0
#define J2534NVCMD_SetISO5Baud 1
#define J2534NVCMD_SetISOFastInit 2
#define J2534NVCMD_SetISOCheckSum 3
#define J2534NVCMD_SetISO9141Parms 4
#define J2534NVCMD_GetISO9141Parms 5
#define J2534NVCMD_ISO9141APIChkSum 6
#define J2534NVCMD_SetNetworkBaudRate 7
#define J2534NVCMD_GetNetworkBaudRate 8
#define J2534NVCMD_EnableTransmitEvent 9
#define J2534NVCMD_SetTransmitEvent 10
#define J2534NVCMD_BlueEnableStopFilters 11
#define J2534NVCMD_Blue15765HWSupport 12
#define J2534NVCMD_GetTXBufferInfo 13
#define J2534NVCMD_GetEncryptionKey 14
#define J2534NVCMD_SetMiscIOForVBATT 15
#define J2534NVCMD_EnableISO_KW_Network 16
#define J2534NVCMD_SetJ1708CheckSum 17
#define J2534NVCMD_GetTimestamp 18
#define J2534NVCMD_GetCANFDRate 19
#define J2534NVCMD_SetCANFDRate 20
#define J2534NVCMD_GetCANFDTermination 21
#define J2534NVCMD_SetCANFDTermination 22
#define J2534NVCMD_GetCANFDFormat 23
#define J2534NVCMD_SetCANFDFormat 24

//ISO9141 Network Setting Stuff
#define ISO9141_SET_BAUD 1
#define ISO9141_SET_HANDLECHKSUM 2
#define ISO9141_SET_USEKLINEONLY 4
#define ISO9141_SET_IBSTIME 8
#define ISO9141_SET_TXIFSTIME 16
#define ISO9141_SET_RXIFSTIME 32
#define ISO9141_SET_INIT 64

typedef struct
{
	unsigned char SettingsFlag;
	unsigned long Baud;
	bool bHandleCheckSum;
	bool bUseKLineOnly;
	float fIBSTime;
	float fTxIFSTime;
	float fRxIFSTime;
	unsigned char fast_init_Nfive_baud;
} ISO9141NetworkParms;

typedef struct
{
	int iNetIDCAN;
	int iFilterMSB;
	int iFilterLSB;
	int iFilterXtdCANID;
	int iFilterXtdAddress;
	icsSpyMessage stFlowControl;
	int iFlowControlXtdAddress;
	int iEnableFlowControlTransmit;
} SetupISO15765HWSupport;

#endif
