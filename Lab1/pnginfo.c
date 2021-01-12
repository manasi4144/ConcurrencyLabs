#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include "is_png.c"


int main(int argc, char *argv[]) 
{
    int i;

	if (argc != 2 )
	{
		printf("Please enter valid command line args\n");
		return -1;
	}


	char *path = argv[1];
	char *dirc, *basec, *bname, *dname;
	dirc = strdup(path);
	basec = strdup(path);
	dname = dirname(dirc);
	bname = basename(basec);
	//printf("dirname=%s, basename=%s\n", dname, bname);

	int result = chdir(dname);

	if (result == -1)
	{
		printf("Please enter a valid directory\n");
		return -1;
	}

	FILE* pfile;
	pfile = fopen(bname,"rb"); 
	if (pfile==NULL) {fputs ("File error",stderr); exit (1);}
	
	unsigned char signature[8];

	int result2 = fread(&signature, 8, 1, pfile);

	int valid = is_png(signature, sizeof(signature));
	
    if (valid == -1)
    {  	
      printf("%s: Not a PNG file\n", bname);
	return -1;
    }

	int IHDR_ret = get_wh_IHDR(bname, 1);

	if (IHDR_ret != -1)
	{
		int IDAT_ret = get_IDAT_chunk(bname, 1);
		if ( IDAT_ret != -1)
		{
				int result4 = get_IEND_chunk(bname, IDAT_ret, 1);
		}
	}
	
	fclose(pfile);
    return 0;
}


