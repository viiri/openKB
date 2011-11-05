#include "kbconf.h"
#include "kbfile.h"

#define DATA_SEGMENT 0x15850

#if 0
/* Another method of loading strings -- by appropriate offsets into DATA_SEGMENT */
char* DOS_read_string_p(KBmodule *mod, int ptroff, int off, int endoff) {
	static char buf[24096];
	static char pbuf[24096];
	
	int segment = DATA_SEGMENT;

	int len = endoff - off;

	KB_File *f;	int n;   

	KB_debuglog(0,"? DOS EXE FILE: %s\n", "KB.EXE");
	f = KB_fopen_with("kb.exe", "rb", mod);
	if (f == NULL) return NULL;

	KB_fseek(f, ptroff, 0);
	n = KB_fread(&pbuf[0], sizeof(char), len, f);

	KB_fseek(f, off, 0);
	n = KB_fread(&buf[0], sizeof(char), len, f);
	KB_fclose(f);

	char *p = &pbuf[0];

	int i;
	for (i = 0; i < 17; i++) {
		word soff;
		soff = READ_WORD(p);
		KB_debuglog(0, "%02x [%04x - %08x] Splitting: '%s' %c %d \n", i, soff, soff + segment, &buf[soff + segment - off]);
		//KB_fseek(f, segment + soff, 0);
	}

	return &buf[0];
}
#endif

char* DOS_read_strings(KBmodule *mod, int off, int endoff) {
	char *buf;

	int len = endoff - off;

	KB_File *f;	
	int n;

	buf = malloc(sizeof(char) * len);
	if (buf == NULL) return NULL;

	KB_debuglog(0,"? DOS EXE FILE: %s\n", "KB.EXE");
	f = KB_fopen_with("kb.exe", "rb", mod);
	if (f == NULL) {
		free(buf);
		return NULL;
	}

	KB_fseek(f, off, 0);
	n = KB_fread(buf, sizeof(char), len, f);
	KB_fclose(f);
	if (n < len) {
		free(buf);
		return NULL;
	}

	return buf;
}

