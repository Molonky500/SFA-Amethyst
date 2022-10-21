#include <stdio.h>
#include <stdlib.h>
#include <gccore.h>
#include <fat.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <stdarg.h>

#define SPI_RXBUF_SIZE 128

//main.c
extern bool gIsVideoInit;
