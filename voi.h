// Copyright 2022 Wolf9466/Wolf0/OhGodAPet

#pragma once

#include <stdint.h>
#include <stdlib.h>

#pragma pack(push, 1)

// The VoltageObjectInfo VBIOS data table, referred to as
// the VOI table, begins with the standard VBIOS table
// header, followed by a variable number of voltage
// objects (VOs). These VOs consist of a header, denoting
// the type (what voltage this VO affects) and mode (the
// format of the VO), and the size of the VO in bytes.

/*
typedef struct
{
	uint8_t VOType;
	uint8_t VOMode;
	uint16_t VOSize;
} VOHdr;
*/

// Known voltage types

#define VOLTAGE_TYPE_VDDC						0x01
#define VOLTAGE_TYPE_MVDDC						0x02
#define VOLTAGE_TYPE_MVDDQ						0x03
#define VOLTAGE_TYPE_VDDCI						0x04
#define VOLTAGE_TYPE_VDDGFX						0x05
#define VOLTAGE_TYPE_PCC						0x06
#define VOLTAGE_TYPE_MVPP						0x07
#define VOLTAGE_TYPE_LEDDPM						0x08
#define VOLTAGE_TYPE_PCC_MVDD					0x09
#define VOLTAGE_TYPE_PCIE_VDDC					0x0A
#define VOLTAGE_TYPE_PCIE_VDDR					0x0B
#define VOLTAGE_TYPE_GENERIC_I2C_1				0x11
#define VOLTAGE_TYPE_GENERIC_I2C_2				0x12
#define VOLTAGE_TYPE_GENERIC_I2C_3				0x13
#define VOLTAGE_TYPE_GENERIC_I2C_4				0x14
#define VOLTAGE_TYPE_GENERIC_I2C_5				0x15
#define VOLTAGE_TYPE_GENERIC_I2C_6				0x16
#define VOLTAGE_TYPE_GENERIC_I2C_7				0x17
#define VOLTAGE_TYPE_GENERIC_I2C_8				0x18
#define VOLTAGE_TYPE_GENERIC_I2C_9				0x19
#define VOLTAGE_TYPE_GENERIC_I2C_10				0x1A

#define VOLTAGE_TYPE_MAX						0x1B

static const char *VoltageTypeNames[VOLTAGE_TYPE_MAX] =
{
	"UNKNOWN/INVALID",
	"VDDC",
	"MVDDC",
	"MVDDQ",
	"VDDCI",
	"VDDGFX",
	"PCC",
	"MVPP",
	"LEDDPM",
	"PCC_MVDD",
	"PCIE_VDDC",
	"PCIE_VDDR",
	"UNKNOWN/INVALID",
	"UNKNOWN/INVALID",
	"UNKNOWN/INVALID",
	"UNKNOWN/INVALID",
	"UNKNOWN/INVALID",
	"VOLTAGE_TYPE_GENERIC_I2C_1",
	"VOLTAGE_TYPE_GENERIC_I2C_2",
	"VOLTAGE_TYPE_GENERIC_I2C_3",
	"VOLTAGE_TYPE_GENERIC_I2C_4",
	"VOLTAGE_TYPE_GENERIC_I2C_5",
	"VOLTAGE_TYPE_GENERIC_I2C_6",
	"VOLTAGE_TYPE_GENERIC_I2C_7",
	"VOLTAGE_TYPE_GENERIC_I2C_8",
	"VOLTAGE_TYPE_GENERIC_I2C_9",
	"VOLTAGE_TYPE_GENERIC_I2C_10"
};

// Known voltage modes

#define	VOLTAGE_MODE_GPIO_LUT					0x00
#define VOLTAGE_MODE_INIT_REGULATOR				0x03
#define VOLTAGE_MODE_VOLTAGE_PHASE				0x04
#define VOLTAGE_MODE_SVID2						0x07
#define VOLTAGE_MODE_EVV						0x08
#define VOLTAGE_MODE_PWRBOOST_LEAKAGE_LUT		0x10
#define VOLTAGE_MODE_HIGH_STATE_LEAKAGE_LUT		0x11
#define VOLTAGE_MODE_HIGH1_STATE_LEAKAGE_LUT	0x12
#define VOLTAGE_MODE_MAX						0x13

static const char *VoltageModeNames[VOLTAGE_MODE_MAX] =
{
	"GPIO_LUT",
	"UNKNOWN/INVALID",
	"UNKNOWN/INVALID",
	"INIT_REGULATOR",
	"VOLTAGE_PHASE",
	"UNKNOWN/INVALID",
	"UNKNOWN/INVALID",
	"SVID2",
	"EVV",
	"UNKNOWN/INVALID",
	"UNKNOWN/INVALID",
	"UNKNOWN/INVALID",
	"UNKNOWN/INVALID",
	"UNKNOWN/INVALID",
	"UNKNOWN/INVALID",
	"UNKNOWN/INVALID",
	"PWRBOOST_LEAKAGE_LUT",
	"HIGH_STATE_LEAKAGE_LUT",
	"HIGH1_STATE_LEAKAGE_LUT",
};

// My personal favorite VO mode is this one, INIT_REGULATOR.
// It sends arbitrary data over I2C to a slave device on the
// bus. Its intention is to be used to configure the registers
// of a VRM controller residing on one of the GPU I2C busses.
// Keep in mind, however, that the I2C device in question
// need not be a VRM controller at all - this should work
// with any I2C device. It consists of an eight-byte header,
// followed by a variable number of address/value pairs,
// where address is the register number, and value is the
// value to write to it.
 
// Its header begins with a regulator ID (I believe 0xFF is
// "any regulator"), followed by the I2C line that the device
// resides on, then the address of the slave device, then a
// field named "ControlOffset" which I have no idea what it
// is for, and VoltageControlFlag, which specifies the I2C
// write format used. The value zero is most common, meaning
// that a SMBus write byte command is used. If nonzero, it
// MAY instead uses an SMBus write word command. TODO: Confirm.
typedef struct
{
	uint8_t RegulatorID;
	uint8_t I2CLine;
	uint8_t I2CAddress;
	uint8_t ControlOffset;
	uint8_t VoltageControlFlag;
	uint8_t Reserved[3];
} VOModeInitRegulator;

typedef struct
{
	uint32_t VoltageID;
	uint16_t VoltageValue;
} VOGPIOLUTEntry;

typedef struct
{
	uint8_t VoltageGPIOCntlID;
	uint8_t GPIOEntryNum;
	uint8_t PhaseDelay;
	uint8_t Reserved;
	uint32_t GPIOMaskValue;
} VOModeGPIOLUT;

//	LoadLineSlopeTrim
// 		- 000: Remove all LL droop from output
//		- 001: Initial LL slope -40%
//		- 010: Initial LL slope -20%
//		- 011: Initial LL slope default value
//		- 100: Initial LL slope +20%
//		- 101: Initial LL slope +40%
//		- 110: Initial LL slope +60%
//		- 111: Initial LL slope +80%

typedef struct
{
	uint16_t OffsetTrim : 2;
	uint16_t LoadLineSlopeTrim : 3;
	uint16_t PSI1 : 1;
	uint16_t PSI0_EN : 1;
	uint16_t PSI0_VID : 8;
	uint16_t Unused : 1;
} SVILLPSI;

typedef struct
{
	union
	{
		uint16_t Value;
		SVILLPSI Info;
	} LoadLinePSI;
	uint8_t SVDGPIOID;
	uint8_t SVCGPIOID;
	uint32_t Reserved;
} VOModeSVID2;
/*
typedef union
{
	VOModeGPIOLUT AsType0;
	VOModeInitRegulator AsType3;
	VOModeSVID2 AsType7;
} VOModeHdr;
*/

typedef struct
{
	
	uint8_t VOType;
	uint8_t VOMode;
	uint16_t VOSize;
	union
	{
		VOModeGPIOLUT AsType0;
		VOModeInitRegulator AsType3;
		VOModeSVID2 AsType7;
	};
} VoltageObject;

// The VO and VOData members should point INTO the VBIOS buffer
typedef struct VOListNode_s
{
	uint32_t VODataLen;
	uint8_t *VOData;
	VoltageObject *VO;
	struct VOListNode_s *prev;
	struct VOListNode_s *next;
} VOListNode;


#pragma pack(pop)

uint16_t CreateVOList(VOListNode **OutputList, uint8_t *VOITableBase, uint8_t DesiredVOMode);
uint16_t SerializeVO(void *OutBuf, const VOListNode *Node, uint32_t OutBufSize);
void DumpVOList(VOListNode *VOList);
void FreeVOList(VOListNode *List);
