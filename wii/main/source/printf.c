/** smallish printf replacement that can work with
 *  interrupts disabled.
 *  copied from https://github.com/RenaKunisaki/micron
 */
#ifdef __cplusplus
	extern "C" {
#endif
#include <main.h>

//XXX verify against http://pubs.opengroup.org/onlinepubs/009695399/functions/fprintf.html
//for correct behaviour

//length:    .......h.j.l.......t.....z ...........L..............
//specifier: a.cdefg.i....nop..s.u..x.. A...EFG................X..
//unused:    .b........k.m...qr...vw.y. .BCD...HIJK.MNOPQRSTUVW.YZ

static const char *digitsUpper = "0123456789ABCDEF";
static const char *digitsLower = "0123456789abcdef";

static void printf_write_file(
printf_context *ctxt, const char *str, size_t len) {
	//ctxt->file->cls->write(ctxt->file, str, len);
	//write(ctxt->file, str, len);
	char buf[512];
	memset(buf, 0, sizeof(buf));
	strncpy(buf, str, MIN(len, sizeof(buf)));
	exiPuts(buf);
}

static void printf_write_str(
printf_context *ctxt, const char *str, size_t len) {
	if(!ctxt->dest) return;
	if(len > ctxt->maxChars - 1) len = ctxt->maxChars - 1;
	memcpy(ctxt->dest, str, len);
	ctxt->dest[len] = '\0';
	ctxt->maxChars -= len;
	ctxt->dest     += len;
}


static inline void read_spec(printf_context *ctxt) {
	//read length/specifier
	ctxt->spec = 0;
	ctxt->length = 0;
	char c;
	while((!ctxt->spec) && (c = *(ctxt->format++))) {
		switch(c) {
			case 'h': case 'l': case 'j': case 'z': case 't': case 'L':
				if(ctxt->length == c) ctxt->length |= 0x80; //double letter
				else ctxt->length = c;
				break;

			//valid specifiers
			case 'a': case 'c': case 'd': case 'e': case 'f': case 'g':
			case 'i': case 'n': case 'o': case 'p': case 's': case 'u':
			case 'x': case 'A': case 'E': case 'F': case 'G': case 'X':
			default: //invalid specifier (we'll deal with that later)
				ctxt->spec = c;
				break;
		}
	}
	if(!ctxt->spec) ctxt->format--; //don't run past end of string if truncated
}


static inline void read_flags(printf_context *ctxt) {
	ctxt->leftJustify  = 0; //reset flags
	ctxt->forcePlus    = 0;
	ctxt->forceSpace   = 0;
	ctxt->forceDecimal = 0;
	ctxt->useZeros     = 0;
	ctxt->uppercase    = 0;
	ctxt->width        = 0;
	ctxt->precision    = -1; //unspecified (0 is a valid precision)

	char c, done = 0;
	while((!done) && (c = *(ctxt->format++))) {
		switch(c) {
			//XXX should we care if a flag is specified more than once?
			case '-': ctxt->leftJustify  = 1; break;
			case '+': ctxt->forcePlus    = 1; break;
			case ' ': ctxt->forceSpace   = 1; break;
			case '#': ctxt->forceDecimal = 1; break;
			case '0': ctxt->useZeros     = 1; break;
			default: done = 1; break;
		}
	}

	//read width
	if(c == '*') ctxt->width = _printf_next_arg(int);
	while(isdigit(c)) {
		ctxt->width = (ctxt->width * 10) + (c - '0');
		//do this at the end of the loop, because we don't want to skip
		//whatever 'c' caused the read-flags loop to end.
		c = *(ctxt->format++);
	}

	//read precision
	if(c == '.') {
		ctxt->precision = 0;
		if(*ctxt->format == '*') ctxt->precision = _printf_next_arg(int);
		else while((c = *(ctxt->format++)) && isdigit(c)) {
			ctxt->precision = (ctxt->precision * 10) + (c - '0');
		}
	}

	ctxt->format--;
}


//used by fprintf() to pad output.
//if num is negative, it writes backward into the buffer.
//does not write a null terminator.
static inline char* write_padding(char *buf, char pad, int num) {
	int offs = (num < 0) ? -1 : 1;
	num = abs(num);
	while(num --> 0) {
		*buf = pad;
		buf += offs;
	}
	return buf;
}


//used by fprintf() to print strings.
static inline void write_str(printf_context *ctxt, const char *str) {
	if(str == NULL) str = "(null)";
	if(!PTR_VALID(str)) {
		char buf[64];
		char *c = buf;
		*(c++) = '<';
		*(c++) = 'I';
		*(c++) = 'N';
		*(c++) = 'V';
		*(c++) = 'A';
		*(c++) = 'L';
		*(c++) = ':';
		u32 p = (u32)str;
		for(int i=0; i<8; i++) {
			*(c++) = digitsUpper[p >> 28];
			p <<= 4;
		}
		*(c++) = '>';
		//*(c++) = 0;
		*c = 0;
		ctxt->write(ctxt, buf, (c-buf));
		return;
	}
	int len = strlen(str);
	ctxt->nChars += len;

	//handle right-justify: output a bunch of spaces first.
	if(!ctxt->leftJustify) {
		int remain = ctxt->width - len;
		if(remain > 0) {
			char buf[remain+2];
			write_padding(buf, ' ', remain);
			ctxt->nChars += remain;
			ctxt->write(ctxt, buf, remain);
		}
	}
	ctxt->write(ctxt, str, len);
}

//Used by fprintf() to print signed integers.
//prints *backward* into buf, so it should point to the *end* of the buffer.
//returns number of characters printed.
static inline int write_sint(printf_context *ctxt, char *buf, int base) {
	signed long long int val;
	switch(ctxt->length) {
		case ('h'|0x80): val = (signed char)_printf_next_arg(int);     break;
		case  'h':       val = (short  int) _printf_next_arg(int);     break;
		case  'l':       val = _printf_next_arg(signed long int);      break;
		case ('l'|0x80): val = _printf_next_arg(signed long long int); break;
		case  'j':       val = _printf_next_arg(intmax_t);             break;
		case  't':       val = _printf_next_arg(ptrdiff_t);            break;
		case  'z':       val = _printf_next_arg(ssize_t);              break;
		default:         val = _printf_next_arg(int);                  break;
	}
	ctxt->sign = (val>0) ? 1 : ((val<0) ? -1 : 0);

	const char *digits = ctxt->uppercase ? digitsUpper : digitsLower;
	int count = 0;
	while(val != 0) {
		*(buf--) = digits[abs(val % base)];
		val /= base;
		count++;
	}
	return count;
}

//Used by fprintf() to print unsigned integers.
//prints *backward* into buf, so it should point to the *end* of the buffer.
//returns number of characters printed.
static inline int write_uint(printf_context *ctxt, char *buf, int base) {
	unsigned long long int val;
	switch(ctxt->length) {
		case  'h':       val = (unsigned short)
			_printf_next_arg(unsigned int); break;
		case ('h'|0x80): val = (unsigned char)_printf_next_arg(int);     break;
		case  'l':       val = _printf_next_arg(unsigned long int);      break;
		case ('l'|0x80): val = _printf_next_arg(unsigned long long int); break;
		case  'j':       val = _printf_next_arg(uintmax_t);              break;
		case  't':       val = _printf_next_arg(ptrdiff_t);              break;
		case  'z':       val = _printf_next_arg(ssize_t);                break;
		default:         val = _printf_next_arg(unsigned int);           break;
	}
	if(val == 0 && ctxt->isPointer) {
		ctxt->useZeros     = 0;
		ctxt->forceDecimal = 0;
		ctxt->precision    = 0;
		strncpy(&buf[-4], "(nil)", 5); //strlen("(nil)") = 5, +1 for \0
		return 5;
	}
	if(val > 0) ctxt->sign = 1;

	int count = 0;
	const char *digits = ctxt->uppercase ? digitsUpper : digitsLower;
	while(val != 0) {
		*(buf--) = digits[val % base];
		val /= base;
		count++;
	}
	return count;
}

//Used by fprintf() to print integers.
static inline char* write_int(printf_context *ctxt) {
	int base, isSigned;

	ctxt->sign      = 0;
	ctxt->uppercase = 0;
	switch(ctxt->spec) {
		case 'd': //same as i
		case 'i': base = 10; isSigned = 1; break;
		case 'u': base = 10; isSigned = 0; break;
		case 'o': base =  8; isSigned = 0; break;
		case 'x': base = 16; isSigned = 0; break;
		case 'X': base = 16; isSigned = 0; ctxt->uppercase = 1; break;
		case 'p': //pointer
			base     = 16;
			isSigned = 0;
			if(ctxt->precision == -1) ctxt->precision = 2 * sizeof(void*);
			ctxt->forceDecimal = 1; //force 0x
			ctxt->isPointer    = 1;
			break;
		default:
			//should never happen, because we only call it in these cases.
			return NULL;
	}
	if(ctxt->precision == -1) ctxt->precision = 1; //default

	//write digits backward into buf
	char *bEnd = &ctxt->buf[ctxt->bufLen - 1];
	char *out  = bEnd;
	*(out--)   = '\0';

	//forceDecimal flag with base 10 == always show a decimal point
	//we know there's nothing after it here, because this function
	//only deals with integers.
	if(ctxt->forceDecimal && base == 10) *(out--) = '.';

	int count = (isSigned ? write_sint : write_uint)(ctxt, out, base);
	out -= count;

	//precision n = minimum n digits (pad with zeros)
	//precision 0 = don't write anything if val == 0
	int width     = ctxt->width     - count;
	int precision = ctxt->precision - count;
	if(ctxt->forceDecimal) width -= (base > 10) ? 2 : 1; //for prefix/decimal


	//sign character
	//XXX should a + be added when val == 0?
	char signChar = 0;
	if     (ctxt->sign <  0)                     signChar = '-';
	else if(ctxt->sign >  0 && ctxt->forcePlus ) signChar = '+';
	else if(ctxt->sign == 0 && ctxt->forceSpace) signChar = ' ';


	//if #digits < precision, pad with zeros.
	//if negative and base > 10, pad with the highest digit instead.
	//(so eg -1 as %04X = FFFF, not 000F; but -1 as %04d = -0001, not -9991)
	char pad;
	if(base > 10 && ctxt->sign < 0) {
		pad = (ctxt->uppercase ? digitsUpper : digitsLower)[base-1];
	}
	else pad = '0';
	while(precision --> 0) {
		*(out--) = pad;
		width--;
	}

	if(!ctxt->useZeros) { //place prefix and sign
		//forceDecimal flag with base 16 == prepend 0x or 0X
		if(ctxt->forceDecimal && base == 16) {
			*(out--) = ctxt->uppercase ? 'X' : 'x';
			*(out--) = '0';
		}
		if(signChar) {
			*(out--) = signChar;
			width--;
		}
	}

	//if #digits < width, pad.
	//if left-justified, we pad after returning from this.
	if(width > 0 && !ctxt->leftJustify) {
		if(!ctxt->useZeros) pad = ' ';
		out   = write_padding(out, pad, -width);
		width = 0;
	}

	if(ctxt->useZeros) { //place prefix and sign
		//forceDecimal flag with base 16 == prepend 0x or 0X
		if(ctxt->forceDecimal && base == 16) {
			*(out--) = ctxt->uppercase ? 'X' : 'x';
			*(out--) = '0';
		}
		if(signChar) *(out--) = signChar;
	}

	ctxt->nChars += (bEnd - out) - 1; //add to character count.
	return ++out; //return pointer to beginning of resulting string.
}


int _printf_internal(printf_context *ctxt) {
	char c;
	char buf[256]; //for writing numbers into
	const char *tail = ctxt->format; //where to begin copying literal string from

	while((c = *(ctxt->format++))) {
		if(c != '%') continue;

		//write what we have so far
		if(tail != ctxt->format) {
			//subtract 1, we don't want to output the %
			ctxt->write(ctxt, tail, (ctxt->format - tail) - 1);
			ctxt->nChars += (ctxt->format - tail) - 1;
			tail = ctxt->format;
		}

		//read the next char after the %
		c = *ctxt->format;
		if(c == '%') { //"%%" => output a %
			tail = ctxt->format++; //start from the %
			continue;
		}

		//we found a %, now handle the args.
		//%[flags][width][.precision][length]specifier
		//%d %8X %08hhX %1.2f %-10s %-03.6Lf %00d
		ctxt->buf    = buf;
		ctxt->bufLen = sizeof(buf);
		read_flags(ctxt);
		read_spec (ctxt);

		//handle this format
		size_t curnChars = ctxt->nChars;
		switch(ctxt->spec) {
			case 'd': case 'i': //signed decimal integer
			case 'u':   //unsigned decimal integer
			case 'o':   //octal integer
			case 'x':   //lowercase hex integer
			case 'X':   //uppercase hex integer
			case 'p': { //pointer
				char *res = write_int(ctxt);
				ctxt->write(ctxt, res, ctxt->nChars - curnChars);
				break;
			}

			case 'c': //character
				//XXX can width/precision apply here?
				buf[0] = (char)_printf_next_arg(int);
				buf[1] = '\0';
				write_str(ctxt, buf);
				break;

			case 's': //string
				write_str(ctxt, _printf_next_arg(char*));
				break;

			case 'n': //write # chars to signed int* arg
				*_printf_next_arg(signed int*) = ctxt->nChars;
				break;

			case 'f': //lowercase float
			case 'F': //uppercase float

			case 'e': //lowercase float, scientific notation
			case 'E': //uppercase float, scientific notation

			case 'g': //whichever of e or f is shorter
			case 'G': //whichever of E or F is shorter

			case 'a': //lowercase hex float
			case 'A': //uppercase hex float
				//XXX TODO implement these

			default:
				//invalid/unsupported format
				//just output the string directly
				ctxt->write(ctxt, tail - 1, (ctxt->format - tail) + 1);
		}

		//handle left-justify (pad at right)
		if(ctxt->leftJustify) {
			int remain = ctxt->width - (ctxt->nChars - curnChars);
			if(remain > 0) {
				*(write_padding(buf, ' ', remain) + 1) = '\0';
				ctxt->write(ctxt, buf, remain);
				ctxt->nChars += remain;
			}
		}

		//next place to copy from is after the format string.
		tail = ctxt->format;
	}

	//write remaining output
	if(tail != ctxt->format) {
		ctxt->write(ctxt, tail, (ctxt->format - tail) - 1);
		ctxt->nChars += (ctxt->format - tail) - 1;
	}
	return ctxt->nChars;
}

int fprintf(FILE *file, const char *format, ...) {
	printf_context ctxt;
	memset((void*)&ctxt, 0, sizeof(ctxt));
	ctxt.format = format;
	ctxt.write  = printf_write_file;
	ctxt.file   = file;
	va_start(ctxt.args, format);
	int r = _printf_internal(&ctxt);
	va_end(ctxt.args);
	return r;
}

int vfprintf(FILE *file, const char *format, va_list args) {
	printf_context ctxt;
	memset((void*)&ctxt, 0, sizeof(ctxt));
	ctxt.format = format;
	ctxt.write  = printf_write_file;
	ctxt.file   = file;
	memcpy(&ctxt.args, &args, sizeof(ctxt.args));
	int r = _printf_internal(&ctxt);
	return r;
}

int sprintf(char *dest, const char *format, ...) {
	printf_context ctxt;
	memset((void*)&ctxt, 0, sizeof(ctxt));
	ctxt.format   = format;
	ctxt.write    = printf_write_str;
	ctxt.dest     = dest;
	ctxt.maxChars = INT_MAX;
	va_start(ctxt.args, format);
	int r = _printf_internal(&ctxt);
	va_end(ctxt.args);
	return r;
}

int vsprintf(char *dest, const char *format, va_list args) {
	printf_context ctxt;
	memset((void*)&ctxt, 0, sizeof(ctxt));
	ctxt.format   = format;
	ctxt.write    = printf_write_str;
	ctxt.dest     = dest;
	ctxt.maxChars = INT_MAX;
	va_copy(ctxt.args, args);
	int r = _printf_internal(&ctxt);
	return r;
}


int snprintf(char *dest, size_t len, const char *format, ...) {
	printf_context ctxt;
	memset((void*)&ctxt, 0, sizeof(ctxt));
	ctxt.format   = format;
	ctxt.write    = printf_write_str;
	ctxt.dest     = dest;
	ctxt.maxChars = len;
	va_start(ctxt.args, format);
	int r = _printf_internal(&ctxt);
	va_end(ctxt.args);
	return r;
}

int vsnprintf(char *dest, size_t len, const char *format, va_list args) {
    printf_context ctxt;
	memset((void*)&ctxt, 0, sizeof(ctxt));
	ctxt.format   = format;
	ctxt.write    = printf_write_str;
	ctxt.dest     = dest;
	ctxt.maxChars = len;
    va_copy(ctxt.args, args);
	int r = _printf_internal(&ctxt);
	return r;
}


int printf(const char *format, ...) {
	printf_context ctxt;
	memset((void*)&ctxt, 0, sizeof(ctxt));
	ctxt.format = format;
	ctxt.write  = printf_write_file;
	ctxt.file   = stdout;
	va_start(ctxt.args, format);
	int r = _printf_internal(&ctxt);
	va_end(ctxt.args);
	return r;
}

int vprintf(const char *format, va_list args) {
	printf_context ctxt;
	memset((void*)&ctxt, 0, sizeof(ctxt));
	ctxt.format = format;
	ctxt.write  = printf_write_file;
	//ctxt.file   = stdout;
	va_copy(ctxt.args, args);
	int r = _printf_internal(&ctxt);
	return r;
}

#ifdef __cplusplus
	} //extern "C"
#endif
