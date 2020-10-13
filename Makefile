all: sheet.c
	gcc -std=c99 -Wall -Wextra sheet.c -o sheet

clean:
	rm sheet.exe
	rm sheet