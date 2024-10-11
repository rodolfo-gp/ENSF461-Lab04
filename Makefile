.PHONY: all

all: scheduler.out

scheduler.out: scheduler.c
	gcc -g -Wall -o $@ $<
# gcc -g -Wall -o scheduler.out scheduler.c  

clean:
	rm -f scheduler.out

