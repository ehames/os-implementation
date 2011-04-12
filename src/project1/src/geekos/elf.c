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
	int result = -1, i = 0;

	// sane parameters
	if (exeFileData == NULL) goto out;
	if (exeFileLength < sizeof(elfHeader)) goto out;
	if (exeFormat == NULL) goto out;

	elfHeader *header = (elfHeader *) exeFileData;

	// sane ELF header
	if (memcmp(header->ident, ELF_MAGIC_NUMBER, ELF_MAGIC_SIZE)) goto out;
	if (header->ehsize != sizeof(elfHeader)) goto out;
	if (header->ident[EI_CLASS] != ELFCLASS32) goto out;
	if (header->ident[EI_DATA] != ELFDATA2LSB) goto out;
	if (header->ident[EI_VERSION] != EV_CURRENT) goto out;
	if (header->type != ET_EXEC) goto out;
	if (header->machine != EM_386) goto out;
	if (header->version != EV_CURRENT) goto out;
	if (header->phentsize != sizeof(programHeader)) goto out;

	// load executable segments
	if (header->entry == 0) goto out;
	exeFormat->entryAddr = header->entry;

	if (header->phnum <= 0 || header->phnum > EXE_MAX_SEGMENTS) goto out;
	exeFormat->numSegments = header->phnum;

	if (header->phoff <= 0 || header->phoff >= exeFileLength) goto out;
	programHeader *ph = (programHeader *) &exeFileData[header->phoff];

	for (i = 0; i < header->phnum; ++i, ++ph) {
		// sane program header
		if (ph->offset >= exeFileLength) goto out;
		if (ph->offset + ph->fileSize >= exeFileLength) goto out;

		load_program_header(&exeFormat->segmentList[i], ph);
	}
	result = 0;
out:
	return result;
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

