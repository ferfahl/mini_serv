#include <unistd.h>
#include <stdlib.h>
#include <string.h>


void	exit_error_message(char *str) {
	write(2, &str, strlen(str));
	exit(1);
}

int main(int argc, char *argv[]) {
	if (argc != 2)
		exit_error_message("Wrong number of arguments\n");
	(void)argv;
}