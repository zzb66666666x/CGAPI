
/* A Bison parser, made by GNU Bison 2.4.1.  */

/* Skeleton interface for Bison's Yacc-like parsers in C
   
      Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.
   
   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */


/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     IN = 258,
     OUT = 259,
     LAYOUT = 260,
     LOC = 261,
     UNIFORM = 262,
     MAT4 = 263,
     MAT3 = 264,
     MAT2 = 265,
     VEC4 = 266,
     VEC3 = 267,
     VEC2 = 268,
     SAMPLER2D = 269,
     LEFT_PAREN = 270,
     RIGHT_PAREN = 271,
     LEFT_BRACE = 272,
     RIGHT_BRACE = 273,
     EQ = 274,
     SEMICOLON = 275,
     DOT = 276,
     COMMA = 277,
     FLOAT = 278,
     DOUBLE = 279,
     INT = 280,
     VOID = 281,
     BOOL = 282,
     IDENTIFIER = 283,
     INTCONSTANT = 284,
     FLOATCONSTANT = 285,
     SIMPLESTATEMENT = 286
   };
#endif
/* Tokens.  */
#define IN 258
#define OUT 259
#define LAYOUT 260
#define LOC 261
#define UNIFORM 262
#define MAT4 263
#define MAT3 264
#define MAT2 265
#define VEC4 266
#define VEC3 267
#define VEC2 268
#define SAMPLER2D 269
#define LEFT_PAREN 270
#define RIGHT_PAREN 271
#define LEFT_BRACE 272
#define RIGHT_BRACE 273
#define EQ 274
#define SEMICOLON 275
#define DOT 276
#define COMMA 277
#define FLOAT 278
#define DOUBLE 279
#define INT 280
#define VOID 281
#define BOOL 282
#define IDENTIFIER 283
#define INTCONSTANT 284
#define FLOATCONSTANT 285
#define SIMPLESTATEMENT 286




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 1676 of yacc.c  */
#line 59 "grammar.y"

	char* str;
	buffer_t* buf;
	int intval;
	float floatval;
	/* double doubleval; */
	/* unsigned int uintval; */



/* Line 1676 of yacc.c  */
#line 125 "y.tab.h"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif

extern YYSTYPE yylval;


