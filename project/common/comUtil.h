#pragma once


/* Calculate the number of bytes needed to represent an integer as string. */
static int lenItoA(int i) {
	int len = 0;
	if (i < 0) {
		len++;
		i = -i;
	}
	do {
		len++;
		i /= 10;
	} while (i);
	return len;
}