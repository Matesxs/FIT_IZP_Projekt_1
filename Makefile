all: sheet.c
	gcc -g3 -std=c99 -Wall -Wextra -Werror sheet.c -lm -o sheet

clean:
	rm sheet