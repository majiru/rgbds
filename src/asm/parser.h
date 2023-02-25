/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2021 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

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

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

#ifndef YY_YY_SRC_ASM_PARSER_H_INCLUDED
# define YY_YY_SRC_ASM_PARSER_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Token kinds.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    YYEMPTY = -2,
    T_EOF = 0,                     /* "end of file"  */
    YYerror = 1,                   /* error  */
    YYUNDEF = 2,                   /* "invalid token"  */
    T_NUMBER = 3,                  /* "number"  */
    T_STRING = 4,                  /* "string"  */
    T_PERIOD = 5,                  /* "."  */
    T_COMMA = 6,                   /* ","  */
    T_COLON = 7,                   /* ":"  */
    T_LBRACK = 8,                  /* "["  */
    T_RBRACK = 9,                  /* "]"  */
    T_LPAREN = 10,                 /* "("  */
    T_RPAREN = 11,                 /* ")"  */
    T_NEWLINE = 12,                /* "newline"  */
    T_OP_LOGICNOT = 13,            /* "!"  */
    T_OP_LOGICAND = 14,            /* "&&"  */
    T_OP_LOGICOR = 15,             /* "||"  */
    T_OP_LOGICGT = 16,             /* ">"  */
    T_OP_LOGICLT = 17,             /* "<"  */
    T_OP_LOGICGE = 18,             /* ">="  */
    T_OP_LOGICLE = 19,             /* "<="  */
    T_OP_LOGICNE = 20,             /* "!="  */
    T_OP_LOGICEQU = 21,            /* "=="  */
    T_OP_ADD = 22,                 /* "+"  */
    T_OP_SUB = 23,                 /* "-"  */
    T_OP_OR = 24,                  /* "|"  */
    T_OP_XOR = 25,                 /* "^"  */
    T_OP_AND = 26,                 /* "&"  */
    T_OP_SHL = 27,                 /* "<<"  */
    T_OP_SHR = 28,                 /* ">>"  */
    T_OP_USHR = 29,                /* ">>>"  */
    T_OP_MUL = 30,                 /* "*"  */
    T_OP_DIV = 31,                 /* "/"  */
    T_OP_MOD = 32,                 /* "%"  */
    T_OP_NOT = 33,                 /* "~"  */
    NEG = 34,                      /* NEG  */
    T_OP_EXP = 35,                 /* "**"  */
    T_OP_DEF = 36,                 /* "DEF"  */
    T_OP_BANK = 37,                /* "BANK"  */
    T_OP_ALIGN = 38,               /* "ALIGN"  */
    T_OP_SIZEOF = 39,              /* "SIZEOF"  */
    T_OP_STARTOF = 40,             /* "STARTOF"  */
    T_OP_SIN = 41,                 /* "SIN"  */
    T_OP_COS = 42,                 /* "COS"  */
    T_OP_TAN = 43,                 /* "TAN"  */
    T_OP_ASIN = 44,                /* "ASIN"  */
    T_OP_ACOS = 45,                /* "ACOS"  */
    T_OP_ATAN = 46,                /* "ATAN"  */
    T_OP_ATAN2 = 47,               /* "ATAN2"  */
    T_OP_FDIV = 48,                /* "FDIV"  */
    T_OP_FMUL = 49,                /* "FMUL"  */
    T_OP_FMOD = 50,                /* "FMOD"  */
    T_OP_POW = 51,                 /* "POW"  */
    T_OP_LOG = 52,                 /* "LOG"  */
    T_OP_ROUND = 53,               /* "ROUND"  */
    T_OP_CEIL = 54,                /* "CEIL"  */
    T_OP_FLOOR = 55,               /* "FLOOR"  */
    T_OP_HIGH = 56,                /* "HIGH"  */
    T_OP_LOW = 57,                 /* "LOW"  */
    T_OP_ISCONST = 58,             /* "ISCONST"  */
    T_OP_STRCMP = 59,              /* "STRCMP"  */
    T_OP_STRIN = 60,               /* "STRIN"  */
    T_OP_STRRIN = 61,              /* "STRRIN"  */
    T_OP_STRSUB = 62,              /* "STRSUB"  */
    T_OP_STRLEN = 63,              /* "STRLEN"  */
    T_OP_STRCAT = 64,              /* "STRCAT"  */
    T_OP_STRUPR = 65,              /* "STRUPR"  */
    T_OP_STRLWR = 66,              /* "STRLWR"  */
    T_OP_STRRPL = 67,              /* "STRRPL"  */
    T_OP_STRFMT = 68,              /* "STRFMT"  */
    T_OP_CHARLEN = 69,             /* "CHARLEN"  */
    T_OP_CHARSUB = 70,             /* "CHARSUB"  */
    T_LABEL = 71,                  /* "label"  */
    T_ID = 72,                     /* "identifier"  */
    T_LOCAL_ID = 73,               /* "local identifier"  */
    T_ANON = 74,                   /* "anonymous label"  */
    T_POP_EQU = 75,                /* "EQU"  */
    T_POP_EQUAL = 76,              /* "="  */
    T_POP_EQUS = 77,               /* "EQUS"  */
    T_POP_ADDEQ = 78,              /* "+="  */
    T_POP_SUBEQ = 79,              /* "-="  */
    T_POP_MULEQ = 80,              /* "*="  */
    T_POP_DIVEQ = 81,              /* "/="  */
    T_POP_MODEQ = 82,              /* "%="  */
    T_POP_OREQ = 83,               /* "|="  */
    T_POP_XOREQ = 84,              /* "^="  */
    T_POP_ANDEQ = 85,              /* "&="  */
    T_POP_SHLEQ = 86,              /* "<<="  */
    T_POP_SHREQ = 87,              /* ">>="  */
    T_POP_INCLUDE = 88,            /* "INCLUDE"  */
    T_POP_PRINT = 89,              /* "PRINT"  */
    T_POP_PRINTLN = 90,            /* "PRINTLN"  */
    T_POP_IF = 91,                 /* "IF"  */
    T_POP_ELIF = 92,               /* "ELIF"  */
    T_POP_ELSE = 93,               /* "ELSE"  */
    T_POP_ENDC = 94,               /* "ENDC"  */
    T_POP_EXPORT = 95,             /* "EXPORT"  */
    T_POP_DB = 96,                 /* "DB"  */
    T_POP_DS = 97,                 /* "DS"  */
    T_POP_DW = 98,                 /* "DW"  */
    T_POP_DL = 99,                 /* "DL"  */
    T_POP_SECTION = 100,           /* "SECTION"  */
    T_POP_FRAGMENT = 101,          /* "FRAGMENT"  */
    T_POP_RB = 102,                /* "RB"  */
    T_POP_RW = 103,                /* "RW"  */
    T_POP_MACRO = 104,             /* "MACRO"  */
    T_POP_ENDM = 105,              /* "ENDM"  */
    T_POP_RSRESET = 106,           /* "RSRESET"  */
    T_POP_RSSET = 107,             /* "RSSET"  */
    T_POP_UNION = 108,             /* "UNION"  */
    T_POP_NEXTU = 109,             /* "NEXTU"  */
    T_POP_ENDU = 110,              /* "ENDU"  */
    T_POP_INCBIN = 111,            /* "INCBIN"  */
    T_POP_REPT = 112,              /* "REPT"  */
    T_POP_FOR = 113,               /* "FOR"  */
    T_POP_CHARMAP = 114,           /* "CHARMAP"  */
    T_POP_NEWCHARMAP = 115,        /* "NEWCHARMAP"  */
    T_POP_SETCHARMAP = 116,        /* "SETCHARMAP"  */
    T_POP_PUSHC = 117,             /* "PUSHC"  */
    T_POP_POPC = 118,              /* "POPC"  */
    T_POP_SHIFT = 119,             /* "SHIFT"  */
    T_POP_ENDR = 120,              /* "ENDR"  */
    T_POP_BREAK = 121,             /* "BREAK"  */
    T_POP_LOAD = 122,              /* "LOAD"  */
    T_POP_ENDL = 123,              /* "ENDL"  */
    T_POP_FAIL = 124,              /* "FAIL"  */
    T_POP_WARN = 125,              /* "WARN"  */
    T_POP_FATAL = 126,             /* "FATAL"  */
    T_POP_ASSERT = 127,            /* "ASSERT"  */
    T_POP_STATIC_ASSERT = 128,     /* "STATIC_ASSERT"  */
    T_POP_PURGE = 129,             /* "PURGE"  */
    T_POP_REDEF = 130,             /* "REDEF"  */
    T_POP_POPS = 131,              /* "POPS"  */
    T_POP_PUSHS = 132,             /* "PUSHS"  */
    T_POP_POPO = 133,              /* "POPO"  */
    T_POP_PUSHO = 134,             /* "PUSHO"  */
    T_POP_OPT = 135,               /* "OPT"  */
    T_SECT_ROM0 = 136,             /* "ROM0"  */
    T_SECT_ROMX = 137,             /* "ROMX"  */
    T_SECT_WRAM0 = 138,            /* "WRAM0"  */
    T_SECT_WRAMX = 139,            /* "WRAMX"  */
    T_SECT_HRAM = 140,             /* "HRAM"  */
    T_SECT_VRAM = 141,             /* "VRAM"  */
    T_SECT_SRAM = 142,             /* "SRAM"  */
    T_SECT_OAM = 143,              /* "OAM"  */
    T_Z80_ADC = 144,               /* "adc"  */
    T_Z80_ADD = 145,               /* "add"  */
    T_Z80_AND = 146,               /* "and"  */
    T_Z80_BIT = 147,               /* "bit"  */
    T_Z80_CALL = 148,              /* "call"  */
    T_Z80_CCF = 149,               /* "ccf"  */
    T_Z80_CP = 150,                /* "cp"  */
    T_Z80_CPL = 151,               /* "cpl"  */
    T_Z80_DAA = 152,               /* "daa"  */
    T_Z80_DEC = 153,               /* "dec"  */
    T_Z80_DI = 154,                /* "di"  */
    T_Z80_EI = 155,                /* "ei"  */
    T_Z80_HALT = 156,              /* "halt"  */
    T_Z80_INC = 157,               /* "inc"  */
    T_Z80_JP = 158,                /* "jp"  */
    T_Z80_JR = 159,                /* "jr"  */
    T_Z80_LD = 160,                /* "ld"  */
    T_Z80_LDI = 161,               /* "ldi"  */
    T_Z80_LDD = 162,               /* "ldd"  */
    T_Z80_LDH = 163,               /* "ldh"  */
    T_Z80_NOP = 164,               /* "nop"  */
    T_Z80_OR = 165,                /* "or"  */
    T_Z80_POP = 166,               /* "pop"  */
    T_Z80_PUSH = 167,              /* "push"  */
    T_Z80_RES = 168,               /* "res"  */
    T_Z80_RET = 169,               /* "ret"  */
    T_Z80_RETI = 170,              /* "reti"  */
    T_Z80_RST = 171,               /* "rst"  */
    T_Z80_RL = 172,                /* "rl"  */
    T_Z80_RLA = 173,               /* "rla"  */
    T_Z80_RLC = 174,               /* "rlc"  */
    T_Z80_RLCA = 175,              /* "rlca"  */
    T_Z80_RR = 176,                /* "rr"  */
    T_Z80_RRA = 177,               /* "rra"  */
    T_Z80_RRC = 178,               /* "rrc"  */
    T_Z80_RRCA = 179,              /* "rrca"  */
    T_Z80_SBC = 180,               /* "sbc"  */
    T_Z80_SCF = 181,               /* "scf"  */
    T_Z80_SET = 182,               /* "set"  */
    T_Z80_STOP = 183,              /* "stop"  */
    T_Z80_SLA = 184,               /* "sla"  */
    T_Z80_SRA = 185,               /* "sra"  */
    T_Z80_SRL = 186,               /* "srl"  */
    T_Z80_SUB = 187,               /* "sub"  */
    T_Z80_SWAP = 188,              /* "swap"  */
    T_Z80_XOR = 189,               /* "xor"  */
    T_TOKEN_A = 190,               /* "a"  */
    T_TOKEN_B = 191,               /* "b"  */
    T_TOKEN_C = 192,               /* "c"  */
    T_TOKEN_D = 193,               /* "d"  */
    T_TOKEN_E = 194,               /* "e"  */
    T_TOKEN_H = 195,               /* "h"  */
    T_TOKEN_L = 196,               /* "l"  */
    T_MODE_AF = 197,               /* "af"  */
    T_MODE_BC = 198,               /* "bc"  */
    T_MODE_DE = 199,               /* "de"  */
    T_MODE_SP = 200,               /* "sp"  */
    T_MODE_HL = 201,               /* "hl"  */
    T_MODE_HL_DEC = 202,           /* "hld/hl-"  */
    T_MODE_HL_INC = 203,           /* "hli/hl+"  */
    T_CC_NZ = 204,                 /* "nz"  */
    T_CC_Z = 205,                  /* "z"  */
    T_CC_NC = 206,                 /* "nc"  */
    T_EOB = 207                    /* "end of buffer"  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 473 "src/asm/parser.y"

	char symName[MAXSYMLEN + 1];
	char string[MAXSTRLEN + 1];
	struct Expression expr;
	int32_t constValue;
	enum RPNCommand compoundEqual;
	enum SectionModifier sectMod;
	struct SectionSpec sectSpec;
	struct MacroArgs *macroArg;
	enum AssertionType assertType;
	struct DsArgList dsArgs;
	struct {
		int32_t start;
		int32_t stop;
		int32_t step;
	} forArgs;
	struct StrFmtArgList strfmtArgs;
	bool captureTerminated;

#line 291 "src/asm/parser.h"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;


int yyparse (void);


#endif /* !YY_YY_SRC_ASM_PARSER_H_INCLUDED  */
