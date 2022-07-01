#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "vbios-tables.h"
#include "voi.h"

// Serializes a VO for writing. It accepts a pointer to an output buffer,
// a pointer to the node to serialize, and the size of the output buffer
// in bytes. It returns the length of the serialized VO written to OutBuf.
// If the output buffer size is too small, nothing is written, and the
// number of bytes required to serialize the node provided is returned.
uint16_t SerializeVO(void *OutBuf, const VOListNode *Node, uint32_t OutBufSize)
{
	const uint16_t NodeBufLen = Node->VO->VOSize;
	uint8_t *BufPtr = (uint8_t *)OutBuf;
	
	if(NodeBufLen > OutBufSize) return(NodeBufLen);
	
	// We only support serializing VOs with mode INIT_REGULATOR
	// for right now, so bail out if the VO has a different mode.
	if(Node->VO->VOMode != VOLTAGE_MODE_INIT_REGULATOR)
		return(1);
	
	// The first 12 bytes are the VO header and the mode
	// header, so they may be copied verbatim.
	memcpy(BufPtr, Node->VO, sizeof(VoltageObject));
	
	// Following those headers (for objects with mode INIT_REGULATOR)
	// is a variable length set of data for the I2C codes.
	memcpy(BufPtr + sizeof(VoltageObject), Node->VOData, Node->VODataLen);
	
	return(NodeBufLen);
}
// TODO/FIXME: Check Content & Format revisions of the VOI table passed by caller
// This function accepts a pointer to the base of a VOI table in VOITableBase, and
// it accepts a VO mode in DesiredVOMode. It outputs a pointer to a new linked list 
// of VOListNode structures, and returns the amount of entries in the list. Only
// Only VOs with the desired mode are returned. To return all VOs, simply set
// DesiredVOMode to 0xFF.
uint16_t CreateVOList(VOListNode **OutputList, uint8_t *VOITableBase, uint8_t DesiredVOMode)
{
	uint16_t TableSize, CurOffset, EntriesFound = 0;

	if(!OutputList || !VOITableBase) return(0);
	
	*OutputList = NULL;
	
	// Get the size of the table so we know when to stop walking it.	
	TableSize = (((ATOM_COMMON_TABLE_HEADER*)VOITableBase)->usStructureSize);
	
	// Begin walking the table immediately following its header
	CurOffset = sizeof(ATOM_COMMON_TABLE_HEADER);

	while(CurOffset < TableSize)
	{
		VOListNode *CurrentNode, *NewNode;
		VoltageObject *CurVO = (VoltageObject *)(VOITableBase + CurOffset);

		if((CurVO->VOMode == DesiredVOMode) || (DesiredVOMode == 0xFF))
		{		
			// Sanity check for invalid sizes, but do not leak memory
			// on failure - because we set the next pointer and VO
			// data pointer to NULL with every new entry, we may still
			// walk the list and free it.
			if(CurVO->VOSize < sizeof(VoltageObject))
			{
				for(NewNode = CurrentNode = *OutputList; CurrentNode; CurrentNode = NewNode)
				{
					NewNode = CurrentNode->next;
					if(CurrentNode->VOData) free(CurrentNode->VOData);
					
					free(CurrentNode);
				}
				
				return(0);
			}			
			
			if(!(*OutputList)) *OutputList = CurrentNode = NewNode = (VOListNode *)calloc(1, sizeof(VOListNode));
			else
			{
				for(CurrentNode = *OutputList; CurrentNode->next; CurrentNode = CurrentNode->next);
				NewNode = CurrentNode->next = (VOListNode *)calloc(1, sizeof(VOListNode));
				NewNode->prev = CurrentNode;
			}
			
			// If the size of the VO is less than or equal to the
			// sum of the VO header and the VO mode header, then
			// there is no data.
			if(CurVO->VOSize <= sizeof(VoltageObject))
			{
				if(NewNode->VOData) free(NewNode->VOData);
				NewNode->VOData = NULL;
				NewNode->VODataLen = 0;
			}
			else
			{
				NewNode->VODataLen = CurVO->VOSize - sizeof(VoltageObject);
				
				if(!NewNode->VOData) 
					NewNode->VOData = (uint8_t *)calloc(1, NewNode->VODataLen);
				else
					NewNode->VOData = (uint8_t *)realloc(NewNode->VOData, NewNode->VODataLen);
				
				memcpy(NewNode->VOData, VOITableBase + CurOffset + sizeof(VoltageObject), NewNode->VODataLen);
			}
			
			NewNode->VO = CurVO;
			NewNode->next = NULL;
			EntriesFound++;
		}

		CurOffset += CurVO->VOSize;
	}
	return(EntriesFound);
}

void DumpVOList(VOListNode *VOList)
{
	VOListNode *CurVO = VOList;
	
	for(int i = 0; CurVO; CurVO = CurVO->next, ++i)
	{
		printf("\nVOI entry %d:\n", i);
		printf("\tVOType = %d\t(Type \"%s\")\n", CurVO->VO->VOType, VoltageTypeNames[CurVO->VO->VOType]);
		printf("\tVOMode = %d\t(Mode \"%s\")\n", CurVO->VO->VOMode, VoltageModeNames[CurVO->VO->VOMode]);
		printf("\tSize = %d\n", CurVO->VO->VOSize);
		
		switch(CurVO->VO->VOMode)
		{
			case VOLTAGE_MODE_GPIO_LUT:
			{
				printf("\tVoltage GPIO Control ID: 0x%02X\n", CurVO->VO->AsType0.VoltageGPIOCntlID);
				printf("\tGPIO Entry Number: 0x%02X\n", CurVO->VO->AsType0.GPIOEntryNum);
				printf("\tPhase Delay: 0x%02X\n", CurVO->VO->AsType0.PhaseDelay);
				printf("\tGPIO Mask Value: 0x%08X\n", CurVO->VO->AsType0.GPIOMaskValue);
				
				// TODO - walk LUT entries
				break;
			}
			case VOLTAGE_MODE_INIT_REGULATOR:
			{
				printf("\tRegulatorID = %d\n", CurVO->VO->AsType3.RegulatorID);
				printf("\tI2CLine = %d\n", CurVO->VO->AsType3.I2CLine);
				printf("\tI2CAddress = %d\n", CurVO->VO->AsType3.I2CAddress);
				printf("\tControlOffset = %d\n", CurVO->VO->AsType3.ControlOffset);
				printf("\tVoltage entries are %d-bit.\n", (CurVO->VO->AsType3.VoltageControlFlag ? 16 : 8));
				printf("\tData = ");
				for(int i = 0; i < CurVO->VODataLen; ++i)
				{
					if(!(i & 15)) printf("\n\t\t");
					printf("%02X", CurVO->VOData[i]);
				}

				putchar('\n');
				break;
			}
			case VOLTAGE_MODE_SVID2:
			{
				printf("\tLoadLinePSI = 0x%04X\n", CurVO->VO->AsType7.LoadLinePSI.Value);
				printf("\t\tOffsetTrim = %d\n", CurVO->VO->AsType7.LoadLinePSI.Info.OffsetTrim);
				printf("\t\tLoadLineSlopeTrim = %d\n", CurVO->VO->AsType7.LoadLinePSI.Info.LoadLineSlopeTrim);
				printf("\t\tPSI1: %d\n", CurVO->VO->AsType7.LoadLinePSI.Info.PSI1);
				printf("\t\tPSI0_EN: %d\n", CurVO->VO->AsType7.LoadLinePSI.Info.PSI0_EN);
				printf("\t\tPSI0_VID: %d\n", CurVO->VO->AsType7.LoadLinePSI.Info.PSI0_VID);
				printf("\tSVD GPIO ID: %d\n", CurVO->VO->AsType7.SVDGPIOID);
				printf("\tSVC GPIO ID: %d\n", CurVO->VO->AsType7.SVCGPIOID);
				break;
			}
			default:
			{
				printf("\tData = ");
				for(int i = 0; i < CurVO->VODataLen; ++i)
				{
					if(!(i & 15)) printf("\n\t\t");
					printf("%02X", CurVO->VOData[i]);
				}
				putchar('\n');
			}
		}

		putchar('\n');
	}
}

void FreeVOList(VOListNode *List)
{
	VOListNode *NextVO;
	
	for(VOListNode *CurVO = List; CurVO; CurVO = NextVO)
	{
		NextVO = CurVO->next;
		if(CurVO->VOData) free(CurVO->VOData);
		CurVO->VODataLen = 0;
		free(CurVO);
	}
}
