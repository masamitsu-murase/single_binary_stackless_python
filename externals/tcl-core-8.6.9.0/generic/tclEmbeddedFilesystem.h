
#ifndef TCL_EMBEDDED_FILESYSTEM_H
#define TCL_EMBEDDED_FILESYSTEM_H

#define EMBEDDED_FILE_INFO_TYPE_DIRECTORY 0
#define EMBEDDED_FILE_INFO_TYPE_FILE 1

typedef struct tag_EmbeddedFileInfo
{
	const char *name;
	int type;
	const char *file_content;
    int file_size;
} EmbeddedFileInfo;

extern EmbeddedFileInfo gEmbeddedFileInfo[];
extern int gEmbeddedFileInfoCount;
void EmbeddedFileInfoDataInitialize();

#endif
