/* A Bison parser, made by GNU Bison 3.2.  */

/* Skeleton interface for Bison GLR parsers in C

   Copyright (C) 2002-2015, 2018 Free Software Foundation, Inc.

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

#ifndef YY_YY_SRC_LIBEXPR_PARSER_TAB_HH_INCLUDED
# define YY_YY_SRC_LIBEXPR_PARSER_TAB_HH_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif
/* "%code requires" blocks.  */
#line 14 "src/libexpr/parser.y" /* glr.c:197  */


#ifndef BISON_HEADER
#define BISON_HEADER

#include "util.hh"

#include "nixexpr.hh"
#include "eval.hh"

namespace nix {

    struct ParseData
    {
        EvalState & state;
        SymbolTable & symbols;
        Expr * result;
        Path basePath;
        Symbol path;
        string error;
        Symbol sLetBody;
        ParseData(EvalState & state)
            : state(state)
            , symbols(state.symbols)
            , sLetBody(symbols.create("<let-body>"))
            { };
    };

}

#define YY_DECL int yylex \
    (YYSTYPE * yylval_param, YYLTYPE * yylloc_param, yyscan_t yyscanner, nix::ParseData * data)

#endif


#line 81 "src/libexpr/parser-tab.hh" /* glr.c:197  */

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    ID = 258,
    ATTRPATH = 259,
    STR = 260,
    IND_STR = 261,
    INTX = 262,
    FLOATX = 263,
    PATH = 264,
    HPATH = 265,
    SPATH = 266,
    URI = 267,
    IF = 268,
    THEN = 269,
    ELSE = 270,
    ASSERT = 271,
    WITH = 272,
    LET = 273,
    INX = 274,
    REC = 275,
    INHERIT = 276,
    EQ = 277,
    NEQ = 278,
    AND = 279,
    OR = 280,
    IMPL = 281,
    OR_KW = 282,
    DOLLAR_CURLY = 283,
    IND_STRING_OPEN = 284,
    IND_STRING_CLOSE = 285,
    ELLIPSIS = 286,
    LEQ = 287,
    GEQ = 288,
    UPDATE = 289,
    NOT = 290,
    CONCAT = 291,
    NEGATE = 292
  };
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED

union YYSTYPE
{
#line 239 "src/libexpr/parser.y" /* glr.c:197  */

  // !!! We're probably leaking stuff here.
  nix::Expr * e;
  nix::ExprList * list;
  nix::ExprAttrs * attrs;
  nix::Formals * formals;
  nix::Formal * formal;
  nix::NixInt n;
  nix::NixFloat nf;
  const char * id; // !!! -> Symbol
  char * path;
  char * uri;
  std::vector<nix::AttrName> * attrNames;
  std::vector<nix::Expr *> * string_parts;

#line 147 "src/libexpr/parser-tab.hh" /* glr.c:197  */
};

typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif

/* Location type.  */
#if ! defined YYLTYPE && ! defined YYLTYPE_IS_DECLARED
typedef struct YYLTYPE YYLTYPE;
struct YYLTYPE
{
  int first_line;
  int first_column;
  int last_line;
  int last_column;
};
# define YYLTYPE_IS_DECLARED 1
# define YYLTYPE_IS_TRIVIAL 1
#endif



int yyparse (void * scanner, nix::ParseData * data);

#endif /* !YY_YY_SRC_LIBEXPR_PARSER_TAB_HH_INCLUDED  */
