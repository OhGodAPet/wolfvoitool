#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "vbios-tables.h"
#include "wolfvoitool.h"
#include "voi.h"

// Parameter len is bytes in rawstr, therefore, asciistr must have
// at least (len << 1) + 1 bytes allocated, the last for the NULL
void BinaryToASCIIHex(char *restrict asciistr, const void *restrict rawstr, size_t len)
{
	for(int i = 0, j = 0; i < len; ++i)
	{
		asciistr[j++] = "0123456789abcdef"[((uint8_t *)rawstr)[i] >> 4];
		asciistr[j++] = "0123456789abcdef"[((uint8_t *)rawstr)[i] & 0x0F];
	}

	asciistr[len << 1] = 0x00;
}

// Parameter len is the size in bytes of asciistr, meaning rawstr
// must have (len >> 1) bytes allocated
// Maybe asciistr just NULL terminated?
// Returns length of rawstr in bytes
int ASCIIHexToBinary(void *restrict rawstr, const char *restrict asciistr, size_t len)
{
	for(int i = 0, j = 0; i < len; ++i)
	{
		char tmp = asciistr[i];
		if(tmp < 'A') tmp -= '0';
		else if(tmp < 'a') tmp = (tmp - 'A') + 10;
		else tmp = (tmp - 'a') + 10;

		if(i & 1) ((uint8_t *)rawstr)[j++] |= tmp & 0x0F;
		else ((uint8_t *)rawstr)[j] = tmp << 4;
	}

	return(len >> 1);
}

// Returns number of bytes read on success, and zero on error.
size_t ReadVBIOSFile(void *VBIOSOut, const char *FileName, size_t BufSize)
{
	size_t BytesRead;
	FILE *VBIOSFile = fopen(FileName, "rb+");;
	
	if(!VBIOSFile)
	{
		printf("Unable to open %s (does it exist?)\n", FileName);
		return(0);
	}

	BytesRead = fread(VBIOSOut, sizeof(uint8_t), BufSize, VBIOSFile);
	
	// Ensure the read succeeded. The fread function should return
	// the amount read on success. If it's not, it could have been
	// shorter (this is okay) or an error (this is not okay.)
	if((BytesRead != AMD_VBIOS_MAX_SIZE) && !(feof(VBIOSFile)))
	{
		printf("Reading the VBIOS file failed.\n");
		fclose(VBIOSFile);
		return(0);
	}
	
	fclose(VBIOSFile);
	
	return(BytesRead);
}

size_t WriteVBIOSFile(const char *FileName, void *VBIOSData, size_t VBIOSSize)
{
	size_t BytesWritten;
	FILE *VBIOSFile = fopen(FileName, "wb+");;
	
	if(!VBIOSFile)
	{
		printf("Unable to open %s (does it exist?)\n", FileName);
		return(0);
	}

	BytesWritten = fwrite(VBIOSData, sizeof(uint8_t), VBIOSSize, VBIOSFile);
	
	fclose(VBIOSFile);

	// Ensure the write succeeded. The fwrite function should return
	// the amount successfully written.
	if(BytesWritten != VBIOSSize)
	{
		printf("Writing to the VBIOS file failed.\n");
		return(0);
	}
	
	return(BytesWritten);
}


void usage(char *self)
{
	printf("Usage: %s <-f | --file> [-e | --edit]\n", self);
	exit(1);
}

#define NEXT_ARG_CHECK(arg) do { if(i == (argc - 1)) { printf("Argument \"%s\" requires a parameter.\n", arg); if(VBIOSImg) free(VBIOSImg); return(-1); } } while(0)

#define WOLFVOITOOL_MAX_EDTIOR_INPUT_LEN			128

void EditorPrepInput(char *Input)
{
	for(int i = 0; (i < WOLFVOITOOL_MAX_EDTIOR_INPUT_LEN) && Input[i]; ++i)
	{
		Input[i] = toupper(Input[i]);
	}
}


#define VBIOS_GET_PADDING_END(Image)		((uint32_t)((((uint8_t *)(Image))[0x02])) * 512UL)
#define VBIOS_OFFSET(Image, OffsetValue)	(((uint8_t *)Image) + (OffsetValue))
#define VBIOS_GET_ROM_HDR_OFFSET(Image)		(*((uint16_t *)(VBIOS_OFFSET((Image), OFFSET_TO_POINTER_TO_ATOM_ROM_HEADER))))


int32_t PromptForVOEntry(VOListNode *DefaultTemplate)
{
	char Input[2048];

	printf("Set Voltage Object Fields\n");
	printf("Entering 'h' or 'help' will print information\n");
	printf("Pressing enter without any input will set the default (shown in square brackets.)\n\n");

	printf("Voltage Mode (fixed at VOLTAGE_MODE_INIT_REGULATOR): %d\n", VOLTAGE_MODE_INIT_REGULATOR);

	fflush(stdin);

	do
	{
		printf("Voltage Type [%d]: ", DefaultTemplate->VO->VOType);
		fgets(Input, WOLFVOITOOL_MAX_EDTIOR_INPUT_LEN, stdin);

		EditorPrepInput(Input);

		if((!strcmp(Input, "HELP")) || (!strcmp(Input, "H")))
		{
			printf("\nThe Voltage Type indicates which voltage the object refers to.\n");
			printf("1. VDDC\n2. MVDDC\n3. MVDDQ\n4. VDDCI\n5. VDDGFX\n\n");
			continue;
		}

		if(!strcmp(Input, "\n")) break;

		DefaultTemplate->VO->VOType = strtoul(Input, NULL, 10);

		if(((DefaultTemplate->VO->VOType) && (DefaultTemplate->VO->VOType > 5)))
		{
			printf("Invalid input '%s'. Type 'h' for help.\n", Input);
			continue;
		}

	} while(0);

	do
	{
		printf("Regulator ID [%d]: ", DefaultTemplate->VO->AsType3.RegulatorID);
		fgets(Input, WOLFVOITOOL_MAX_EDTIOR_INPUT_LEN, stdin);

		EditorPrepInput(Input);

		if((!strcmp(Input, "HELP")) || (!strcmp(Input, "H")))
		{
			printf("\nRegulator ID is a number identifying the VRM controller.\n");
			continue;
		}

		if(!strcmp(Input, "\n")) break;

		DefaultTemplate->VO->AsType3.RegulatorID = strtoul(Input, NULL, 10);

		if(((!DefaultTemplate->VO->AsType3.RegulatorID) && (errno == EINVAL)))
		{
			printf("Invalid input '%s'. Type 'h' for help.\n", Input);
			continue;
		}

	} while(0);

	do
	{
		printf("I2C Line [%d]: ", DefaultTemplate->VO->AsType3.I2CLine);
		fgets(Input, WOLFVOITOOL_MAX_EDTIOR_INPUT_LEN, stdin);

		EditorPrepInput(Input);

		if((!strcmp(Input, "HELP")) || (!strcmp(Input, "H")))
		{
			printf("\nNumeric identifier for the relevant I2C line.\n");
			printf("May be any integer from 0 - 255 (inclusive.)\n");
			continue;
		}

		if(!strcmp(Input, "\n")) break;

		DefaultTemplate->VO->AsType3.I2CLine = strtoul(Input, NULL, 10);

		if(((!DefaultTemplate->VO->AsType3.I2CLine) && (errno == EINVAL)))
		{
			printf("Invalid input '%s'. Type 'h' for help.\n", Input);
			continue;
		}

	} while(0);

	do
	{
		printf("I2C Address [%d]: ", DefaultTemplate->VO->AsType3.I2CAddress);
		fgets(Input, WOLFVOITOOL_MAX_EDTIOR_INPUT_LEN, stdin);

		EditorPrepInput(Input);

		if((!strcmp(Input, "HELP")) || (!strcmp(Input, "H")))
		{
			printf("\nI2C address in 8-bit format (LSB set to 0.)\n");
			printf("May be any integer from 3 - 119 (inclusive.)\n");
			continue;
		}

		if(!strcmp(Input, "\n")) break;

		DefaultTemplate->VO->AsType3.I2CAddress = strtoul(Input, NULL, 10);

		if(((DefaultTemplate->VO->AsType3.I2CAddress < 3) || (DefaultTemplate->VO->AsType3.I2CAddress > 119)))
		{
			printf("Invalid input '%s'. Type 'h' for help.\n", Input);
			continue;
		}

	} while(0);

	do
	{
		printf("Control Offset [%d]: ", DefaultTemplate->VO->AsType3.ControlOffset);
		fgets(Input, WOLFVOITOOL_MAX_EDTIOR_INPUT_LEN, stdin);

		EditorPrepInput(Input);

		if((!strcmp(Input, "HELP")) || (!strcmp(Input, "H")))
		{
			printf("\nI'll be honest - I don't know what this is for.\n");
			printf("May be any integer from 0 - 255 (inclusive.)\n");
			continue;
		}

		DefaultTemplate->VO->AsType3.ControlOffset = strtoul(Input, NULL, 10);

		if(((!strcmp(Input, "\n")) && (!DefaultTemplate->VO->AsType3.ControlOffset)) && (errno == EINVAL))
		{
			printf("Invalid input '%s'. Type 'h' for help.\n", Input);
			continue;
		}

	} while(0);

	do
	{
		printf("Voltage Control Flag [%d]: ", DefaultTemplate->VO->AsType3.VoltageControlFlag);
		fgets(Input, WOLFVOITOOL_MAX_EDTIOR_INPUT_LEN, stdin);

		EditorPrepInput(Input);

		if((!strcmp(Input, "HELP")) || (!strcmp(Input, "H")))
		{
			printf("\nSet to zero, this indicates the I2C data operates on single bytes.\n");
			printf("If set to 1, the I2C transactions will be 2-byte words. May be only zero or one.\n");
			continue;
		}

		DefaultTemplate->VO->AsType3.VoltageControlFlag = strtoul(Input, NULL, 10);

		if(((!strcmp(Input, "\n")) && (!DefaultTemplate->VO->AsType3.VoltageControlFlag)) && (errno == EINVAL))
		{
			printf("Invalid input '%s'. Type 'h' for help.\n", Input);
			continue;
		}

	} while(0);

	do
	{
		uint8_t I2CTmpBuf[1024];
		uint32_t BufSize = ((WOLFVOITOOL_MAX_EDTIOR_INPUT_LEN > (DefaultTemplate->VO->VOSize - 12)) ? WOLFVOITOOL_MAX_EDTIOR_INPUT_LEN : (DefaultTemplate->VO->VOSize - 12));
		char *I2CInfo = (char *)malloc(sizeof(char) * ((BufSize << 1) + 1));

		BinaryToASCIIHex(I2CInfo, DefaultTemplate->VOData, DefaultTemplate->VODataLen);
		printf("I2C Info [%s]: ", I2CInfo);
		fgets(Input, 1024, stdin);

		EditorPrepInput(Input);

		if((!strcmp(Input, "HELP")) || (!strcmp(Input, "H")))
		{
			printf("\nI2C info for the regulator which this voltage object refers to.\n");
			printf("If you don't know what to put here, you probably should.\n");
			continue;
		}

		if(!strcmp(Input, "\n")) break;

		int32_t NewI2CInfoLen = ASCIIHexToBinary(I2CTmpBuf, Input, strlen(Input));
		int32_t CurI2CInfoLen = DefaultTemplate->VODataLen;
		
		// If there is a buffer for the I2C data, resize it. If there
		// is no buffer for the I2C data, create one.
		if(DefaultTemplate->VOData) DefaultTemplate->VOData = (uint8_t *)realloc(DefaultTemplate->VOData, NewI2CInfoLen);
		else DefaultTemplate->VOData = (uint8_t *)calloc(NewI2CInfoLen, sizeof(uint8_t));
			
		
		DefaultTemplate->VODataLen = NewI2CInfoLen;
		
		if(NewI2CInfoLen > CurI2CInfoLen)
			DefaultTemplate->VO->VOSize += (NewI2CInfoLen - CurI2CInfoLen);
		else
			DefaultTemplate->VO->VOSize -= (CurI2CInfoLen - NewI2CInfoLen);
		
		// Copy the new data in.
		memcpy(DefaultTemplate->VOData, I2CTmpBuf, NewI2CInfoLen);

	} while(0);

	return(0);
}

// Beginning at the padding (UEFI image start minus the modification size),
// copy all bytes up by ModLength bytes, creating empty space at the
// offset specified by ModificationOffset that is ModLength bytes in size.
// Does NOT fix ANY length/size entries!
void PrepareROMForInsertionMod(void *VBIOSImage, size_t PadOffset, size_t ModificationOffset, int32_t ModLength)
{
	uint8_t *ModPosition = ((uint8_t *)VBIOSImage) + ModificationOffset;
	
	memmove(ModPosition + ModLength, ModPosition, PadOffset - ModificationOffset);
}

// Detects the amount of useless filler bytes at the end of the legacy
// ROM image. 
uint32_t VBIOSGetPaddingLength(void *VBIOSImage)
{
	uint32_t PaddingLen = 0;
	
	// Find the UEFI VBIOS image, then walk backward until you
	// find a byte that is not 0xFF Remember the EndOffset would
	// point to the first byte of the UEFI image if we didn't
	// subtract one.
	for(uint32_t EndOffset = (((uint8_t *)VBIOSImage)[0x03] << 9) - 1; ((uint8_t *)VBIOSImage)[EndOffset - PaddingLen] == 0xFF; PaddingLen++);

	return(PaddingLen);
}


void FixTableOffsets(void *VBIOSImage, ATOM_ROM_HEADER *VBIOSROMHdr, uint32_t ChangeStartOffset, int32_t ChangeSize)
{
	uint16_t *VBIOSTableEntry;
	uint32_t DataTableSize, CommandTableSize;
	ATOM_MASTER_DATA_TABLE *MasterDataTable = (ATOM_MASTER_DATA_TABLE *)(VBIOS_OFFSET(VBIOSImage, VBIOSROMHdr->usMasterDataTableOffset));
	ATOM_MASTER_COMMAND_TABLE *MasterCommandTable = (ATOM_MASTER_COMMAND_TABLE *)(VBIOS_OFFSET(VBIOSImage, VBIOSROMHdr->usMasterCommandTableOffset));

	// Since every entry is two bytes, size divided by 2 is all entries
	DataTableSize = MasterDataTable->sHeader.usStructureSize;
	CommandTableSize = MasterCommandTable->sHeader.usStructureSize;

	VBIOSTableEntry = (uint16_t *)(&MasterDataTable->ListOfDataTables);
	for(int i = 0; i < (DataTableSize >> 1); ++i) VBIOSTableEntry[i] += ((VBIOSTableEntry[i] > ChangeStartOffset) ?  ChangeSize : 0);

	VBIOSTableEntry = (uint16_t *)(&MasterCommandTable->ListOfCommandTables);
	for(int i = 0; i < (CommandTableSize >> 1); ++i) VBIOSTableEntry[i] += ((VBIOSTableEntry[i] > ChangeStartOffset) ?  ChangeSize : 0);
}

#if 1

void EditorMenu(VOListNode *NodeList, void *VBIOSImg)
{
	uint8_t *VBIOSImgBase = (uint8_t *)VBIOSImg;
	
	// Outermost loop of editor menu. Offers the choices to
	// add an entry, edit an existing entry, or quit.
	do
	{
		char InputStr[WOLFVOITOOL_MAX_EDTIOR_INPUT_LEN + 1];
		uint8_t I2CInfoTmpBuf[1024];

		printf("\nPress 'e' to edit an entry, 'a' to add one, or 'q' to quit: ");

		fflush(stdin);
		fgets(InputStr, WOLFVOITOOL_MAX_EDTIOR_INPUT_LEN, stdin);

		EditorPrepInput(InputStr);
		
		VOListNode *CurNode = NodeList;
		ATOM_ROM_HEADER *ROMHdr = (ATOM_ROM_HEADER *)(VBIOSImg + *((uint16_t *)(VBIOSImgBase + OFFSET_TO_POINTER_TO_ATOM_ROM_HEADER)));
		const uint32_t VOITblOffset = ((ATOM_MASTER_DATA_TABLE *)(VBIOSImg + ROMHdr->usMasterDataTableOffset))->ListOfDataTables.VoltageObjectInfo;
		ATOM_COMMON_TABLE_HEADER *VOIHdr = (ATOM_COMMON_TABLE_HEADER *)(VBIOSImg + VOITblOffset);

		// Option 'E' - editing an existing VO.
		if(!strcmp(InputStr, "E\n"))
		{
			if(!NodeList)
			{
				printf("No supported entries to edit!\n");
				continue;
			}
			
			// Submenu for edit option, getting an index from the
			// user and allowing them to input all fields of a
			// VO with mode INIT_REGULATOR. 
			do
			{
				uint32_t SelectedIdx;
				
				printf("Enter index of entry to edit, or 'q' to return to the main menu: ");

				fgets(InputStr, WOLFVOITOOL_MAX_EDTIOR_INPUT_LEN, stdin);

				EditorPrepInput(InputStr);
				
				// Exit edit submenu early.
				if(!strcmp(InputStr, "Q\n")) break;

				// Empty line - ask for input again.
				if(!strcmp(InputStr, "\n")) continue;

				
				// Fetch index and sanity check it.
				SelectedIdx = strtoul(InputStr, NULL, 10);

				if((!SelectedIdx) && (errno == EINVAL))
				{
					printf("Invalid input '%s'.\n", InputStr);
					continue;
				}
				
				// Loop over entries in the VO list until we either
				// find the one the user wanted, or we reach the end.
				
				CurNode = NodeList;
				
				for(int idx = 0; (idx < SelectedIdx) && CurNode; ++idx, CurNode = CurNode->next);
				
				// We reached the end before finding the requested
				// entry in the list. Ask again.
				if(!CurNode)
				{
					printf("Entry with index '%d' does not exist.\n", SelectedIdx);
					continue;
				}
				
				// Note that the PromptForVOEntry() function modifies the node
				// that was passed to it as a template.
				uint16_t OldVOSize = CurNode->VO->VOSize;
				PromptForVOEntry(CurNode);
				
				ssize_t SizeDiff = CurNode->VO->VOSize - OldVOSize;
				const uint32_t VBIOSPadLen = VBIOSGetPaddingLength(VBIOSImgBase);
				
				if(SizeDiff > VBIOSPadLen) abort();
				
				// Remember that the VO pointer within the node actually
				// points inside our VBIOS image. Get the offset of our
				// target VO - we will shift everything past that point,
				// including the old VO data, forward by ModLen bytes.
				size_t ModOffset = ((uint8_t *)CurNode->VO) - VBIOSImgBase;
				
				// This shifts everything from our VO onward forward
				// or backward by SizeDiff bytes. It lops off the last
				// SizeDiff bytes from the end of the legacy ROM, in
				// the case that SizeDiff is positive! These bytes are
				// taken from the legacy VBIOS padding area - just
				// ensure you have enough padding bytes!
				PrepareROMForInsertionMod(VBIOSImgBase, VBIOS_GET_PADDING_END(VBIOSImgBase) - SizeDiff, ModOffset, SizeDiff);
				
				// Insert the entire new VO, now that there is room.
				SerializeVO(CurNode->VO, CurNode, CurNode->VO->VOSize);
				
				// Correct length of entire VOI table header
				VOIHdr->usStructureSize += SizeDiff;
				
				// Correct all table offsets for tables (data and command) which were affected
				FixTableOffsets(VBIOSImg, ROMHdr, ModOffset, SizeDiff);
			} while(0);
		}
		// Option 'A' - append to VO list
		else if(!strcmp(InputStr, "A\n"))
		{
			VOListNode *NewNode, *TempNode;
			
			// Two extra bytes for the 0xFF 0x00 default I2C data
			//VOIEntry *NewEntry = (VOIEntry *)calloc(1, sizeof(VOIEntry) + 2);
			//VoltageObject *NewVO = (VoltageObject *)calloc(1, sizeof(VoltageObject) + 2);
			
			// Walk the list until we get to the end. WARNING: assumes
			// that NodeList is not NULL!
			for(CurNode = NodeList; CurNode->next; CurNode = CurNode->next);
			
			// Add a node to the list, and fill in a few sane defaults.
			TempNode = (VOListNode *)calloc(1, sizeof(VOListNode));
			VoltageObject *VO = (VoltageObject *)calloc(1, sizeof(VoltageObject));
						
			// Fields not set have been zeroed during allocation
			TempNode->VO->VOType = VOLTAGE_TYPE_VDDC;
			TempNode->VO->VOMode = VOLTAGE_MODE_INIT_REGULATOR;
			TempNode->VO->VOSize = sizeof(VoltageObject) + 2;
			
			TempNode->VO->AsType3.RegulatorID = 0x08;
			TempNode->VO->AsType3.I2CLine = 150;
			TempNode->VO->AsType3.I2CAddress = 0x10;
			
			// Set the data portion to the terminator (0xFF00)
			TempNode->VOData = (uint8_t *)malloc(sizeof(uint8_t) * 2);
			TempNode->VODataLen = 2;
			((uint8_t *)TempNode->VOData)[0] = 0xFF;
			((uint8_t *)TempNode->VOData)[1] = 0x00;

			// In this case, since there is no object being replaced,
			// the entire object is going to have to be inserted. This
			// is performed in PromptForVOEntry() - the template entry
			// it is passed gets filled with user input.
			PromptForVOEntry(TempNode);
			
			// Since we are inserting an entire VO, the size difference is
			// simply the size of the VO itself.
			ssize_t SizeDiff = TempNode->VO->VOSize;
			const uint32_t VBIOSPadLen = VBIOSGetPaddingLength(VBIOSImgBase);
			
			// Our modification offset is the very end of VOI.
			size_t ModOffset = VOIHdr->usStructureSize + VOITblOffset;
			
			if(SizeDiff > VBIOSPadLen) abort();
			
			// Make some space at the end of the VOI table.
			PrepareROMForInsertionMod(VBIOSImgBase, VBIOS_GET_PADDING_END(VBIOSImgBase) - SizeDiff, ModOffset, SizeDiff);
			
			// Insert the new VO.
			SerializeVO(VBIOSImgBase + ModOffset, TempNode, TempNode->VO->VOSize);
			
			// Correct all table offsets for tables (data and command) which were affected
			FixTableOffsets(VBIOSImg, ROMHdr, ModOffset, SizeDiff);
			
			// Correct length of entire VOI table header
			VOIHdr->usStructureSize += SizeDiff;
		}
		else if(!strcmp(InputStr, "Q\n"))
		{
			break;
		}
	} while(1);
	
	return;
}

#endif

int main(int argc, char **argv)
{
	uint8_t *VBIOSImg = NULL;
	uint32_t VOITblOffset;
	uint16_t VOCount;
	size_t VBIOSSize;
	char *VBIOSFileName;
	uint32_t OrigUEFIVBIOSLen, OrigLegacyVBIOSLen;
	VOListNode *VOList;
	ATOM_ROM_HEADER *ROMHdr;
	ATOM_COMMON_TABLE_HEADER *VOIHdr;
	bool Editing = false;
	
	fprintf(stderr, "wolfvoitool v%s by Wolf9466 (aka Wolf0/OhGodAPet)\n", WOLFVOITOOL_VERSION_STR);
	fprintf(stderr, "Donation address (BTC): 1WoLFumNUvjCgaCyjFzvFrbGfDddYrKNR\n");
	
	if(argc < 2) usage(argv[0]);
	
	for(int i = 1; i < argc; ++i)
	{
		if(!strcmp(argv[i], "-f") || !strcmp(argv[i], "--file"))
		{
			NEXT_ARG_CHECK(argv[i]);
			
			VBIOSFileName = argv[++i];
			VBIOSImg = (uint8_t *)malloc(sizeof(uint8_t) * AMD_VBIOS_MAX_SIZE);
			
			VBIOSSize = ReadVBIOSFile(VBIOSImg, VBIOSFileName, AMD_VBIOS_MAX_SIZE);
			
			if(!VBIOSSize)
			{
				free(VBIOSImg);
				return(-1);
			}
			
			// Record the sizes of both VBIOS images for later.
			OrigLegacyVBIOSLen = ((uint8_t *)VBIOSImg)[0x03] << 9;
			OrigUEFIVBIOSLen = VBIOSSize - OrigLegacyVBIOSLen;
		}
		else if(!strcmp(argv[i], "-e") || !strcmp(argv[i], "--edit"))
		{
			Editing = true;
		}
		else
		{
			printf("Unknown parameter \"%s\".\n", argv[i]);
			if(VBIOSImg) free(VBIOSImg);
			usage(argv[0]);
		}
	}
	
	ROMHdr = (ATOM_ROM_HEADER *)(VBIOSImg + *((uint16_t *)(VBIOSImg + OFFSET_TO_POINTER_TO_ATOM_ROM_HEADER)));
	VOITblOffset = ((ATOM_MASTER_DATA_TABLE *)(VBIOSImg + ROMHdr->usMasterDataTableOffset))->ListOfDataTables.VoltageObjectInfo;

	VOIHdr = (ATOM_COMMON_TABLE_HEADER *)(VBIOSImg + VOITblOffset);

	printf("VOI Table Format Revision 0x%02X, Content Revision 0x%02X.\n", VOIHdr->ucTableFormatRevision, VOIHdr->ucTableContentRevision);
	
	// If the user wants to modify the VOI table, then only show
	// entries which we support editing (which is only those with
	// mode INIT_REGULATOR at the moment.) Otherwise, display all
	// VOI entries.
	if(Editing) VOCount = CreateVOList(&VOList, VBIOSImg + VOITblOffset, VOLTAGE_MODE_INIT_REGULATOR);
	else VOCount = CreateVOList(&VOList, VBIOSImg + VOITblOffset, 0xFF);
	
	DumpVOList(VOList);
	
	if(Editing)
	{
		
		EditorMenu(VOList, VBIOSImg);
		
		// We need the length of the total image, because we
		// did not track how much the VBIOS may have changed
		// in size. As such, take the current length of the
		// legacy image (this value is in byte at offset 0x03,
		// in 512-byte units), and subtract the original UEFI
		// VBIOS length.
		// Because we know we never edit the UEFI VBIOS, its
		// size MUST have remained the same, and if we added
		// blocks to the legacy image, this will be reflected
		// in the length byte.
	
		uint32_t NewImgLen = (((uint8_t *)VBIOSImg)[0x03] << 9) + OrigUEFIVBIOSLen;
		
		// Sanity check
		if(NewImgLen > AMD_VBIOS_MAX_SIZE)
		{
			printf("VBIOS metadata is badly fucked.\n");
			free(VBIOSImg);
			return(-1);
		}
		
		WriteVBIOSFile(VBIOSFileName, VBIOSImg, NewImgLen);
	}
	
	FreeVOList(VOList);
	
	free(VBIOSImg);
	
	return(0);
}

	
	
	
