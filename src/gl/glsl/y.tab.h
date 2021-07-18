
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
     LEFT_PAREN = 269,
     RIGHT_PAREN = 270,
     LEFT_BRACE = 271,
     RIGHT_BRACE = 272,
     EQ = 273,
     SEMICOLON = 274,
     DOT = 275,
     COMMA = 276,
     FLOAT = 277,
     DOUBLE = 278,
     INT = 279,
     VOID = 280,
     BOOL = 281,
     IDENTIFIER = 282,
     INTCONSTANT = 283,
     FLOATCONSTANT = 284,
     FUNCTION_CODE_BODY = 285
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
#define LEFT_PAREN 269
#define RIGHT_PAREN 270
#define LEFT_BRACE 271
#define RIGHT_BRACE 272
#define EQ 273
#define SEMICOLON 274
#define DOT 275
#define COMMA 276
#define FLOAT 277
#define DOUBLE 278
#define INT 279
#define VOID 280
#define BOOL 281
#define IDENTIFIER 282
#define INTCONSTANT 283
#define FLOATCONSTANT 284
#define FUNCTION_CODE_BODY 285




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 1676 of yacc.c  */
#line 57 "grammar.y"

	char* str;
	buffer_t* buf;
	int intval;
	float floatval;
	/* double doubleval; */
	/* unsigned int uintval; */



/* Line 1676 of yacc.c  */
#line 123 "y.tab.h"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif

extern YYSTYPE yylval;


