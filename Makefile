all: sheet.c
	gcc -g -std=c99 -Wall -Wextra -Werror sheet.c -o sheet

clean:
	rm sheet