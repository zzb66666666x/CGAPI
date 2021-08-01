%{
#include "parse.h"
#include "y.tab.h"
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdbool.h>
void yyerror(char *);

int brace_depth = 0;
	
%}

digit			[0-9]
nondigit		[_a-zA-Z]
identifier		{nondigit}({nondigit}|{digit})*
int_const		{digit}+
float_const		{digit}+\.{digit}+

%x in_brace

%%

"in"		return IN;
"out"		return OUT;
"layout"	return LAYOUT;
"location"	return LOC;
"uniform"	return UNIFORM;
"(" 		return LEFT_PAREN;
")" 		return RIGHT_PAREN;
<*>"{" 		{
				BEGIN(in_brace);
				brace_depth ++;
				return LEFT_BRACE;
			}
<*>"}"		{
				brace_depth --;
				if (brace_depth == 0){
					BEGIN(0);
				}
				return RIGHT_BRACE;
			}
"="			return EQ;
";"			return SEMICOLON;
"." 		return DOT;
"," 		return COMMA;
"float"		return FLOAT;
"double"	return DOUBLE;
"int"		return INT;
"void"		return VOID;
"bool"		return BOOL;
"mat4"		return MAT4;
"mat3"		return MAT3;
"mat2"		return MAT2;
"vec4"		return VEC4;
"vec3"		return VEC3;
"vec2"		return VEC2;
"sampler2D" return SAMPLER2D;

[ \t\n]+
"//".*

{identifier}	{ yylval.str = strdup(yytext); return IDENTIFIER; }
{int_const}		{ yylval.intval = atoi(yytext); return INTCONSTANT; }
{float_const}	{ yylval.floatval = atof(yytext); return FLOATCONSTANT; }
<in_brace>[^{}]* 	{ yylval.str = strdup(yytext); return SIMPLESTATEMENT; }

%%

void free_lexer_buffer(){
	yy_delete_buffer(YY_CURRENT_BUFFER);
}