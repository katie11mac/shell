CFLAGS=-Wall -pedantic -g 

mysh: mysh.c
	gcc $(CFLAGS) -o $@ $^

mysh.strace: mysh
	strace -o -f $@ ./$<

.PHONY: clean
clean:
	rm -f mysh mysh.strace