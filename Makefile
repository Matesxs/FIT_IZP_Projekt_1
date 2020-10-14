all: sheet.c
	gcc -std=c99 -Wall -Wextra -Werror sheet.c -lm -o sheet

clean:
	rm sheet