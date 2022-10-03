#ifndef __PRINTF_H__
#define __PRINTF_H__

//TODO:
//-float formats: %f, %F, %e, %E, %g, %G, %a, %A
//-support %n$x notation
//-more testing

#ifdef __cplusplus
	extern "C" {
#endif

#include <stdarg.h> //va_list
#include <limits.h> //INT_MAX
#include <ctype.h>  //isdigit

//extern int fprintf  (FILE *file, const char *format, ...);
//extern int vfprintf (FILE *file, const char *format, va_list args);
extern int sprintf  (char *dest, const char *format, ...);
extern int vsprintf (char *dest, const char *format, va_list args);
extern int snprintf (char *dest, size_t len, const char *format, ...);
extern int vsnprintf(char *dest, size_t len, const char *format, va_list args);
extern int printf   (const char *format, ...);
extern int vprintf  (const char *format, va_list args);

#define _printf_next_arg(tp) (ctxt->argn++, va_arg(ctxt->args, tp))

//internal variables used by printf(). kept in a struct
//so that they can easily be shared by the various subroutines.
typedef struct printf_context printf_context;
typedef struct printf_context {
	//                            flag    description
	unsigned int leftJustify  :1; //-     left justify this field
	unsigned int forcePlus    :1; //+     prepend a + for positive nums
	unsigned int forceSpace   :1; //space prepend a space if no sign
	unsigned int forceDecimal :1; //#     force a . or 0x
	unsigned int useZeros     :1; //0     left-pad with 0, not space
	unsigned int uppercase    :1; //use uppercase
	unsigned int isPointer    :1; //is a pointer (write (nil) instead of 0)
	union {
		//FILE  *file; //destination file
		char  *dest; //destination buffer
	};
	void (*write)(printf_context*, const char*, size_t); //write func
	const char  *format;     //format string
	int           sign;      //sign of number printed
	int           width;     //pad to this many chars
	char          length;    //data type indicator
	unsigned char spec;      //format specifier
	int           precision; //this many decimals
	int           argn;      //current arg index we're processing
	int           nChars;    //# characters written
	size_t        maxChars;  //max chars to write to dest (for snprintf)
	char         *buf;       //a place to write digits to while formatting
	size_t        bufLen;    //size of that buffer
	va_list       args;      //the argument list
} printf_context;

#ifdef __cplusplus
	} //extern "C"
#endif

#endif //__PRINTF_H__
