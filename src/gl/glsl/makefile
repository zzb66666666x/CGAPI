parser: grammar.y lexer.l
	bison --yacc -dv grammar.y
	flex lexer.l

clean:
	rm *.tab.c
	rm *.tab.h
	rm *.yy.c
	rm *.output
