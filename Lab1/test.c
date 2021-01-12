#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include "is_png.c"


int tree(char *basePath, const int root);

int main()
{
    // Directory path to list files
    char path[100];

    // Input path from user
    printf("Enter path to list files: ");
    scanf("%s", path);

    int r = tree(path, 0);

    return 0;
}


/**
 * Tree, prints all files and sub-directories of a given 
 * directory in tree structure.
 * 
 * @param basePath Base path to traverse directory
 * @param root     Integer representing indention for current directory
 */
int tree(char *basePath, const int root)
{
    int i;
    char path[1000];
    struct dirent *dp;
    DIR *dir = opendir(basePath);

    if (!dir)
        return;

    while ((dp = readdir(dir)) != NULL)
    {
        if (strcmp(dp->d_name, ".") != 0 && strcmp(dp->d_name, "..") != 0)
        {
            for (i=0; i<root; i++) 
            {
                if (i%2 == 0 || i == 0)
                    printf("%c", 179);
                else
                    printf(" ");

            }

            printf("%c%c%s\n", 195, 196, dp->d_name);

            char *ptr;
            struct stat buf;

            int r = lstat(dp->d_name, &buf);

            printf("after lstat");
            if(S_ISREG(buf.st_mode))
        {
            ptr = "regular";
            printf("%s %s \n", ptr, dp->d_name);

            FILE* pfile;
            pfile = fopen(dp->d_name,"rb");

            if (pfile==NULL) {
                 printf("File error, %s", dp->d_name); 
                 return -1;
            }

            unsigned char signature[8];

            int result2 = fread(&signature, 8, 1, pfile);

            int valid = is_png(signature, sizeof(signature));

            if (valid == 0)
            {
                printf("PNG FILE!");
            }
            else
            {
                printf("NOT PNG FILE");
            }

            fclose(pfile);
        }

            printf("after type");
            strcpy(path, basePath);
            strcat(path, "/");
            strcat(path, dp->d_name);
            tree(path, root + 2);
        }
    }

    closedir(dir);
    return 0;
}
