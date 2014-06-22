/*
 *  dos-exe.c -- kb.exe reader
 *  Copyright (C) 2013 Vitaly Driedfruit
 *
 *  This file is part of openkb.
 *
 *  openkb is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  openkb is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with openkb.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "kbsys.h"
#include "kbfile.h"
#include "kbdir.h"

#include "kbres.h"

#include "kbstd.h" /* for malloc */
#include "dos-cc.h" /* for KB_funLZW */

#define KBEXETYPE_RAW   	0
#define KBEXETYPE_EXEPACK	1
#define KBEXETYPE_COMPRESS	2

struct exeHeader {
	unsigned short signature; /* == 0x5a4D, "MZ" */
	unsigned short bytes_in_last_block;
	unsigned short blocks_in_file;
	unsigned short num_relocs;
	unsigned short header_paragraphs;
	unsigned short min_extra_paragraphs;
	unsigned short max_extra_paragraphs;
	unsigned short ss;
	unsigned short sp;
	unsigned short checksum;
	unsigned short ip;
	unsigned short cs;
	unsigned short reloc_table_offset;
	unsigned short overlay_number;
};

const int mzSize = 0x1C; /* size of "struct exeHeader" */
const int unpackerDataLen = 0x12; /* size of "struct exepackHeader" */
const int unpackerLen = 0x105; /* size of unpacker code */
const char errString[0x16] = "Packed file is corrupt";
const int errLen = sizeof(errString);

struct exepackHeader {
	unsigned short real_start_OFFSET;	/* real start address (offset) */
	unsigned short real_start_SEGMENT;	/* real start address (segment) */
	unsigned short mem_start_SEGMENT;	/* start of the exe in memory (segment), 0000, must be set by program */
	unsigned short unpacker_len;    	/* size of <unpacker vars + unpacker code + error string + packed reloc table> in bytes */
	unsigned short real_stack_OFFSET;	/* real stack (offset) */
	unsigned short real_stack_SEGMENT;	/* real stack (segment) */
	unsigned short dest_len;        	/* destination of the unpacker code (in paragraphs, relative to start of exe in memory) */
	unsigned short skip_len;        	/* <number of paragraphs between packed exe and unpacker variables> + 1 */
	unsigned short signature; /* == 0x5242, "RB" */
};

/*
 * _The_ algorithm. Thanks aowen.
 *
 * This function returns number of bytes unpacked (or -1), and writes number of
 *  bytes read into "res".
 *
 * srcPos and dstPos should point to ENDs of respective input and output locations,
 *  as unpacking is performed backwards. No error-checking on size is done, beware
 *  of buffer overflows.
 */
int exepack_unpack(unsigned char *dstPos, unsigned char *srcPos, int *res) {
  int i, n = 0;
  unsigned char *lastPos = srcPos;

  int commandByte, lengthWord, fillByte;

  /* skip all 0xff bytes (they're just padding to make the packed exe's size a multiple of 16 */
  while (*srcPos == 0xff) {
    srcPos--;
  }
  /* unpack */
  do {
    commandByte = *(srcPos--);
    switch (commandByte & 0xFE) {
      /* (byte)value (word)length (byte)0xb0 */
      /* writes a run of <length> bytes with a value of <value> */
      case 0xb0:
        lengthWord = (*(srcPos--))*0x100;
        lengthWord += *(srcPos--);
        fillByte = *(srcPos--);
        for (i = 0; i < lengthWord; i++) {
          *(dstPos--) = fillByte;
        }
        n += lengthWord;
        break;
      /* (word)length (byte)0xb2 */
      /* copies the next <length> bytes */
      case 0xb2:
        lengthWord = (*(srcPos--))*0x100;
        lengthWord += *(srcPos--);
        for (i = 0; i < lengthWord; i++) {
          *(dstPos--) = *(srcPos--);
        }
        n += lengthWord;
        break;
      /* unknown command */
      default:
        KB_debuglog(0, "[exepack] Unknown command %2x at position %d\n", commandByte, lastPos - srcPos);
        n = -1;
        break;
    }
  } while ((commandByte & 1) != 1); /* lowest bit set => last block */
  *res = lastPos - srcPos;
  return n;
}


void exeHeader_read(struct exeHeader *h, unsigned char *buffer) { /* This boring code is portable, unlike fread(struct..) */
	h->signature = READ_WORD(buffer);
	h->bytes_in_last_block = READ_WORD(buffer);
	h->blocks_in_file = READ_WORD(buffer);
	h->num_relocs = READ_WORD(buffer);
	h->header_paragraphs = READ_WORD(buffer);
	h->min_extra_paragraphs = READ_WORD(buffer);
	h->max_extra_paragraphs = READ_WORD(buffer);
	h->ss = READ_WORD(buffer);
	h->sp = READ_WORD(buffer);
	h->checksum = READ_WORD(buffer);
	h->ip = READ_WORD(buffer);
	h->cs = READ_WORD(buffer);
	h->reloc_table_offset = READ_WORD(buffer);
	h->overlay_number = READ_WORD(buffer);
}

void exeHeader_write(unsigned char *buffer, struct exeHeader *h) { /* This boring code is portable, unlike fwrite(struct..) */
	WRITE_WORD(buffer, h->signature);
	WRITE_WORD(buffer, h->bytes_in_last_block);
	WRITE_WORD(buffer, h->blocks_in_file);
	WRITE_WORD(buffer, h->num_relocs);
	WRITE_WORD(buffer, h->header_paragraphs);
	WRITE_WORD(buffer, h->min_extra_paragraphs); 
	WRITE_WORD(buffer, h->max_extra_paragraphs);
	WRITE_WORD(buffer, h->ss);
	WRITE_WORD(buffer, h->sp);
	WRITE_WORD(buffer, h->checksum);
	WRITE_WORD(buffer, h->ip);
	WRITE_WORD(buffer, h->cs);
	WRITE_WORD(buffer, h->reloc_table_offset);
	WRITE_WORD(buffer, h->overlay_number);
}


void exepackHeader_read(struct exepackHeader *h, unsigned char *buffer) { /* This boring code is portable, unlike fread(struct..) */
	h->real_start_OFFSET = READ_WORD(buffer);
	h->real_start_SEGMENT = READ_WORD(buffer);
	h->mem_start_SEGMENT = READ_WORD(buffer);
	h->unpacker_len = READ_WORD(buffer);
	h->real_stack_OFFSET = READ_WORD(buffer);
	h->real_stack_SEGMENT = READ_WORD(buffer);
	h->dest_len = READ_WORD(buffer);
	h->skip_len = READ_WORD(buffer);
	h->signature = READ_WORD(buffer);
}

//int KB_funLZW(char *result, unsigned int max, KB_File *f); /* from dos-cc.c */

KB_File* execomp_uncompress(KB_File *f)
{
	KB_File *uf = NULL;	// unpacked version

	unsigned char mzHeader[0x20];

	struct exeHeader mz;

	int n;

	KB_fseek(f, 0, SEEK_SET);
	n = KB_fread(mzHeader, sizeof(char), mzSize, f); /* read bytes */

	if (n < mzSize) {
		KB_debuglog(0, "[execomp] Unable to read EXE header\n");
		return NULL;
	}

	exeHeader_read(&mz, &mzHeader[0]); /* move bytes to struct */

	if (mz.signature != 0x5a4D) {
		KB_debuglog(0, "[execomp] Unable to parse EXE header\n");
		return NULL;
	}

	/* Verify this file is actually compressed, and not just an exe */
	/* We check if 0x200 contains "e99900". Another approach is to check
	 * if 0x24d contains "KB.EXE" */
	char mini_buf[4];
	KB_fseek(f, 0x200, SEEK_SET);
	n = KB_fread(mzHeader, sizeof(char), 4, f); /* read bytes */
	if (n < 4 || mini_buf[0] != 0xE9 || mini_buf[1] != 0x99 || mini_buf[2] != 0x00) {
		KB_debuglog(0, "[execomp] No signature found (%02x%02x%02x at 0x200)\n", mini_buf[0],mini_buf[1],mini_buf[2]);
		return NULL;
	}

	//print_EXE(&mz, inputFile); /* show some info to stdout */

	int exe_data_start = mz.header_paragraphs * 16L;

	int extra_data_start = mz.blocks_in_file * 512L;
	if (mz.bytes_in_last_block)
  		extra_data_start -= (512 - mz.bytes_in_last_block);

	//printf("Compressed header starts at %d  %08x\n", extra_data_start, extra_data_start);

	unsigned char mzHeader2[0x25];

	KB_fseek(f, extra_data_start, SEEK_SET);

	/* read packed exe header */
	n = KB_fread(mzHeader2, sizeof(char), 0x25, f); /* read bytes */

	struct exeHeader innerMz;

	exeHeader_read(&innerMz, &mzHeader2[0]); /* move bytes to struct */

	int exe_data_start2 = innerMz.header_paragraphs * 16L;

	int extra_data_start2 = innerMz.blocks_in_file * 512L;
	if (innerMz.bytes_in_last_block)
  		extra_data_start2 -= (512 - innerMz.bytes_in_last_block);

	//printf("Compressed   data starts at +%d +%08x\n", exe_data_start2, exe_data_start2);

	int expected_size = extra_data_start2 - exe_data_start2;

//	printf("Expected data size: %d (%04x)\n",expected_size,expected_size); 
//	print_EXE(&innerMz, "<packed>");

	KB_fseek(f, extra_data_start + exe_data_start2, SEEK_SET);

	unsigned char mzHeader3[0x25];

	innerMz.header_paragraphs = 0x02;
	innerMz.reloc_table_offset = mzSize;

	int fullSize = expected_size + 0x20;

	innerMz.blocks_in_file = fullSize / 512 + 1;
	innerMz.bytes_in_last_block = fullSize % 512;

	innerMz.min_extra_paragraphs = fullSize / 64;	/* I don't know how to determine BSS properly :/ */

	/* Write everything out */
	char *buffer;

	buffer = malloc(sizeof(char) * fullSize);

	if (buffer == NULL) {
		return NULL;
	}

	/* un-LZW */
	//KB_fseek(f, grp->head.files[i].offset + 4, 0);
	n = KB_funLZW(&buffer[0x20], expected_size, f);
	//WAS: n = load_infile();

	if (n != expected_size) {
		return NULL;
	}

	/* add new header */
	exeHeader_write(&buffer[0], &innerMz);
	buffer[0x1c] = buffer[0x1d] = buffer[0x1e] = buffer[0x1f] = 0x00; //garbage to fill from 0x1c to 0x20

	/* allocate resulting KB_file */
	uf = malloc(sizeof(KB_File));

	if (uf == NULL) {
		return NULL;
	}

	/* setup resulting KB_file */
	uf->type = KBFTYPE_BUF;
	uf->d = (void*)buffer;
	uf->len = fullSize;
	uf->pos = 0;
	uf->prev = NULL;
	uf->ref_count = 0;

	return uf;
}

KB_File* exepack_uncompress(KB_File *f)
{
	KB_File *uf;	/* output */
	
	/* read exe header */
	unsigned char mzHeader[0x20];

	struct exeHeader mz;

	int n;

	KB_fseek(f, 0, SEEK_SET);
	n = KB_fread(mzHeader, sizeof(char), mzSize, f); /* read bytes */

	if (n < mzSize) {
		KB_debuglog(0, "[exepack] Unable to read EXE header\n");
		return NULL;
	}

	exeHeader_read(&mz, &mzHeader[0]); /* move bytes to struct */

	if (mz.signature != 0x5a4D) {
		KB_debuglog(0, "[exepack] Unable to parse EXE header\n");
		return NULL;
	}

	//print_EXE(&mz, inputFile); /* show some info to stdout */

	int exe_data_start = mz.header_paragraphs * 16L;

	int extra_data_start = mz.blocks_in_file * 512L;
	if (mz.bytes_in_last_block)
		extra_data_start -= (512 - mz.bytes_in_last_block);

	int first_offset = mz.cs * 0x10;// + exe_data_start;

	int exeLen = first_offset;
	//printf("\t Predicted packed exe : %d bytes\n", exeLen);

	/* read packed data */
	unsigned char buffer[exeLen];

	KB_fseek(f, exe_data_start, SEEK_SET);

	n = KB_fread(&buffer[0], sizeof(char), exeLen, f);

	if (n < exeLen) {
		KB_debuglog(0, "[exepack] Unable to read PACKED data\n");
		return NULL;
	}

	/* read unpacker data */
	unsigned char packData[unpackerDataLen];

	struct exepackHeader packed;

	n = KB_fread(packData, sizeof(char), unpackerDataLen, f);
	
	if (n < unpackerDataLen) {
		KB_debuglog(0, "[exepack] Unable to read EXEPACK variables from %08x\n", exe_data_start + exeLen);
		return NULL;
	}

	exepackHeader_read(&packed, &packData[0]);	/* move bytes to struct */

	if (packed.signature != 0x4252) {
		KB_debuglog(0, "Not an EXEPACK file / corrupt EXEPACK header. %08x\n", packed.signature);
		return NULL;
	}

	/* show some info */
	/* printf("Packed CS:IP\t\t\t%04x:%04x\nPacked SS:SP\t\t\t%04x:%04x\nStart of EXE: %04x\n", 
		packed.real_start_SEGMENT, packed.real_start_OFFSET,
		packed.real_stack_SEGMENT, packed.real_stack_OFFSET,
		packed.mem_start_SEGMENT);

	printf("Skip len (para)\t\t\t%04x\t\t%d\nexepack size (bytes)\t\t%04x\t\t%d\nFull length: %d*16 = %d\n",
		packed.skip_len, packed.skip_len,
		packed.unpacker_len, packed.unpacker_len,
		packed.dest_len, packed.dest_len * 16);
	*/
	/* read the rest of unpacker code/data */
	int pack_buffer = packed.unpacker_len - unpackerDataLen; /* everything after unpacker variables */

	unsigned char pbuffer[pack_buffer];

	n = KB_fread(pbuffer, sizeof(char), pack_buffer, f);

	if (n < pack_buffer) {
		KB_debuglog(0, "[exepack] Unable to read %d bytes of unpacker code/data, have %d\n", pack_buffer, n);
		return NULL;
	}

	int reloc_table_size = packed.unpacker_len - errLen - unpackerLen - unpackerDataLen;
	int reloc_num_entries = (reloc_table_size - 16 * sizeof(word)) / 2;
	int reloc_table_full = reloc_num_entries * 2 * sizeof(word);
	//printf("\t Predicted packed reloc.table : %d bytes (%d entries)\n", reloc_table_size, reloc_num_entries);
	//printf("\t Unpacked relocation table : %d bytes\n", reloc_table_full);

	//int exeLen = extra_data_start - packed.unpacker_len;

	int finalSize = packed.dest_len * 16L;
	//printf("\tExpected unpacked size: %d bytes\n", finalSize);

	char out[finalSize];
	memset(&out[0], 0xFF, finalSize);
	memcpy(&out[0], &buffer[0], exeLen);

	int pLen = unpackerLen + errLen;

	if (memcmp(&pbuffer[pLen - errLen], errString, errLen)) {
		KB_debuglog(0, "[exepack] RB String is wrong!\n");
	}

	int r;

	n = exepack_unpack(&out[finalSize - 1], &buffer[exeLen - 1], &r);

	if (n == -1) {
		KB_debuglog(0, "[exepack] Unable to UNPACK data\n");
		return NULL;
	}

	//printf("Unpacked %d bytes (from %d packed bytes) into [%08x--%08x]\n", n, r, exeLen - 1, exeLen - 1 - r);

	/* unpack relocation table */
	int section = 0;

	char rout[reloc_table_full];

	int relocSize = 0;
	
	char *pb = &pbuffer[pLen];

	char *pw = &rout[0];

	for (section = 0; section < 16; section++) {

		int num_entries;
		
		num_entries = READ_WORD(pb);

		if (num_entries == 0) break;

		int k;
		for (k = 0; k < num_entries; k++) {

			int entry;
			
			entry = READ_WORD(pb);

			word patchSegment = 0x1000 * section;
			word patchOffset = entry;

			WRITE_WORD(pw, patchOffset);relocSize += 2;
			WRITE_WORD(pw, patchSegment);relocSize += 2;

//			patchOffset += 0x20;	// MZ header size
//			printf("%d. Entry -- (%d) %08x --- (%d) %08x\n", k, entry, entry, patchOffset, patchOffset);
/*			short *patch = &rout[patchOffset];
//			printf("%d. Must patch DI %08x -- %08x + %08x\n", k, patchOffset, *patch, EXEPACK_mem_start_SEGMENT);
			*patch += EXEPACK_mem_start_SEGMENT;
*/

		}
	}

	/* prepare new exe header */
	struct exeHeader newexe;

	char mzOut[mzSize];

	int headerSize = mzSize + relocSize;

	newexe.signature = 0x5a4D;
	newexe.header_paragraphs = headerSize / 16L;
#if 1
	/* For some OSes (which?), header size must be in multiple of 512 bytes,
	 * i.e. paragraphs must be in multiple of 32 */
	newexe.header_paragraphs = (newexe.header_paragraphs / 32L + 1) * 32L;
#endif

	int needHeaderSize = newexe.header_paragraphs * 16L;

	int mzGarbage = 0;	/* padding bytes between mz header and reloc table */
	int relocGarbage = needHeaderSize - headerSize - mzGarbage; /* padding bytes between reloc table and exe data */

	int fullSize = mzSize + mzGarbage + relocSize + relocGarbage + finalSize; 

	newexe.sp = packed.real_stack_OFFSET;
	newexe.ss = packed.real_stack_SEGMENT;
	newexe.ip = packed.real_start_OFFSET;
	newexe.cs = packed.real_start_SEGMENT;

	newexe.max_extra_paragraphs = 0xFFFF;
	newexe.min_extra_paragraphs = fullSize / 60;	/* I don't know how to determine BSS properly :/ */

	newexe.reloc_table_offset = mzSize + mzGarbage;
	newexe.num_relocs = relocSize / (2 * sizeof(unsigned short));

	newexe.blocks_in_file = fullSize / 512 + 1;
	newexe.bytes_in_last_block = fullSize % 512;

	newexe.overlay_number = 0;

	//printf("\n");
	//print_EXE(&newexe, "newexe"); /* show some info */

	/* save new exe */
	int j;

	char *obuffer;
	
	obuffer = malloc(sizeof(char) * fullSize);

	if (!obuffer) {
		return NULL;
	}

	char *p = &obuffer[0];

	exeHeader_write(&obuffer[0], &newexe);	p += mzSize;

	for (j = 0; j < mzGarbage; j++) *p++ = 0;

	memcpy(p, rout, relocSize); p += relocSize;

	for (j = 0; j < relocGarbage; j++) *p++ = 0;

	memcpy(p, out, finalSize);

	/* allocate resulting KB_file */
	uf = malloc(sizeof(KB_File));

	if (uf == NULL) {
		free(obuffer);
		return NULL;
	}

	/* setup resulting KB_file */
	uf->type = KBFTYPE_BUF;
	uf->d = (void*)obuffer;
	uf->len = fullSize;
	uf->pos = 0;
	uf->prev = NULL;
	uf->ref_count = 0;

	return uf;
}

#if 0
void print_EXE(struct exeHeader *h, const char *name) {
	int exe_data_start = h->header_paragraphs * 16L;

	int extra_data_start = h->blocks_in_file * 512L;
	if (h->bytes_in_last_block)
  		extra_data_start -= (512 - h->bytes_in_last_block);

	printf("%s\t\t\t(hex)\t\t(dec)\n", name);

	printf(".EXE size (bytes)\t\t%04x\t\t%d\n", extra_data_start, extra_data_start);
	printf("Overlay number\t\t\t%04x\t\t%d\n", h->overlay_number, h->overlay_number);
	printf("Initial CS:IP\t\t\t%04x:%04x\n", h->cs, h->ip);
	printf("Initial SS:SP\t\t\t%04x:%04x\n", h->ss, h->sp);
	printf("Minimum allocation (para)\t%4x\t\t%d\n", h->min_extra_paragraphs, h->min_extra_paragraphs);
	printf("Maximum allocation (para)\t%4x\t\t%d\n", h->max_extra_paragraphs, h->max_extra_paragraphs);
	printf("Header size (para)\t\t%4x\t\t%d\n", h->header_paragraphs, h->header_paragraphs);
	printf("Relocation table offset\t\t%4x\t\t%d\n", h->reloc_table_offset, h->reloc_table_offset);
	printf("Relocation entries\t\t%4x\t\t%d\n", h->num_relocs, h->num_relocs);

	printf("\n");
}
#endif

KB_File* DOS_UnpackExe(KB_File *f, int freesrc)
{
	KB_File *uncomp, *orig = f;

	if (f == NULL) return NULL;

	uncomp = execomp_uncompress(orig);

	if (uncomp != NULL) { //actually uncompressed something
		KB_debuglog(0, "[dos] removed execomp compression from exe file\n");
		orig = uncomp;
	}

	uncomp = exepack_uncompress(orig);

	if (uncomp != NULL) { //unpacked something again!!
		KB_debuglog(0, "[dos] removed exepack compression from exe file\n");
		if (orig != f) { // and we had an itermediate file
			// so we close it...
			KB_fclose(orig);
		}
	}
	else {
		KB_debuglog(0, "[dos] exe file is uncompressed\n");
		uncomp = orig; // the original IS the uncompressed file
	}
	
	if (freesrc == 1 && uncomp != f) {
		KB_fclose(f);
	}
	
	return uncomp;
}
