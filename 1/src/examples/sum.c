#include <stdio.h>
#include <stdlib.h>
#include <syscall.h>

int main (int argc, char **argv) {
	int i;
	int n[4];

	for (i = 1 ; i < argc ; i++)
		n[i - 1] = atoi (argv[i]);

	printf ("%d %d\n", fibonacci(n[0]), sum_of_four_int(n[0], n[1], n[2], n[3]));

	return EXIT_SUCCESS;
}
