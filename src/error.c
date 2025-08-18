#include "leo/error.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define ERROR_BUFFER_SIZE 4096
static char error_msg[ERROR_BUFFER_SIZE] = { 0 };

void leo_SetError(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	vsnprintf(error_msg, ERROR_BUFFER_SIZE, fmt, args);
	va_end(args);
	error_msg[ERROR_BUFFER_SIZE - 1] = '\0'; // Ensure null-termination
}

const char* leo_GetError(void)
{
	return error_msg;
}

void leo_ClearError(void)
{
	error_msg[0] = '\0';
}
