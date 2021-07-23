all:
	bison --yacc -dv test.y
	flex test.l
	gcc -o test y.tab.c lex.yy.c 