/*
 * ELF executable loading
 * Copyright (c) 2003, Jeffrey K. Hollingsworth <hollings@cs.umd.edu>
 * Copyright (c) 2003, David H. Hovemeyer <daveho@cs.umd.edu>
 * $Revision: 1.29 $
 * 
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "COPYING".
 */

#include <geekos/errno.h>
#include <geekos/kassert.h>
#include <geekos/ktypes.h>
#include <geekos/screen.h>  /* for debug Print() statements */
#include <geekos/pfat.h>
#include <geekos/malloc.h>
#include <geekos/string.h>
#include <geekos/elf.h>


static const char ELF_MAGIC_NUMBER[4] = {0x7f, 'E', 'L', 'F'};
#define ELF_MAGIC_SIZE	4

#define EI_CLASS	4 // file class
#define ELFCLASS32	1 // 32-bit objects
#define EI_DATA		5 // data encoding
#define ELFDATA2LSB	1 // LSB encoding
#define EI_VERSION	6 // file version
#define EI_NIDENT	16 // size of ident[]
#define IDENT_SIZE	16 // this is hardcoded in elf.h
#define ET_EXEC		2 // executable type
#define EM_386		3 // 386 architecture
#define EV_CURRENT	1 // current version


static void load_program_header(struct Exe_Segment *exeSegment, programHeader *ph);

/**
 * From the data of an ELF executable, determine how its segments
 * need to be loaded into memory.
 * @param exeFileData buffer containing the executable file
 * @param exeFileLength length of the executable file in bytes
 * @param exeFormat structure describing the executable's segments
 *   and entry address; to be filled in
 * @return 0 if successful, < 0 on error
 */
int Parse_ELF_Executable(char *exeFileData, ulong_t exeFileLength,
    struct Exe_Format *exeFormat)
{
	// sane parameters
	KASSERT(exeFileData != NULL);
	KASSERT(exeFileLength >= sizeof(elfHeader));
	KASSERT(exeFormat != NULL);

	elfHeader *header = (elfHeader *) exeFileData;

	// sane ELF header
	KASSERT(! memcmp(header->ident, ELF_MAGIC_NUMBER, ELF_MAGIC_SIZE));
	KASSERT(header->ehsize == sizeof(elfHeader));
	KASSERT(header->ident[EI_CLASS] == ELFCLASS32);
	KASSERT(header->ident[EI_DATA] == ELFDATA2LSB);
	KASSERT(header->ident[EI_VERSION] == EV_CURRENT);
	KASSERT(header->type == ET_EXEC);
	KASSERT(header->machine == EM_386);
	KASSERT(header->version == EV_CURRENT);
	KASSERT(header->phentsize == sizeof(programHeader));

	// load executable segments
	KASSERT(header->entry != 0);
	exeFormat->entryAddr = header->entry;

	KASSERT(header->phnum > 0 && header->phnum <= EXE_MAX_SEGMENTS);
	exeFormat->numSegments = header->phnum;

	KASSERT(header->phoff > 0 && header->phoff < exeFileLength);
	programHeader *ph = (programHeader *) &exeFileData[header->phoff];

	int i = 0;
	for (; i < header->phnum; ++i, ++ph) {
		// sane program header
		KASSERT(ph->offset < exeFileLength);
		KASSERT(ph->offset + ph->fileSize < exeFileLength);

		load_program_header(&exeFormat->segmentList[i], ph);
	}

	return 0;
}

static void load_program_header(struct Exe_Segment *exeSegment, programHeader *ph)
{
	// copy program header data
	exeSegment->offsetInFile = ph->offset;
	exeSegment->lengthInFile = ph->fileSize;
	exeSegment->startAddress = ph->vaddr;
	exeSegment->sizeInMemory = ph->memSize;
	exeSegment->protFlags = ph->flags;
}

