#include <stdio.h>
char *c = "#include <stdio.h>%cchar *c = %c%s%c;%c%cint main()%c{%c%cFILE *f = fopen(%csource.c%c, %cw%c);%c%cfprintf(f, c, 0x0A, 0x22, c, 0x22, 0x0A, 0x0A, 0x0A, 0x0A, 0x09, 0x22, 0x22, 0x22, 0x22, 0x0A, 0x09, 0x0A, 0x09, 0x0A, 0x09, 0x0A);%c%cfclose(f);%c%creturn 0;%c}";

int main()
{
	FILE *f = fopen("source.c", "w");
	fprintf(f, c, 0x0A, 0x22, c, 0x22, 0x0A, 0x0A, 0x0A, 0x0A, 0x09, 0x22, 0x22, 0x22, 0x22, 0x0A, 0x09, 0x0A, 0x09, 0x0A, 0x09, 0x0A);
	fclose(f);
	return 0;
}