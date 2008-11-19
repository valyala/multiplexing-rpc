#if defined(WIN32)
	#define _CRT_SECURE_NO_WARNINGS
#endif

#include "common.h"

#include <stdarg.h>

#define FILENAME_BUF_SIZE 200

static char filename_buf[FILENAME_BUF_SIZE];
static FILE *file = NULL;

void open_file(const char *format, ...)
{
	va_list args_ptr;
	int len;

	va_start(args_ptr, format);
	len = vsnprintf(filename_buf, FILENAME_BUF_SIZE, format, args_ptr);
	if (len >= FILENAME_BUF_SIZE)
	{
		die("cannot generate filename");
	}
	va_end(args_ptr);

	assert(file == NULL);
	file = fopen(filename_buf, "wt");
	if (file == NULL)
	{
		die("canot create the file=[%s]. errno=%d", filename_buf, errno);
	}
}

void close_file()
{
	int rv;

	assert(file != NULL);
	rv = fflush(file);
	assert(rv == 0);
	rv = fclose(file);
	assert(rv == 0);
	file = NULL;
}

void die(const char *format, ...)
{
	va_list args_ptr;
	
	va_start(args_ptr, format);
	vfprintf(stderr, format, args_ptr);
	fprintf(stderr, "\n");
	va_end(args_ptr);
	exit(EXIT_FAILURE);
}

void dump(const char *format, ...)
{
	va_list args_ptr;
	int len;

	assert(file != NULL);

	va_start(args_ptr, format);
	len = vfprintf(file, format, args_ptr);
	if (len <= 0)
	{
		die("error when writing data to the output file");
		exit(EXIT_FAILURE);
	}
	va_end(args_ptr);
}
