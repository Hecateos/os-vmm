#include <string.h>
#include <errno.h>
#include <stdio.h>


void error_sys(const char *fmt)
{
	puts(fmt);
  	exit(1);
}


