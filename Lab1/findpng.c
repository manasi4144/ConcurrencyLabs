#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include "is_png.c"

typedef unsigned char U8;

int count = 0;
int tree(char* basePath, const int root);

int main(int argc, char *argv[])
{
    // Directory path to list files

    if (argc != 2 )
    {
                printf("Please enter valid command line args\n");
                return -1;
    }

   char *path = argv[1]; //name of the directory

    // Input path from user
    int r = tree(path, 0);

    if (count == 0)
    {
	printf("findpng: No PNG file found\n");
    }

    return 0;
}


int tree(char* basePath, const int root)
{
    int i;
    char path[1000];
    struct dirent* dp;
    DIR* dir = opendir(basePath);
    //printf("base path: %s \n", basePath);

    if (!dir)
        return -1;

    while ((dp = readdir(dir)) != NULL)
    {
        if (strcmp(dp->d_name, ".") != 0 && strcmp(dp->d_name, "..") != 0)
        {

            strcpy(path, basePath);
            strcat(path, "/");
            strcat(path, dp->d_name);

            char* ptr;
            struct stat buf;

            if (lstat(path, &buf) < 0)
            {
                perror("lstat error");
            }

            if (S_ISREG(buf.st_mode)) {
                ptr = "regular";

                FILE* pfile;
                pfile = fopen(path, "rb");

                if (pfile != NULL) {
                unsigned char signature[8];

                int result2 = fread(&signature, 8, 1, pfile);

                int valid = is_png(signature, sizeof(signature));

                if (valid == 0)
                {
                    printf("%s\n", path);
		    count++;
                }

                }
            }

            else if (S_ISDIR(buf.st_mode))  ptr = "directory";
            else if (S_ISCHR(buf.st_mode))  ptr = "character special";
            else if (S_ISBLK(buf.st_mode))  ptr = "block special";
            else if (S_ISFIFO(buf.st_mode)) ptr = "fifo";
#ifdef S_ISLNK
        else if (S_ISLNK(buf.st_mode))  ptr = "symbolic link";
#endif
#ifdef S_ISSOCK
        else if (S_ISSOCK(buf.st_mode)) ptr = "socket";
#endif
        else                            ptr = "**unknown mode**";
        
            tree(path, root + 2);
        }
    }

    closedir(dir);
    return 0;
}

