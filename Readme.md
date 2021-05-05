# ICS_VSBIO
This module is designed to help users read Intrepid Control Systems VSB file. The file consists of messages that have been logged on the network. 

### Pip Installation
To install with ```pip``` first download all of the source code from the repo by clicking on the dropdown at the green Code button and click on Download ZIP. 

Then you must install a compiler on your PC. If running Windows install Microsoft Visual C++ 14.0 or newer. You can get this by installing "Build Tools for Visual Sudio" at https://visualstudio.microsoft.com/downloads/.

Then navigate to the root of the downloaded source code and type "pip install -e ." at the command prompt without quotes. 

The compiler should make a folder in the source code directory called ICS_VSBIO.egg-info. 


## Usage

The module contains three Classes ```MsgFileClass```, ```VSBIOFlags```, ```VSBReader```, ```VSBWriter```, ```VSBSplit``` and ```VSBConcatenate```.  

Please see the examples directory for sample programs:
* VSBIOExample.py - Reads the messages from a file and writes some of them to a new file.
* GenerateVSBFileInfoSummary.py - Generates an xlsx file with a summary of all of the MsgIDs on all networks among a list of vsb files including number of records for each. See comments at top of file for more info. 
* SplitVSB_ByArbIdAndNetwork.py - Extracts a subset of message data from a list of vsb files and combines the result into a single vsb file. You can split by time, network, and MsgID.  See comments at top of file for more info. 

#### VSBReader functions
```__init__``` Takes filename to initialize process 

```get_progress()``` returns the progress as a integer percentage

```get_error_message()``` return Error messages if any.

```get_display_message()``` return Display messages if any

```get_message_time()``` return the message seconds since Jan 1, 2007

```get_status()``` returns the current state

```get_status_as_string()``` returns the current state in string format

#### VSBWriter functions
```__init__``` Takes filename to initialize process 

```write_msg(message)``` writes vsb message to file.

### Message
```info``` the vsb object that is stored in the vsbio

```exData``` the extra data pointer used by protocols such as ethernet to store dynamic data. in the case of ethernet the full payload is stored here. note ```message.info.ExtraDataPtr``` or ```sizeOfMsg``` can be used to identify if there is content in the ```exData```

```sizeOfMsg``` the size of the ```exData```


#### Info

```c++
typedef struct _icsSpyMessageVSB
{
	uint32_t StatusBitField;
	uint32_t StatusBitField2;
	uint32_t TimeHardware;
	uint32_t TimeHardware2;
	uint32_t TimeSystem;
	uint32_t TimeSystem2;
	uint8_t TimeStampHardwareID;
	uint8_t TimeStampSystemID;
	uint8_t NetworkID;
	uint8_t NodeID;
	uint8_t Protocol;
	uint8_t MessagePieceID;
	uint8_t ExtraDataPtrEnabled;
	uint8_t NumberBytesHeader;
	uint8_t NumberBytesData;
	uint8_t NetworkID2;
	int16_t DescriptionID;
	uint32_t ArbIDOrHeader;
	uint8_t Data[8];
	union {
		struct
		{
			uint32_t StatusBitField3;
			uint32_t StatusBitField4;
		};
		uint8_t AckBytes[8];
	};
	uint32_t ExtraDataPtr;
	uint8_t MiscData;
	uint8_t Reserved[3];
} icsSpyMessageVSB;
#define icsSpyMessageVSB_SIZE 64
```


#### flags

```c++

/* OpenPort "OpenType" Argument Constants -- deprecated, use OpenNeoDevice */
#define NEOVI_COMMTYPE_RS232 0
#define NEOVI_COMMTYPE_USB_BULK 1
#define NEOVI_COMMTYPE_TCPIP 3
#define NEOVI_COMMTYPE_FIRE_USB 5

/* Network IDs -- value of NetworkID member of icsSpyMessage */
#define NETID_DEVICE 0
#define NETID_HSCAN 1
#define NETID_MSCAN 2
#define NETID_SWCAN 3
#define NETID_LSFTCAN 4
#define NETID_FORDSCP 5
#define NETID_J1708 6
#define NETID_AUX 7
#define NETID_JVPW 8
#define NETID_ISO 9
#define NETID_ISOPIC 10
#define NETID_MAIN51 11
#define NETID_RED 12
#define NETID_SCI 13
#define NETID_ISO2 14
#define NETID_ISO14230 15
#define NETID_LIN 16
#define NETID_OP_ETHERNET1 17
#define NETID_OP_ETHERNET2 18
#define NETID_OP_ETHERNET3 19
#define NETID_ISO3 41
#define NETID_HSCAN2 42
#define NETID_HSCAN3 44
#define NETID_OP_ETHERNET4 45
#define NETID_OP_ETHERNET5 46
#define NETID_ISO4 47
#define NETID_LIN2 48
#define NETID_LIN3 49
#define NETID_LIN4 50
#define NETID_MOST 51
#define NETID_RED_APP_ERROR 52
#define NETID_CGI 53
#define NETID_3G_RESET_STATUS 54
#define NETID_3G_FB_STATUS 55
#define NETID_3G_APP_SIGNAL_STATUS 56
#define NETID_3G_READ_DATALINK_CM_TX_MSG 57
#define NETID_3G_READ_DATALINK_CM_RX_MSG 58
#define NETID_3G_LOGGING_OVERFLOW 59
#define NETID_3G_READ_SETTINGS_EX 60
#define NETID_HSCAN4 61
#define NETID_HSCAN5 62
#define NETID_RS232 63
#define NETID_UART 64
#define NETID_UART2 65
#define NETID_UART3 66
#define NETID_UART4 67
#define NETID_SWCAN2 68
#define NETID_ETHERNET_DAQ 69
#define NETID_DATA_TO_HOST 70
#define NETID_TEXTAPI_TO_HOST 71
#define NETID_I2C1 71
#define NETID_SPI1 72
#define NETID_OP_ETHERNET6 73
#define NETID_RED_VBAT 74
#define NETID_OP_ETHERNET7 75
#define NETID_OP_ETHERNET8 76
#define NETID_OP_ETHERNET9 77
#define NETID_OP_ETHERNET10 78
#define NETID_OP_ETHERNET11 79
#define NETID_FLEXRAY1A 80
#define NETID_FLEXRAY1B 81
#define NETID_FLEXRAY2A 82
#define NETID_FLEXRAY2B 83
#define NETID_LIN5 84
#define NETID_FLEXRAY 85
#define NETID_FLEXRAY2 86
#define NETID_OP_ETHERNET12 87
#define NETID_MOST25 90
#define NETID_MOST50 91
#define NETID_MOST150 92
#define NETID_ETHERNET 93
#define NETID_GMFSA 94
#define NETID_TCP 95
#define NETID_HSCAN6 96
#define NETID_HSCAN7 97
#define NETID_LIN6 98
#define NETID_LSFTCAN2 99
/**
 * To the next person to add a network, please make it 512!
 */
#define NETID_HW_COM_LATENCY_TEST 512
#define NETID_DEVICE_STATUS 513

/* Upper boundry of Network IDs */
#define NETID_MAX 100
#define NETID_INVALID 0xffff

/* Device types -- value of DeviceType of NeoDevice */
/* Older devices have a value on a specific bit. Those values have not changed 
 * to support existing apps using the api. New devices can fill in between the 
 * existing ones. I know it hurts, but it's just a number!
 */
//clang-format off
#define NEODEVICE_UNKNOWN (0x00000000)
#define NEODEVICE_BLUE (0x00000001)
#define NEODEVICE_ECU_AVB (0x00000002)
#define NEODEVICE_RADSUPERMOON (0x00000003)
#define NEODEVICE_DW_VCAN (0x00000004)
#define NEODEVICE_RADMOON2 (0x00000005)
#define NEODEVICE_RADGIGALOG (0x00000006)
#define NEODEVICE_VCAN41 (0x00000007)
#define NEODEVICE_FIRE (0x00000008)
#define NEODEVICE_VCAN3 (0x00000010)
#define NEODEVICE_RED (0x00000040)
#define NEODEVICE_ECU (0x00000080)
#define NEODEVICE_IEVB (0x00000100)
#define NEODEVICE_PENDANT (0x00000200)
#define NEODEVICE_OBD2_PRO (0x00000400)
#define NEODEVICE_ECUCHIP_UART (0x00000800)
#define NEODEVICE_PLASMA (0x00001000)
#define NEODEVICE_DONT_REUSE0 (0x00002000)//NEODEVICE_FIRE_VNET
#define NEODEVICE_NEOANALOG (0x00004000)
#define NEODEVICE_CT_OBD (0x00008000)
#define NEODEVICE_DONT_REUSE1 (0x00010000)//NEODEVICE_PLASMA_1_12
#define NEODEVICE_DONT_REUSE2 (0x00020000)//NEODEVICE_PLASMA_1_13
#define NEODEVICE_ION (0x00040000)
#define NEODEVICE_RADSTAR (0x00080000)
#define NEODEVICE_DONT_REUSE3 (0x00100000)//NEODEVICE_ION3
#define NEODEVICE_VCAN4 (0x00200000)
#define NEODEVICE_VCAN42 (0x00400000)
#define NEODEVICE_CMPROBE (0x00800000)
#define NEODEVICE_EEVB (0x01000000)
#define NEODEVICE_VCANRF (0x02000000)
#define NEODEVICE_FIRE2 (0x04000000)
#define NEODEVICE_FLEX (0x08000000)
#define NEODEVICE_RADGALAXY (0x10000000)
#define NEODEVICE_RADSTAR2 (0x20000000)
#define NEODEVICE_VIVIDCAN (0x40000000)
#define NEODEVICE_OBD2_SIM (0x80000000)
#define NEODEVICE_ANY_PLASMA (NEODEVICE_PLASMA)
#define NEODEVICE_ANY_ION (NEODEVICE_ION)
#define NEODEVICE_NEOECUCHIP NEODEVICE_IEVB
//clang-format on

#define ISO15765_2_NETWORK_HSCAN 0x01
#define ISO15765_2_NETWORK_MSCAN 0x02
#define ISO15765_2_NETWORK_HSCAN2 0x04
#define ISO15765_2_NETWORK_HSCAN3 0x08
#define ISO15765_2_NETWORK_SWCAN 0x10
#define ISO15765_2_NETWORK_HSCAN4 0x14
#define ISO15765_2_NETWORK_HSCAN5 0x18
#define ISO15765_2_NETWORK_HSCAN6 0x1C
#define ISO15765_2_NETWORK_HSCAN7 0x20
#define ISO15765_2_NETWORK_SWCAN2 0x24

#define PLASMA_SLAVE1_OFFSET 100
#define PLASMA_SLAVE2_OFFSET 200
#define PLASMA_SLAVE_NUM 51

#define PLASMA_SLAVE1_OFFSET_RANGE2 4608
#define PLASMA_SLAVE2_OFFSET_RANGE2 8704
#define PLASMA_SLAVE3_OFFSET_RANGE2 12800

#define SCRIPT_STATUS_STOPPED 0
#define SCRIPT_STATUS_RUNNING 1

#define SCRIPT_LOCATION_FLASH_MEM 0
#define SCRIPT_LOCATION_INTERNAL_FLASH 2
#define SCRIPT_LOCATION_SDCARD 1
#define SCRIPT_LOCATION_VCAN3_MEM 4

/* Protocols -- value of Protocol member of icsSpyMessage */
#define SPY_PROTOCOL_CUSTOM 0
#define SPY_PROTOCOL_CAN 1
#define SPY_PROTOCOL_GMLAN 2
#define SPY_PROTOCOL_J1850VPW 3
#define SPY_PROTOCOL_J1850PWM 4
#define SPY_PROTOCOL_ISO9141 5
#define SPY_PROTOCOL_Keyword2000 6
#define SPY_PROTOCOL_GM_ALDL_UART 7
#define SPY_PROTOCOL_CHRYSLER_CCD 8
#define SPY_PROTOCOL_CHRYSLER_SCI 9
#define SPY_PROTOCOL_FORD_UBP 10
#define SPY_PROTOCOL_BEAN 11
#define SPY_PROTOCOL_LIN 12
#define SPY_PROTOCOL_J1708 13
#define SPY_PROTOCOL_CHRYSLER_JVPW 14
#define SPY_PROTOCOL_J1939 15
#define SPY_PROTOCOL_FLEXRAY 16
#define SPY_PROTOCOL_MOST 17
#define SPY_PROTOCOL_CGI 18
#define SPY_PROTOCOL_GME_CIM_SCL_KLINE 19
#define SPY_PROTOCOL_SPI 20
#define SPY_PROTOCOL_I2C 21
#define SPY_PROTOCOL_GENERIC_UART 22
#define SPY_PROTOCOL_JTAG 23
#define SPY_PROTOCOL_UNIO 24
#define SPY_PROTOCOL_DALLAS_1WIRE 25
#define SPY_PROTOCOL_GENERIC_MANCHSESTER 26
#define SPY_PROTOCOL_SENT_PROTOCOL 27
#define SPY_PROTOCOL_UART 28
#define SPY_PROTOCOL_ETHERNET 29
#define SPY_PROTOCOL_CANFD 30
#define SPY_PROTOCOL_GMFSA 31
#define SPY_PROTOCOL_TCP 32

/* Bitmasks for StatusBitField member of icsSpyMessage */
#define SPY_STATUS_GLOBAL_ERR 0x01
#define SPY_STATUS_TX_MSG 0x02
#define SPY_STATUS_XTD_FRAME 0x04
#define SPY_STATUS_REMOTE_FRAME 0x08
#define SPY_STATUS_CRC_ERROR 0x10
#define SPY_STATUS_CAN_ERROR_PASSIVE 0x20
#define SPY_STATUS_HEADERCRC_ERROR 0x20
#define SPY_STATUS_INCOMPLETE_FRAME 0x40
#define SPY_STATUS_LOST_ARBITRATION 0x80
#define SPY_STATUS_UNDEFINED_ERROR 0x100
#define SPY_STATUS_CAN_BUS_OFF 0x200
#define SPY_STATUS_BUS_RECOVERED 0x400
#define SPY_STATUS_BUS_SHORTED_PLUS 0x800
#define SPY_STATUS_BUS_SHORTED_GND 0x1000
#define SPY_STATUS_CHECKSUM_ERROR 0x2000
#define SPY_STATUS_BAD_MESSAGE_BIT_TIME_ERROR 0x4000
#define SPY_STATUS_TX_NOMATCH 0x8000
#define SPY_STATUS_COMM_IN_OVERFLOW 0x10000
#define SPY_STATUS_EXPECTED_LEN_MISMATCH 0x20000
#define SPY_STATUS_MSG_NO_MATCH 0x40000
#define SPY_STATUS_BREAK 0x80000
#define SPY_STATUS_AVSI_REC_OVERFLOW 0x100000
#define SPY_STATUS_TEST_TRIGGER 0x200000
#define SPY_STATUS_AUDIO_COMMENT 0x400000
#define SPY_STATUS_GPS_DATA 0x800000
#define SPY_STATUS_ANALOG_DIGITAL_INPUT 0x1000000
#define SPY_STATUS_TEXT_COMMENT 0x2000000
#define SPY_STATUS_NETWORK_MESSAGE_TYPE 0x4000000
#define SPY_STATUS_VSI_TX_UNDERRUN 0x8000000
#define SPY_STATUS_VSI_IFR_CRC_BIT 0x10000000
#define SPY_STATUS_INIT_MESSAGE 0x20000000
#define SPY_STATUS_LIN_MASTER 0x20000000
#define SPY_STATUS_CANFD 0x20000000
#define SPY_STATUS_PDU 0x10000000
#define SPY_STATUS_FLEXRAY_PDU SPY_STATUS_PDU
#define SPY_STATUS_HIGH_SPEED 0x40000000
#define SPY_STATUS_EXTENDED 0x80000000 /* if this bit is set than decode StatusBitField3 in AckBytes */
#define SPY_STATUS_FLEXRAY_PDU_UPDATE_BIT_SET 0x40000000
#define SPY_STATUS_FLEXRAY_PDU_NO_UPDATE_BIT 0x08

/* Bitmasks for StatusBitField2 member of icsSpyMessage */
#define SPY_STATUS2_HAS_VALUE 0x1
#define SPY_STATUS2_VALUE_IS_BOOLEAN 0x2
#define SPY_STATUS2_HIGH_VOLTAGE 0x4
#define SPY_STATUS2_LONG_MESSAGE 0x8
#define SPY_STATUS2_GLOBAL_CHANGE 0x10000
#define SPY_STATUS2_ERROR_FRAME 0x20000
#define SPY_STATUS2_END_OF_LONG_MESSAGE 0x100000

/* LIN/ISO Specific - check protocol before handling  */
#define SPY_STATUS2_LIN_ERR_RX_BREAK_NOT_0 0x200000
#define SPY_STATUS2_LIN_ERR_RX_BREAK_TOO_SHORT 0x400000
#define SPY_STATUS2_LIN_ERR_RX_SYNC_NOT_55 0x800000
#define SPY_STATUS2_LIN_ERR_RX_DATA_GREATER_8 0x1000000
#define SPY_STATUS2_LIN_ERR_TX_RX_MISMATCH 0x2000000
#define SPY_STATUS2_LIN_ERR_MSG_ID_PARITY 0x4000000
#define SPY_STATUS2_ISO_FRAME_ERROR 0x8000000
#define SPY_STATUS2_LIN_SYNC_FRAME_ERROR 0x8000000
#define SPY_STATUS2_ISO_OVERFLOW_ERROR 0x10000000
#define SPY_STATUS2_LIN_ID_FRAME_ERROR 0x10000000
#define SPY_STATUS2_ISO_PARITY_ERROR 0x20000000
#define SPY_STATUS2_LIN_SLAVE_BYTE_ERROR 0x20000000
#define SPY_STATUS2_RX_TIMEOUT_ERROR 0x40000000
#define SPY_STATUS2_LIN_NO_SLAVE_DATA 0x80000000
#define SPY_STATUS3_LIN_JUST_BREAK_SYNC 0x1
#define SPY_STATUS3_LIN_SLAVE_DATA_TOO_SHORT 0x2
#define SPY_STATUS3_LIN_ONLY_UPDATE_SLAVE_TABLE_ONCE 0x4

/* MOST Specific - check protocol before handling */
#define SPY_STATUS2_MOST_PACKET_DATA 0x200000
#define SPY_STATUS2_MOST_STATUS 0x400000 /* reflects changes in light/lock/MPR/SBC/etc... */
#define SPY_STATUS2_MOST_LOW_LEVEL 0x800000 /* MOST low level message, allocs, deallocs, remote requests...*/
#define SPY_STATUS2_MOST_CONTROL_DATA 0x1000000
#define SPY_STATUS2_MOST_MHP_USER_DATA 0x2000000 /* MOST HIGH User Data Frame */
#define SPY_STATUS2_MOST_MHP_CONTROL_DATA 0x4000000 /* MOST HIGH Control Data */
#define SPY_STATUS2_MOST_I2S_DUMP 0x8000000
#define SPY_STATUS2_MOST_TOO_SHORT 0x10000000
#define SPY_STATUS2_MOST_MOST50 0x20000000 /* absence of MOST50 and MOST150 implies it's MOST25 */
#define SPY_STATUS2_MOST_MOST150 0x40000000
#define SPY_STATUS2_MOST_CHANGED_PAR 0x80000000 /* first byte in ack reflects what changed. */

/* Ethernet Specific - check protocol before handling */
#define SPY_STATUS2_ETHERNET_CRC_ERROR 0x200000
#define SPY_STATUS2_ETHERNET_FRAME_TOO_SHORT 0x400000
#define SPY_STATUS2_ETHERNET_FCS_AVAILABLE \
	0x800000 /* This frame contains FCS (4 bytes) obtained from ICS Ethernet hardware (ex. RAD-STAR) */
#define SPY_STATUS2_ETHERNET_NO_PADDING 0x1000000
#define SPY_STATUS2_ETHERNET_PREEMPTION_ENABLED 0x2000000

/* FlexRay Specific - check protocol before handling */
#define SPY_STATUS2_FLEXRAY_TX_AB 0x200000
#define SPY_STATUS2_FLEXRAY_TX_AB_NO_A 0x400000
#define SPY_STATUS2_FLEXRAY_TX_AB_NO_B 0x800000
#define SPY_STATUS2_FLEXRAY_TX_AB_NO_MATCH 0x1000000
#define SPY_STATUS2_FLEXRAY_NO_CRC 0x2000000
#define SPY_STATUS2_FLEXRAY_NO_HEADERCRC 0x4000000

/* CAN/CAN-FD Specific - check protocol before handling */
#define SPY_STATUS2_CAN_ISO15765_LOGICAL_FRAME 0x200000
#define SPY_STATUS2_CAN_HAVE_LINK_DATA 0x400000

/* CAN-FD Specific - check protocol before handling */
#define SPY_STATUS3_CANFD_ESI 0x01
#define SPY_STATUS3_CANFD_IDE 0x02
#define SPY_STATUS3_CANFD_RTR 0x04
#define SPY_STATUS3_CANFD_FDF 0x08
#define SPY_STATUS3_CANFD_BRS 0x10

/* Configuration Array constants */
/* HSCAN neoVI or ValueCAN */
#define NEO_CFG_MPIC_HS_CAN_CNF1 (512 + 10)
#define NEO_CFG_MPIC_HS_CAN_CNF2 (512 + 9)
#define NEO_CFG_MPIC_HS_CAN_CNF3 (512 + 8)
#define NEO_CFG_MPIC_HS_CAN_MODE (512 + 54)

/* med speed neoVI CAN */
#define NEO_CFG_MPIC_MS_CAN_CNF1 (512 + 22)
#define NEO_CFG_MPIC_MS_CAN_CNF2 (512 + 21)
#define NEO_CFG_MPIC_MS_CAN_CNF3 (512 + 20)

/* med speed neoVI CAN */
#define NEO_CFG_MPIC_SW_CAN_CNF1 (512 + 34)
#define NEO_CFG_MPIC_SW_CAN_CNF2 (512 + 33)
#define NEO_CFG_MPIC_SW_CAN_CNF3 (512 + 32)

/* med speed neoVI CAN */
#define NEO_CFG_MPIC_LSFT_CAN_CNF1 (512 + 46)
#define NEO_CFG_MPIC_LSFT_CAN_CNF2 (512 + 45)
#define NEO_CFG_MPIC_LSFT_CAN_CNF3 (512 + 44)

/* Constants used to calculate timestamps */
#define NEOVI_TIMESTAMP_2 0.1048576
#define NEOVI_TIMESTAMP_1 0.0000016

#define NEOVIPRO_VCAN_TIMESTAMP_2 0.065536
#define NEOVIPRO_VCAN_TIMESTAMP_1 0.000001

#define NEOVI6_VCAN_TIMESTAMP_2 0.065536
#define NEOVI6_VCAN_TIMESTAMP_1 0.000001

#define NEOVI_RED_TIMESTAMP_2_25NS 107.3741824
#define NEOVI_RED_TIMESTAMP_1_25NS 0.000000025

#define NEOVI_RED_TIMESTAMP_2_10NS 429.4967296
#define NEOVI_RED_TIMESTAMP_1_10NS 0.000000010

#define NEOVI_RED_TIMESTAMP_2_10US 42949.67296
#define NEOVI_RED_TIMESTAMP_1_10US 0.00001

#define NEOVIPRO_VCAN_TIMESTAMP_2_US 65536.0
#define NEOVIPRO_VCAN_TIMESTAMP_1_US 1.0

#define HARDWARE_TIMESTAMP_ID_NONE (unsigned char)0
#define HARDWARE_TIMESTAMP_ID_VSI (unsigned char)1
#define HARDWARE_TIMESTAMP_ID_AVT_716 (unsigned char)2
#define HARDWARE_TIMESTAMP_ID_NI_CAN (unsigned char)3
#define HARDWARE_TIMESTAMP_ID_NEOVI (unsigned char)4
#define HARDWARE_TIMESTAMP_ID_AVT_717 (unsigned char)5
#define HARDWARE_TIMESTAMP_ID_NEOv6_VCAN (unsigned char)6
#define HARDWARE_TIMESTAMP_ID_DOUBLE_SEC (unsigned char)7
#define HARDWARE_TIMESTAMP_ID_NEORED_10US (unsigned char)8
#define HARDWARE_TIMESTAMP_ID_NEORED_25NS (unsigned char)9
#define HARDWARE_TIMESTAMP_ID_NEORED_10NS (unsigned char)10
```





