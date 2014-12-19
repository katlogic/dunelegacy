#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv)
{
	FILE *f;
	int sz;
	char *buf, *p;
	assert(f = fopen(argv[1], "rb"));
	fseek(f, 0, SEEK_END);
	sz = ftell(f);
	rewind(f);
	printf("Unpacking %s, %d bytes:\n", argv[1], sz);
	buf = p = malloc(sz);
	assert(fread(p, 1, sz, f) == sz);
	fclose(f);
	f = NULL;

	assert(sizeof(int)==4);
	int off, prevoff = 0;
	while (1) {
		off = *((int *)p);
		if (!off)
			off = sz;
		p += 4;
		if (f) {
			int n = off - prevoff;
			printf(".. offset %d, len %d\n", prevoff, n);
			fwrite(buf + prevoff, 1, n, f);
			fclose(f);
		}
		if (off == sz) break;
		f = fopen(p, "wb+");
		printf(" -> %s ", p);
		p = p + strlen(p) + 1;
		prevoff = off;
	}
	free(buf);
	return 0;
}
