#include "stdio.h"
#include "string.h"

/* For portability, we must not use fread, or C datatypes
 * but if you're on a x86 machine, this code might just work. */
#ifdef _WIN32
#define __LITTLE_ENDIAN 1234
#define __BYTE_ORDER __LITTLE_ENDIAN
#else
#include <endian.h>
#endif
#if (__BYTE_ORDER == __LITTLE_ENDIAN)

#else
#error "BIG-ENDIAN machines are not supported. Fix this code!"
#endif

#include <stdint.h>
typedef uint16_t   word; /* 16-bit */
typedef uint8_t    byte; /* 8-bit */
typedef uint32_t word24; /* 24-bit :( */

#define GRAPHICS_KEPT	64

#define MBUFFER_SIZE 1024

#define MBUFFER_EDGE (MBUFFER_SIZE - 3) 

/*
 * Each .CC file has a header with a list of archived files.
 * But no actual filenames are stored, only their CRC values.
 * Therefore, we must provide our own list of names.
 *
 * Since the original contents of .CC files are known, the lists are built-in.
 * Alternatively, a plain-text file containing the list could be used. 
 */
 int list_count = 0;
char list_names[256][32];
word list_keys[256];

/*
 * The CRC is calculated as follows:
 */
word crcFilename(char *filename) {
	word key = 0;
	byte next;	

	while ((next = *filename++)) 
	{
		next &= 0x7F;
		if (next >= 0x60) 
			next -= 0x20;

		/* Swap high and low bits */
		key = ((key >> 8) & 0x00FF) | ((key << 8) & 0xFF00);

		/* Rotate left by 1 bit */
		key = (key << 1) | ((key >> 15) & 0x0001);

		key += next;
	}

	return key;
}

/* Helper function to find matching name by key */ 
int match_name(word crc) {
	int i;
	for (i = 0; i < list_count; i++) {
		if (list_keys[i] == crc) return i;
	}
	return -1;
}

/*
 * Prepare built-in list:
 */
void add_filenames(char* ext) {

	char archlist[GRAPHICS_KEPT][9] = {
		"peas","spri","mili","wolf","skel","zomb","gnom","orcs","arcr","elfs",
		"pike","noma","dwar","ghos","kght","ogre","brbn","trol","cavl","drui",
		"arcm","vamp","gian","demo","drag","mury","hack","ammi","baro","drea",
		"cane","mora","barr","barg","rina","ragf","mahk","auri","czar","magu",
		"urth","arec","knig","pala","sorc","barb","nwcp","title","select",
		"tileseta","tilesetb","tilesalt","cursor","town","cstl","plai",
		"frst","dngn","cave","comtiles","view","endpic","land",	"org",
	};
	//char ext[3][5] = { ".4", ".16", ".256\0" }; 

	int i, j;
	for (i = 0; i < GRAPHICS_KEPT; i++) {

		list_names[list_count + i][0] = 0;
		strcpy(list_names[list_count + i], archlist[i]);
		strcat(list_names[list_count + i], ext);

		list_keys[list_count + i] = crcFilename(list_names[list_count + i]);
	}

	list_count += i;
}
void add_filename(char *filename) {
	strcpy(list_names[list_count], filename);
	list_keys[list_count] = crcFilename(filename);
	list_count++;
}
void add_fake_filename(char *filename, word crc) {
	strcpy(list_names[list_count], filename);
	list_keys[list_count] = crc;
	list_count++;
}

void builtin_list(int mode) {

	list_count = 0;

	if (!mode) {

		add_filenames(".4");
		add_filenames(".16");	

		add_filename("endpic.256");

		add_filename("CGA.DRV");
		add_filename("EGA.DRV");
		add_filename("TGA.DRV");
		add_filename("HGA.DRV");	

	} else {

		add_filenames(".256");

		add_filename("MCGA.DRV");

	}

	add_filename("KB.CH");
	add_filename("LAND.ORG");

	add_filename("TIMER.DRV");
	add_filename("SOUND.DRV");

}

/*
 * Load list from file:
 */
void load_list(char *listfile) {

	FILE *f;

}




struct ccHeader {	/* 1122 bytes header */

	word ccTableLen;

	struct ccFile {	/* 8 bytes per file entry */

		word key;
		word offset;
		byte page;
		byte _align; /* imagine this is AFTER dunno	*/
		word dunno;

	} files[132];
	
	char unkown[64];/* Could be more files, could be something else */
};

/* 
 * Load header from .CC archive into struct
 * TODO: Rewrite it to read byte-by-byte. 
 */
void load_ccHeader(struct ccHeader* head, char *archivename) {

	FILE *f;
	int n;

	f = fopen(archivename, "rb");

	if (!f) {
		fprintf(stderr, "Unable to open %s \n", archivename);
		return;
	}

	n = fread(head, sizeof(struct ccHeader), 1, f);

	if (n != 1) {
		fprintf(stderr, "Unable to read CC header from file %s \n", archivename);
		return;
	}
	
	printf("Loaded %d byte header.\n ", sizeof(struct ccHeader));

	fclose(f);
}

/* Ascii Hex to INT from MSDN! :( */
word _ttoh(char *String)
{
    word Value = 0, Digit;
    char c;

    while ((c = *String++) != 0) {

        if (c >= '0' && c <= '9')
            Digit = (word) (c - '0');
        else if (c >= 'a' && c <= 'f')
            Digit = (word) (c - 'a') + 10;
        else if (c >= 'A' && c <= 'F')
            Digit = (word) (c - 'A') + 10;
        else
            break;

        Value = (Value << 4) + Digit;
    }

    return Value;
}
int find_infile(struct ccHeader *head, char *filename) {

	int i, j = -1;

	word key = crcFilename(filename);
	
	/* EVIL HACK for unknown.XXX */
	if (strlen(filename) == 12) {
		char tmp[80]; strcpy(tmp, filename);
		if (tmp[7] == '.') {
			tmp[7] = 0;	tmp[12] = 0;
			if (!strcasecmp(tmp, "unknown")) key = _ttoh(&tmp[8]);
		}
	}

	for (i = 0; i < head->ccTableLen; i++ ) {

		//printf("Offset: %04x, File %d = key %04x = offset %04x = page %04x = unknown %04x\n", i*8+2, i, head->files[i].key, head->files[i].offset, head->files[i].page, head->files[i].dunno);

		if (head->files[i].key == key) {
			j = i;
			break;
		}
	}

	return j;
}	


/*
 * List archived files (the --list option)
 */
void list_infiles(char *archivename) {

	int i, j;
	
	FILE *f;

	/* Load header */
	struct ccHeader head;
	load_ccHeader(&head, archivename);

	/* Printf it */
	printf(" NAME       \tOFFSET\t??\tLEN\tKEY\n");	

	f = fopen(archivename, "rb");
	if (!f) {
		printf("FAIL %s!\n", archivename);
		return;
	}

	for (i = 0; i < head.ccTableLen; i++ ) {

		char *name = NULL;
		word24 offset; // in bytes

		char buf[16]; // name
		sprintf(buf, "[%04x] ", head.files[i].key);

		name = buf; // use crc key by default

		j = match_name(head.files[i].key); // but switch to literal name 
		if (j != -1) name = list_names[j]; // if we have one

		offset = head.files[i].page;
		offset <<= 16;
		offset &= 0x00FF0000;
		offset |= head.files[i].offset;
		
		fseek(f, offset, 0);
		word bkey[2];
		fread(bkey, sizeof(word), 2, f);

		long bdunno = 0;
		//long bdunno = head.files[i].dunno;
		//bdunno &= 0x00FFFF00;
		//bdunno <<= 16;
		//bdunno |= head.files[i]._align;
		
		if (i < head.ccTableLen-1) {
		int noffset = head.files[i+1].page;
		noffset <<= 16;
		noffset &= 0x00FF0000;
		noffset |= head.files[i+1].offset;
		bdunno = noffset - offset;
		}
		
		

		/* Output! */
//		printf("%12s\t%06x\t%06x\t%04x\t%04x\n", name , offset, bdunno, bkey[0], head.files[i].key);
		printf("%s\n", name);
		
		//printf("Offset: %04x, File %d = key %04x = offset %04x = page %04x = unknown %04x\n", i*8+2, i, head.files[i].key, head.files[i].offset, head.files[i].page, head.files[i].dunno);
	}
	
	fclose(f);
}

/*
 * Load FILENAME from ARCHIVENAME into BUFFER limited by MAX bytes
 * Returns Number of bytes read, 0 on error 
 */
int load_infile(char *archivename, char *filename, char *buffer, struct ccHeader *head) {

	int i, n, j;

	FILE *f;

	/* Load archive header */
	struct ccHeader localHead;
	if (head == NULL) {
		printf(">:(\n");
		load_ccHeader(&localHead, archivename);
		head = &localHead;
	}

	/* Find file */
	i = find_infile(head, filename);
	if (i == -1) return 0;

	/* Read block key */
	word bkey[2];

	f = fopen(archivename, "rb");

	word24 offset = head->files[i].page;
	offset <<= 16;
	offset &= 0x00FF0000;

	offset |= head->files[i].offset;

	fseek(f, offset, 0);

	//printf("Going to seek %06x\n", offset);

	n = fread(bkey, sizeof(word), 2, f);

	//printf("Read block key: %04x %04x \n", bkey[0],bkey[1]);

	unsigned long long res_pos = 0;

	char result[bkey[0]];

	//printf("Expected result: %d (%04x)\n", bkey[0], bkey[0]);

	/*
	 * The data is kept in BIT-positioned "blocks".
	 * We use several variables to iterate it with some
	 * level of sanity:
	 */
	int pos = 0;	/* Position in bits */

	int byte_pos = 0; /* Position in bytes */
	int bit_pos = 0; /* Extra shift in bits */

	/*
	 * Each "block" can take from 9 to 12 bits of data.
	 * Last 8 bits are "the value" 
	 * First N bits are "the key"
	 */
	int step = 9;	/* Bits per step */

	/* Those masks help us get the relevant bits */ 
	word keyMask[4] = {
		0x01FF, 	// 0001 1111 
		0x03FF,		// 0011 1111
		0x07FF,		// 0111 1111
		0x0FFF,		// 1111 1111
	};

	/*
	 * If the "the key" is unset (00), the value is read as-is.
	 * Otherwise, the whole "block" (N+8 bytes) is treated
	 * as a dictionary key, and the dictionary is queried.
	 * The exceptions are the 0x0100 value, which means
	 * the dictionary must be cleared, and the 0x0101 value,
	 * which marks END OF FILE.
	 */

	/*
	 * The dictionary.
	 * Each entry consists of an index to a previous entry
	 * and a value, forming a tree.
	 */
	word dict_key[768 * 16];
	char dict_val[768 * 16];
	word dict_index = 0x0102; /* Start populating dictionary from 0x0102 */
	word dict_range = 0x0200; /* Allow that much entries before increasing step */

	/* Since data stored this way is backwards, we need a small queue */
	char queue[0xFF];
	int queued = 0;

	/* Read buffer */ 
	char mbuffer[MBUFFER_SIZE];

	/* Data */
	word next_index = 0;	/* block of data we currently examine */

	char last_char  = 0;	/* value from previous iteration */
	word last_index = 0;	/* block from previous iteration */

	word24 big_index;	/* temp. variable to safely load and shift 3 bytes */
	word keep_index;	/* temp. variable to keep "next_index" before making it "last_index" */
	int reset_hack=0;	/* HACK -- delay dictionary reset to next iteration */ 

	/* Read first chunk of data */
	n = fread(mbuffer, sizeof(char), MBUFFER_SIZE, f);

	/* Decompress */
	while (1) 
	{
		/* We need a dictionary reset */
		if (reset_hack) 
		{
			step = 9;
			dict_range = 0x0200;
			dict_index = 0x0102;
		}

		/* Since "pos" is in bits, we get position in bytes + small offset in bits */
		byte_pos = pos / 8;
		bit_pos = pos % 8;

		pos += step;	/* And advance to the next chunk */

		/* Edge of buffer, read more data from file */
		if (byte_pos >= MBUFFER_EDGE)
		{
				int bytes_extra = MBUFFER_SIZE - byte_pos;//~= 3
				int bytes_left = MBUFFER_SIZE - bytes_extra;//~= 1021

				/* Copy leftovers */
				for (j = 0; j < bytes_extra; j++) mbuffer[j] = mbuffer[bytes_left + j];

				/* Read in the rest */				
				n = fread(&mbuffer[bytes_extra], sizeof(char), bytes_left, f);

				/* Reset cursor */
				pos = bit_pos + step;	/* Add all unused bits */
				byte_pos = 0;
				/* On dictionary reset, use byte offset as bit offset*/
				if (reset_hack) bit_pos = bytes_extra;
		}
#ifdef DEBUG
printf("%04d\t0x%04X:%01X\t", pos, byte_pos, bit_pos);
#endif
		/* Read index from position "byte_pos", bit offset "bit_pos" */
		big_index = 
			((mbuffer[byte_pos+2] & 0x00FF) << 16) | 
			((mbuffer[byte_pos+1] & 0x00FF) << 8) | 
			(mbuffer[byte_pos] & 0x00FF);

		big_index >>= bit_pos;
		big_index &= 0x0000FFFF;

		next_index = big_index;
		next_index &= keyMask[ (step - 9) ];

		/* Apply the value as-is, continuing with dictionary reset, C) */
		if (reset_hack) 
		{
			/* Save index */
			last_index = next_index;
			/* Output char value */
			last_char = (next_index & 0x00FF);
			result[res_pos++] = last_char;
			/* We're done with the hack */
			reset_hack = 0;
			continue;
		}

		if (next_index == 0x0101)	/* End Of File */
		{
			/* DONE */
			break;
		}

		if (next_index == 0x0100) 	/* Reset dictionary */
		{
			/* Postpone it into next iteration */
			reset_hack = 1;
			/* Note: this hack avoids code duplication, but	makes the algorithm
			 * harder to follow. Basically, what happens, when the "reset" 
			 * command is being hit, is that 
			 * A) the dictionary is reset
			 * B) one more value is being read (this is the code duplication bit)
			 * C) the value is applied to output as-is
			 * D) we continue as normal
			 */
			continue;
		}

		/* Remember *real* "next_index" */
		keep_index = next_index;

		/* No dictionary entry to query, step back */
		if (next_index >= dict_index)
		{
			next_index = last_index;
			/* Queue 1 char */
			queue[queued++] = last_char;
		}

		/* Quering dictionary? */
		while (next_index > 0x00ff)
		{
			/* Queue 1 char */
			queue[queued++] = dict_val[next_index];
			/* Next query: */
			next_index = dict_key[next_index];
		}

		/* Queue 1 char */
		last_char = (next_index & 0x00FF);
		queue[queued++] = last_char;

		/* Unqueue */
		while (queued) 
		{
			result[res_pos++] = queue[--queued];
		}

		/* Save value to the dictionary */
		dict_key[dict_index] = last_index; /* "goto prev entry" */
		dict_val[dict_index] = last_char;  /* the value */
		dict_index++;

		/* Save *real* "next_index" */
		last_index = keep_index;

		/* Edge of dictionary, increase the bit-step, making range twice as large. */
		if (dict_index >= dict_range && step < 12) 
		{
			step += 1;
			dict_range *= 2;
		}
	}

	/* Done */
	fclose(f);

	/* Print-out the result */
//	fprintf(stderr,"0000:0000   ");
	{
		int kk, jj = 0;//	for (kk = 0; kk < res_pos; kk++) {fprintf(stderr, "%02hhx ", result[kk]);if (++jj >= 16) { fprintf(stderr, "\n%04d:%04x   ",kk/16,kk+1); jj= 0;}}
		for (kk = 0; kk < res_pos; kk++) {buffer[kk] = result[kk];}
	}
//	fprintf(stderr,"\n");

	if (res_pos != bkey[0]) 
	{
		printf("WARNING!! Filesize mismatch, most likely an error occured in %s.\n",filename);
	}
	printf("COMP LEN: %d, %04x | UN LEN: %04x\n", byte_pos, byte_pos, res_pos, res_pos);
	return res_pos;
}


void show_usage() {

	printf("KB CC Extractor, Usage: \n");
	printf(" kbcc [OPTIONS] ARCHIVE.CC [LIST.LST]\n");
	printf("\tARCHIVE.CC\t416.CC, 256.CC or another KB .CC archive.\n");
	printf("\tLIST.LST\tFile with a list of potential filenames for the archive.\n");
	printf("\tOPTIONS:\n");
	printf("\t-l,--list\t\tList files in archive\n");
	printf("\t-x,--extract [TARGET]\tExtract TARGET or *all* files.\n");
//	printf("\t-d,--dir PATH\t\tExtraction directory, \".\" by default.\n");	

}


int main(int argc, char *argv[]) {

	int i;

	char filename[80];
	char listfile[80];
	char target_file[80];
	int next = 0;	// next arg is "archive name"	

	int mode = -1; // error
	filename[0] = 0;
	listfile[0] = 0;
	target_file[0] = 0;

	for (i = 0; i < argc; i++) {
		if (i == 0) continue;
		
		if (!strcasecmp(argv[i], "--help") || !strcasecmp(argv[i], "-h")) {
			show_usage();
			return 1;
		}
		if (!strcasecmp(argv[i], "--list") || !strcasecmp(argv[i], "-l")) {
			mode = 0;
			continue;
		}
		if (!strcasecmp(argv[i], "--extract") || !strcasecmp(argv[i], "-x")) {
			mode = 1;
			if (i < argc-2)
				next = 2;
			continue;
		}
		
		if (next == 2) {
			strncpy(target_file, argv[i], 80);
			next = 0;
			continue;
		}
		if (next == 0) {
			strncpy(filename, argv[i], 80);
			next++;
			continue;
		}
	}

	if (mode == -1) {
		printf("No options provided, see kbcc --help.\n");
		return -1; 
	}
	if (filename[0] == 0) {
		printf("No archive file provided, see kbcc --help.\n");
		return -1;
	}
	
	/* A) List all files */
	if (mode == 0) {

		if (filename[0] == '4')	builtin_list(0);
		else if (filename[0] == '2') builtin_list(1);

		list_infiles(filename);

		return 0;
	}

	/* B) Extract one specific file */
	if (target_file[0] != 0) {

		int n;

		char buffer[1024 * 1024];

		FILE * f;

		n = load_infile(filename, target_file, &buffer[0], NULL);

		if (n == 0) {
			fprintf(stderr, "Inner file `%s` not found in archive '%s'\n", target_file, filename);
			return 2;
		}

		f = fopen(target_file, "wb");
		if (!f) {
			fprintf(stderr, "Unable to open file `%s` for writing\n", target_file);
			return 3;
		}
		fwrite(buffer, sizeof(char), n, f);
		fclose(f);
		
		printf("* %s  \t[extracted]\n", target_file);
		return 0;
	}

	/* C) Extract ALL files */
	{
		int n,j;

		char buffer[1024 * 1024];

		FILE * f;	

		if (filename[0] == '4')	builtin_list(0);
		else if (filename[0] == '2') builtin_list(1);

		/* Load header */
		struct ccHeader head;
		load_ccHeader(&head, filename);	

		for (i = 0; i < head.ccTableLen; i++ ) {

			char *name = NULL;
			word24 offset; // in bytes
	
			char buf[16]; // name
			sprintf(buf, "unknown.%04x", head.files[i].key);
			name = buf; // use crc key by default

			j = match_name(head.files[i].key); // but switch to literal name 
			if (j != -1) name = list_names[j]; // if we have one

			n = load_infile(filename, name, &buffer[0], &head);
			if (n == 0) {
				fprintf(stderr, "Inner file `%s` not found in archive '%s'\n", name, filename);
				continue;
			}

			f = fopen(name, "wb");
			if (!f) {
				fprintf(stderr, "Unable to open file `%s` for writing\n", name);
				return 3;
			}

			fwrite(buffer, sizeof(char), n, f);
			printf("%d/%d * %s  \t[extracted]\n", i, head.ccTableLen , name);
			fclose(f);

		}

	}

	return 0;
}
