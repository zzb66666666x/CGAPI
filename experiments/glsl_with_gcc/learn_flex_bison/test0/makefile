all: keyword.l
	flex -okeyword.c keyword.l
	gcc keyword.c -o test

clean:
	rm *.exe *.yy.c