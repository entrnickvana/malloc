

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"

int main()
{

	mm_init();
	void* ptr0 = mm_malloc(1);
	void* ptr1 = mm_malloc(2);
	void* ptr2 = mm_malloc(3);


	return 0;
}