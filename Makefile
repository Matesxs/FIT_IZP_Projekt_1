all: sheet.c
	gcc -std=c99 -Wall -Wextra -Werror sheet.c -o sheet

clean:
	rm sheet