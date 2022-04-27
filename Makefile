ALL: parent

parent: parent.o child.o
	gcc parent.o child.o -o parent -lpthread

parent.o: parent.c
	gcc -g -Wall -c parent.c

child.o: child.c
	gcc -g -Wall -c child.c

clean:
	rm -f parent parent.o child.o
