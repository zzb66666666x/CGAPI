test: main.cpp shader.hpp lex.yy.o y.tab.o parse.o symbols.o translate.o
	g++ -g -o main.o -c main.cpp
	g++ -g main.o lex.yy.o y.tab.o parse.o symbols.o translate.o -o test

lex.yy.o: lex.yy.c
	gcc -g -o lex.yy.o -c lex.yy.c

y.tab.o: y.tab.c y.tab.h
	gcc -g -o y.tab.o -c y.tab.c

symbols.o: symbols.cpp symbols.h
	g++ -g -o symbols.o -c symbols.cpp

translate.o: translate.cpp translate.h
	g++ -g -o translate.o -c translate.cpp

parse.o: parse.c parse.h
	gcc -g -o parse.o -c parse.c

parser: test.y test.l
	bison --yacc -dv test.y
	flex test.l

clean:
	rm *.tab.c
	rm *.tab.h
	rm *.yy.c
	rm *.o
	rm *.output
	rm *.exe
	rm *.dll
