/*
 * KB CC archive tool. extract/compress/remove files.
 * 
 * Uses part of openkb lib, so is licensed under
 * GPL! I'll work on "liberating" it.
 *
 * This is a replacement tool for "kbcc". Due to 
 * to openkb lib, it is cross-platform and endian-aware.
 * See kbcc --help for usage.
 */

#ifdef HAVE_LIBSDL
//#error "You should probably adjust config.h and undefine HAVE_LIBSDL."
#endif

#include <string.h>
#include <malloc.h>

#include "../lib/kbsys.h" /* Here we grab bits of openkb lib */
#include "../lib/kbfile.h"

extern word KB_ccHash(const char *filename);

extern int KB_funLZW(char *result, unsigned int max, KB_File *f);

extern int DOS_LZW(char *dst, int dst_max, char *src, int src_len);


#define GRAPHICS_KEPT	62

#define MAX_CC_FILES	140
#define HEADER_SIZE_CC 1122

struct ccGroup {

	struct ccHeader {	/* 1122 bytes header */

		word num_files;

		struct {	/* 8 bytes per file entry */

			word key;
			sword offset;
			sword size;

		} files[MAX_CC_FILES];

	} head;

	struct {	/* Meta-data contained else-where in file */

		struct {

			dword size;

		} files[MAX_CC_FILES];

	} cache;

	struct { 	/* Our own filename list, not part of .CC format */
				/* And buffers for compressed/uncompressed data (default=NULL) */
		struct {

			char name[16];
			char *compressed_data;
			char *uncompressed_data;

		} files[MAX_CC_FILES];

		char *hidden_payload_data;
		sword hidden_payload_size;
		sword hidden_payload_offset;

	} list;

	KB_File *top;	//driver
	int i;  	//iterator
};

struct ccGroup* l_ccGroup_load(KB_File *f) {

	char buf[HEADER_SIZE_CC];

	int i, j;

	struct ccGroup *grp;
	struct ccHeader *head;

	int n;
	char *p;

	n = KB_fread(buf, sizeof(char), HEADER_SIZE_CC, f);

	if (n != HEADER_SIZE_CC) { perror("fread"); return NULL; }

	grp = malloc(sizeof(struct ccGroup));

	if (grp == NULL) {
		return NULL;
	}

	head = &grp->head;

	/* Parse data */
	p = &buf[0];

	/* Number of files */
	head->num_files = READ_WORD(p);

	if (head->num_files > MAX_CC_FILES) {
		free(grp);
		return NULL;
	}

	/* Files entries */
	for (i = 0; i < head->num_files; i++ ) {
		head->files[i].key = READ_WORD(p);
		head->files[i].offset = READ_SWORD(p);
		head->files[i].size = READ_SWORD(p);
		grp->cache.files[i].size = 0; // unknown full size
		grp->list.files[i].name[0] = '\0'; // no name yet
		grp->list.files[i].compressed_data = NULL; // no data yet
		grp->list.files[i].uncompressed_data = NULL; // no data yet
	}

	/* Save FILE handler, reset iterator */
	grp->top = f;
	grp->i = 0;
	f->ref_count++;

	return grp;
}

void l_ccGroup_unload(struct ccGroup* grp) {
	/* Close FILE handler */
	grp->top->ref_count--;
	KB_fclose(grp->top);
	free(grp);
}

struct ccGroup* l_ccGroup_create(const char *filename) {
	KB_File *f = KB_fopen(filename, "wb");
	
	struct ccGroup *grp;

	grp = malloc(sizeof(struct ccGroup));

	if (grp == NULL) {
		return NULL;
	}

	memset(grp, 0, sizeof(struct ccGroup));

	grp->top = f;
	grp->i = 0;
	f->ref_count++;

	return grp;
}

void l_ccGroup_read(struct ccGroup* grp, int first, int frames) {
	int i;
	for (i = first; i < first + frames; i++) {
		/* Read out "uncompressed size" if we haven't already */
		if (!grp->cache.files[i].size) {
			char buf[4];

			/* Seek to offset, read 4 bytes */
			KB_fseek(grp->top, grp->head.files[i].offset, SEEK_SET);
			KB_fread(buf, 1, 4, grp->top);

			grp->cache.files[i].size = KB_UNPACK_DWORD(buf[0],buf[1],buf[2],buf[3]);
		}
	}
}

void ccGroup_read_payload(struct ccGroup* grp, const char *filename) {

	int total_len = file_size(filename);
	int i = grp->head.num_files - 1;

	int offset = grp->head.files[i].offset;

	offset += grp->head.files[i].size;

	int payload_len = total_len - offset;

	grp->list.hidden_payload_data = malloc(sizeof(char) * payload_len);

	KB_fseek(grp->top, offset, SEEK_SET);
	KB_fread(grp->list.hidden_payload_data, sizeof(char), payload_len, grp->top);

	grp->list.hidden_payload_size = payload_len;
	grp->list.hidden_payload_offset = offset;

}

void ccGroup_read_comp(struct ccGroup* grp, int first, int frames) {

	/* Read out "uncompressed size" if we haven't already */
	l_ccGroup_read(grp, first, frames);

	int i, n;
	for (i = first; i < first + frames; i++) {
		/* Allocate that much bytes */
		int len = grp->head.files[i].size;
		grp->list.files[i].compressed_data = malloc(sizeof(char) * len);

		KB_fseek(grp->top, grp->head.files[i].offset + 4, SEEK_SET);
		KB_fread(grp->list.files[i].compressed_data, sizeof(char), len, grp->top);
	}
}

void ccGroup_read_uncomp(struct ccGroup* grp, int first, int frames) {

	int i, n;

	/* Read out "compressed size" */
	ccGroup_read_comp(grp, first, frames);

	for (i = first; i < first + frames; i++) {
		/* Allocate that much bytes */
		int len = grp->cache.files[i].size;
		grp->list.files[i].uncompressed_data = malloc(sizeof(char) * len);

		/* Unpack LZW data */
		KB_fseek(grp->top, grp->head.files[i].offset + 4, 0);
		n = KB_funLZW(grp->list.files[i].uncompressed_data, len, grp->top);

		if (n != len) {
			printf("Expected %d bytes, got %d\n", len, n);
		}		
	}

}

void ccGroup_update_offsets(struct ccGroup* grp, int first, int frames) {

	int i, n;

	int last = (first + frames) * 8 + 2;//HEADER_SIZE_CC;

	for (i = first; i < first + frames; i++) {

		int len = grp->head.files[i].size;

		grp->head.files[i].offset = last;
		
		last += len;
		//last += 4;

	}

}

void l_ccGroup_save(struct ccGroup *grp, const char *filename) {

	FILE *f = fopen(filename, "wb");
	char buf[HEADER_SIZE_CC] = { 0 };
	char *p;
	int i, n;

	f = fopen(filename, "wb");
	
	if (f == NULL) {
		perror("fopen");
		fprintf(stderr, "Unable to open file `%s` for writing\n", filename);
		return;
	}

	/* Parse data */
	p = &buf[0];

	/* Number of files */
	WRITE_WORD(p, grp->head.num_files); 

	/* Files entries */
	for (i = 0; i < grp->head.num_files; i++ ) {
		WRITE_WORD(p, grp->head.files[i].key);
		WRITE_SWORD(p, grp->head.files[i].offset); 
		WRITE_SWORD(p, grp->head.files[i].size);
	}

	/* Re-write header */
	//n = KB_fwrite(buf, sizeof(char), HEADER_SIZE_CC, grp->top);
	n = fwrite(buf, sizeof(char), HEADER_SIZE_CC, f);

	if (n != HEADER_SIZE_CC) { perror("fwrite"); return; }

	for (i = 0; i < grp->head.num_files; i++) {

		int full_len = grp->cache.files[i].size; 
		int len = grp->head.files[i].size - 4;
		char *buf = grp->list.files[i].compressed_data;

		char hbuf[4] = { 0 };
		char *hptr = &hbuf[0];

		WRITE_WORD(hptr, full_len);

//printf("%d) %s Seeking to %d to write %d bytes\n", i, grp->list.files[i].name, grp->head.files[i].offset, len + 4);

		fseek(f, grp->head.files[i].offset, SEEK_SET);
		n = fwrite(hbuf, sizeof(char), 4, f);
		n = fwrite(buf, sizeof(char), len, f);
	}

	fclose(f);
}

/* Helper function to find matching name by key */ 
int ccGroup_find(struct ccGroup *grp, const char *name) {
	word crc = KB_ccHash(name);
	int i;
	for (i = 0; i < grp->head.num_files; i++) {
		if (grp->head.files[i].key == crc) return i;
	}
	return -1;
}


int ccGroup_insert_file(struct ccGroup *grp, const char *target_file, int i)
{
	FILE *f;
	int len, n;		

	f = fopen(target_file, "rb");
	if (f == NULL) {
		perror("fopen");
		fprintf(stderr, "Can't open `%s` for reading\n", target_file);
		return -2;
	}

	char *buf;

	len = file_size(target_file);
	buf = malloc(sizeof(char) * len);
	n = fread(buf, sizeof(char), len, f);
	grp->list.files[i].uncompressed_data = buf;
	grp->cache.files[i].size = len;

	int len2 = len;
	int out_n;
	char *buf2;

	buf2 = malloc(sizeof(char) * len2);
	memset(buf2, 0, len2);
	out_n = DOS_LZW(buf2, len2, buf, len);

	grp->list.files[i].compressed_data = buf2;
	grp->head.files[i].size = out_n + 4;

	return 0;
}


void ccGroup_remove_file(struct ccGroup *grp, int ind)
{
	int i;

	free(grp->list.files[ind].compressed_data);
	free(grp->list.files[ind].uncompressed_data);	

	grp->head.num_files--;
	for (i = ind; i < grp->head.num_files; i++) {
		int n = i + 1;

		grp->head.files[i].key = grp->head.files[n].key;
		grp->head.files[i].offset = grp->head.files[n].offset;
		grp->head.files[i].size = grp->head.files[n].size;

		grp->cache.files[i].size = grp->cache.files[n].size;

		strcpy(grp->list.files[i].name, grp->list.files[n].name);
		grp->list.files[i].compressed_data = grp->list.files[n].compressed_data;
		grp->list.files[i].uncompressed_data = grp->list.files[n].uncompressed_data;
	}
	grp->list.files[i].compressed_data = NULL;
	grp->list.files[i].uncompressed_data = NULL;
}
#if 0
int l_ccGroup_append_list(struct ccGroup *grp, const char *filename) {

	char listfile[1024];
	int i;

	FILE *f;

	/* Clean list */
	for (i = 0; i < MAX_CC_FILES; i++) {
		grp->list.files[i].name[0] = '\0';
	}

	/* Prepare filename */
	KB_strcpy(listfile, filename);
	KB_strcat(listfile, (filename[strlen(filename)-1] == 'C' ? "L" : "l"));

	/* Try reading */
	f = fopen(listfile, "r");
	if (f != NULL) {
		char buf[1024] = { 0 };
		while (fgets(buf, 1024, f) != NULL) {
			int j;
			word key;
			for (j = 0; j < 16; j++) {
				if (buf[j] == '\n') buf[j] = '\0';
				if (!isascii(buf[j])) buf[j] = '\0';
			}
			key = KB_ccHash(buf);
			for (j = 0; j < grp->head.num_files; j++) {
				if (grp->head.files[j].key == key) {
					KB_strcpy(grp->list.files[j].name, buf);
					break;
				}
			}
		}
		fclose(f);
	} else {
		KB_stdlog("%s not found\n", listfile);
		return 1;
	}
	return 0;
}
#endif
/*
 * Prepare built-in list:
 */
int list_count;
void add_filename(struct ccGroup *grp, char *filename) {
	int i;
	word crc = KB_ccHash(filename);
	for (i = 0; i < grp->head.num_files; i++) {
		if (grp->head.files[i].key == crc) {
			strcpy(grp->list.files[i].name, filename);
			return;
		}
	}
}
void add_fake_filename(struct ccGroup *grp, char *filename, word crc) {
	strcpy(grp->list.files[list_count].name, filename);
	grp->head.files[list_count].key = crc;
	list_count++;
}

void add_filenames(struct ccGroup *grp, char* ext) {

	char archlist[GRAPHICS_KEPT][9] = {
		"peas","spri","mili","wolf","skel","zomb","gnom","orcs","arcr","elfs",
		"pike","noma","dwar","ghos","kght","ogre","brbn","trol","cavl","drui",
		"arcm","vamp","gian","demo","drag","mury","hack","ammi","baro","drea",
		"cane","mora","barr","barg","rina","ragf","mahk","auri","czar","magu",
		"urth","arec","knig","pala","sorc","barb","nwcp","title","select",
		"tileseta","tilesetb","tilesalt","cursor","town","cstl","plai",
		"frst","dngn","cave","comtiles","view","endpic",
	};
	//char ext[3][5] = { ".4", ".16", ".256\0" }; 

	int i, j;
	for (i = 0; i < GRAPHICS_KEPT; i++) {
		char fullname[16];
		sprintf(fullname, "%s%s", archlist[i], ext);
		add_filename(grp, fullname);
/*		grp->list.files[list_count + i].name[0] = 0;
		strcpy(grp->list.files[list_count + i].name, archlist[i]);
		strcat(grp->list.files[list_count + i].name, ext);
*/
//		list_keys[list_count + i] = crcFilename(list_names[list_count + i]);
	}

	list_count += i;
}

void builtin_list(struct ccGroup *grp, int mode) {

	int i;
	list_count = 0;

	/* Clean list */
	for (i = 0; i < MAX_CC_FILES; i++) {
		grp->list.files[i].name[0] = '\0';
	}

	if (!mode) {

		add_filenames(grp, ".4");
		add_filenames(grp, ".16");	

		add_filename(grp, "endpic.256");

		add_filename(grp, "CGA.DRV");
		add_filename(grp, "EGA.DRV");
		add_filename(grp, "TGA.DRV");
		add_filename(grp, "HGA.DRV");

	} else {

		add_filenames(grp, ".256");

		add_filename(grp, "MCGA.DRV");

	}

	add_filename(grp, "KB.CH");
	add_filename(grp, "LAND.ORG");

	add_filename(grp, "TIMER.DRV");
	add_filename(grp, "SOUND.DRV");

}

void list_files_in_archive(const char *filename) {

	struct ccGroup *grp;
	KB_File *f;

	f = KB_fopen(filename, "rb");
	grp = l_ccGroup_load(f);

	l_ccGroup_read(grp, 0, grp->head.num_files);

	builtin_list(grp, 0);
	builtin_list(grp, 1);

	ccGroup_read_payload(grp, filename);

	int i;

	printf("    key   name        offset  compressed uncompressed\n");
	for (i = 0; i < grp->head.num_files; i++) {
		printf("%3d %04x %12s %7d %7d %7d\n", 
			i,
			grp->head.files[i].key,
			grp->list.files[i].name[0] ? grp->list.files[i].name : "unknown" ,
			grp->head.files[i].offset, 
			grp->head.files[i].size, 
			grp->cache.files[i].size);
	}
	printf("    <hidden payload>  %7d %7d %7d\n",
		 grp->list.hidden_payload_offset,
		 grp->list.hidden_payload_size,
		 0
		);

}
#if 0
/* Open file "filename" in CC directory "dirp" */
KB_File * KB_fopenCC_in( const char * filename, const char * mode, KB_DIR *dirp )
{
	if (dirp == NULL || dirp->type != KBDTYPE_GRPCC) {
		KB_errlog("Error! Unable to read CC file, incorrect CC directory %p.\n", filename, dirp);
		return NULL;
	}
	struct ccGroup *grp = (struct ccGroup *)dirp->d;
	word hash = KB_ccHash(filename);
	int i;

	for (i = 0; i < grp->head.num_files; i++) {
		if (grp->head.files[i].key == hash) {
			return KB_fopenCC_by( i, dirp );
		}
	}

	/* HACK! If file extension ends in "%04x" hex, try to use _that_ as hash.
	 * Allows us to load files in "unknown.xxxx" format, i.e. files with no names. */
	int ext_len;
	const char *ext = strrchr(filename, '.');
	if (ext && (ext_len = strlen(ext)) >= 4) {
		hash = hex2dec(&ext[ext_len-4]);
		for (i = 0; i < grp->head.num_files; i++) {
			if (grp->head.files[i].key == hash) {
				return KB_fopenCC_by( i, dirp );
			}
		}
	}
	return NULL;
}
#endif

int extract_single_file(const char *filename, const char *target_file) {

	struct ccGroup *grp;
	KB_File *f;

	f = KB_fopen(filename, "rb");
	grp = l_ccGroup_load(f);

	int i;

	//l_ccGroup_read(grp, 0, grp->head.num_files);

	builtin_list(grp, 0);
	builtin_list(grp, 1);

	i = ccGroup_find(grp, target_file);

	if (i == -1) {
		fprintf(stderr, "Inner file `%s` not found in archive '%s'\n", target_file, filename);
		return 2;
	}

	//ccGroup_read_comp(grp, i, 1);
	ccGroup_read_uncomp(grp, i, 1);

	int n;
	FILE *fw;

	n = grp->cache.files[i].size;
	char *buffer = grp->list.files[i].uncompressed_data;

	printf("Uncompressed size: %d\n", n);

	fw = fopen(target_file, "wb");
	if (!fw) {
		perror("fopen");
		fprintf(stderr, "Unable to open file `%s` for writing\n", target_file);
		return 3;
	}
	fwrite(buffer, sizeof(char), n, fw);
	fclose(fw);

	printf("* %s  \t[extracted]\n", target_file);
	return 0;
}

void extract_all_files(const char *filename) {

	struct ccGroup *grp;
	KB_File *f;

	f = KB_fopen(filename, "rb");
	grp = l_ccGroup_load(f);

	int i;

	//l_ccGroup_read(grp, 0, grp->head.num_files);

	if (filename[0] == '4')	builtin_list(grp, 0);
	else if (filename[0] == '2') builtin_list(grp, 1);

	ccGroup_read_uncomp(grp, 0, grp->head.num_files);

	for (i = 0; i < grp->head.num_files; i++) {

		char *name = NULL;
		sword offset; // in bytes

		char buf[16]; // name
		sprintf(buf, "unknown.%04x", grp->head.files[i].key);
		name = buf; // use crc key by default

		name = grp->list.files[i].name;

		//j = ccGroup_find(grp, target_file); // but switch to literal name 
		//if (j != -1) name = list_names[j]; // if we have one
/*
		if (j == -1) {
			fprintf(stderr, "Inner file `%s` not found in archive '%s'\n", target_file, filename);
			return 2;
		}
*/
		int n;
		FILE *fw;

		n = grp->cache.files[i].size;
		char *buffer = grp->list.files[i].uncompressed_data;

		//printf("Uncompressed size: %d\n", n);

		fw = fopen(name, "wb");
		if (!fw) {
			perror("fopen");
			fprintf(stderr, "Unable to open file `%s` for writing\n", name);
			return;
		}
		fwrite(buffer, sizeof(char), n, fw);
		fclose(fw);

		printf("%d/%d * %s   \t[extracted]\n", i, grp->head.num_files, name);

	}
}

int file_exists(const char *filename) {
	FILE *f;
	f = fopen(filename, "rb");
	if (!f) return 0;
	fclose(f);
	return 1;
}


int compress_single_file(const char *filename, const char *target_file) {

	struct ccGroup *grp;
	
	if (file_exists(filename)) {

		KB_File *f = KB_fopen(filename, "rb");
		grp = l_ccGroup_load(f);
		ccGroup_read_comp(grp, 0, grp->head.num_files);

		builtin_list(grp, 0);
		builtin_list(grp, 1);

	} else {
		printf("New archive %s\n", filename);
		grp = l_ccGroup_create(filename);
	}

	int i;

	i = ccGroup_find(grp, target_file);

	if (i == -1) {
		i = grp->head.num_files;
		grp->head.files[i].key = KB_ccHash(target_file);
		grp->head.num_files++;
	}
	else {
		
	}

	ccGroup_insert_file(grp, target_file, i);

	printf("* %s  \t[compressed]\n", target_file);

	ccGroup_update_offsets(grp, 0, grp->head.num_files);
	l_ccGroup_save(grp, filename);

	l_ccGroup_unload(grp); 
	return 0;
}

int compress_all_files(const char *filename) {

	struct ccGroup *grp;

	if (file_exists(filename)) {

		KB_File *f = KB_fopen(filename, "rb");
		grp = l_ccGroup_load(f);
		ccGroup_read_comp(grp, 0, grp->head.num_files);

		builtin_list(grp, 0);
		builtin_list(grp, 1);

	} else {
		printf("New archive %s\n", filename);
		grp = l_ccGroup_create(filename);
	}

	int i;
/*
	i = ccGroup_find(grp, target_file);

	if (i == -1) {
		i = grp->head.num_files;
		grp->head.files[i].key = KB_ccHash(target_file);
		grp->head.num_files++;
	}
*/
	for (i = 0; i < grp->head.num_files; i++) {
		FILE *f;
		int len, n;
		
		char *name;
		
		name = grp->list.files[i].name;

		len = file_size(name);
		f = fopen(name, "rb");

		if (!f) {
			perror("fopen");
			fprintf(stderr, "Unable to open file `%s` for reading\n", name);
			l_ccGroup_unload(grp); 
			return -1;
		}

		char *buf;

		buf = malloc(sizeof(char) * len);
		n = fread(buf, sizeof(char), len, f);
		fclose(f);
		grp->list.files[i].uncompressed_data = buf;
		grp->cache.files[i].size = len;

		int len2 = len;
		int out_n;
		char *buf2;

		buf2 = malloc(sizeof(char) * len2);
		memset(buf2, 0, len2);
		out_n = DOS_LZW(buf2, len2, buf, len);

		grp->list.files[i].compressed_data = buf2;
		grp->head.files[i].size = out_n + 4;

		printf("* %12s \t[compressed]\n", name);

	}

	ccGroup_update_offsets(grp, 0, grp->head.num_files);
	l_ccGroup_save(grp, filename);

	l_ccGroup_unload(grp);
	return 0;
}

int remove_single_file(const char *filename, const char *target_file) {

	struct ccGroup *grp;
	
	if (file_exists(filename)) {

		KB_File *f = KB_fopen(filename, "rb");
		grp = l_ccGroup_load(f);
		ccGroup_read_comp(grp, 0, grp->head.num_files);

		builtin_list(grp, 0);
		builtin_list(grp, 1);

	} else {
		fprintf(stderr, "Archive `%s` does not exist.\n", filename);
		return -1;
	}

	int i;

	i = ccGroup_find(grp, target_file);

	if (i == -1) {
		fprintf(stderr, "File %s not found in %s\n", target_file, filename);
		return -2;	
	}

	ccGroup_remove_file(grp, i);

	printf("* %s  \t[removed]\n", target_file);

	ccGroup_update_offsets(grp, 0, grp->head.num_files);
	l_ccGroup_save(grp, filename);

	l_ccGroup_unload(grp); 
	return 0;
}

void show_usage() {

	printf("KB CC compressor/extractor, Usage: \n");
	printf(" kbcc [OPTIONS] ARCHIVE.CC [LIST.LST]\n");
	printf("\tARCHIVE.CC\t416.CC, 256.CC or another KB .CC archive.\n");
	printf("\tLIST.LST\tFile with a list of potential filenames.\n");
	printf("\tOPTIONS:\n");
	printf("\t-l,--list\t\tList files in archive\n");
	printf("\t-x,--extract [TARGET]\tExtract TARGET or *all* files.\n");
	printf("\t-c,--compress [TARGET]\tAdd TARGET or *all* files to ARCHIVE.\n");
	printf("\t-r,--remove TARGET\tRemove TARGET file from ARCHIVE.\n");
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
		if (!strcasecmp(argv[i], "--compress") || !strcasecmp(argv[i], "-c")) {
			mode = 2;
			if (i < argc-2)
				next = 2;
			continue;
		}
		if (!strcasecmp(argv[i], "--remove") || !strcasecmp(argv[i], "-r")) {
			mode = 3;
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

		list_files_in_archive(filename);
		return 0;
	}

	/* B) Extract one specific file */
	if (mode == 1 && target_file[0] != 0) {

		extract_single_file(filename, target_file);
		return 0;
	}

	/* C) Extract ALL files */
	if (mode == 1 && target_file[0] == 0) {
	
		extract_all_files(filename);	
		return 0;
	}

	/* D) Compress one specific file */
	if (mode == 2 && target_file[0] != 0) {

		compress_single_file(filename, target_file);
		return 0;

	}

	/* E) Compress ALL files */
	if (mode == 2 && target_file[0] == 0) {

		compress_all_files(filename);
		return 0;

	}


	/* F) Remove one specific file */
	if (mode == 3 && target_file[0] != 0) {

		remove_single_file(filename, target_file);
		return 0;

	}


	return 0;
}
