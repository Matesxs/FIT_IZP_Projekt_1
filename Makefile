all: code

code: sheet.c
	gcc -g -std=c99 -Wall -Wextra -Werror -O1 sheet.c -o sheet