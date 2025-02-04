var1 = gcc
var2 = -Wall -Werror -Wextra -std=c11

all: graph

graph: main.o
	mkdir -p ../build  
	$(var1) $(var2) -o ../build/graph graph.c main.o
	cppcheck --enable=all --suppress=missingIncludeSystem main.c graph.c main.h
	clang-format -i *.c *.h

main.o:
	$(var1) $(var2) -c main.c -o main.o


clean:
	rm -f ../build/graph
	rm -f *.o


rebuild: clean all

