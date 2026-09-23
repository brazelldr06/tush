all:
	gcc -o tush tush.c parser.c -Wall -Werror

clean:
	rm -f tush