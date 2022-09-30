/*-------------------------------------------------------------

Copyright (C) 2008
Michael Wiedenbauer (shagkur)
Dave Murphy (WinterMute)

This software is provided 'as-is', without any express or implied
warranty.  In no event will the authors be held liable for any
damages arising from the use of this software.

Permission is granted to anyone to use this software for any
purpose, including commercial applications, and to alter it and
redistribute it freely, subject to the following restrictions:

1.	The origin of this software must not be misrepresented; you
must not claim that you wrote the original software. If you use
this software in a product, an acknowledgment in the product
documentation would be appreciated but is not required.

2.	Altered source versions must be plainly marked as such, and
must not be misrepresented as being the original software.

3.	This notice may not be removed or altered from any source
distribution.

-------------------------------------------------------------*/
#include "main.h"

char _argv_raw[ARGV_MAX];
char *_argc_raw[ARGC_MAX];
int _argc = 0;

void __CheckARGV() {
	if(__system_argv->argvMagic != ARGV_MAGIC) {
		__system_argv->argc = 0;
		__system_argv->argv = NULL;
		return;
	}

	int len = MIN(__system_argv->length, ARGV_MAX);
	memmove(_argv_raw, __system_argv->commandLine, len);
	__system_argv->commandLine = _argv_raw;

	_argc_raw[0] = _argv_raw;
	int cArg = 1;
	_argc = 1;
	while(_argc < __system_argv->argc && _argc < ARGC_MAX
	&& cArg < len) {
		if(_argv_raw[cArg++] == '\0') {
			_argc_raw[_argc++] = &_argv_raw[cArg];
		}
	}
}
