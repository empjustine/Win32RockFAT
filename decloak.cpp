#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>

#include "array.h"
#include "rs.h"

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

int main()
{	
	Array<unsigned char> sectors(2880*512);
	Array<unsigned char> newsectors(2880*512);
	int i,j,k;
	FILE *fp;

	// Initialize Reed-Solomon code
	init_rs();

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

#define INVALID_SET_FILE_POINTER (DWORD) -1

	for(dw=0; dw<2880; dw+=8) {
		if (INVALID_SET_FILE_POINTER == SetFilePointer(
			hDrive,
			dw*512,
			NULL,
			FILE_BEGIN))
			panic("Couldn't seek!");
		if(0 == ReadFile(
			hDrive,
			((UCHAR *)pDriveSectors)+512*dw,
			512*8,
			&bytesWritten,
			NULL))
			inform("Defective sector: %d\n", dw);
		printf("\b\b\b\b\b\b\b\b\b%03d/%03d", 100*dw/2879, 100);
	}
	CopyMemory(sectors, pDriveSectors, 512*2880);

	VirtualFree(
		pDriveSectors,
		0,
		MEM_RELEASE);

	CloseHandle(hDrive);

    j=0; k=0;
	for(j=0;j<2880*512;j++) {
		newsectors[j] = sectors[k];
		k+=4096;
		if(k>=2880*512) {
			k-=2880*512;
			k++;
		}
	}

	int iFileLen = 0;

	fp = fopen("datanew", "wb");	
	i = 0;
	switch(eras_dec_rs(((unsigned char *)newsectors)+i*(NN+1), NULL, 0)) {
	case -1:
		panic("File impaired beyond correction\n");
		break;
	case 0:		
		printf(".");
		iFileLen = *(unsigned int *) (unsigned char *)newsectors;
		fwrite(((unsigned char *)newsectors)+i*(NN+1)+20, 
			1, (iFileLen>KK-20)?KK-20:iFileLen, fp);
		break;
	default:
		printf("E");
		iFileLen = *(unsigned int *) (unsigned char *)newsectors;
		fwrite(((unsigned char *)newsectors)+i*(NN+1)+20, 
			1, (iFileLen>KK-20)?KK-20:iFileLen, fp);
		break;
	}
	i++;
	iFileLen -= (iFileLen>KK-20)?KK-20:iFileLen;

	while(iFileLen) {
		switch(eras_dec_rs(((unsigned char *)newsectors)+i*(NN+1), NULL, 0)) {
		case -1:
			panic("File impaired beyond correction\n");
			break;
		case 0:
			if((i%80) == 0)
			    printf(".");
			fwrite(((unsigned char *)newsectors)+i*(NN+1), 
				1, (iFileLen>KK)?KK:iFileLen, fp);
			break;
		default:
			printf("E");
			fwrite(((unsigned char *)newsectors)+i*(NN+1), 
				1, (iFileLen>KK)?KK:iFileLen, fp);
			break;
		}
		i++;
		iFileLen -= (iFileLen>KK)?KK:iFileLen;
	}	
	fclose(fp);
	rename("datanew", (const char*) (unsigned char *)newsectors+4);
	printf("\nCreated file '%s' (%d bytes)", 
	    (unsigned char *)newsectors+4,
	    *((unsigned int *)(unsigned char *)newsectors));

	return 0;
}
