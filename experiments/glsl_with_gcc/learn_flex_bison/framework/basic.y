%{
int yylex(void);
void yyerror(char *);
%}
%token 

%%

%%
void yyerror(char *str){
    fprintf(stderr,"error:%s\n",str);
}

int yywrap(){
    return 1;
}
int main()
{
    yyparse();
}
