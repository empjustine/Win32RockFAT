#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>

#include "array.h"
#include "rs.h"

unsigned char *data;
unsigned char *sectors;
unsigned char *newsectors;

void inform(char *fmt, ...)
{
    va_list arg;
    
    va_start(arg, fmt);
    vfprintf(stderr, fmt, arg);
    va_end(arg);
}

void panic(char *fmt, ...)
{
    va_list arg;
    
    va_start(arg, fmt);
    vfprintf(stderr, fmt, arg);
    va_end(arg);
    
    exit(0);
}

unsigned int filesize(char *fname)
{
    FILE *fp = fopen(fname, "rb");
    if (!fp)
        return 0;
    fseek(fp, 0, SEEK_END);
    int len = ftell(fp);
    fclose(fp);
    return len;
}

/////////////////////////////////////////
//
//          Reed-Solomon
// 1336300 -------------> 1474560
//
/////////////////////////////////////////
main(int argc, char *argv[])
{
    int i,j;
    FILE *fp;
    
    if(argc != 2)
        panic("Usage: shield <filename>");
    
    Array<unsigned char> sectors(512*2880);
    Array<unsigned char> newsectors(512*2880);  
    Array<unsigned char> info(20);

    if (strlen(argv[1])>15)
	panic("Filename will be chopped!... "
              "Please rename file '%s' to less than 15 chars\n", argv[1]);
    
    unsigned int uiFileSize = filesize(argv[1]);
    if (1336300<uiFileSize)
	panic("A 1.44MB disk can (safely!) carry up to 1336300 bytes!\n");
    
    *(unsigned int*)(unsigned char*)info = uiFileSize;
    strcpy((char *)((unsigned char*)info)+4, argv[1]);    
    
    // Initialize Reed-Solomon code
    init_rs();
    
    // Shield the data with RS
    fp = fopen(argv[1], "rb");
    if(!fp) 
        panic("Couldn't open file '%s'!\n", argv[1]);

    // Strengthen the header part...
    i = 0;
    memset(((unsigned char *)sectors)+i*(NN+1), 0, NN+1);
    memcpy(((unsigned char *)sectors)+i*(NN+1), (unsigned char *)info, 20);
    fread(((unsigned char *)sectors)+i*(NN+1)+20, 1, KK-20, fp);
    encode_rs(
        ((unsigned char *)sectors)+i*(NN+1), 
        ((unsigned char *)sectors)+i*(NN+1)+KK);            
    i++;

    // Strengthen the rest...
    while(i<2880*2) {
	if ((i%80) == 0)
	    printf(".");
        memset(((unsigned char *)sectors)+i*(NN+1), 0, NN+1);
          
	fread(((unsigned char *)sectors)+i*(NN+1), 1, KK, fp);
        encode_rs(
            ((unsigned char *)sectors)+i*(NN+1), 
            ((unsigned char *)sectors)+i*(NN+1)+KK);            
        i++;
    }
    fclose(fp);
    
    // Produce the out-of-order file
    i=j=0;
    for(i=0;i<2880*512;i++) {
        newsectors[j] = sectors[i];
        j+=4096;
        if(j>=2880*512) {
            j-=2880*512;
            j++;
        }
    }

	HANDLE hDrive;
	LPVOID pDriveSectors;
	DWORD bytesWritten,dw;

	hDrive = CreateFile(
		"\\\\.\\A:",
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_NO_BUFFERING,
		NULL);
	if (!hDrive)
		panic("Couldn't open disk drive A:");

	pDriveSectors = VirtualAlloc(
		NULL,
		512*2880,
		MEM_COMMIT,
		PAGE_READWRITE);
	if (!pDriveSectors)
		panic("Couldn't allocate enough buffers for a 1.44MB diskette");

	CopyMemory(pDriveSectors, newsectors, 2880*512);

#define INVALID_SET_FILE_POINTER (DWORD) -1

	printf("\n");
	for(dw=0; dw<2880; dw+=8) {
		if (INVALID_SET_FILE_POINTER == SetFilePointer(
			hDrive,
			dw*512,
			NULL,
			FILE_BEGIN))
			panic("Couldn't seek!");
		if(0 == WriteFile(
			hDrive,
			((UCHAR *)pDriveSectors)+512*dw,
			512*8,
			&bytesWritten,
			NULL))
			inform("Defective sector: %d\n", dw);
		printf("\b\b\b\b\b\b\b\b\b%03d/%03d", 100*dw/2879, 100);
	}
	VirtualFree(
		pDriveSectors,
		0,
		MEM_RELEASE);

	CloseHandle(hDrive);

    return 0;
}
