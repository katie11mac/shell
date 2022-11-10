CFLAGS=-Wall -pedantic -g 

mysh: mysh.c
	gcc $(CFLAGS) -o $@ $^

mysh.strace: mysh
	strace -f -o $@ ./$<

.PHONY: clean
clean:
	rm -f mysh mysh.strace