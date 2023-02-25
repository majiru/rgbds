/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison implementation for Yacc-like parsers in C

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

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output, and Bison version.  */
#define YYBISON 30802

/* Bison version string.  */
#define YYBISON_VERSION "3.8.2"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1




/* First part of user prologue.  */
#line 9 "src/asm/parser.y"

#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "asm/charmap.h"
#include "asm/fixpoint.h"
#include "asm/format.h"
#include "asm/fstack.h"
#include "asm/lexer.h"
#include "asm/macro.h"
#include "asm/main.h"
#include "asm/opt.h"
#include "asm/output.h"
#include "asm/rpn.h"
#include "asm/section.h"
#include "asm/symbol.h"
#include "asm/util.h"
#include "asm/warning.h"

#include "extern/utf8decoder.h"

#include "linkdefs.h"
#include "platform.h" // strncasecmp, strdup

static struct CaptureBody captureBody; // Captures a REPT/FOR or MACRO

static void upperstring(char *dest, char const *src)
{
	while (*src)
		*dest++ = toupper(*src++);
	*dest = '\0';
}

static void lowerstring(char *dest, char const *src)
{
	while (*src)
		*dest++ = tolower(*src++);
	*dest = '\0';
}

static uint32_t str2int2(uint8_t *s, uint32_t length)
{
	if (length > 4)
		warning(WARNING_NUMERIC_STRING_1,
			"Treating string as a number ignores first %" PRIu32 " character%s\n",
			length - 4, length == 5 ? "" : "s");
	else if (length > 1)
		warning(WARNING_NUMERIC_STRING_2,
			"Treating %" PRIu32 "-character string as a number\n", length);

	uint32_t r = 0;

	for (uint32_t i = length < 4 ? 0 : length - 4; i < length; i++) {
		r <<= 8;
		r |= s[i];
	}

	return r;
}

static const char *strrstr(char const *s1, char const *s2)
{
	size_t len1 = strlen(s1);
	size_t len2 = strlen(s2);

	if (len2 > len1)
		return NULL;

	for (char const *p = s1 + len1 - len2; p >= s1; p--)
		if (!strncmp(p, s2, len2))
			return p;

	return NULL;
}

static void errorInvalidUTF8Byte(uint8_t byte, char const *functionName)
{
	error("%s: Invalid UTF-8 byte 0x%02hhX\n", functionName, byte);
}

static size_t strlenUTF8(char const *s)
{
	size_t len = 0;
	uint32_t state = 0;

	for (uint32_t codep = 0; *s; s++) {
		uint8_t byte = *s;

		switch (decode(&state, &codep, byte)) {
		case 1:
			errorInvalidUTF8Byte(byte, "STRLEN");
			state = 0;
			// fallthrough
		case 0:
			len++;
			break;
		}
	}

	// Check for partial code point.
	if (state != 0)
		error("STRLEN: Incomplete UTF-8 character\n");

	return len;
}

static void strsubUTF8(char *dest, size_t destLen, char const *src, uint32_t pos, uint32_t len)
{
	size_t srcIndex = 0;
	size_t destIndex = 0;
	uint32_t state = 0;
	uint32_t codep = 0;
	uint32_t curLen = 0;
	uint32_t curPos = 1;

	// Advance to starting position in source string.
	while (src[srcIndex] && curPos < pos) {
		switch (decode(&state, &codep, src[srcIndex])) {
		case 1:
			errorInvalidUTF8Byte(src[srcIndex], "STRSUB");
			state = 0;
			// fallthrough
		case 0:
			curPos++;
			break;
		}
		srcIndex++;
	}

	// A position 1 past the end of the string is allowed, but will trigger the
	// "Length too big" warning below if the length is nonzero.
	if (!src[srcIndex] && pos > curPos)
		warning(WARNING_BUILTIN_ARG,
			"STRSUB: Position %" PRIu32 " is past the end of the string\n", pos);

	// Copy from source to destination.
	while (src[srcIndex] && destIndex < destLen - 1 && curLen < len) {
		switch (decode(&state, &codep, src[srcIndex])) {
		case 1:
			errorInvalidUTF8Byte(src[srcIndex], "STRSUB");
			state = 0;
			// fallthrough
		case 0:
			curLen++;
			break;
		}
		dest[destIndex++] = src[srcIndex++];
	}

	if (curLen < len)
		warning(WARNING_BUILTIN_ARG, "STRSUB: Length too big: %" PRIu32 "\n", len);

	// Check for partial code point.
	if (state != 0)
		error("STRSUB: Incomplete UTF-8 character\n");

	dest[destIndex] = '\0';
}

static size_t charlenUTF8(char const *s)
{
	size_t len;

	for (len = 0; charmap_ConvertNext(&s, NULL); len++)
		;

	return len;
}

static void charsubUTF8(char *dest, char const *src, uint32_t pos)
{
	size_t charLen = 1;

	// Advance to starting position in source string.
	for (uint32_t curPos = 1; charLen && curPos < pos; curPos++)
		charLen = charmap_ConvertNext(&src, NULL);

	char const *start = src;

	if (!charmap_ConvertNext(&src, NULL))
		warning(WARNING_BUILTIN_ARG,
			"CHARSUB: Position %" PRIu32 " is past the end of the string\n", pos);

	// Copy from source to destination.
	memcpy(dest, start, src - start);

	dest[src - start] = '\0';
}

static uint32_t adjustNegativePos(int32_t pos, size_t len, char const *functionName)
{
	// STRSUB and CHARSUB adjust negative `pos` arguments the same way,
	// such that position -1 is the last character of a string.
	if (pos < 0)
		pos += len + 1;
	if (pos < 1) {
		warning(WARNING_BUILTIN_ARG, "%s: Position starts at 1\n", functionName);
		pos = 1;
	}
	return (uint32_t)pos;
}

static void strrpl(char *dest, size_t destLen, char const *src, char const *old, char const *rep)
{
	size_t oldLen = strlen(old);
	size_t repLen = strlen(rep);
	size_t i = 0;

	if (!oldLen) {
		warning(WARNING_EMPTY_STRRPL, "STRRPL: Cannot replace an empty string\n");
		strcpy(dest, src);
		return;
	}

	for (char const *next = strstr(src, old); next && *next; next = strstr(src, old)) {
		// Copy anything before the substring to replace
		unsigned int lenBefore = next - src;

		memcpy(dest + i, src, lenBefore < destLen - i ? lenBefore : destLen - i);
		i += next - src;
		if (i >= destLen)
			break;

		// Copy the replacement substring
		memcpy(dest + i, rep, repLen < destLen - i ? repLen : destLen - i);
		i += repLen;
		if (i >= destLen)
			break;

		src = next + oldLen;
	}

	if (i < destLen) {
		size_t srcLen = strlen(src);

		// Copy anything after the last replaced substring
		memcpy(dest + i, src, srcLen < destLen - i ? srcLen : destLen - i);
		i += srcLen;
	}

	if (i >= destLen) {
		warning(WARNING_LONG_STR, "STRRPL: String too long, got truncated\n");
		i = destLen - 1;
	}
	dest[i] = '\0';
}

static void initStrFmtArgList(struct StrFmtArgList *args)
{
	args->nbArgs = 0;
	args->capacity = INITIAL_STRFMT_ARG_SIZE;
	args->args = malloc(args->capacity * sizeof(*args->args));
	if (!args->args)
		fatalerror("Failed to allocate memory for STRFMT arg list: %s\n",
			   strerror(errno));
}

static size_t nextStrFmtArgListIndex(struct StrFmtArgList *args)
{
	if (args->nbArgs == args->capacity) {
		args->capacity = (args->capacity + 1) * 2;
		args->args = realloc(args->args, args->capacity * sizeof(*args->args));
		if (!args->args)
			fatalerror("realloc error while resizing STRFMT arg list: %s\n",
				   strerror(errno));
	}
	return args->nbArgs++;
}

static void freeStrFmtArgList(struct StrFmtArgList *args)
{
	free(args->format);
	for (size_t i = 0; i < args->nbArgs; i++)
		if (!args->args[i].isNumeric)
			free(args->args[i].string);
	free(args->args);
}

static void strfmt(char *dest, size_t destLen, char const *fmt, size_t nbArgs, struct StrFmtArg *args)
{
	size_t a = 0;
	size_t i = 0;

	while (i < destLen) {
		int c = *fmt++;

		if (c == '\0') {
			break;
		} else if (c != '%') {
			dest[i++] = c;
			continue;
		}

		c = *fmt++;

		if (c == '%') {
			dest[i++] = c;
			continue;
		}

		struct FormatSpec spec = fmt_NewSpec();

		while (c != '\0') {
			fmt_UseCharacter(&spec, c);
			if (fmt_IsFinished(&spec))
				break;
			c = *fmt++;
		}

		if (fmt_IsEmpty(&spec)) {
			error("STRFMT: Illegal '%%' at end of format string\n");
			dest[i++] = '%';
			break;
		} else if (!fmt_IsValid(&spec)) {
			error("STRFMT: Invalid format spec for argument %zu\n", a + 1);
			dest[i++] = '%';
			a++;
			continue;
		} else if (a >= nbArgs) {
			// Will warn after formatting is done.
			dest[i++] = '%';
			a++;
			continue;
		}

		struct StrFmtArg *arg = &args[a++];
		static char buf[MAXSTRLEN + 1];

		if (arg->isNumeric)
			fmt_PrintNumber(buf, sizeof(buf), &spec, arg->number);
		else
			fmt_PrintString(buf, sizeof(buf), &spec, arg->string);

		i += snprintf(&dest[i], destLen - i, "%s", buf);
	}

	if (a < nbArgs)
		error("STRFMT: %zu unformatted argument(s)\n", nbArgs - a);
	else if (a > nbArgs)
		error("STRFMT: Not enough arguments for format spec, got: %zu, need: %zu\n", nbArgs, a);

	if (i > destLen - 1) {
		warning(WARNING_LONG_STR, "STRFMT: String too long, got truncated\n");
		i = destLen - 1;
	}
	dest[i] = '\0';
}

static void compoundAssignment(const char *symName, enum RPNCommand op, int32_t constValue) {
	struct Expression oldExpr, constExpr, newExpr;
	int32_t newValue;

	rpn_Symbol(&oldExpr, symName);
	rpn_Number(&constExpr, constValue);
	rpn_BinaryOp(op, &newExpr, &oldExpr, &constExpr);
	newValue = rpn_GetConstVal(&newExpr);
	sym_AddVar(symName, newValue);
}

static void initDsArgList(struct DsArgList *args)
{
	args->nbArgs = 0;
	args->capacity = INITIAL_DS_ARG_SIZE;
	args->args = malloc(args->capacity * sizeof(*args->args));
	if (!args->args)
		fatalerror("Failed to allocate memory for ds arg list: %s\n",
			   strerror(errno));
}

static void appendDsArgList(struct DsArgList *args, const struct Expression *expr)
{
	if (args->nbArgs == args->capacity) {
		args->capacity = (args->capacity + 1) * 2;
		args->args = realloc(args->args, args->capacity * sizeof(*args->args));
		if (!args->args)
			fatalerror("realloc error while resizing ds arg list: %s\n",
				   strerror(errno));
	}
	args->args[args->nbArgs++] = *expr;
}

static void freeDsArgList(struct DsArgList *args)
{
	free(args->args);
}

static void failAssert(enum AssertionType type)
{
	switch (type) {
		case ASSERT_FATAL:
			fatalerror("Assertion failed\n");
		case ASSERT_ERROR:
			error("Assertion failed\n");
			break;
		case ASSERT_WARN:
			warning(WARNING_ASSERT, "Assertion failed\n");
			break;
	}
}

static void failAssertMsg(enum AssertionType type, char const *msg)
{
	switch (type) {
		case ASSERT_FATAL:
			fatalerror("Assertion failed: %s\n", msg);
		case ASSERT_ERROR:
			error("Assertion failed: %s\n", msg);
			break;
		case ASSERT_WARN:
			warning(WARNING_ASSERT, "Assertion failed: %s\n", msg);
			break;
	}
}

void yyerror(char const *str)
{
	error("%s\n", str);
}

// The CPU encodes instructions in a logical way, so most instructions actually follow patterns.
// These enums thus help with bit twiddling to compute opcodes
enum {
	REG_B = 0,
	REG_C,
	REG_D,
	REG_E,
	REG_H,
	REG_L,
	REG_HL_IND,
	REG_A
};

enum {
	REG_BC_IND = 0,
	REG_DE_IND,
	REG_HL_INDINC,
	REG_HL_INDDEC,
};

enum {
	REG_BC = 0,
	REG_DE = 1,
	REG_HL = 2,
	// LD/INC/ADD/DEC allow SP, PUSH/POP allow AF
	REG_SP = 3,
	REG_AF = 3
};

enum {
	CC_NZ = 0,
	CC_Z,
	CC_NC,
	CC_C
};


#line 534 "src/asm/parser.c"

# ifndef YY_CAST
#  ifdef __cplusplus
#   define YY_CAST(Type, Val) static_cast<Type> (Val)
#   define YY_REINTERPRET_CAST(Type, Val) reinterpret_cast<Type> (Val)
#  else
#   define YY_CAST(Type, Val) ((Type) (Val))
#   define YY_REINTERPRET_CAST(Type, Val) ((Type) (Val))
#  endif
# endif
# ifndef YY_NULLPTR
#  if defined __cplusplus
#   if 201103L <= __cplusplus
#    define YY_NULLPTR nullptr
#   else
#    define YY_NULLPTR 0
#   endif
#  else
#   define YY_NULLPTR ((void*)0)
#  endif
# endif

#include "parser.h"
/* Symbol kind.  */
enum yysymbol_kind_t
{
  YYSYMBOL_YYEMPTY = -2,
  YYSYMBOL_YYEOF = 0,                      /* "end of file"  */
  YYSYMBOL_YYerror = 1,                    /* error  */
  YYSYMBOL_YYUNDEF = 2,                    /* "invalid token"  */
  YYSYMBOL_T_NUMBER = 3,                   /* "number"  */
  YYSYMBOL_T_STRING = 4,                   /* "string"  */
  YYSYMBOL_T_PERIOD = 5,                   /* "."  */
  YYSYMBOL_T_COMMA = 6,                    /* ","  */
  YYSYMBOL_T_COLON = 7,                    /* ":"  */
  YYSYMBOL_T_LBRACK = 8,                   /* "["  */
  YYSYMBOL_T_RBRACK = 9,                   /* "]"  */
  YYSYMBOL_T_LPAREN = 10,                  /* "("  */
  YYSYMBOL_T_RPAREN = 11,                  /* ")"  */
  YYSYMBOL_T_NEWLINE = 12,                 /* "newline"  */
  YYSYMBOL_T_OP_LOGICNOT = 13,             /* "!"  */
  YYSYMBOL_T_OP_LOGICAND = 14,             /* "&&"  */
  YYSYMBOL_T_OP_LOGICOR = 15,              /* "||"  */
  YYSYMBOL_T_OP_LOGICGT = 16,              /* ">"  */
  YYSYMBOL_T_OP_LOGICLT = 17,              /* "<"  */
  YYSYMBOL_T_OP_LOGICGE = 18,              /* ">="  */
  YYSYMBOL_T_OP_LOGICLE = 19,              /* "<="  */
  YYSYMBOL_T_OP_LOGICNE = 20,              /* "!="  */
  YYSYMBOL_T_OP_LOGICEQU = 21,             /* "=="  */
  YYSYMBOL_T_OP_ADD = 22,                  /* "+"  */
  YYSYMBOL_T_OP_SUB = 23,                  /* "-"  */
  YYSYMBOL_T_OP_OR = 24,                   /* "|"  */
  YYSYMBOL_T_OP_XOR = 25,                  /* "^"  */
  YYSYMBOL_T_OP_AND = 26,                  /* "&"  */
  YYSYMBOL_T_OP_SHL = 27,                  /* "<<"  */
  YYSYMBOL_T_OP_SHR = 28,                  /* ">>"  */
  YYSYMBOL_T_OP_USHR = 29,                 /* ">>>"  */
  YYSYMBOL_T_OP_MUL = 30,                  /* "*"  */
  YYSYMBOL_T_OP_DIV = 31,                  /* "/"  */
  YYSYMBOL_T_OP_MOD = 32,                  /* "%"  */
  YYSYMBOL_T_OP_NOT = 33,                  /* "~"  */
  YYSYMBOL_NEG = 34,                       /* NEG  */
  YYSYMBOL_T_OP_EXP = 35,                  /* "**"  */
  YYSYMBOL_T_OP_DEF = 36,                  /* "DEF"  */
  YYSYMBOL_T_OP_BANK = 37,                 /* "BANK"  */
  YYSYMBOL_T_OP_ALIGN = 38,                /* "ALIGN"  */
  YYSYMBOL_T_OP_SIZEOF = 39,               /* "SIZEOF"  */
  YYSYMBOL_T_OP_STARTOF = 40,              /* "STARTOF"  */
  YYSYMBOL_T_OP_SIN = 41,                  /* "SIN"  */
  YYSYMBOL_T_OP_COS = 42,                  /* "COS"  */
  YYSYMBOL_T_OP_TAN = 43,                  /* "TAN"  */
  YYSYMBOL_T_OP_ASIN = 44,                 /* "ASIN"  */
  YYSYMBOL_T_OP_ACOS = 45,                 /* "ACOS"  */
  YYSYMBOL_T_OP_ATAN = 46,                 /* "ATAN"  */
  YYSYMBOL_T_OP_ATAN2 = 47,                /* "ATAN2"  */
  YYSYMBOL_T_OP_FDIV = 48,                 /* "FDIV"  */
  YYSYMBOL_T_OP_FMUL = 49,                 /* "FMUL"  */
  YYSYMBOL_T_OP_FMOD = 50,                 /* "FMOD"  */
  YYSYMBOL_T_OP_POW = 51,                  /* "POW"  */
  YYSYMBOL_T_OP_LOG = 52,                  /* "LOG"  */
  YYSYMBOL_T_OP_ROUND = 53,                /* "ROUND"  */
  YYSYMBOL_T_OP_CEIL = 54,                 /* "CEIL"  */
  YYSYMBOL_T_OP_FLOOR = 55,                /* "FLOOR"  */
  YYSYMBOL_T_OP_HIGH = 56,                 /* "HIGH"  */
  YYSYMBOL_T_OP_LOW = 57,                  /* "LOW"  */
  YYSYMBOL_T_OP_ISCONST = 58,              /* "ISCONST"  */
  YYSYMBOL_T_OP_STRCMP = 59,               /* "STRCMP"  */
  YYSYMBOL_T_OP_STRIN = 60,                /* "STRIN"  */
  YYSYMBOL_T_OP_STRRIN = 61,               /* "STRRIN"  */
  YYSYMBOL_T_OP_STRSUB = 62,               /* "STRSUB"  */
  YYSYMBOL_T_OP_STRLEN = 63,               /* "STRLEN"  */
  YYSYMBOL_T_OP_STRCAT = 64,               /* "STRCAT"  */
  YYSYMBOL_T_OP_STRUPR = 65,               /* "STRUPR"  */
  YYSYMBOL_T_OP_STRLWR = 66,               /* "STRLWR"  */
  YYSYMBOL_T_OP_STRRPL = 67,               /* "STRRPL"  */
  YYSYMBOL_T_OP_STRFMT = 68,               /* "STRFMT"  */
  YYSYMBOL_T_OP_CHARLEN = 69,              /* "CHARLEN"  */
  YYSYMBOL_T_OP_CHARSUB = 70,              /* "CHARSUB"  */
  YYSYMBOL_T_LABEL = 71,                   /* "label"  */
  YYSYMBOL_T_ID = 72,                      /* "identifier"  */
  YYSYMBOL_T_LOCAL_ID = 73,                /* "local identifier"  */
  YYSYMBOL_T_ANON = 74,                    /* "anonymous label"  */
  YYSYMBOL_T_POP_EQU = 75,                 /* "EQU"  */
  YYSYMBOL_T_POP_EQUAL = 76,               /* "="  */
  YYSYMBOL_T_POP_EQUS = 77,                /* "EQUS"  */
  YYSYMBOL_T_POP_ADDEQ = 78,               /* "+="  */
  YYSYMBOL_T_POP_SUBEQ = 79,               /* "-="  */
  YYSYMBOL_T_POP_MULEQ = 80,               /* "*="  */
  YYSYMBOL_T_POP_DIVEQ = 81,               /* "/="  */
  YYSYMBOL_T_POP_MODEQ = 82,               /* "%="  */
  YYSYMBOL_T_POP_OREQ = 83,                /* "|="  */
  YYSYMBOL_T_POP_XOREQ = 84,               /* "^="  */
  YYSYMBOL_T_POP_ANDEQ = 85,               /* "&="  */
  YYSYMBOL_T_POP_SHLEQ = 86,               /* "<<="  */
  YYSYMBOL_T_POP_SHREQ = 87,               /* ">>="  */
  YYSYMBOL_T_POP_INCLUDE = 88,             /* "INCLUDE"  */
  YYSYMBOL_T_POP_PRINT = 89,               /* "PRINT"  */
  YYSYMBOL_T_POP_PRINTLN = 90,             /* "PRINTLN"  */
  YYSYMBOL_T_POP_IF = 91,                  /* "IF"  */
  YYSYMBOL_T_POP_ELIF = 92,                /* "ELIF"  */
  YYSYMBOL_T_POP_ELSE = 93,                /* "ELSE"  */
  YYSYMBOL_T_POP_ENDC = 94,                /* "ENDC"  */
  YYSYMBOL_T_POP_EXPORT = 95,              /* "EXPORT"  */
  YYSYMBOL_T_POP_DB = 96,                  /* "DB"  */
  YYSYMBOL_T_POP_DS = 97,                  /* "DS"  */
  YYSYMBOL_T_POP_DW = 98,                  /* "DW"  */
  YYSYMBOL_T_POP_DL = 99,                  /* "DL"  */
  YYSYMBOL_T_POP_SECTION = 100,            /* "SECTION"  */
  YYSYMBOL_T_POP_FRAGMENT = 101,           /* "FRAGMENT"  */
  YYSYMBOL_T_POP_RB = 102,                 /* "RB"  */
  YYSYMBOL_T_POP_RW = 103,                 /* "RW"  */
  YYSYMBOL_T_POP_MACRO = 104,              /* "MACRO"  */
  YYSYMBOL_T_POP_ENDM = 105,               /* "ENDM"  */
  YYSYMBOL_T_POP_RSRESET = 106,            /* "RSRESET"  */
  YYSYMBOL_T_POP_RSSET = 107,              /* "RSSET"  */
  YYSYMBOL_T_POP_UNION = 108,              /* "UNION"  */
  YYSYMBOL_T_POP_NEXTU = 109,              /* "NEXTU"  */
  YYSYMBOL_T_POP_ENDU = 110,               /* "ENDU"  */
  YYSYMBOL_T_POP_INCBIN = 111,             /* "INCBIN"  */
  YYSYMBOL_T_POP_REPT = 112,               /* "REPT"  */
  YYSYMBOL_T_POP_FOR = 113,                /* "FOR"  */
  YYSYMBOL_T_POP_CHARMAP = 114,            /* "CHARMAP"  */
  YYSYMBOL_T_POP_NEWCHARMAP = 115,         /* "NEWCHARMAP"  */
  YYSYMBOL_T_POP_SETCHARMAP = 116,         /* "SETCHARMAP"  */
  YYSYMBOL_T_POP_PUSHC = 117,              /* "PUSHC"  */
  YYSYMBOL_T_POP_POPC = 118,               /* "POPC"  */
  YYSYMBOL_T_POP_SHIFT = 119,              /* "SHIFT"  */
  YYSYMBOL_T_POP_ENDR = 120,               /* "ENDR"  */
  YYSYMBOL_T_POP_BREAK = 121,              /* "BREAK"  */
  YYSYMBOL_T_POP_LOAD = 122,               /* "LOAD"  */
  YYSYMBOL_T_POP_ENDL = 123,               /* "ENDL"  */
  YYSYMBOL_T_POP_FAIL = 124,               /* "FAIL"  */
  YYSYMBOL_T_POP_WARN = 125,               /* "WARN"  */
  YYSYMBOL_T_POP_FATAL = 126,              /* "FATAL"  */
  YYSYMBOL_T_POP_ASSERT = 127,             /* "ASSERT"  */
  YYSYMBOL_T_POP_STATIC_ASSERT = 128,      /* "STATIC_ASSERT"  */
  YYSYMBOL_T_POP_PURGE = 129,              /* "PURGE"  */
  YYSYMBOL_T_POP_REDEF = 130,              /* "REDEF"  */
  YYSYMBOL_T_POP_POPS = 131,               /* "POPS"  */
  YYSYMBOL_T_POP_PUSHS = 132,              /* "PUSHS"  */
  YYSYMBOL_T_POP_POPO = 133,               /* "POPO"  */
  YYSYMBOL_T_POP_PUSHO = 134,              /* "PUSHO"  */
  YYSYMBOL_T_POP_OPT = 135,                /* "OPT"  */
  YYSYMBOL_T_SECT_ROM0 = 136,              /* "ROM0"  */
  YYSYMBOL_T_SECT_ROMX = 137,              /* "ROMX"  */
  YYSYMBOL_T_SECT_WRAM0 = 138,             /* "WRAM0"  */
  YYSYMBOL_T_SECT_WRAMX = 139,             /* "WRAMX"  */
  YYSYMBOL_T_SECT_HRAM = 140,              /* "HRAM"  */
  YYSYMBOL_T_SECT_VRAM = 141,              /* "VRAM"  */
  YYSYMBOL_T_SECT_SRAM = 142,              /* "SRAM"  */
  YYSYMBOL_T_SECT_OAM = 143,               /* "OAM"  */
  YYSYMBOL_T_Z80_ADC = 144,                /* "adc"  */
  YYSYMBOL_T_Z80_ADD = 145,                /* "add"  */
  YYSYMBOL_T_Z80_AND = 146,                /* "and"  */
  YYSYMBOL_T_Z80_BIT = 147,                /* "bit"  */
  YYSYMBOL_T_Z80_CALL = 148,               /* "call"  */
  YYSYMBOL_T_Z80_CCF = 149,                /* "ccf"  */
  YYSYMBOL_T_Z80_CP = 150,                 /* "cp"  */
  YYSYMBOL_T_Z80_CPL = 151,                /* "cpl"  */
  YYSYMBOL_T_Z80_DAA = 152,                /* "daa"  */
  YYSYMBOL_T_Z80_DEC = 153,                /* "dec"  */
  YYSYMBOL_T_Z80_DI = 154,                 /* "di"  */
  YYSYMBOL_T_Z80_EI = 155,                 /* "ei"  */
  YYSYMBOL_T_Z80_HALT = 156,               /* "halt"  */
  YYSYMBOL_T_Z80_INC = 157,                /* "inc"  */
  YYSYMBOL_T_Z80_JP = 158,                 /* "jp"  */
  YYSYMBOL_T_Z80_JR = 159,                 /* "jr"  */
  YYSYMBOL_T_Z80_LD = 160,                 /* "ld"  */
  YYSYMBOL_T_Z80_LDI = 161,                /* "ldi"  */
  YYSYMBOL_T_Z80_LDD = 162,                /* "ldd"  */
  YYSYMBOL_T_Z80_LDH = 163,                /* "ldh"  */
  YYSYMBOL_T_Z80_NOP = 164,                /* "nop"  */
  YYSYMBOL_T_Z80_OR = 165,                 /* "or"  */
  YYSYMBOL_T_Z80_POP = 166,                /* "pop"  */
  YYSYMBOL_T_Z80_PUSH = 167,               /* "push"  */
  YYSYMBOL_T_Z80_RES = 168,                /* "res"  */
  YYSYMBOL_T_Z80_RET = 169,                /* "ret"  */
  YYSYMBOL_T_Z80_RETI = 170,               /* "reti"  */
  YYSYMBOL_T_Z80_RST = 171,                /* "rst"  */
  YYSYMBOL_T_Z80_RL = 172,                 /* "rl"  */
  YYSYMBOL_T_Z80_RLA = 173,                /* "rla"  */
  YYSYMBOL_T_Z80_RLC = 174,                /* "rlc"  */
  YYSYMBOL_T_Z80_RLCA = 175,               /* "rlca"  */
  YYSYMBOL_T_Z80_RR = 176,                 /* "rr"  */
  YYSYMBOL_T_Z80_RRA = 177,                /* "rra"  */
  YYSYMBOL_T_Z80_RRC = 178,                /* "rrc"  */
  YYSYMBOL_T_Z80_RRCA = 179,               /* "rrca"  */
  YYSYMBOL_T_Z80_SBC = 180,                /* "sbc"  */
  YYSYMBOL_T_Z80_SCF = 181,                /* "scf"  */
  YYSYMBOL_T_Z80_SET = 182,                /* "set"  */
  YYSYMBOL_T_Z80_STOP = 183,               /* "stop"  */
  YYSYMBOL_T_Z80_SLA = 184,                /* "sla"  */
  YYSYMBOL_T_Z80_SRA = 185,                /* "sra"  */
  YYSYMBOL_T_Z80_SRL = 186,                /* "srl"  */
  YYSYMBOL_T_Z80_SUB = 187,                /* "sub"  */
  YYSYMBOL_T_Z80_SWAP = 188,               /* "swap"  */
  YYSYMBOL_T_Z80_XOR = 189,                /* "xor"  */
  YYSYMBOL_T_TOKEN_A = 190,                /* "a"  */
  YYSYMBOL_T_TOKEN_B = 191,                /* "b"  */
  YYSYMBOL_T_TOKEN_C = 192,                /* "c"  */
  YYSYMBOL_T_TOKEN_D = 193,                /* "d"  */
  YYSYMBOL_T_TOKEN_E = 194,                /* "e"  */
  YYSYMBOL_T_TOKEN_H = 195,                /* "h"  */
  YYSYMBOL_T_TOKEN_L = 196,                /* "l"  */
  YYSYMBOL_T_MODE_AF = 197,                /* "af"  */
  YYSYMBOL_T_MODE_BC = 198,                /* "bc"  */
  YYSYMBOL_T_MODE_DE = 199,                /* "de"  */
  YYSYMBOL_T_MODE_SP = 200,                /* "sp"  */
  YYSYMBOL_T_MODE_HL = 201,                /* "hl"  */
  YYSYMBOL_T_MODE_HL_DEC = 202,            /* "hld/hl-"  */
  YYSYMBOL_T_MODE_HL_INC = 203,            /* "hli/hl+"  */
  YYSYMBOL_T_CC_NZ = 204,                  /* "nz"  */
  YYSYMBOL_T_CC_Z = 205,                   /* "z"  */
  YYSYMBOL_T_CC_NC = 206,                  /* "nc"  */
  YYSYMBOL_T_EOB = 207,                    /* "end of buffer"  */
  YYSYMBOL_YYACCEPT = 208,                 /* $accept  */
  YYSYMBOL_asmfile = 209,                  /* asmfile  */
  YYSYMBOL_lines = 210,                    /* lines  */
  YYSYMBOL_endofline = 211,                /* endofline  */
  YYSYMBOL_opt_diff_mark = 212,            /* opt_diff_mark  */
  YYSYMBOL_line = 213,                     /* line  */
  YYSYMBOL_214_1 = 214,                    /* $@1  */
  YYSYMBOL_215_2 = 215,                    /* $@2  */
  YYSYMBOL_line_directive = 216,           /* line_directive  */
  YYSYMBOL_if = 217,                       /* if  */
  YYSYMBOL_elif = 218,                     /* elif  */
  YYSYMBOL_else = 219,                     /* else  */
  YYSYMBOL_plain_directive = 220,          /* plain_directive  */
  YYSYMBOL_endc = 221,                     /* endc  */
  YYSYMBOL_def_id = 222,                   /* def_id  */
  YYSYMBOL_223_3 = 223,                    /* $@3  */
  YYSYMBOL_redef_id = 224,                 /* redef_id  */
  YYSYMBOL_225_4 = 225,                    /* $@4  */
  YYSYMBOL_scoped_id = 226,                /* scoped_id  */
  YYSYMBOL_scoped_anon_id = 227,           /* scoped_anon_id  */
  YYSYMBOL_label = 228,                    /* label  */
  YYSYMBOL_macro = 229,                    /* macro  */
  YYSYMBOL_230_5 = 230,                    /* $@5  */
  YYSYMBOL_macroargs = 231,                /* macroargs  */
  YYSYMBOL_assignment_directive = 232,     /* assignment_directive  */
  YYSYMBOL_directive = 233,                /* directive  */
  YYSYMBOL_trailing_comma = 234,           /* trailing_comma  */
  YYSYMBOL_compoundeq = 235,               /* compoundeq  */
  YYSYMBOL_equ = 236,                      /* equ  */
  YYSYMBOL_assignment = 237,               /* assignment  */
  YYSYMBOL_equs = 238,                     /* equs  */
  YYSYMBOL_rb = 239,                       /* rb  */
  YYSYMBOL_rw = 240,                       /* rw  */
  YYSYMBOL_rl = 241,                       /* rl  */
  YYSYMBOL_align = 242,                    /* align  */
  YYSYMBOL_opt = 243,                      /* opt  */
  YYSYMBOL_244_6 = 244,                    /* $@6  */
  YYSYMBOL_opt_list = 245,                 /* opt_list  */
  YYSYMBOL_opt_list_entry = 246,           /* opt_list_entry  */
  YYSYMBOL_popo = 247,                     /* popo  */
  YYSYMBOL_pusho = 248,                    /* pusho  */
  YYSYMBOL_pops = 249,                     /* pops  */
  YYSYMBOL_pushs = 250,                    /* pushs  */
  YYSYMBOL_fail = 251,                     /* fail  */
  YYSYMBOL_warn = 252,                     /* warn  */
  YYSYMBOL_assert_type = 253,              /* assert_type  */
  YYSYMBOL_assert = 254,                   /* assert  */
  YYSYMBOL_shift = 255,                    /* shift  */
  YYSYMBOL_load = 256,                     /* load  */
  YYSYMBOL_rept = 257,                     /* rept  */
  YYSYMBOL_258_7 = 258,                    /* @7  */
  YYSYMBOL_for = 259,                      /* for  */
  YYSYMBOL_260_8 = 260,                    /* $@8  */
  YYSYMBOL_261_9 = 261,                    /* $@9  */
  YYSYMBOL_262_10 = 262,                   /* @10  */
  YYSYMBOL_for_args = 263,                 /* for_args  */
  YYSYMBOL_break = 264,                    /* break  */
  YYSYMBOL_macrodef = 265,                 /* macrodef  */
  YYSYMBOL_266_11 = 266,                   /* $@11  */
  YYSYMBOL_267_12 = 267,                   /* $@12  */
  YYSYMBOL_268_13 = 268,                   /* @13  */
  YYSYMBOL_269_14 = 269,                   /* @14  */
  YYSYMBOL_rsset = 270,                    /* rsset  */
  YYSYMBOL_rsreset = 271,                  /* rsreset  */
  YYSYMBOL_rs_uconst = 272,                /* rs_uconst  */
  YYSYMBOL_union = 273,                    /* union  */
  YYSYMBOL_nextu = 274,                    /* nextu  */
  YYSYMBOL_endu = 275,                     /* endu  */
  YYSYMBOL_ds = 276,                       /* ds  */
  YYSYMBOL_ds_args = 277,                  /* ds_args  */
  YYSYMBOL_db = 278,                       /* db  */
  YYSYMBOL_dw = 279,                       /* dw  */
  YYSYMBOL_dl = 280,                       /* dl  */
  YYSYMBOL_def_equ = 281,                  /* def_equ  */
  YYSYMBOL_redef_equ = 282,                /* redef_equ  */
  YYSYMBOL_def_set = 283,                  /* def_set  */
  YYSYMBOL_def_rb = 284,                   /* def_rb  */
  YYSYMBOL_def_rw = 285,                   /* def_rw  */
  YYSYMBOL_def_rl = 286,                   /* def_rl  */
  YYSYMBOL_def_equs = 287,                 /* def_equs  */
  YYSYMBOL_redef_equs = 288,               /* redef_equs  */
  YYSYMBOL_purge = 289,                    /* purge  */
  YYSYMBOL_290_15 = 290,                   /* $@15  */
  YYSYMBOL_purge_list = 291,               /* purge_list  */
  YYSYMBOL_purge_list_entry = 292,         /* purge_list_entry  */
  YYSYMBOL_export = 293,                   /* export  */
  YYSYMBOL_export_list = 294,              /* export_list  */
  YYSYMBOL_export_list_entry = 295,        /* export_list_entry  */
  YYSYMBOL_include = 296,                  /* include  */
  YYSYMBOL_incbin = 297,                   /* incbin  */
  YYSYMBOL_charmap = 298,                  /* charmap  */
  YYSYMBOL_newcharmap = 299,               /* newcharmap  */
  YYSYMBOL_setcharmap = 300,               /* setcharmap  */
  YYSYMBOL_pushc = 301,                    /* pushc  */
  YYSYMBOL_popc = 302,                     /* popc  */
  YYSYMBOL_print = 303,                    /* print  */
  YYSYMBOL_println = 304,                  /* println  */
  YYSYMBOL_print_exprs = 305,              /* print_exprs  */
  YYSYMBOL_print_expr = 306,               /* print_expr  */
  YYSYMBOL_const_3bit = 307,               /* const_3bit  */
  YYSYMBOL_constlist_8bit = 308,           /* constlist_8bit  */
  YYSYMBOL_constlist_8bit_entry = 309,     /* constlist_8bit_entry  */
  YYSYMBOL_constlist_16bit = 310,          /* constlist_16bit  */
  YYSYMBOL_constlist_16bit_entry = 311,    /* constlist_16bit_entry  */
  YYSYMBOL_constlist_32bit = 312,          /* constlist_32bit  */
  YYSYMBOL_constlist_32bit_entry = 313,    /* constlist_32bit_entry  */
  YYSYMBOL_reloc_8bit = 314,               /* reloc_8bit  */
  YYSYMBOL_reloc_8bit_no_str = 315,        /* reloc_8bit_no_str  */
  YYSYMBOL_reloc_8bit_offset = 316,        /* reloc_8bit_offset  */
  YYSYMBOL_reloc_16bit = 317,              /* reloc_16bit  */
  YYSYMBOL_reloc_16bit_no_str = 318,       /* reloc_16bit_no_str  */
  YYSYMBOL_relocexpr = 319,                /* relocexpr  */
  YYSYMBOL_relocexpr_no_str = 320,         /* relocexpr_no_str  */
  YYSYMBOL_321_16 = 321,                   /* $@16  */
  YYSYMBOL_uconst = 322,                   /* uconst  */
  YYSYMBOL_const = 323,                    /* const  */
  YYSYMBOL_const_no_str = 324,             /* const_no_str  */
  YYSYMBOL_const_8bit = 325,               /* const_8bit  */
  YYSYMBOL_opt_q_arg = 326,                /* opt_q_arg  */
  YYSYMBOL_string = 327,                   /* string  */
  YYSYMBOL_strcat_args = 328,              /* strcat_args  */
  YYSYMBOL_strfmt_args = 329,              /* strfmt_args  */
  YYSYMBOL_strfmt_va_args = 330,           /* strfmt_va_args  */
  YYSYMBOL_section = 331,                  /* section  */
  YYSYMBOL_sectmod = 332,                  /* sectmod  */
  YYSYMBOL_sectiontype = 333,              /* sectiontype  */
  YYSYMBOL_sectorg = 334,                  /* sectorg  */
  YYSYMBOL_sectattrs = 335,                /* sectattrs  */
  YYSYMBOL_cpu_command = 336,              /* cpu_command  */
  YYSYMBOL_z80_adc = 337,                  /* z80_adc  */
  YYSYMBOL_z80_add = 338,                  /* z80_add  */
  YYSYMBOL_z80_and = 339,                  /* z80_and  */
  YYSYMBOL_z80_bit = 340,                  /* z80_bit  */
  YYSYMBOL_z80_call = 341,                 /* z80_call  */
  YYSYMBOL_z80_ccf = 342,                  /* z80_ccf  */
  YYSYMBOL_z80_cp = 343,                   /* z80_cp  */
  YYSYMBOL_z80_cpl = 344,                  /* z80_cpl  */
  YYSYMBOL_z80_daa = 345,                  /* z80_daa  */
  YYSYMBOL_z80_dec = 346,                  /* z80_dec  */
  YYSYMBOL_z80_di = 347,                   /* z80_di  */
  YYSYMBOL_z80_ei = 348,                   /* z80_ei  */
  YYSYMBOL_z80_halt = 349,                 /* z80_halt  */
  YYSYMBOL_z80_inc = 350,                  /* z80_inc  */
  YYSYMBOL_z80_jp = 351,                   /* z80_jp  */
  YYSYMBOL_z80_jr = 352,                   /* z80_jr  */
  YYSYMBOL_z80_ldi = 353,                  /* z80_ldi  */
  YYSYMBOL_z80_ldd = 354,                  /* z80_ldd  */
  YYSYMBOL_z80_ldio = 355,                 /* z80_ldio  */
  YYSYMBOL_c_ind = 356,                    /* c_ind  */
  YYSYMBOL_z80_ld = 357,                   /* z80_ld  */
  YYSYMBOL_z80_ld_hl = 358,                /* z80_ld_hl  */
  YYSYMBOL_z80_ld_sp = 359,                /* z80_ld_sp  */
  YYSYMBOL_z80_ld_mem = 360,               /* z80_ld_mem  */
  YYSYMBOL_z80_ld_cind = 361,              /* z80_ld_cind  */
  YYSYMBOL_z80_ld_rr = 362,                /* z80_ld_rr  */
  YYSYMBOL_z80_ld_r = 363,                 /* z80_ld_r  */
  YYSYMBOL_z80_ld_a = 364,                 /* z80_ld_a  */
  YYSYMBOL_z80_ld_ss = 365,                /* z80_ld_ss  */
  YYSYMBOL_z80_nop = 366,                  /* z80_nop  */
  YYSYMBOL_z80_or = 367,                   /* z80_or  */
  YYSYMBOL_z80_pop = 368,                  /* z80_pop  */
  YYSYMBOL_z80_push = 369,                 /* z80_push  */
  YYSYMBOL_z80_res = 370,                  /* z80_res  */
  YYSYMBOL_z80_ret = 371,                  /* z80_ret  */
  YYSYMBOL_z80_reti = 372,                 /* z80_reti  */
  YYSYMBOL_z80_rl = 373,                   /* z80_rl  */
  YYSYMBOL_z80_rla = 374,                  /* z80_rla  */
  YYSYMBOL_z80_rlc = 375,                  /* z80_rlc  */
  YYSYMBOL_z80_rlca = 376,                 /* z80_rlca  */
  YYSYMBOL_z80_rr = 377,                   /* z80_rr  */
  YYSYMBOL_z80_rra = 378,                  /* z80_rra  */
  YYSYMBOL_z80_rrc = 379,                  /* z80_rrc  */
  YYSYMBOL_z80_rrca = 380,                 /* z80_rrca  */
  YYSYMBOL_z80_rst = 381,                  /* z80_rst  */
  YYSYMBOL_z80_sbc = 382,                  /* z80_sbc  */
  YYSYMBOL_z80_scf = 383,                  /* z80_scf  */
  YYSYMBOL_z80_set = 384,                  /* z80_set  */
  YYSYMBOL_z80_sla = 385,                  /* z80_sla  */
  YYSYMBOL_z80_sra = 386,                  /* z80_sra  */
  YYSYMBOL_z80_srl = 387,                  /* z80_srl  */
  YYSYMBOL_z80_stop = 388,                 /* z80_stop  */
  YYSYMBOL_z80_sub = 389,                  /* z80_sub  */
  YYSYMBOL_z80_swap = 390,                 /* z80_swap  */
  YYSYMBOL_z80_xor = 391,                  /* z80_xor  */
  YYSYMBOL_op_mem_ind = 392,               /* op_mem_ind  */
  YYSYMBOL_op_a_r = 393,                   /* op_a_r  */
  YYSYMBOL_op_a_n = 394,                   /* op_a_n  */
  YYSYMBOL_T_MODE_A = 395,                 /* T_MODE_A  */
  YYSYMBOL_T_MODE_B = 396,                 /* T_MODE_B  */
  YYSYMBOL_T_MODE_C = 397,                 /* T_MODE_C  */
  YYSYMBOL_T_MODE_D = 398,                 /* T_MODE_D  */
  YYSYMBOL_T_MODE_E = 399,                 /* T_MODE_E  */
  YYSYMBOL_T_MODE_H = 400,                 /* T_MODE_H  */
  YYSYMBOL_T_MODE_L = 401,                 /* T_MODE_L  */
  YYSYMBOL_ccode_expr = 402,               /* ccode_expr  */
  YYSYMBOL_ccode = 403,                    /* ccode  */
  YYSYMBOL_reg_r = 404,                    /* reg_r  */
  YYSYMBOL_reg_tt = 405,                   /* reg_tt  */
  YYSYMBOL_reg_ss = 406,                   /* reg_ss  */
  YYSYMBOL_reg_rr = 407,                   /* reg_rr  */
  YYSYMBOL_hl_ind_inc = 408,               /* hl_ind_inc  */
  YYSYMBOL_hl_ind_dec = 409                /* hl_ind_dec  */
};
typedef enum yysymbol_kind_t yysymbol_kind_t;




#ifdef short
# undef short
#endif

/* On compilers that do not define __PTRDIFF_MAX__ etc., make sure
   <limits.h> and (if available) <stdint.h> are included
   so that the code can choose integer types of a good width.  */

#ifndef __PTRDIFF_MAX__
# include <limits.h> /* INFRINGES ON USER NAME SPACE */
# if defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stdint.h> /* INFRINGES ON USER NAME SPACE */
#  define YY_STDINT_H
# endif
#endif

/* Narrow types that promote to a signed type and that can represent a
   signed or unsigned integer of at least N bits.  In tables they can
   save space and decrease cache pressure.  Promoting to a signed type
   helps avoid bugs in integer arithmetic.  */

#ifdef __INT_LEAST8_MAX__
typedef __INT_LEAST8_TYPE__ yytype_int8;
#elif defined YY_STDINT_H
typedef int_least8_t yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef __INT_LEAST16_MAX__
typedef __INT_LEAST16_TYPE__ yytype_int16;
#elif defined YY_STDINT_H
typedef int_least16_t yytype_int16;
#else
typedef short yytype_int16;
#endif

/* Work around bug in HP-UX 11.23, which defines these macros
   incorrectly for preprocessor constants.  This workaround can likely
   be removed in 2023, as HPE has promised support for HP-UX 11.23
   (aka HP-UX 11i v2) only through the end of 2022; see Table 2 of
   <https://h20195.www2.hpe.com/V2/getpdf.aspx/4AA4-7673ENW.pdf>.  */
#ifdef __hpux
# undef UINT_LEAST8_MAX
# undef UINT_LEAST16_MAX
# define UINT_LEAST8_MAX 255
# define UINT_LEAST16_MAX 65535
#endif

#if defined __UINT_LEAST8_MAX__ && __UINT_LEAST8_MAX__ <= __INT_MAX__
typedef __UINT_LEAST8_TYPE__ yytype_uint8;
#elif (!defined __UINT_LEAST8_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST8_MAX <= INT_MAX)
typedef uint_least8_t yytype_uint8;
#elif !defined __UINT_LEAST8_MAX__ && UCHAR_MAX <= INT_MAX
typedef unsigned char yytype_uint8;
#else
typedef short yytype_uint8;
#endif

#if defined __UINT_LEAST16_MAX__ && __UINT_LEAST16_MAX__ <= __INT_MAX__
typedef __UINT_LEAST16_TYPE__ yytype_uint16;
#elif (!defined __UINT_LEAST16_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST16_MAX <= INT_MAX)
typedef uint_least16_t yytype_uint16;
#elif !defined __UINT_LEAST16_MAX__ && USHRT_MAX <= INT_MAX
typedef unsigned short yytype_uint16;
#else
typedef int yytype_uint16;
#endif

#ifndef YYPTRDIFF_T
# if defined __PTRDIFF_TYPE__ && defined __PTRDIFF_MAX__
#  define YYPTRDIFF_T __PTRDIFF_TYPE__
#  define YYPTRDIFF_MAXIMUM __PTRDIFF_MAX__
# elif defined PTRDIFF_MAX
#  ifndef ptrdiff_t
#   include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  endif
#  define YYPTRDIFF_T ptrdiff_t
#  define YYPTRDIFF_MAXIMUM PTRDIFF_MAX
# else
#  define YYPTRDIFF_T long
#  define YYPTRDIFF_MAXIMUM LONG_MAX
# endif
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned
# endif
#endif

#define YYSIZE_MAXIMUM                                  \
  YY_CAST (YYPTRDIFF_T,                                 \
           (YYPTRDIFF_MAXIMUM < YY_CAST (YYSIZE_T, -1)  \
            ? YYPTRDIFF_MAXIMUM                         \
            : YY_CAST (YYSIZE_T, -1)))

#define YYSIZEOF(X) YY_CAST (YYPTRDIFF_T, sizeof (X))


/* Stored state numbers (used for stacks). */
typedef yytype_int16 yy_state_t;

/* State numbers in computations.  */
typedef int yy_state_fast_t;

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif


#ifndef YY_ATTRIBUTE_PURE
# if defined __GNUC__ && 2 < __GNUC__ + (96 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_PURE __attribute__ ((__pure__))
# else
#  define YY_ATTRIBUTE_PURE
# endif
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# if defined __GNUC__ && 2 < __GNUC__ + (7 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_UNUSED __attribute__ ((__unused__))
# else
#  define YY_ATTRIBUTE_UNUSED
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YY_USE(E) ((void) (E))
#else
# define YY_USE(E) /* empty */
#endif

/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
#if defined __GNUC__ && ! defined __ICC && 406 <= __GNUC__ * 100 + __GNUC_MINOR__
# if __GNUC__ * 100 + __GNUC_MINOR__ < 407
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")
# else
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")              \
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# endif
# define YY_IGNORE_MAYBE_UNINITIALIZED_END      \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif

#if defined __cplusplus && defined __GNUC__ && ! defined __ICC && 6 <= __GNUC__
# define YY_IGNORE_USELESS_CAST_BEGIN                          \
    _Pragma ("GCC diagnostic push")                            \
    _Pragma ("GCC diagnostic ignored \"-Wuseless-cast\"")
# define YY_IGNORE_USELESS_CAST_END            \
    _Pragma ("GCC diagnostic pop")
#endif
#ifndef YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_END
#endif


#define YY_ASSERT(E) ((void) (0 && (E)))

#if 1

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
# define YYCOPY_NEEDED 1
#endif /* 1 */

#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yy_state_t yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (YYSIZEOF (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (YYSIZEOF (yy_state_t) + YYSIZEOF (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYPTRDIFF_T yynewbytes;                                         \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * YYSIZEOF (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / YYSIZEOF (*yyptr);                        \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, YY_CAST (YYSIZE_T, (Count)) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYPTRDIFF_T yyi;                      \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  3
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   2338

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  208
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  202
/* YYNRULES -- Number of rules.  */
#define YYNRULES  517
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  931

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   462


/* YYTRANSLATE(TOKEN-NUM) -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, with out-of-bounds checking.  */
#define YYTRANSLATE(YYX) YY_CAST (yysymbol_kind_t, YYX)

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   689,   689,   692,   693,   696,   696,   699,   700,   703,
     708,   709,   711,   711,   719,   719,   737,   738,   739,   740,
     741,   742,   744,   745,   748,   758,   775,   791,   792,   793,
     794,   795,   798,   803,   803,   811,   811,   819,   819,   820,
     820,   822,   823,   826,   829,   832,   835,   839,   845,   845,
     853,   856,   862,   863,   864,   865,   866,   867,   870,   871,
     872,   873,   874,   875,   876,   877,   878,   879,   880,   881,
     882,   883,   884,   885,   886,   887,   888,   889,   890,   891,
     892,   893,   894,   895,   896,   897,   898,   899,   900,   901,
     902,   903,   904,   905,   906,   907,   908,   909,   912,   912,
     915,   916,   917,   918,   919,   920,   921,   922,   923,   924,
     927,   930,   931,   934,   937,   943,   949,   955,   961,   972,
     972,   978,   979,   982,   985,   988,   991,   994,   997,  1000,
    1003,  1004,  1005,  1006,  1009,  1020,  1031,  1035,  1041,  1042,
    1045,  1048,  1051,  1051,  1060,  1062,  1064,  1060,  1072,  1077,
    1082,  1089,  1095,  1097,  1099,  1095,  1106,  1106,  1116,  1119,
    1122,  1123,  1126,  1129,  1132,  1135,  1136,  1142,  1146,  1152,
    1153,  1156,  1157,  1160,  1161,  1164,  1167,  1170,  1171,  1172,
    1173,  1176,  1182,  1188,  1194,  1197,  1200,  1200,  1207,  1208,
    1211,  1214,  1217,  1218,  1221,  1224,  1231,  1236,  1241,  1248,
    1253,  1254,  1257,  1260,  1263,  1266,  1269,  1273,  1279,  1280,
    1283,  1284,  1287,  1299,  1300,  1303,  1306,  1315,  1316,  1319,
    1322,  1331,  1332,  1335,  1338,  1348,  1354,  1360,  1364,  1370,
    1376,  1383,  1384,  1395,  1396,  1397,  1400,  1403,  1406,  1409,
    1412,  1415,  1418,  1421,  1424,  1427,  1430,  1433,  1436,  1439,
    1442,  1445,  1448,  1451,  1454,  1457,  1460,  1461,  1462,  1463,
    1464,  1465,  1466,  1470,  1471,  1472,  1473,  1473,  1480,  1483,
    1486,  1489,  1492,  1495,  1498,  1501,  1504,  1507,  1510,  1513,
    1516,  1519,  1522,  1525,  1528,  1533,  1538,  1541,  1544,  1547,
    1554,  1557,  1560,  1563,  1564,  1574,  1575,  1581,  1587,  1593,
    1596,  1599,  1602,  1605,  1608,  1612,  1627,  1628,  1639,  1647,
    1650,  1657,  1666,  1671,  1672,  1673,  1676,  1677,  1678,  1679,
    1680,  1681,  1682,  1683,  1686,  1687,  1697,  1702,  1705,  1709,
    1716,  1717,  1718,  1719,  1720,  1721,  1722,  1723,  1724,  1725,
    1726,  1727,  1728,  1729,  1730,  1731,  1732,  1733,  1734,  1735,
    1736,  1737,  1738,  1739,  1740,  1741,  1742,  1743,  1744,  1745,
    1746,  1747,  1748,  1749,  1750,  1751,  1752,  1753,  1754,  1755,
    1756,  1757,  1758,  1759,  1760,  1761,  1764,  1768,  1771,  1775,
    1776,  1777,  1784,  1788,  1791,  1797,  1801,  1807,  1810,  1814,
    1817,  1820,  1823,  1824,  1827,  1830,  1833,  1845,  1846,  1849,
    1853,  1857,  1862,  1866,  1872,  1875,  1880,  1883,  1888,  1894,
    1900,  1903,  1908,  1909,  1915,  1916,  1917,  1918,  1919,  1920,
    1921,  1922,  1925,  1929,  1935,  1936,  1942,  1946,  1963,  1968,
    1973,  1977,  1985,  1991,  1997,  2019,  2023,  2031,  2034,  2038,
    2041,  2044,  2047,  2053,  2054,  2057,  2060,  2066,  2069,  2075,
    2078,  2084,  2087,  2093,  2096,  2106,  2110,  2113,  2116,  2122,
    2128,  2134,  2140,  2144,  2150,  2154,  2157,  2163,  2167,  2170,
    2173,  2174,  2177,  2178,  2181,  2182,  2185,  2186,  2189,  2190,
    2193,  2194,  2197,  2198,  2201,  2202,  2205,  2206,  2209,  2210,
    2215,  2216,  2217,  2218,  2221,  2222,  2223,  2224,  2225,  2226,
    2227,  2228,  2231,  2232,  2233,  2234,  2237,  2238,  2239,  2240,
    2243,  2244,  2245,  2246,  2249,  2250,  2253,  2254
};
#endif

/** Accessing symbol of state STATE.  */
#define YY_ACCESSING_SYMBOL(State) YY_CAST (yysymbol_kind_t, yystos[State])

#if 1
/* The user-facing name of the symbol whose (internal) number is
   YYSYMBOL.  No bounds checking.  */
static const char *yysymbol_name (yysymbol_kind_t yysymbol) YY_ATTRIBUTE_UNUSED;

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  static const char *const yy_sname[] =
  {
  "end of file", "error", "invalid token", "number", "string", ".", ",",
  ":", "[", "]", "(", ")", "newline", "!", "&&", "||", ">", "<", ">=",
  "<=", "!=", "==", "+", "-", "|", "^", "&", "<<", ">>", ">>>", "*", "/",
  "%", "~", "NEG", "**", "DEF", "BANK", "ALIGN", "SIZEOF", "STARTOF",
  "SIN", "COS", "TAN", "ASIN", "ACOS", "ATAN", "ATAN2", "FDIV", "FMUL",
  "FMOD", "POW", "LOG", "ROUND", "CEIL", "FLOOR", "HIGH", "LOW", "ISCONST",
  "STRCMP", "STRIN", "STRRIN", "STRSUB", "STRLEN", "STRCAT", "STRUPR",
  "STRLWR", "STRRPL", "STRFMT", "CHARLEN", "CHARSUB", "label",
  "identifier", "local identifier", "anonymous label", "EQU", "=", "EQUS",
  "+=", "-=", "*=", "/=", "%=", "|=", "^=", "&=", "<<=", ">>=", "INCLUDE",
  "PRINT", "PRINTLN", "IF", "ELIF", "ELSE", "ENDC", "EXPORT", "DB", "DS",
  "DW", "DL", "SECTION", "FRAGMENT", "RB", "RW", "MACRO", "ENDM",
  "RSRESET", "RSSET", "UNION", "NEXTU", "ENDU", "INCBIN", "REPT", "FOR",
  "CHARMAP", "NEWCHARMAP", "SETCHARMAP", "PUSHC", "POPC", "SHIFT", "ENDR",
  "BREAK", "LOAD", "ENDL", "FAIL", "WARN", "FATAL", "ASSERT",
  "STATIC_ASSERT", "PURGE", "REDEF", "POPS", "PUSHS", "POPO", "PUSHO",
  "OPT", "ROM0", "ROMX", "WRAM0", "WRAMX", "HRAM", "VRAM", "SRAM", "OAM",
  "adc", "add", "and", "bit", "call", "ccf", "cp", "cpl", "daa", "dec",
  "di", "ei", "halt", "inc", "jp", "jr", "ld", "ldi", "ldd", "ldh", "nop",
  "or", "pop", "push", "res", "ret", "reti", "rst", "rl", "rla", "rlc",
  "rlca", "rr", "rra", "rrc", "rrca", "sbc", "scf", "set", "stop", "sla",
  "sra", "srl", "sub", "swap", "xor", "a", "b", "c", "d", "e", "h", "l",
  "af", "bc", "de", "sp", "hl", "hld/hl-", "hli/hl+", "nz", "z", "nc",
  "end of buffer", "$accept", "asmfile", "lines", "endofline",
  "opt_diff_mark", "line", "$@1", "$@2", "line_directive", "if", "elif",
  "else", "plain_directive", "endc", "def_id", "$@3", "redef_id", "$@4",
  "scoped_id", "scoped_anon_id", "label", "macro", "$@5", "macroargs",
  "assignment_directive", "directive", "trailing_comma", "compoundeq",
  "equ", "assignment", "equs", "rb", "rw", "rl", "align", "opt", "$@6",
  "opt_list", "opt_list_entry", "popo", "pusho", "pops", "pushs", "fail",
  "warn", "assert_type", "assert", "shift", "load", "rept", "@7", "for",
  "$@8", "$@9", "@10", "for_args", "break", "macrodef", "$@11", "$@12",
  "@13", "@14", "rsset", "rsreset", "rs_uconst", "union", "nextu", "endu",
  "ds", "ds_args", "db", "dw", "dl", "def_equ", "redef_equ", "def_set",
  "def_rb", "def_rw", "def_rl", "def_equs", "redef_equs", "purge", "$@15",
  "purge_list", "purge_list_entry", "export", "export_list",
  "export_list_entry", "include", "incbin", "charmap", "newcharmap",
  "setcharmap", "pushc", "popc", "print", "println", "print_exprs",
  "print_expr", "const_3bit", "constlist_8bit", "constlist_8bit_entry",
  "constlist_16bit", "constlist_16bit_entry", "constlist_32bit",
  "constlist_32bit_entry", "reloc_8bit", "reloc_8bit_no_str",
  "reloc_8bit_offset", "reloc_16bit", "reloc_16bit_no_str", "relocexpr",
  "relocexpr_no_str", "$@16", "uconst", "const", "const_no_str",
  "const_8bit", "opt_q_arg", "string", "strcat_args", "strfmt_args",
  "strfmt_va_args", "section", "sectmod", "sectiontype", "sectorg",
  "sectattrs", "cpu_command", "z80_adc", "z80_add", "z80_and", "z80_bit",
  "z80_call", "z80_ccf", "z80_cp", "z80_cpl", "z80_daa", "z80_dec",
  "z80_di", "z80_ei", "z80_halt", "z80_inc", "z80_jp", "z80_jr", "z80_ldi",
  "z80_ldd", "z80_ldio", "c_ind", "z80_ld", "z80_ld_hl", "z80_ld_sp",
  "z80_ld_mem", "z80_ld_cind", "z80_ld_rr", "z80_ld_r", "z80_ld_a",
  "z80_ld_ss", "z80_nop", "z80_or", "z80_pop", "z80_push", "z80_res",
  "z80_ret", "z80_reti", "z80_rl", "z80_rla", "z80_rlc", "z80_rlca",
  "z80_rr", "z80_rra", "z80_rrc", "z80_rrca", "z80_rst", "z80_sbc",
  "z80_scf", "z80_set", "z80_sla", "z80_sra", "z80_srl", "z80_stop",
  "z80_sub", "z80_swap", "z80_xor", "op_mem_ind", "op_a_r", "op_a_n",
  "T_MODE_A", "T_MODE_B", "T_MODE_C", "T_MODE_D", "T_MODE_E", "T_MODE_H",
  "T_MODE_L", "ccode_expr", "ccode", "reg_r", "reg_tt", "reg_ss", "reg_rr",
  "hl_ind_inc", "hl_ind_dec", YY_NULLPTR
  };
  return yy_sname[yysymbol];
}
#endif

#define YYPACT_NINF (-475)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-312)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
    -475,    15,    77,  -475,  -475,  -475,  1072,  -475,  -475,   522,
      11,  2238,  2238,     8,  -475,  2238,  -475,  -475,  -475,  -475,
    -475,  -475,    22,  2051,  -475,  -475,  -475,  -475,  -475,  -475,
    -475,  -475,  -475,  -475,  -475,  -475,    22,  -475,     9,  2238,
    2238,   518,  -475,  -475,  -475,  -475,  -475,  -475,  -475,  -475,
    -475,  -475,  2238,  2238,  2238,  2238,    71,  -475,  -475,  2238,
    2238,  2238,  2238,  2238,  -475,    70,    80,   125,   131,   156,
     169,   170,   173,   177,   215,   225,   227,   229,   236,   240,
     245,   247,   248,   255,   257,   258,   264,   265,   273,   279,
     280,   281,   291,   296,   298,   300,   301,   302,  -475,  -475,
    -475,   318,  -475,  -475,  1392,  -475,   126,  -475,   214,  -475,
      36,   317,  -475,   221,  -475,  -475,  -475,  -475,  2238,  -475,
     518,  2238,  2238,  -475,   -16,  2238,  2238,  2238,  2238,   -19,
    -475,  2238,  -475,  -475,  -475,   518,   518,   259,   263,  -475,
    -475,  2238,    22,   -19,  -475,   518,   518,   138,   138,  -475,
    -475,  -475,  -475,  -475,  -475,  -475,  1838,  1331,  1838,  2238,
    1259,  -475,  1838,  -475,  -475,    86,  -475,  -475,  -475,    86,
     949,  1259,   104,    30,    58,    98,  -475,  1838,   -34,   -34,
    2238,    55,  -475,  2238,   216,  -475,   216,  -475,   216,  -475,
     216,  -475,  1838,  -475,  2238,  2238,   216,   216,   216,  1838,
     216,  1838,  -475,   371,   803,  -475,  -475,  -475,  -475,  -475,
    -475,  -475,  -475,  -475,  -475,  -475,  -475,  -475,  -475,  -475,
    -475,  -475,  -475,  -475,  -475,  -475,  -475,  -475,  -475,  -475,
    -475,  -475,  -475,  -475,  -475,  -475,  -475,  -475,  -475,  -475,
    -475,  -475,  -475,  -475,  -475,  -475,  -475,  -475,  -475,  -475,
    -475,  -475,  -475,  -475,  -475,  -475,  -475,  -475,  -475,  -475,
    -475,  -475,  -475,  -475,  -475,  -475,  -475,  -475,  -475,  -475,
    -475,  -475,  -475,  -475,  -475,  -475,  -475,  -475,  -475,  -475,
    -475,  -475,  -475,  -475,  -475,  -475,  -475,  -475,  -475,  -475,
    -475,  -475,  -475,  -475,  -475,  -475,  -475,  -475,  -475,  -475,
    -475,  -475,    22,  -475,   324,  -475,  -475,  -475,  -475,  -475,
    -475,  -475,  -475,  -475,   831,   303,   303,   303,   303,   327,
     326,   518,   518,  2238,  2238,  2238,  2238,  2238,  2238,  2238,
    2238,  2238,  2238,  2238,  2238,  2238,  2238,  2238,  2238,  2238,
    2238,   518,   518,   518,   518,   518,   685,   518,   518,   518,
     518,   518,   518,    72,  2238,  2238,  2238,  2238,  2238,  2238,
    2238,  2238,  2238,  2238,  2238,  2238,  2238,  2238,  2238,  2238,
    2238,  2238,  2238,  2238,  -475,  -475,  -475,  -475,  -475,   267,
     334,  -475,    22,   336,  -475,  1392,    20,  -475,    21,   336,
    -475,   337,  -475,   338,  -475,  -475,    31,    46,   339,   340,
    -475,  -475,    49,    59,   359,  -475,    63,    64,  -475,  -475,
     518,  -475,   360,   361,   362,  -475,  -475,  -475,   518,  -475,
    -475,   368,   369,   370,  2238,  2238,   -16,   305,   374,   178,
     376,   377,  -475,  -475,  -475,  -475,  -475,  -475,  -475,  -475,
    1392,  -475,  -475,   383,  -475,  -475,  -475,  -475,  -475,  -475,
    -475,   389,   395,  -475,  -475,  -475,  -475,   396,  -475,  1259,
    -475,  -475,  -475,  -475,  -475,  1392,   397,  -475,  -475,  -475,
     394,   403,  -475,  -475,  -475,  -475,  -475,  -475,  -475,  -475,
    -475,  -475,  -475,   399,  -475,   408,   149,   410,   411,   412,
     413,   414,   415,   417,   418,  -475,  -475,   224,   419,   421,
     232,   428,  2002,   430,   433,   436,  -475,  -475,  -475,  -475,
    -475,  -475,  -475,  -475,   437,    55,  -475,  -475,  -475,  -475,
    -475,  -475,  -475,  -475,   438,  -475,  -475,  -475,  -475,  -475,
    -475,  -475,  -475,  -475,  2238,  2238,   518,  2238,  2238,  2238,
    2238,  2238,  2238,   518,  2238,  -475,  -475,  -475,    72,   448,
     450,   451,   452,   458,   458,   458,   458,   458,   458,   459,
     463,   465,   470,   471,   472,   458,   458,   458,  1013,  1036,
    1098,   473,   477,   479,   480,   481,  -475,  -475,    13,   483,
     484,   485,  -475,   490,   492,   500,   497,  2297,  1736,   748,
     748,   748,   748,   748,   748,   762,   762,   146,   146,   146,
      92,    92,    92,   303,   303,   303,  -475,   416,    22,   504,
    -475,  2238,   486,  -475,  2238,  -475,  -475,   -16,  -475,  2238,
    -475,  2238,  2238,  -475,  2238,  -475,   506,  2238,  2238,   447,
     515,  -475,  -475,  -475,   799,   529,  -475,   531,  -475,  -475,
    -475,   374,  -475,   533,  1433,  1506,  1838,  2238,    43,   216,
    -475,  2238,    35,    53,  2238,  2238,   530,   536,   537,    65,
     538,   540,   541,  1567,   542,  2238,  2238,  1600,  1673,    41,
      40,  1926,    41,   543,   357,   563,   564,   566,    41,    41,
     570,   216,   216,  -475,  -475,  -475,  -475,  -475,  -475,  -475,
    -475,  -475,  -475,  -475,    22,   561,  -475,  -475,  -475,  -475,
    2238,   568,   576,   578,   579,   580,   581,  2238,  2238,  2238,
    2238,  2238,  2238,   582,   583,   584,  -475,  -475,  -475,   518,
     518,   518,  2238,  -475,   518,  -475,  -475,  -475,   518,   575,
    -475,  -475,  2238,  -475,  -475,  -475,  2238,  -475,  -475,  -475,
    -475,  -475,   606,  -475,  -475,  -475,   422,   607,  -475,  -475,
    -475,   422,   518,   518,   -16,  -475,  -475,  -475,   603,   605,
     608,   609,   610,   611,   612,  -475,  -475,  -475,  -475,  -475,
    -475,  -475,  -475,  1766,  -475,  -475,   618,   619,  -475,  -475,
    -475,  2002,  -475,  -475,  -475,  -475,  -475,   128,  -475,  -475,
    -475,  -475,  -475,  -475,  -475,  -475,  -475,  -475,   623,   431,
     627,   434,  -475,  -475,  -475,  -475,  -475,  -475,  -475,  -475,
    -475,  -475,  -475,  -475,  -475,  -475,  -475,   458,   458,   458,
     458,   458,   458,  -475,  -475,  -475,   628,   629,   630,    96,
    -475,   632,  2238,   631,    22,   634,   637,  2238,  -475,  -475,
    -475,  -475,  -475,  -475,  -475,  -475,  -475,   636,  2238,   636,
    -475,  -475,  -475,  -475,  -475,  -475,  -475,  -475,  -475,  -475,
    -475,  -475,   638,  2238,  2238,  -475,    41,   642,    41,   644,
     645,   646,   647,   648,   649,   651,  -475,  -475,  -475,  2238,
    -475,   518,  -475,    99,  -475,  -475,  -475,  2238,  -475,  2238,
    -475,  -475,  -475,  -475,  1392,  1392,  -475,  -475,  -475,  -475,
    -475,  -475,  -475,  -475,  -475,  -475,   652,   673,    22,   679,
     677,   681,   681,  -475,  -475,  -475,  2238,  -475,   118,  -475,
     680,   683,  2238,  2238,   684,    95,  -475,  2238,  -475,   686,
    -475
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int16 yydefact[] =
{
       3,     0,     7,     1,     8,     9,     0,    12,    42,     0,
      43,     0,     0,     0,   152,     0,   144,     4,    11,    21,
      22,    23,     0,    27,    31,    52,    53,    57,    54,    55,
      56,    17,    18,    19,    16,    20,     0,    14,    45,     0,
       0,     0,   100,   101,   102,   103,   104,   106,   105,   107,
     108,   109,   160,   160,   160,     0,    44,   234,   295,     0,
       0,     0,     0,     0,   266,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    37,    38,
      40,     0,    39,   233,   290,   231,     0,   232,     0,    26,
       0,     0,   289,     0,     5,     6,    10,    33,     0,    48,
       0,     0,   206,    32,     0,   169,     0,   171,   173,   313,
     159,     0,   162,   163,   164,     0,     0,     0,     0,   203,
     204,   138,     0,   313,   141,     0,     0,   130,   130,   186,
      35,   126,   127,   124,   125,   119,     0,     0,     0,     0,
       0,   387,     0,   390,   391,     0,   394,   395,   396,     0,
       0,     0,     0,     0,     0,     0,   437,     0,     0,     0,
       0,   443,   445,     0,     0,   447,     0,   449,     0,   451,
       0,   453,     0,   457,     0,   462,     0,     0,     0,     0,
       0,     0,    58,     0,     0,    29,    30,    97,    96,    94,
      95,    92,    93,    80,    81,    82,    79,    78,    68,    67,
      69,    70,    71,    65,    62,    63,    64,    83,    84,    85,
      86,    87,    88,    89,    90,    91,    61,    72,    73,    74,
      75,    76,    77,    59,    60,    66,    28,   330,   331,   332,
     333,   334,   335,   336,   337,   338,   339,   340,   341,   342,
     343,   344,   345,   348,   347,   349,   346,   418,   419,   414,
     415,   416,   420,   421,   417,   350,   351,   352,   353,   354,
     355,   356,   357,   358,   359,   360,   361,   362,   363,   364,
     365,   366,   367,   368,   369,   370,   371,   372,   373,   374,
     375,    13,     0,    47,     0,   110,   111,   113,   114,   161,
     115,   116,   112,    46,     0,   235,   256,   257,   258,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    24,    25,   153,   142,   145,     0,
     117,    50,     0,    98,   208,     0,   231,   210,   232,    98,
     194,    98,   192,    98,   213,   215,   231,   232,   165,    98,
     217,   219,   231,   232,    98,   221,   231,   232,   315,   314,
       0,   158,   196,     0,   200,   202,   139,   151,     0,   128,
     129,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   474,   476,   478,   480,   482,   484,   486,   472,
     225,   377,   376,   501,   494,   495,   496,   497,   498,   499,
     470,     0,     0,   379,   378,   383,   382,     0,   212,     0,
     493,   490,   491,   492,   385,   229,     0,   488,   389,   388,
       0,     0,   506,   507,   509,   508,   501,   392,   393,   397,
     398,   401,   399,     0,   402,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   512,   513,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   439,   438,   505,   502,
     503,   504,   440,   441,     0,     0,   444,   454,   446,   448,
     450,   452,   456,   455,     0,   463,   459,   460,   461,   465,
     464,   466,   468,   467,     0,     0,     0,   160,   160,   160,
       0,     0,     0,     0,     0,    15,   156,   288,     0,     0,
       0,     0,     0,   293,   293,   293,   293,   293,   293,     0,
       0,     0,     0,     0,     0,   293,   293,   293,     0,     0,
       0,     0,     0,     0,     0,     0,   299,   306,     0,     0,
       0,     0,   309,     0,     0,     0,     0,   237,   236,   239,
     240,   241,   242,   243,   238,   244,   245,   247,   246,   248,
     249,   250,   251,   252,   253,   254,   255,     0,     0,     0,
      34,     0,    49,   195,    99,   205,   207,    99,   191,    99,
     170,     0,    99,   172,    99,   174,     0,     0,     0,     0,
       0,   132,   131,   133,   134,   136,   190,    98,   188,    36,
     123,   120,   121,     0,     0,     0,     0,     0,     0,     0,
     489,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   229,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   175,   177,   184,   181,   182,   183,   179,
     176,   178,   185,   180,     0,     0,   262,   263,   264,   265,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   259,   260,   261,     0,
       0,     0,     0,   286,     0,   300,   301,   302,     0,   308,
     304,   287,     0,   305,   154,   143,     0,   118,    51,   209,
     193,   214,    98,   167,   218,   222,     0,   197,   292,   199,
     201,     0,     0,     0,    99,   187,   122,   500,     0,     0,
       0,     0,     0,     0,     0,   473,   471,   381,   380,   384,
     386,   400,   403,     0,   510,   511,     0,     0,   516,   514,
     469,     0,   412,   435,   436,   424,   425,     0,   423,   428,
     426,   427,   430,   432,   434,   431,   433,   429,     0,     0,
       0,     0,   411,   409,   410,   408,   442,   458,   157,   267,
     294,   276,   277,   278,   279,   280,   281,   293,   293,   293,
     293,   293,   293,   268,   269,   270,     0,     0,     0,     0,
     307,     0,     0,     0,     0,     0,   148,    99,   166,   319,
     318,   316,   321,   320,   317,   322,   323,   324,     0,   324,
     135,   137,   189,   475,   477,   481,   485,   479,   483,   487,
     515,   517,     0,     0,     0,   422,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   283,   284,   285,     0,
     297,     0,   310,   232,   298,   155,   146,     0,   168,     0,
     326,   198,   326,   413,   227,   228,   404,   405,   406,   407,
     282,   271,   272,   273,   274,   275,     0,     0,     0,   149,
       0,   312,   140,   296,   303,   147,     0,   325,     0,   150,
       0,     0,     0,     0,     0,     0,   329,     0,   327,     0,
     328
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -475,  -475,  -475,   -33,  -475,  -475,  -475,  -475,  -475,  -475,
    -475,  -475,  -475,  -475,  -475,  -475,  -475,  -475,  -117,  -303,
    -475,  -475,  -475,  -475,  -475,  -475,  -345,   -35,  -475,  -475,
    -475,  -475,  -475,  -475,  -475,  -475,  -475,  -475,    14,  -475,
    -475,  -475,  -475,  -475,  -475,   544,  -475,  -475,  -475,  -475,
    -475,  -475,  -475,  -475,  -475,  -475,  -475,  -475,  -475,  -475,
    -475,  -475,  -475,  -475,   -40,  -475,  -475,  -475,  -475,  -475,
    -475,  -475,  -475,  -475,  -475,  -475,  -475,  -475,  -475,  -475,
    -475,  -475,  -475,  -475,   -51,  -475,  -475,    87,  -475,  -475,
    -475,  -475,  -475,  -475,  -475,  -475,  -475,   585,    94,   -96,
    -475,    90,  -475,    84,  -475,    89,  -132,  -475,  -475,   -98,
    -475,   310,   -86,  -475,   -15,   -10,  -122,  -475,  -437,    12,
    -475,  -475,  -475,  -475,   572,   -32,  -138,  -175,  -475,  -475,
    -475,  -475,  -475,  -475,  -475,  -475,  -475,  -475,  -475,  -475,
    -475,  -475,  -475,  -475,  -475,  -475,  -475,  -475,  -167,  -475,
    -475,  -475,  -475,  -475,  -475,  -475,  -475,  -475,  -475,  -475,
    -475,  -475,  -475,  -475,  -475,  -475,  -475,  -475,  -475,  -475,
    -475,  -475,  -475,  -475,  -475,  -475,  -475,  -475,  -475,  -475,
    -475,  -475,  -475,  -475,  -164,   283,   453,  -152,  -475,  -474,
    -475,  -475,  -475,  -475,   -45,  -475,  -105,   539,  -160,    50,
    -475,  -475
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
       0,     1,     2,   116,     6,    17,    36,   302,    18,    19,
      20,    21,    22,   202,   203,   379,   204,   427,   102,   103,
      23,   205,   381,   612,    24,   206,   615,    55,    25,    26,
      27,    28,    29,    30,   207,   208,   428,   641,   642,   209,
     210,   211,   212,   213,   214,   424,   215,   216,   217,    31,
     608,    32,   113,   609,   908,   835,    33,    34,   110,   607,
     834,   694,   218,   219,   308,   220,   221,   222,   223,   742,
     224,   225,   226,   227,   228,   229,   230,   231,   232,   233,
     234,   235,   426,   637,   638,   236,   391,   392,    35,   237,
     238,   239,   240,   241,   242,   243,   244,   383,   384,   457,
     393,   394,   399,   400,   404,   405,   439,   395,   865,   662,
     401,   104,   105,   319,   309,   112,   387,   749,   701,   107,
     578,   583,   729,   245,   410,   847,   890,   911,   246,   247,
     248,   249,   250,   251,   252,   253,   254,   255,   256,   257,
     258,   259,   260,   261,   262,   263,   264,   265,   491,   266,
     267,   268,   269,   270,   271,   272,   273,   274,   275,   276,
     277,   278,   279,   280,   281,   282,   283,   284,   285,   286,
     287,   288,   289,   290,   291,   292,   293,   294,   295,   296,
     297,   298,   299,   300,   492,   441,   442,   476,   444,   445,
     446,   447,   448,   449,   650,   467,   450,   512,   478,   494,
     495,   496
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
     111,   106,   108,   301,   443,   443,   443,   390,   503,   480,
     443,   504,   664,   310,   311,     3,   303,   549,    56,   724,
     109,   499,   501,   505,   725,   443,  -291,  -211,   664,   305,
     306,  -291,  -291,  -211,   114,   386,   386,  -226,   497,   396,
     443,   402,   406,  -226,   616,   312,   618,   443,   620,   443,
     586,   517,  -216,   307,   623,  -230,    98,    99,  -216,   625,
     477,  -230,   464,   525,   479,  -220,   500,   493,   515,  -223,
    -224,  -220,   482,   484,   757,  -223,  -224,    -2,   313,   518,
     320,   519,   408,   520,   514,   521,   498,   776,   777,   409,
     321,   526,   527,   528,   429,   531,   498,   498,   524,     4,
       5,   927,   879,   380,   928,  -311,   502,   880,   376,   417,
    -311,   398,   486,   304,   498,   466,   411,   702,   703,   704,
     705,   706,   370,   371,   372,   483,   485,   373,   713,   714,
     715,   416,   382,   388,   388,   322,   516,   397,   374,   403,
     407,   323,   470,   471,    98,    99,   100,   412,   413,   458,
     863,   864,    57,    58,   498,   920,   921,   419,   420,    59,
     470,   471,    60,   508,   509,   510,   324,   511,   540,   544,
     458,    61,    62,   367,   368,   369,   370,   371,   372,   325,
     326,   373,    63,   327,   458,    64,    65,   328,    66,    67,
      68,    69,    70,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,    83,   656,    85,    86,    87,
      88,    89,    90,    91,    92,    93,    94,    95,    96,    97,
     432,    98,    99,   100,   429,   329,   375,  -291,  -211,   115,
     432,   432,   758,   759,   760,   330,   761,   331,  -226,   332,
     790,   472,   473,   474,   475,   695,   333,   460,   432,   101,
     334,   762,   763,  -216,   764,   335,  -230,   336,   337,   461,
     462,   463,   421,   422,   423,   338,  -220,   339,   340,   545,
    -223,  -224,   470,   471,   341,   342,   432,   433,   434,   435,
     436,   437,   438,   343,   472,   473,   474,   475,   432,   344,
     345,   346,   755,   378,   432,   433,   434,   435,   436,   437,
     438,   347,   487,   488,   489,   490,   348,   862,   349,   636,
     350,   351,   352,   553,   554,   555,   556,   557,   558,   559,
     560,   561,   562,   563,   564,   565,   566,   567,   353,   377,
      58,   414,   550,   551,   552,   415,   546,   548,   373,   610,
     611,   434,   614,   617,   619,   621,   622,   657,   658,   613,
     659,   660,   661,   571,   572,   573,   574,   575,   577,   579,
     580,   581,   582,   584,   585,   624,   627,   628,   629,   314,
     315,   316,   317,   318,   631,   632,   633,   639,   640,   643,
     870,   871,   872,   873,   874,   875,   644,   645,    89,   646,
      91,    92,    93,    94,    95,   647,    97,   838,    98,    99,
     100,   648,   649,   651,   652,   654,   432,   433,   434,   435,
     436,   437,   438,   653,   655,   635,   665,   666,   667,   668,
     669,   670,   626,   671,   672,   673,   101,   675,   734,   674,
     630,   385,   385,   676,   677,   385,   678,   385,   385,   679,
     453,   455,   680,   681,   682,   468,   534,   535,   536,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,   696,
     506,   697,   698,   699,   700,   707,   440,   440,   440,   708,
     465,   709,   440,   537,   538,   522,   710,   711,   712,   719,
     465,   465,   529,   720,   532,   721,   722,   440,   768,   743,
     738,   728,   723,   440,   726,   727,   748,   686,   687,   688,
     390,   730,   440,   731,   793,   440,   732,   794,   733,   440,
     736,   440,   746,   804,   765,   767,   805,   789,   791,   750,
     797,   751,    58,    37,   683,   684,   802,   803,   386,    38,
     689,   690,   691,   396,   693,   753,   402,   754,   406,   792,
     773,   766,   757,   539,   769,   774,   775,   778,   685,   779,
     780,   782,   798,   770,   758,   692,   771,   772,   839,   840,
     841,   842,   843,   844,   845,   846,   795,   783,   784,   786,
     788,   799,   809,   800,   801,   735,   806,   807,   502,   811,
      89,   832,    91,    92,    93,    94,    95,   812,    97,   813,
     814,   815,   816,   823,   824,   825,   737,    39,    40,    41,
      42,    43,    44,    45,    46,    47,    48,    49,    50,    51,
     454,   456,   837,   848,   853,   469,   854,   747,   101,   855,
     856,   857,   858,   859,    52,    53,   388,   860,   861,   866,
     507,   397,   867,   868,   403,   869,   407,   636,   881,   876,
     877,   878,   884,   887,   889,   523,   886,   893,   568,   569,
     570,   897,   530,   899,   533,   756,   900,   901,   902,   903,
     904,   808,   905,   913,   587,   588,   589,   590,   591,   592,
     593,   594,   595,   596,   597,   598,   599,   600,   601,   602,
     603,   604,   605,   606,   914,   916,   917,   918,   922,    58,
     810,   923,   425,   926,    54,   930,   576,   817,   818,   819,
     820,   821,   822,   852,   740,   888,   744,   389,   739,   741,
     882,   892,   829,   745,   896,   418,   898,   912,   513,   849,
       0,   796,   833,     0,     0,     0,   836,     0,     0,     0,
       0,   826,   827,   828,   634,     0,   830,     0,     0,     0,
     831,     0,     0,     0,     0,     0,   386,    89,     0,    91,
      92,    93,    94,    95,     0,    97,     0,     0,     0,     0,
       0,     0,     0,     0,   850,   851,     0,     0,     0,   315,
     362,   363,   364,   365,   366,   367,   368,   369,   370,   371,
     372,     0,     0,   373,     0,   101,   364,   365,   366,   367,
     368,   369,   370,   371,   372,     0,   663,   373,     0,     0,
       0,   885,     0,     0,     0,   752,     0,     0,     0,     0,
       0,     0,   663,   354,   355,   356,   357,   358,   359,   360,
     361,   362,   363,   364,   365,   366,   367,   368,   369,   370,
     371,   372,     0,     0,   373,     0,     0,     0,   891,     0,
       0,     0,   547,     0,   883,   354,   355,   356,   357,   358,
     359,   360,   361,   362,   363,   364,   365,   366,   367,   368,
     369,   370,   371,   372,   906,     0,   373,     0,     0,     0,
       0,     0,     0,     0,   910,   915,     0,   909,   541,   542,
     543,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,     0,     0,   907,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   919,   924,   925,     0,
       0,     0,   929,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   385,     0,     0,     0,     0,   385,
       0,   440,   385,     0,   385,     0,     0,     0,   440,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    57,    58,   568,   569,   440,   440,     0,    59,
       0,   465,   459,     0,   465,   465,     0,     0,     0,     0,
       0,    61,    62,     0,     0,   465,   465,   465,   465,     0,
       0,   440,    63,     0,     0,    64,    65,     0,    66,    67,
      68,    69,    70,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    92,    93,    94,    95,    96,    97,
       0,    98,    99,   100,   716,     0,     0,   354,   355,   356,
     357,   358,   359,   360,   361,   362,   363,   364,   365,   366,
     367,   368,   369,   370,   371,   372,     0,   717,   373,   101,
     354,   355,   356,   357,   358,   359,   360,   361,   362,   363,
     364,   365,   366,   367,   368,   369,   370,   371,   372,     0,
       0,   373,     0,     7,     0,     0,     0,     0,     0,     8,
       0,     0,     0,   569,   -41,     0,     0,     0,     0,     0,
       0,   595,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   -41,   718,
     -41,     0,   354,   355,   356,   357,   358,   359,   360,   361,
     362,   363,   364,   365,   366,   367,   368,   369,   370,   371,
     372,     0,     0,   373,     0,     0,     0,     0,     0,     0,
       0,   460,   385,     9,   -41,    10,     0,   440,     0,     0,
     481,     0,     0,   461,   462,   463,     0,     0,     0,     0,
     -41,   -41,   -41,    11,    12,    13,   -41,   -41,   -41,   -41,
     -41,   -41,   -41,   894,   895,     0,    14,     0,   -41,   -41,
     -41,   -41,   -41,   -41,    15,    16,   -41,   -41,   -41,   -41,
     -41,   -41,     0,   -41,   -41,   -41,   -41,   -41,     0,   -41,
     -41,   -41,   -41,   -41,   -41,   -41,   -41,   -41,     0,     0,
       0,     0,     0,     0,     0,     0,   -41,   -41,   -41,   -41,
     -41,   -41,   -41,   -41,   -41,   -41,   -41,   -41,   -41,   -41,
     -41,   -41,   -41,   -41,   -41,   -41,   -41,   -41,   -41,   -41,
     -41,   -41,   -41,   -41,   -41,   -41,   -41,   -41,   -41,   -41,
     -41,   -41,   -41,   -41,   -41,   -41,   -41,   -41,   -41,   -41,
     -41,   -41,    57,    58,     0,     0,     0,     0,     0,    59,
       0,     0,   459,     0,     0,     0,     0,     0,     0,   -41,
       0,    61,    62,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    63,     0,     0,    64,    65,     0,    66,    67,
      68,    69,    70,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    92,    93,    94,    95,    96,    97,
       0,    98,    99,   100,    57,    58,     0,     0,     0,   429,
       0,    59,     0,     0,    60,     0,     0,     0,     0,     0,
       0,     0,     0,    61,    62,     0,     0,     0,     0,   101,
       0,     0,     0,     0,    63,     0,     0,    64,    65,     0,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,   430,   431,    85,
      86,    87,    88,    89,    90,    91,    92,    93,    94,    95,
      96,    97,     0,    98,    99,   100,   354,   355,   356,   357,
     358,   359,   360,   361,   362,   363,   364,   365,   366,   367,
     368,   369,   370,   371,   372,     0,     0,   373,     0,     0,
       0,   101,     0,     0,     0,     0,    57,    58,     0,     0,
       0,     0,     0,    59,     0,     0,    60,     0,     0,     0,
       0,   460,     0,     0,     0,    61,    62,     0,     0,     0,
       0,     0,     0,   461,   462,   463,    63,     0,     0,    64,
      65,     0,    66,    67,    68,    69,    70,    71,    72,    73,
      74,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,    88,    89,    90,    91,    92,    93,
      94,    95,    96,    97,     0,    98,    99,   100,     0,    57,
      58,     0,     0,     0,     0,     0,    59,     0,     0,    60,
       0,   432,   433,   434,   435,   436,   437,   438,    61,    62,
       0,   451,   452,   101,     0,     0,     0,     0,     0,    63,
       0,     0,    64,    65,     0,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    92,    93,    94,    95,    96,    97,     0,    98,    99,
     100,   354,   355,   356,   357,   358,   359,   360,   361,   781,
     363,   364,   365,   366,   367,   368,   369,   370,   371,   372,
       0,     0,   373,    57,    58,     0,   101,     0,     0,     0,
      59,     0,     0,    60,     0,     0,     0,     0,     0,     0,
       0,     0,    61,    62,     0,     0,     0,     0,     0,     0,
     758,   759,   760,    63,   761,     0,    64,    65,     0,    66,
      67,    68,    69,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    92,    93,    94,    95,    96,
      97,     0,    98,    99,   100,     0,    57,    58,     0,     0,
       0,     0,     0,    59,     0,     0,    60,     0,     0,     0,
       0,     0,     0,     0,     0,    61,    62,     0,     0,     0,
     101,     0,     0,     0,   762,   763,    63,   764,     0,    64,
      65,     0,    66,    67,    68,    69,    70,    71,    72,    73,
      74,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,    88,    89,    90,    91,    92,    93,
      94,    95,    96,    97,     0,    98,    99,   100,     0,     0,
     354,     0,   356,   357,   358,   359,   360,   361,   362,   363,
     364,   365,   366,   367,   368,   369,   370,   371,   372,    57,
      58,   373,     0,   101,     0,     0,    59,     0,     0,    60,
       0,     0,     0,     0,     0,     0,     0,     0,    61,    62,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    63,
       0,   785,    64,    65,     0,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    92,    93,    94,    95,    96,    97,     0,    98,    99,
     100,    57,    58,     0,     0,     0,   429,     0,    59,     0,
       0,    60,     0,     0,     0,     0,     0,     0,     0,     0,
      61,    62,     0,     0,     0,     0,   101,     0,     0,     0,
       0,    63,     0,   787,    64,    65,     0,    66,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,   430,   431,    85,    86,    87,    88,
      89,    90,    91,    92,    93,    94,    95,    96,    97,     0,
      98,    99,   100,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    57,
      58,     0,     0,     0,   486,     0,    59,     0,   101,    60,
       0,     0,     0,     0,     0,     0,     0,     0,    61,    62,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    63,
       0,     0,    64,    65,   762,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,   430,   431,    85,    86,    87,    88,    89,    90,
      91,    92,    93,    94,    95,    96,    97,     0,    98,    99,
     100,     0,     0,     0,     0,    57,    58,     0,     0,     0,
       0,     0,    59,     0,     0,    60,     0,     0,     0,     0,
       0,     0,     0,     0,    61,    62,   101,     0,   432,   433,
     434,   435,   436,   437,   438,    63,     0,     0,    64,    65,
       0,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,   656,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97,     0,    98,    99,   100,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   117,     0,   118,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   101,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   432,   433,   434,   435,
     436,   437,   438,   119,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   120,
     121,   122,     0,     0,     0,   123,   124,   125,   126,   127,
     128,   129,     0,     0,     0,     0,     0,   130,   131,   132,
     133,   134,   135,     0,     0,   136,   137,   138,   139,   140,
     141,     0,   142,   143,   144,   145,   146,     0,   147,   148,
     149,   150,   151,   152,   153,   154,   155,     0,     0,     0,
       0,     0,     0,     0,   434,   156,   157,   158,   159,   160,
     161,   162,   163,   164,   165,   166,   167,   168,   169,   170,
     171,   172,   173,   174,   175,   176,   177,   178,   179,   180,
     181,   182,   183,   184,   185,   186,   187,   188,   189,   190,
     191,   192,   193,   194,   195,   196,   197,   198,   199,   200,
     201,    57,    58,     0,     0,     0,     0,     0,    59,     0,
       0,    60,     0,     0,     0,     0,     0,     0,     0,     0,
      61,    62,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    63,     0,     0,    64,    65,     0,    66,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,    86,    87,    88,
      89,    90,    91,    92,    93,    94,    95,    96,    97,     0,
      98,    99,   100,   356,   357,   358,   359,   360,   361,   362,
     363,   364,   365,   366,   367,   368,   369,   370,   371,   372,
       0,     0,   373,     0,     0,     0,     0,     0,   101
};

static const yytype_int16 yycheck[] =
{
      15,    11,    12,    36,   156,   157,   158,   124,   175,   169,
     162,   175,   486,    53,    54,     0,     7,   320,     7,     6,
      12,   173,   174,   175,    11,   177,     6,     6,   502,    39,
      40,    11,    12,    12,    12,   121,   122,     6,     8,   125,
     192,   127,   128,    12,   389,    55,   391,   199,   393,   201,
     353,   183,     6,    41,   399,     6,    72,    73,    12,   404,
     165,    12,   160,   195,   169,     6,     8,   172,    13,     6,
       6,    12,   170,   171,     9,    12,    12,     0,     7,   184,
      10,   186,   101,   188,   180,   190,    56,    22,    23,   108,
      10,   196,   197,   198,     8,   200,    56,    56,   194,    22,
      23,     6,     6,   118,     9,     6,     8,    11,    72,   142,
      11,   126,     8,   104,    56,   160,   131,   554,   555,   556,
     557,   558,    30,    31,    32,   170,   171,    35,   565,   566,
     567,   141,   120,   121,   122,    10,   181,   125,    12,   127,
     128,    10,    56,    57,    72,    73,    74,   135,   136,   159,
      22,    23,     3,     4,    56,    37,    38,   145,   146,    10,
      56,    57,    13,   197,   198,   199,    10,   201,   203,   204,
     180,    22,    23,    27,    28,    29,    30,    31,    32,    10,
      10,    35,    33,    10,   194,    36,    37,    10,    39,    40,
      41,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
     190,    72,    73,    74,     8,    10,    12,   207,   207,   207,
     190,   190,   197,   198,   199,    10,   201,    10,   207,    10,
     200,   198,   199,   200,   201,   548,    10,   192,   190,   100,
      10,   198,   199,   207,   201,    10,   207,    10,    10,   204,
     205,   206,   124,   125,   126,    10,   207,    10,    10,   302,
     207,   207,    56,    57,    10,    10,   190,   191,   192,   193,
     194,   195,   196,    10,   198,   199,   200,   201,   190,    10,
      10,    10,   637,    72,   190,   191,   192,   193,   194,   195,
     196,    10,   198,   199,   200,   201,    10,   781,    10,   426,
      10,    10,    10,   323,   324,   325,   326,   327,   328,   329,
     330,   331,   332,   333,   334,   335,   336,   337,    10,    12,
       4,    72,   320,   321,   322,    72,    12,    10,    35,    72,
       6,   192,     6,     6,     6,     6,     6,   198,   199,   382,
     201,   202,   203,   341,   342,   343,   344,   345,   346,   347,
     348,   349,   350,   351,   352,     6,     6,     6,     6,    59,
      60,    61,    62,    63,     6,     6,     6,    72,     4,   201,
     817,   818,   819,   820,   821,   822,    10,    10,    62,     6,
      64,    65,    66,    67,    68,     6,    70,   742,    72,    73,
      74,     6,     6,     6,    10,     6,   190,   191,   192,   193,
     194,   195,   196,    10,     6,   425,     6,     6,     6,     6,
       6,     6,   410,     6,     6,   201,   100,     6,    12,    10,
     418,   121,   122,   201,     6,   125,     6,   127,   128,     6,
     157,   158,     6,     6,     6,   162,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,    86,    87,    11,
     177,    11,    11,    11,     6,     6,   156,   157,   158,     6,
     160,     6,   162,   102,   103,   192,     6,     6,     6,     6,
     170,   171,   199,     6,   201,     6,     6,   177,   648,   621,
       4,     6,    11,   183,    11,    11,   628,   537,   538,   539,
     617,    11,   192,    11,   671,   195,     6,   671,    11,   199,
       6,   201,     6,   680,   646,   647,   680,   669,   670,    72,
     672,     6,     4,     1,   534,   535,   678,   679,   614,     7,
     540,   541,   542,   619,   544,     6,   622,     6,   624,   671,
      10,   646,     9,   172,   649,     9,     9,     9,   536,     9,
       9,     9,     9,   651,   197,   543,   654,   655,   136,   137,
     138,   139,   140,   141,   142,   143,   671,   665,   666,   667,
     668,     8,    11,     9,     8,   608,   681,   682,     8,    11,
      62,     6,    64,    65,    66,    67,    68,    11,    70,    11,
      11,    11,    11,    11,    11,    11,   611,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
     157,   158,     6,     6,    11,   162,    11,   627,   100,    11,
      11,    11,    11,    11,   102,   103,   614,     9,     9,     6,
     177,   619,   201,     6,   622,   201,   624,   754,     6,    11,
      11,    11,    11,     6,     8,   192,    12,     9,   338,   339,
     340,     9,   199,     9,   201,   641,    11,    11,    11,    11,
      11,   694,    11,    11,   354,   355,   356,   357,   358,   359,
     360,   361,   362,   363,   364,   365,   366,   367,   368,   369,
     370,   371,   372,   373,    11,     6,     9,     6,     8,     4,
     700,     8,   148,     9,   172,     9,    11,   707,   708,   709,
     710,   711,   712,   754,   617,   837,   622,   122,   614,   619,
     832,   849,   722,   624,   866,   143,   868,   892,   179,   751,
      -1,   671,   732,    -1,    -1,    -1,   736,    -1,    -1,    -1,
      -1,   719,   720,   721,   424,    -1,   724,    -1,    -1,    -1,
     728,    -1,    -1,    -1,    -1,    -1,   832,    62,    -1,    64,
      65,    66,    67,    68,    -1,    70,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   752,   753,    -1,    -1,    -1,   459,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    -1,    -1,    35,    -1,   100,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    -1,   486,    35,    -1,    -1,
      -1,   834,    -1,    -1,    -1,     6,    -1,    -1,    -1,    -1,
      -1,    -1,   502,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    -1,    -1,    35,    -1,    -1,    -1,   848,    -1,
      -1,    -1,    11,    -1,   832,    14,    15,    16,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,   879,    -1,    35,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   889,   908,    -1,   887,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,    86,
      87,    -1,    -1,   881,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   916,   922,   923,    -1,
      -1,    -1,   927,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   614,    -1,    -1,    -1,    -1,   619,
      -1,   621,   622,    -1,   624,    -1,    -1,    -1,   628,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,     3,     4,   644,   645,   646,   647,    -1,    10,
      -1,   651,    13,    -1,   654,   655,    -1,    -1,    -1,    -1,
      -1,    22,    23,    -1,    -1,   665,   666,   667,   668,    -1,
      -1,   671,    33,    -1,    -1,    36,    37,    -1,    39,    40,
      41,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      -1,    72,    73,    74,    11,    -1,    -1,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    -1,    11,    35,   100,
      14,    15,    16,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    -1,
      -1,    35,    -1,     1,    -1,    -1,    -1,    -1,    -1,     7,
      -1,    -1,    -1,   773,    12,    -1,    -1,    -1,    -1,    -1,
      -1,   781,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    36,    11,
      38,    -1,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    -1,    -1,    35,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   192,   832,    71,    72,    73,    -1,   837,    -1,    -1,
     201,    -1,    -1,   204,   205,   206,    -1,    -1,    -1,    -1,
      88,    89,    90,    91,    92,    93,    94,    95,    96,    97,
      98,    99,   100,   863,   864,    -1,   104,    -1,   106,   107,
     108,   109,   110,   111,   112,   113,   114,   115,   116,   117,
     118,   119,    -1,   121,   122,   123,   124,   125,    -1,   127,
     128,   129,   130,   131,   132,   133,   134,   135,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   144,   145,   146,   147,
     148,   149,   150,   151,   152,   153,   154,   155,   156,   157,
     158,   159,   160,   161,   162,   163,   164,   165,   166,   167,
     168,   169,   170,   171,   172,   173,   174,   175,   176,   177,
     178,   179,   180,   181,   182,   183,   184,   185,   186,   187,
     188,   189,     3,     4,    -1,    -1,    -1,    -1,    -1,    10,
      -1,    -1,    13,    -1,    -1,    -1,    -1,    -1,    -1,   207,
      -1,    22,    23,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    33,    -1,    -1,    36,    37,    -1,    39,    40,
      41,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      -1,    72,    73,    74,     3,     4,    -1,    -1,    -1,     8,
      -1,    10,    -1,    -1,    13,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    22,    23,    -1,    -1,    -1,    -1,   100,
      -1,    -1,    -1,    -1,    33,    -1,    -1,    36,    37,    -1,
      39,    40,    41,    42,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    -1,    72,    73,    74,    14,    15,    16,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    -1,    -1,    35,    -1,    -1,
      -1,   100,    -1,    -1,    -1,    -1,     3,     4,    -1,    -1,
      -1,    -1,    -1,    10,    -1,    -1,    13,    -1,    -1,    -1,
      -1,   192,    -1,    -1,    -1,    22,    23,    -1,    -1,    -1,
      -1,    -1,    -1,   204,   205,   206,    33,    -1,    -1,    36,
      37,    -1,    39,    40,    41,    42,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    -1,    72,    73,    74,    -1,     3,
       4,    -1,    -1,    -1,    -1,    -1,    10,    -1,    -1,    13,
      -1,   190,   191,   192,   193,   194,   195,   196,    22,    23,
      -1,   200,   201,   100,    -1,    -1,    -1,    -1,    -1,    33,
      -1,    -1,    36,    37,    -1,    39,    40,    41,    42,    43,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    -1,    72,    73,
      74,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      -1,    -1,    35,     3,     4,    -1,   100,    -1,    -1,    -1,
      10,    -1,    -1,    13,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    22,    23,    -1,    -1,    -1,    -1,    -1,    -1,
     197,   198,   199,    33,   201,    -1,    36,    37,    -1,    39,
      40,    41,    42,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    -1,    72,    73,    74,    -1,     3,     4,    -1,    -1,
      -1,    -1,    -1,    10,    -1,    -1,    13,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    22,    23,    -1,    -1,    -1,
     100,    -1,    -1,    -1,   198,   199,    33,   201,    -1,    36,
      37,    -1,    39,    40,    41,    42,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    -1,    72,    73,    74,    -1,    -1,
      14,    -1,    16,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,     3,
       4,    35,    -1,   100,    -1,    -1,    10,    -1,    -1,    13,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    22,    23,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    33,
      -1,   201,    36,    37,    -1,    39,    40,    41,    42,    43,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    -1,    72,    73,
      74,     3,     4,    -1,    -1,    -1,     8,    -1,    10,    -1,
      -1,    13,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      22,    23,    -1,    -1,    -1,    -1,   100,    -1,    -1,    -1,
      -1,    33,    -1,   200,    36,    37,    -1,    39,    40,    41,
      42,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    -1,
      72,    73,    74,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,     3,
       4,    -1,    -1,    -1,     8,    -1,    10,    -1,   100,    13,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    22,    23,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    33,
      -1,    -1,    36,    37,   198,    39,    40,    41,    42,    43,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    -1,    72,    73,
      74,    -1,    -1,    -1,    -1,     3,     4,    -1,    -1,    -1,
      -1,    -1,    10,    -1,    -1,    13,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    22,    23,   100,    -1,   190,   191,
     192,   193,   194,   195,   196,    33,    -1,    -1,    36,    37,
      -1,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    -1,    72,    73,    74,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    36,    -1,    38,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   100,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   190,   191,   192,   193,
     194,   195,   196,    72,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    88,
      89,    90,    -1,    -1,    -1,    94,    95,    96,    97,    98,
      99,   100,    -1,    -1,    -1,    -1,    -1,   106,   107,   108,
     109,   110,   111,    -1,    -1,   114,   115,   116,   117,   118,
     119,    -1,   121,   122,   123,   124,   125,    -1,   127,   128,
     129,   130,   131,   132,   133,   134,   135,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   192,   144,   145,   146,   147,   148,
     149,   150,   151,   152,   153,   154,   155,   156,   157,   158,
     159,   160,   161,   162,   163,   164,   165,   166,   167,   168,
     169,   170,   171,   172,   173,   174,   175,   176,   177,   178,
     179,   180,   181,   182,   183,   184,   185,   186,   187,   188,
     189,     3,     4,    -1,    -1,    -1,    -1,    -1,    10,    -1,
      -1,    13,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      22,    23,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    33,    -1,    -1,    36,    37,    -1,    39,    40,    41,
      42,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    -1,
      72,    73,    74,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      -1,    -1,    35,    -1,    -1,    -1,    -1,    -1,   100
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int16 yystos[] =
{
       0,   209,   210,     0,    22,    23,   212,     1,     7,    71,
      73,    91,    92,    93,   104,   112,   113,   213,   216,   217,
     218,   219,   220,   228,   232,   236,   237,   238,   239,   240,
     241,   257,   259,   264,   265,   296,   214,     1,     7,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,   102,   103,   172,   235,     7,     3,     4,    10,
      13,    22,    23,    33,    36,    37,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    72,    73,
      74,   100,   226,   227,   319,   320,   323,   327,   323,    12,
     266,   322,   323,   260,    12,   207,   211,    36,    38,    72,
      88,    89,    90,    94,    95,    96,    97,    98,    99,   100,
     106,   107,   108,   109,   110,   111,   114,   115,   116,   117,
     118,   119,   121,   122,   123,   124,   125,   127,   128,   129,
     130,   131,   132,   133,   134,   135,   144,   145,   146,   147,
     148,   149,   150,   151,   152,   153,   154,   155,   156,   157,
     158,   159,   160,   161,   162,   163,   164,   165,   166,   167,
     168,   169,   170,   171,   172,   173,   174,   175,   176,   177,
     178,   179,   180,   181,   182,   183,   184,   185,   186,   187,
     188,   189,   221,   222,   224,   229,   233,   242,   243,   247,
     248,   249,   250,   251,   252,   254,   255,   256,   270,   271,
     273,   274,   275,   276,   278,   279,   280,   281,   282,   283,
     284,   285,   286,   287,   288,   289,   293,   297,   298,   299,
     300,   301,   302,   303,   304,   331,   336,   337,   338,   339,
     340,   341,   342,   343,   344,   345,   346,   347,   348,   349,
     350,   351,   352,   353,   354,   355,   357,   358,   359,   360,
     361,   362,   363,   364,   365,   366,   367,   368,   369,   370,
     371,   372,   373,   374,   375,   376,   377,   378,   379,   380,
     381,   382,   383,   384,   385,   386,   387,   388,   389,   390,
     391,   211,   215,     7,   104,   323,   323,   327,   272,   322,
     272,   272,   323,     7,   319,   319,   319,   319,   319,   321,
      10,    10,    10,    10,    10,    10,    10,    10,    10,    10,
      10,    10,    10,    10,    10,    10,    10,    10,    10,    10,
      10,    10,    10,    10,    10,    10,    10,    10,    10,    10,
      10,    10,    10,    10,    14,    15,    16,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    35,    12,    12,    72,    12,    72,   223,
     322,   230,   327,   305,   306,   319,   320,   324,   327,   305,
     226,   294,   295,   308,   309,   315,   320,   327,   322,   310,
     311,   318,   320,   327,   312,   313,   320,   327,   101,   108,
     332,   322,   327,   327,    72,    72,   323,   211,   332,   327,
     327,   124,   125,   126,   253,   253,   290,   225,   244,     8,
      56,    57,   190,   191,   192,   193,   194,   195,   196,   314,
     319,   393,   394,   395,   396,   397,   398,   399,   400,   401,
     404,   200,   201,   393,   394,   393,   394,   307,   323,    13,
     192,   204,   205,   206,   317,   319,   402,   403,   393,   394,
      56,    57,   198,   199,   200,   201,   395,   404,   406,   404,
     406,   201,   317,   402,   317,   402,     8,   198,   199,   200,
     201,   356,   392,   404,   407,   408,   409,     8,    56,   395,
       8,   395,     8,   356,   392,   395,   393,   394,   197,   198,
     199,   201,   405,   405,   307,    13,   402,   314,   404,   404,
     404,   404,   393,   394,   307,   314,   404,   404,   404,   393,
     394,   404,   393,   394,    75,    76,    77,   102,   103,   172,
     235,    75,    76,    77,   235,   211,    12,    11,    10,   227,
     327,   327,   327,   323,   323,   323,   323,   323,   323,   323,
     323,   323,   323,   323,   323,   323,   323,   323,   319,   319,
     319,   327,   327,   327,   327,   327,    11,   327,   328,   327,
     327,   327,   327,   329,   327,   327,   227,   319,   319,   319,
     319,   319,   319,   319,   319,   319,   319,   319,   319,   319,
     319,   319,   319,   319,   319,   319,   319,   267,   258,   261,
      72,     6,   231,   211,     6,   234,   234,     6,   234,     6,
     234,     6,     6,   234,     6,   234,   327,     6,     6,     6,
     327,     6,     6,     6,   319,   323,   226,   291,   292,    72,
       4,   245,   246,   201,    10,    10,     6,     6,     6,     6,
     402,     6,    10,    10,     6,     6,    57,   198,   199,   201,
     202,   203,   317,   319,   397,     6,     6,     6,     6,     6,
       6,     6,     6,   201,    10,     6,   201,     6,     6,     6,
       6,     6,     6,   323,   323,   327,   272,   272,   272,   323,
     323,   323,   327,   323,   269,   227,    11,    11,    11,    11,
       6,   326,   326,   326,   326,   326,   326,     6,     6,     6,
       6,     6,     6,   326,   326,   326,    11,    11,    11,     6,
       6,     6,     6,    11,     6,    11,    11,    11,     6,   330,
      11,    11,     6,    11,    12,   211,     6,   322,     4,   306,
     295,   309,   277,   314,   311,   313,     6,   323,   314,   325,
      72,     6,     6,     6,     6,   234,   246,     9,   197,   198,
     199,   201,   198,   199,   201,   314,   404,   314,   406,   404,
     317,   317,   317,    10,     9,     9,    22,    23,     9,     9,
       9,    22,     9,   317,   317,   201,   317,   200,   317,   395,
     200,   395,   314,   356,   392,   404,   407,   395,     9,     8,
       9,     8,   395,   395,   356,   392,   404,   404,   211,    11,
     323,    11,    11,    11,    11,    11,    11,   323,   323,   323,
     323,   323,   323,    11,    11,    11,   327,   327,   327,   323,
     327,   327,     6,   323,   268,   263,   323,     6,   234,   136,
     137,   138,   139,   140,   141,   142,   143,   333,     6,   333,
     327,   327,   292,    11,    11,    11,    11,    11,    11,    11,
       9,     9,   397,    22,    23,   316,     6,   201,     6,   201,
     326,   326,   326,   326,   326,   326,    11,    11,    11,     6,
      11,     6,   324,   327,    11,   211,    12,     6,   314,     8,
     334,   323,   334,     9,   319,   319,   395,     9,   395,     9,
      11,    11,    11,    11,    11,    11,   322,   327,   262,   323,
     322,   335,   335,    11,    11,   211,     6,     9,     6,   323,
      37,    38,     8,     8,   322,   322,     9,     6,     9,   322,
       9
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int16 yyr1[] =
{
       0,   208,   209,   210,   210,   211,   211,   212,   212,   212,
     213,   213,   214,   213,   215,   213,   216,   216,   216,   216,
     216,   216,   216,   216,   217,   218,   219,   220,   220,   220,
     220,   220,   221,   223,   222,   225,   224,   226,   226,   227,
     227,   228,   228,   228,   228,   228,   228,   228,   230,   229,
     231,   231,   232,   232,   232,   232,   232,   232,   233,   233,
     233,   233,   233,   233,   233,   233,   233,   233,   233,   233,
     233,   233,   233,   233,   233,   233,   233,   233,   233,   233,
     233,   233,   233,   233,   233,   233,   233,   233,   233,   233,
     233,   233,   233,   233,   233,   233,   233,   233,   234,   234,
     235,   235,   235,   235,   235,   235,   235,   235,   235,   235,
     236,   237,   237,   238,   239,   240,   241,   242,   242,   244,
     243,   245,   245,   246,   247,   248,   249,   250,   251,   252,
     253,   253,   253,   253,   254,   254,   254,   254,   255,   255,
     256,   256,   258,   257,   260,   261,   262,   259,   263,   263,
     263,   264,   266,   267,   268,   265,   269,   265,   270,   271,
     272,   272,   273,   274,   275,   276,   276,   277,   277,   278,
     278,   279,   279,   280,   280,   281,   282,   283,   283,   283,
     283,   284,   285,   286,   287,   288,   290,   289,   291,   291,
     292,   293,   294,   294,   295,   296,   297,   297,   297,   298,
     299,   299,   300,   301,   302,   303,   304,   304,   305,   305,
     306,   306,   307,   308,   308,   309,   309,   310,   310,   311,
     311,   312,   312,   313,   313,   314,   315,   316,   316,   317,
     318,   319,   319,   320,   320,   320,   320,   320,   320,   320,
     320,   320,   320,   320,   320,   320,   320,   320,   320,   320,
     320,   320,   320,   320,   320,   320,   320,   320,   320,   320,
     320,   320,   320,   320,   320,   320,   321,   320,   320,   320,
     320,   320,   320,   320,   320,   320,   320,   320,   320,   320,
     320,   320,   320,   320,   320,   320,   320,   320,   320,   322,
     323,   324,   325,   326,   326,   327,   327,   327,   327,   327,
     327,   327,   327,   327,   327,   327,   328,   328,   329,   330,
     330,   330,   331,   332,   332,   332,   333,   333,   333,   333,
     333,   333,   333,   333,   334,   334,   335,   335,   335,   335,
     336,   336,   336,   336,   336,   336,   336,   336,   336,   336,
     336,   336,   336,   336,   336,   336,   336,   336,   336,   336,
     336,   336,   336,   336,   336,   336,   336,   336,   336,   336,
     336,   336,   336,   336,   336,   336,   336,   336,   336,   336,
     336,   336,   336,   336,   336,   336,   337,   337,   338,   338,
     338,   338,   339,   339,   340,   341,   341,   342,   343,   343,
     344,   345,   346,   346,   347,   348,   349,   350,   350,   351,
     351,   351,   352,   352,   353,   353,   354,   354,   355,   355,
     355,   355,   356,   356,   357,   357,   357,   357,   357,   357,
     357,   357,   358,   358,   359,   359,   360,   360,   361,   362,
     363,   363,   364,   364,   364,   365,   365,   366,   367,   367,
     368,   369,   370,   371,   371,   372,   373,   374,   375,   376,
     377,   378,   379,   380,   381,   382,   382,   383,   384,   385,
     386,   387,   388,   388,   389,   389,   390,   391,   391,   392,
     393,   393,   394,   394,   395,   395,   396,   396,   397,   397,
     398,   398,   399,   399,   400,   400,   401,   401,   402,   402,
     403,   403,   403,   403,   404,   404,   404,   404,   404,   404,
     404,   404,   405,   405,   405,   405,   406,   406,   406,   406,
     407,   407,   407,   407,   408,   408,   409,   409
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     1,     0,     3,     1,     1,     0,     1,     1,
       2,     1,     0,     3,     0,     4,     1,     1,     1,     1,
       1,     1,     1,     1,     3,     3,     2,     1,     2,     2,
       2,     1,     1,     0,     3,     0,     3,     1,     1,     1,
       1,     0,     1,     1,     2,     2,     3,     3,     0,     3,
       0,     2,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     0,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       3,     3,     3,     3,     3,     3,     3,     2,     4,     0,
       3,     1,     2,     1,     1,     1,     1,     1,     2,     2,
       0,     2,     2,     2,     3,     5,     3,     5,     1,     2,
       7,     1,     0,     5,     0,     0,     0,     9,     1,     3,
       5,     3,     0,     0,     0,     7,     0,     6,     2,     1,
       0,     1,     1,     1,     1,     2,     5,     1,     3,     1,
       3,     1,     3,     1,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     0,     4,     1,     3,
       1,     3,     1,     3,     1,     4,     2,     4,     6,     4,
       2,     4,     2,     1,     1,     3,     1,     3,     1,     3,
       1,     1,     1,     1,     3,     1,     1,     1,     3,     1,
       1,     1,     3,     1,     1,     1,     1,     2,     2,     1,
       1,     1,     1,     1,     1,     2,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     2,     2,     2,     4,
       4,     4,     4,     4,     4,     4,     0,     5,     5,     5,
       5,     7,     7,     7,     7,     7,     5,     5,     5,     5,
       5,     5,     7,     6,     6,     6,     4,     4,     3,     1,
       1,     1,     1,     0,     2,     1,     8,     6,     6,     3,
       4,     4,     4,     8,     4,     4,     1,     3,     2,     0,
       3,     3,     7,     0,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     0,     3,     0,     6,     8,     6,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     2,     2,     2,     2,
       4,     4,     2,     2,     4,     2,     4,     1,     2,     2,
       1,     1,     2,     2,     1,     1,     1,     2,     2,     2,
       4,     2,     2,     4,     6,     6,     6,     6,     4,     4,
       4,     4,     3,     5,     1,     1,     1,     1,     1,     1,
       1,     1,     5,     4,     4,     4,     4,     4,     4,     4,
       4,     4,     4,     4,     4,     4,     4,     1,     2,     2,
       2,     2,     4,     1,     2,     1,     2,     1,     2,     1,
       2,     1,     2,     1,     2,     2,     2,     1,     4,     2,
       2,     2,     1,     2,     2,     2,     2,     2,     2,     3,
       1,     3,     1,     3,     1,     4,     1,     4,     1,     4,
       1,     4,     1,     4,     1,     4,     1,     4,     1,     2,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       3,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       3,     3,     1,     1,     3,     4,     3,     4
};


enum { YYENOMEM = -2 };

#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab
#define YYNOMEM         goto yyexhaustedlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                    \
  do                                                              \
    if (yychar == YYEMPTY)                                        \
      {                                                           \
        yychar = (Token);                                         \
        yylval = (Value);                                         \
        YYPOPSTACK (yylen);                                       \
        yystate = *yyssp;                                         \
        YY_LAC_DISCARD ("YYBACKUP");                              \
        goto yybackup;                                            \
      }                                                           \
    else                                                          \
      {                                                           \
        yyerror (YY_("syntax error: cannot back up")); \
        YYERROR;                                                  \
      }                                                           \
  while (0)

/* Backward compatibility with an undocumented macro.
   Use YYerror or YYUNDEF. */
#define YYERRCODE YYUNDEF


/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)




# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Kind, Value); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo,
                       yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep)
{
  FILE *yyoutput = yyo;
  YY_USE (yyoutput);
  if (!yyvaluep)
    return;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/*---------------------------.
| Print this symbol on YYO.  |
`---------------------------*/

static void
yy_symbol_print (FILE *yyo,
                 yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep)
{
  YYFPRINTF (yyo, "%s %s (",
             yykind < YYNTOKENS ? "token" : "nterm", yysymbol_name (yykind));

  yy_symbol_value_print (yyo, yykind, yyvaluep);
  YYFPRINTF (yyo, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yy_state_t *yybottom, yy_state_t *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yy_state_t *yyssp, YYSTYPE *yyvsp,
                 int yyrule)
{
  int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %d):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       YY_ACCESSING_SYMBOL (+yyssp[yyi + 1 - yynrhs]),
                       &yyvsp[(yyi + 1) - (yynrhs)]);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, Rule); \
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args) ((void) 0)
# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif


/* Given a state stack such that *YYBOTTOM is its bottom, such that
   *YYTOP is either its top or is YYTOP_EMPTY to indicate an empty
   stack, and such that *YYCAPACITY is the maximum number of elements it
   can hold without a reallocation, make sure there is enough room to
   store YYADD more elements.  If not, allocate a new stack using
   YYSTACK_ALLOC, copy the existing elements, and adjust *YYBOTTOM,
   *YYTOP, and *YYCAPACITY to reflect the new capacity and memory
   location.  If *YYBOTTOM != YYBOTTOM_NO_FREE, then free the old stack
   using YYSTACK_FREE.  Return 0 if successful or if no reallocation is
   required.  Return YYENOMEM if memory is exhausted.  */
static int
yy_lac_stack_realloc (YYPTRDIFF_T *yycapacity, YYPTRDIFF_T yyadd,
#if YYDEBUG
                      char const *yydebug_prefix,
                      char const *yydebug_suffix,
#endif
                      yy_state_t **yybottom,
                      yy_state_t *yybottom_no_free,
                      yy_state_t **yytop, yy_state_t *yytop_empty)
{
  YYPTRDIFF_T yysize_old =
    *yytop == yytop_empty ? 0 : *yytop - *yybottom + 1;
  YYPTRDIFF_T yysize_new = yysize_old + yyadd;
  if (*yycapacity < yysize_new)
    {
      YYPTRDIFF_T yyalloc = 2 * yysize_new;
      yy_state_t *yybottom_new;
      /* Use YYMAXDEPTH for maximum stack size given that the stack
         should never need to grow larger than the main state stack
         needs to grow without LAC.  */
      if (YYMAXDEPTH < yysize_new)
        {
          YYDPRINTF ((stderr, "%smax size exceeded%s", yydebug_prefix,
                      yydebug_suffix));
          return YYENOMEM;
        }
      if (YYMAXDEPTH < yyalloc)
        yyalloc = YYMAXDEPTH;
      yybottom_new =
        YY_CAST (yy_state_t *,
                 YYSTACK_ALLOC (YY_CAST (YYSIZE_T,
                                         yyalloc * YYSIZEOF (*yybottom_new))));
      if (!yybottom_new)
        {
          YYDPRINTF ((stderr, "%srealloc failed%s", yydebug_prefix,
                      yydebug_suffix));
          return YYENOMEM;
        }
      if (*yytop != yytop_empty)
        {
          YYCOPY (yybottom_new, *yybottom, yysize_old);
          *yytop = yybottom_new + (yysize_old - 1);
        }
      if (*yybottom != yybottom_no_free)
        YYSTACK_FREE (*yybottom);
      *yybottom = yybottom_new;
      *yycapacity = yyalloc;
    }
  return 0;
}

/* Establish the initial context for the current lookahead if no initial
   context is currently established.

   We define a context as a snapshot of the parser stacks.  We define
   the initial context for a lookahead as the context in which the
   parser initially examines that lookahead in order to select a
   syntactic action.  Thus, if the lookahead eventually proves
   syntactically unacceptable (possibly in a later context reached via a
   series of reductions), the initial context can be used to determine
   the exact set of tokens that would be syntactically acceptable in the
   lookahead's place.  Moreover, it is the context after which any
   further semantic actions would be erroneous because they would be
   determined by a syntactically unacceptable token.

   YY_LAC_ESTABLISH should be invoked when a reduction is about to be
   performed in an inconsistent state (which, for the purposes of LAC,
   includes consistent states that don't know they're consistent because
   their default reductions have been disabled).  Iff there is a
   lookahead token, it should also be invoked before reporting a syntax
   error.  This latter case is for the sake of the debugging output.

   For parse.lac=full, the implementation of YY_LAC_ESTABLISH is as
   follows.  If no initial context is currently established for the
   current lookahead, then check if that lookahead can eventually be
   shifted if syntactic actions continue from the current context.
   Report a syntax error if it cannot.  */
#define YY_LAC_ESTABLISH                                                \
do {                                                                    \
  if (!yy_lac_established)                                              \
    {                                                                   \
      YYDPRINTF ((stderr,                                               \
                  "LAC: initial context established for %s\n",          \
                  yysymbol_name (yytoken)));                            \
      yy_lac_established = 1;                                           \
      switch (yy_lac (yyesa, &yyes, &yyes_capacity, yyssp, yytoken))    \
        {                                                               \
        case YYENOMEM:                                                  \
          YYNOMEM;                                                      \
        case 1:                                                         \
          goto yyerrlab;                                                \
        }                                                               \
    }                                                                   \
} while (0)

/* Discard any previous initial lookahead context because of Event,
   which may be a lookahead change or an invalidation of the currently
   established initial context for the current lookahead.

   The most common example of a lookahead change is a shift.  An example
   of both cases is syntax error recovery.  That is, a syntax error
   occurs when the lookahead is syntactically erroneous for the
   currently established initial context, so error recovery manipulates
   the parser stacks to try to find a new initial context in which the
   current lookahead is syntactically acceptable.  If it fails to find
   such a context, it discards the lookahead.  */
#if YYDEBUG
# define YY_LAC_DISCARD(Event)                                           \
do {                                                                     \
  if (yy_lac_established)                                                \
    {                                                                    \
      YYDPRINTF ((stderr, "LAC: initial context discarded due to "       \
                  Event "\n"));                                          \
      yy_lac_established = 0;                                            \
    }                                                                    \
} while (0)
#else
# define YY_LAC_DISCARD(Event) yy_lac_established = 0
#endif

/* Given the stack whose top is *YYSSP, return 0 iff YYTOKEN can
   eventually (after perhaps some reductions) be shifted, return 1 if
   not, or return YYENOMEM if memory is exhausted.  As preconditions and
   postconditions: *YYES_CAPACITY is the allocated size of the array to
   which *YYES points, and either *YYES = YYESA or *YYES points to an
   array allocated with YYSTACK_ALLOC.  yy_lac may overwrite the
   contents of either array, alter *YYES and *YYES_CAPACITY, and free
   any old *YYES other than YYESA.  */
static int
yy_lac (yy_state_t *yyesa, yy_state_t **yyes,
        YYPTRDIFF_T *yyes_capacity, yy_state_t *yyssp, yysymbol_kind_t yytoken)
{
  yy_state_t *yyes_prev = yyssp;
  yy_state_t *yyesp = yyes_prev;
  /* Reduce until we encounter a shift and thereby accept the token.  */
  YYDPRINTF ((stderr, "LAC: checking lookahead %s:", yysymbol_name (yytoken)));
  if (yytoken == YYSYMBOL_YYUNDEF)
    {
      YYDPRINTF ((stderr, " Always Err\n"));
      return 1;
    }
  while (1)
    {
      int yyrule = yypact[+*yyesp];
      if (yypact_value_is_default (yyrule)
          || (yyrule += yytoken) < 0 || YYLAST < yyrule
          || yycheck[yyrule] != yytoken)
        {
          /* Use the default action.  */
          yyrule = yydefact[+*yyesp];
          if (yyrule == 0)
            {
              YYDPRINTF ((stderr, " Err\n"));
              return 1;
            }
        }
      else
        {
          /* Use the action from yytable.  */
          yyrule = yytable[yyrule];
          if (yytable_value_is_error (yyrule))
            {
              YYDPRINTF ((stderr, " Err\n"));
              return 1;
            }
          if (0 < yyrule)
            {
              YYDPRINTF ((stderr, " S%d\n", yyrule));
              return 0;
            }
          yyrule = -yyrule;
        }
      /* By now we know we have to simulate a reduce.  */
      YYDPRINTF ((stderr, " R%d", yyrule - 1));
      {
        /* Pop the corresponding number of values from the stack.  */
        YYPTRDIFF_T yylen = yyr2[yyrule];
        /* First pop from the LAC stack as many tokens as possible.  */
        if (yyesp != yyes_prev)
          {
            YYPTRDIFF_T yysize = yyesp - *yyes + 1;
            if (yylen < yysize)
              {
                yyesp -= yylen;
                yylen = 0;
              }
            else
              {
                yyesp = yyes_prev;
                yylen -= yysize;
              }
          }
        /* Only afterwards look at the main stack.  */
        if (yylen)
          yyesp = yyes_prev -= yylen;
      }
      /* Push the resulting state of the reduction.  */
      {
        yy_state_fast_t yystate;
        {
          const int yylhs = yyr1[yyrule] - YYNTOKENS;
          const int yyi = yypgoto[yylhs] + *yyesp;
          yystate = (0 <= yyi && yyi <= YYLAST && yycheck[yyi] == *yyesp
                     ? yytable[yyi]
                     : yydefgoto[yylhs]);
        }
        if (yyesp == yyes_prev)
          {
            yyesp = *yyes;
            YY_IGNORE_USELESS_CAST_BEGIN
            *yyesp = YY_CAST (yy_state_t, yystate);
            YY_IGNORE_USELESS_CAST_END
          }
        else
          {
            if (yy_lac_stack_realloc (yyes_capacity, 1,
#if YYDEBUG
                                      " (", ")",
#endif
                                      yyes, yyesa, &yyesp, yyes_prev))
              {
                YYDPRINTF ((stderr, "\n"));
                return YYENOMEM;
              }
            YY_IGNORE_USELESS_CAST_BEGIN
            *++yyesp = YY_CAST (yy_state_t, yystate);
            YY_IGNORE_USELESS_CAST_END
          }
        YYDPRINTF ((stderr, " G%d", yystate));
      }
    }
}

/* Context of a parse error.  */
typedef struct
{
  yy_state_t *yyssp;
  yy_state_t *yyesa;
  yy_state_t **yyes;
  YYPTRDIFF_T *yyes_capacity;
  yysymbol_kind_t yytoken;
} yypcontext_t;

/* Put in YYARG at most YYARGN of the expected tokens given the
   current YYCTX, and return the number of tokens stored in YYARG.  If
   YYARG is null, return the number of expected tokens (guaranteed to
   be less than YYNTOKENS).  Return YYENOMEM on memory exhaustion.
   Return 0 if there are more than YYARGN expected tokens, yet fill
   YYARG up to YYARGN. */
static int
yypcontext_expected_tokens (const yypcontext_t *yyctx,
                            yysymbol_kind_t yyarg[], int yyargn)
{
  /* Actual size of YYARG. */
  int yycount = 0;

  int yyx;
  for (yyx = 0; yyx < YYNTOKENS; ++yyx)
    {
      yysymbol_kind_t yysym = YY_CAST (yysymbol_kind_t, yyx);
      if (yysym != YYSYMBOL_YYerror && yysym != YYSYMBOL_YYUNDEF)
        switch (yy_lac (yyctx->yyesa, yyctx->yyes, yyctx->yyes_capacity, yyctx->yyssp, yysym))
          {
          case YYENOMEM:
            return YYENOMEM;
          case 1:
            continue;
          default:
            if (!yyarg)
              ++yycount;
            else if (yycount == yyargn)
              return 0;
            else
              yyarg[yycount++] = yysym;
          }
    }
  if (yyarg && yycount == 0 && 0 < yyargn)
    yyarg[0] = YYSYMBOL_YYEMPTY;
  return yycount;
}




#ifndef yystrlen
# if defined __GLIBC__ && defined _STRING_H
#  define yystrlen(S) (YY_CAST (YYPTRDIFF_T, strlen (S)))
# else
/* Return the length of YYSTR.  */
static YYPTRDIFF_T
yystrlen (const char *yystr)
{
  YYPTRDIFF_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
# endif
#endif

#ifndef yystpcpy
# if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#  define yystpcpy stpcpy
# else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
yystpcpy (char *yydest, const char *yysrc)
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
# endif
#endif



static int
yy_syntax_error_arguments (const yypcontext_t *yyctx,
                           yysymbol_kind_t yyarg[], int yyargn)
{
  /* Actual size of YYARG. */
  int yycount = 0;
  /* There are many possibilities here to consider:
     - If this state is a consistent state with a default action, then
       the only way this function was invoked is if the default action
       is an error action.  In that case, don't check for expected
       tokens because there are none.
     - The only way there can be no lookahead present (in yychar) is if
       this state is a consistent state with a default action.  Thus,
       detecting the absence of a lookahead is sufficient to determine
       that there is no unexpected or expected token to report.  In that
       case, just report a simple "syntax error".
     - Don't assume there isn't a lookahead just because this state is a
       consistent state with a default action.  There might have been a
       previous inconsistent state, consistent state with a non-default
       action, or user semantic action that manipulated yychar.
       In the first two cases, it might appear that the current syntax
       error should have been detected in the previous state when yy_lac
       was invoked.  However, at that time, there might have been a
       different syntax error that discarded a different initial context
       during error recovery, leaving behind the current lookahead.
  */
  if (yyctx->yytoken != YYSYMBOL_YYEMPTY)
    {
      int yyn;
      YYDPRINTF ((stderr, "Constructing syntax error message\n"));
      if (yyarg)
        yyarg[yycount] = yyctx->yytoken;
      ++yycount;
      yyn = yypcontext_expected_tokens (yyctx,
                                        yyarg ? yyarg + 1 : yyarg, yyargn - 1);
      if (yyn == YYENOMEM)
        return YYENOMEM;
      else if (yyn == 0)
        YYDPRINTF ((stderr, "No expected tokens.\n"));
      else
        yycount += yyn;
    }
  return yycount;
}

/* Copy into *YYMSG, which is of size *YYMSG_ALLOC, an error message
   about the unexpected token YYTOKEN for the state stack whose top is
   YYSSP.  In order to see if a particular token T is a
   valid looakhead, invoke yy_lac (YYESA, YYES, YYES_CAPACITY, YYSSP, T).

   Return 0 if *YYMSG was successfully written.  Return -1 if *YYMSG is
   not large enough to hold the message.  In that case, also set
   *YYMSG_ALLOC to the required number of bytes.  Return YYENOMEM if the
   required number of bytes is too large to store or if
   yy_lac returned YYENOMEM.  */
static int
yysyntax_error (YYPTRDIFF_T *yymsg_alloc, char **yymsg,
                const yypcontext_t *yyctx)
{
  enum { YYARGS_MAX = 5 };
  /* Internationalized format string. */
  const char *yyformat = YY_NULLPTR;
  /* Arguments of yyformat: reported tokens (one for the "unexpected",
     one per "expected"). */
  yysymbol_kind_t yyarg[YYARGS_MAX];
  /* Cumulated lengths of YYARG.  */
  YYPTRDIFF_T yysize = 0;

  /* Actual size of YYARG. */
  int yycount = yy_syntax_error_arguments (yyctx, yyarg, YYARGS_MAX);
  if (yycount == YYENOMEM)
    return YYENOMEM;

  switch (yycount)
    {
#define YYCASE_(N, S)                       \
      case N:                               \
        yyformat = S;                       \
        break
    default: /* Avoid compiler warnings. */
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
#undef YYCASE_
    }

  /* Compute error message size.  Don't count the "%s"s, but reserve
     room for the terminator.  */
  yysize = yystrlen (yyformat) - 2 * yycount + 1;
  {
    int yyi;
    for (yyi = 0; yyi < yycount; ++yyi)
      {
        YYPTRDIFF_T yysize1
          = yysize + yystrlen (yysymbol_name (yyarg[yyi]));
        if (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM)
          yysize = yysize1;
        else
          return YYENOMEM;
      }
  }

  if (*yymsg_alloc < yysize)
    {
      *yymsg_alloc = 2 * yysize;
      if (! (yysize <= *yymsg_alloc
             && *yymsg_alloc <= YYSTACK_ALLOC_MAXIMUM))
        *yymsg_alloc = YYSTACK_ALLOC_MAXIMUM;
      return -1;
    }

  /* Avoid sprintf, as that infringes on the user's name space.
     Don't have undefined behavior even if the translation
     produced a string with the wrong number of "%s"s.  */
  {
    char *yyp = *yymsg;
    int yyi = 0;
    while ((*yyp = *yyformat) != '\0')
      if (*yyp == '%' && yyformat[1] == 's' && yyi < yycount)
        {
          yyp = yystpcpy (yyp, yysymbol_name (yyarg[yyi++]));
          yyformat += 2;
        }
      else
        {
          ++yyp;
          ++yyformat;
        }
  }
  return 0;
}


/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg,
            yysymbol_kind_t yykind, YYSTYPE *yyvaluep)
{
  YY_USE (yyvaluep);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yykind, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/* Lookahead token kind.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;
/* Number of syntax errors so far.  */
int yynerrs;




/*----------.
| yyparse.  |
`----------*/

int
yyparse (void)
{
    yy_state_fast_t yystate = 0;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus = 0;

    /* Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* Their size.  */
    YYPTRDIFF_T yystacksize = YYINITDEPTH;

    /* The state stack: array, bottom, top.  */
    yy_state_t yyssa[YYINITDEPTH];
    yy_state_t *yyss = yyssa;
    yy_state_t *yyssp = yyss;

    /* The semantic value stack: array, bottom, top.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs = yyvsa;
    YYSTYPE *yyvsp = yyvs;

    yy_state_t yyesa[20];
    yy_state_t *yyes = yyesa;
    YYPTRDIFF_T yyes_capacity = 20 < YYMAXDEPTH ? 20 : YYMAXDEPTH;

  /* Whether LAC context is established.  A Boolean.  */
  int yy_lac_established = 0;
  int yyn;
  /* The return value of yyparse.  */
  int yyresult;
  /* Lookahead symbol kind.  */
  yysymbol_kind_t yytoken = YYSYMBOL_YYEMPTY;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;

  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYPTRDIFF_T yymsg_alloc = sizeof yymsgbuf;

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yychar = YYEMPTY; /* Cause a token to be read.  */

  goto yysetstate;


/*------------------------------------------------------------.
| yynewstate -- push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;


/*--------------------------------------------------------------------.
| yysetstate -- set current state (the top of the stack) to yystate.  |
`--------------------------------------------------------------------*/
yysetstate:
  YYDPRINTF ((stderr, "Entering state %d\n", yystate));
  YY_ASSERT (0 <= yystate && yystate < YYNSTATES);
  YY_IGNORE_USELESS_CAST_BEGIN
  *yyssp = YY_CAST (yy_state_t, yystate);
  YY_IGNORE_USELESS_CAST_END
  YY_STACK_PRINT (yyss, yyssp);

  if (yyss + yystacksize - 1 <= yyssp)
#if !defined yyoverflow && !defined YYSTACK_RELOCATE
    YYNOMEM;
#else
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYPTRDIFF_T yysize = yyssp - yyss + 1;

# if defined yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        yy_state_t *yyss1 = yyss;
        YYSTYPE *yyvs1 = yyvs;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * YYSIZEOF (*yyssp),
                    &yyvs1, yysize * YYSIZEOF (*yyvsp),
                    &yystacksize);
        yyss = yyss1;
        yyvs = yyvs1;
      }
# else /* defined YYSTACK_RELOCATE */
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        YYNOMEM;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yy_state_t *yyss1 = yyss;
        union yyalloc *yyptr =
          YY_CAST (union yyalloc *,
                   YYSTACK_ALLOC (YY_CAST (YYSIZE_T, YYSTACK_BYTES (yystacksize))));
        if (! yyptr)
          YYNOMEM;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YY_IGNORE_USELESS_CAST_BEGIN
      YYDPRINTF ((stderr, "Stack size increased to %ld\n",
                  YY_CAST (long, yystacksize)));
      YY_IGNORE_USELESS_CAST_END

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }
#endif /* !defined yyoverflow && !defined YYSTACK_RELOCATE */


  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;


/*-----------.
| yybackup.  |
`-----------*/
yybackup:
  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either empty, or end-of-input, or a valid lookahead.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token\n"));
      yychar = yylex ();
    }

  if (yychar <= T_EOF)
    {
      yychar = T_EOF;
      yytoken = YYSYMBOL_YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else if (yychar == YYerror)
    {
      /* The scanner already issued an error message, process directly
         to error recovery.  But do not keep the error token as
         lookahead, it is too special and may lead us to an endless
         loop in error recovery. */
      yychar = YYUNDEF;
      yytoken = YYSYMBOL_YYerror;
      goto yyerrlab1;
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    {
      YY_LAC_ESTABLISH;
      goto yydefault;
    }
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      YY_LAC_ESTABLISH;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);
  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  /* Discard the shifted token.  */
  yychar = YYEMPTY;
  YY_LAC_DISCARD ("shift");
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     '$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  {
    int yychar_backup = yychar;
    switch (yyn)
      {
  case 8: /* opt_diff_mark: "+"  */
#line 700 "src/asm/parser.y"
                           {
			error("syntax error, unexpected + at the beginning of the line (is it a leftover diff mark?)\n");
		}
#line 3332 "src/asm/parser.c"
    break;

  case 9: /* opt_diff_mark: "-"  */
#line 703 "src/asm/parser.y"
                           {
			error("syntax error, unexpected - at the beginning of the line (is it a leftover diff mark?)\n");
		}
#line 3340 "src/asm/parser.c"
    break;

  case 12: /* $@1: %empty  */
#line 711 "src/asm/parser.y"
                        {
			lexer_SetMode(LEXER_NORMAL);
			lexer_ToggleStringExpansion(true);
		}
#line 3349 "src/asm/parser.c"
    break;

  case 13: /* line: error $@1 endofline  */
#line 714 "src/asm/parser.y"
                            {
			fstk_StopRept();
			yyerrok;
		}
#line 3358 "src/asm/parser.c"
    break;

  case 14: /* $@2: %empty  */
#line 719 "src/asm/parser.y"
                                {
			lexer_SetMode(LEXER_NORMAL);
			lexer_ToggleStringExpansion(true);
		}
#line 3367 "src/asm/parser.c"
    break;

  case 15: /* line: "label" error $@2 endofline  */
#line 722 "src/asm/parser.y"
                            {
			struct Symbol *macro = sym_FindExactSymbol((yyvsp[-3].symName));

			if (macro && macro->type == SYM_MACRO)
				fprintf(stderr,
					"    To invoke `%s` as a macro it must be indented\n", (yyvsp[-3].symName));
			fstk_StopRept();
			yyerrok;
		}
#line 3381 "src/asm/parser.c"
    break;

  case 24: /* if: "IF" const "newline"  */
#line 748 "src/asm/parser.y"
                                           {
			lexer_IncIFDepth();

			if ((yyvsp[-1].constValue))
				lexer_RunIFBlock();
			else
				lexer_SetMode(LEXER_SKIP_TO_ELIF);
		}
#line 3394 "src/asm/parser.c"
    break;

  case 25: /* elif: "ELIF" const "newline"  */
#line 758 "src/asm/parser.y"
                                             {
			if (lexer_GetIFDepth() == 0)
				fatalerror("Found ELIF outside an IF construct\n");

			if (lexer_RanIFBlock()) {
				if (lexer_ReachedELSEBlock())
					fatalerror("Found ELIF after an ELSE block\n");

				lexer_SetMode(LEXER_SKIP_TO_ENDC);
			} else if ((yyvsp[-1].constValue)) {
				lexer_RunIFBlock();
			} else {
				lexer_SetMode(LEXER_SKIP_TO_ELIF);
			}
		}
#line 3414 "src/asm/parser.c"
    break;

  case 26: /* else: "ELSE" "newline"  */
#line 775 "src/asm/parser.y"
                                       {
			if (lexer_GetIFDepth() == 0)
				fatalerror("Found ELSE outside an IF construct\n");

			if (lexer_RanIFBlock()) {
				if (lexer_ReachedELSEBlock())
					fatalerror("Found ELSE after an ELSE block\n");

				lexer_SetMode(LEXER_SKIP_TO_ENDC);
			} else {
				lexer_RunIFBlock();
				lexer_ReachELSEBlock();
			}
		}
#line 3433 "src/asm/parser.c"
    break;

  case 32: /* endc: "ENDC"  */
#line 798 "src/asm/parser.y"
                             {
			lexer_DecIFDepth();
		}
#line 3441 "src/asm/parser.c"
    break;

  case 33: /* $@3: %empty  */
#line 803 "src/asm/parser.y"
                           {
			lexer_ToggleStringExpansion(false);
		}
#line 3449 "src/asm/parser.c"
    break;

  case 34: /* def_id: "DEF" $@3 "identifier"  */
#line 805 "src/asm/parser.y"
                       {
			lexer_ToggleStringExpansion(true);
			strcpy((yyval.symName), (yyvsp[0].symName));
		}
#line 3458 "src/asm/parser.c"
    break;

  case 35: /* $@4: %empty  */
#line 811 "src/asm/parser.y"
                              {
			lexer_ToggleStringExpansion(false);
		}
#line 3466 "src/asm/parser.c"
    break;

  case 36: /* redef_id: "REDEF" $@4 "identifier"  */
#line 813 "src/asm/parser.y"
                       {
			lexer_ToggleStringExpansion(true);
			strcpy((yyval.symName), (yyvsp[0].symName));
		}
#line 3475 "src/asm/parser.c"
    break;

  case 42: /* label: ":"  */
#line 823 "src/asm/parser.y"
                          {
			sym_AddAnonLabel();
		}
#line 3483 "src/asm/parser.c"
    break;

  case 43: /* label: "local identifier"  */
#line 826 "src/asm/parser.y"
                             {
			sym_AddLocalLabel((yyvsp[0].symName));
		}
#line 3491 "src/asm/parser.c"
    break;

  case 44: /* label: "local identifier" ":"  */
#line 829 "src/asm/parser.y"
                                     {
			sym_AddLocalLabel((yyvsp[-1].symName));
		}
#line 3499 "src/asm/parser.c"
    break;

  case 45: /* label: "label" ":"  */
#line 832 "src/asm/parser.y"
                                  {
			sym_AddLabel((yyvsp[-1].symName));
		}
#line 3507 "src/asm/parser.c"
    break;

  case 46: /* label: "local identifier" ":" ":"  */
#line 835 "src/asm/parser.y"
                                             {
			sym_AddLocalLabel((yyvsp[-2].symName));
			sym_Export((yyvsp[-2].symName));
		}
#line 3516 "src/asm/parser.c"
    break;

  case 47: /* label: "label" ":" ":"  */
#line 839 "src/asm/parser.y"
                                          {
			sym_AddLabel((yyvsp[-2].symName));
			sym_Export((yyvsp[-2].symName));
		}
#line 3525 "src/asm/parser.c"
    break;

  case 48: /* $@5: %empty  */
#line 845 "src/asm/parser.y"
                       {
			// Parsing 'macroargs' will restore the lexer's normal mode
			lexer_SetMode(LEXER_RAW);
		}
#line 3534 "src/asm/parser.c"
    break;

  case 49: /* macro: "identifier" $@5 macroargs  */
#line 848 "src/asm/parser.y"
                            {
			fstk_RunMacro((yyvsp[-2].symName), (yyvsp[0].macroArg));
		}
#line 3542 "src/asm/parser.c"
    break;

  case 50: /* macroargs: %empty  */
#line 853 "src/asm/parser.y"
                         {
			(yyval.macroArg) = macro_NewArgs();
		}
#line 3550 "src/asm/parser.c"
    break;

  case 51: /* macroargs: macroargs "string"  */
#line 856 "src/asm/parser.y"
                                     {
			macro_AppendArg(&((yyval.macroArg)), strdup((yyvsp[0].string)));
		}
#line 3558 "src/asm/parser.c"
    break;

  case 100: /* compoundeq: "+="  */
#line 915 "src/asm/parser.y"
                              { (yyval.compoundEqual) = RPN_ADD; }
#line 3564 "src/asm/parser.c"
    break;

  case 101: /* compoundeq: "-="  */
#line 916 "src/asm/parser.y"
                              { (yyval.compoundEqual) = RPN_SUB; }
#line 3570 "src/asm/parser.c"
    break;

  case 102: /* compoundeq: "*="  */
#line 917 "src/asm/parser.y"
                              { (yyval.compoundEqual) = RPN_MUL; }
#line 3576 "src/asm/parser.c"
    break;

  case 103: /* compoundeq: "/="  */
#line 918 "src/asm/parser.y"
                              { (yyval.compoundEqual) = RPN_DIV; }
#line 3582 "src/asm/parser.c"
    break;

  case 104: /* compoundeq: "%="  */
#line 919 "src/asm/parser.y"
                              { (yyval.compoundEqual) = RPN_MOD; }
#line 3588 "src/asm/parser.c"
    break;

  case 105: /* compoundeq: "^="  */
#line 920 "src/asm/parser.y"
                              { (yyval.compoundEqual) = RPN_XOR; }
#line 3594 "src/asm/parser.c"
    break;

  case 106: /* compoundeq: "|="  */
#line 921 "src/asm/parser.y"
                             { (yyval.compoundEqual) = RPN_OR; }
#line 3600 "src/asm/parser.c"
    break;

  case 107: /* compoundeq: "&="  */
#line 922 "src/asm/parser.y"
                              { (yyval.compoundEqual) = RPN_AND; }
#line 3606 "src/asm/parser.c"
    break;

  case 108: /* compoundeq: "<<="  */
#line 923 "src/asm/parser.y"
                              { (yyval.compoundEqual) = RPN_SHL; }
#line 3612 "src/asm/parser.c"
    break;

  case 109: /* compoundeq: ">>="  */
#line 924 "src/asm/parser.y"
                              { (yyval.compoundEqual) = RPN_SHR; }
#line 3618 "src/asm/parser.c"
    break;

  case 110: /* equ: "label" "EQU" const  */
#line 927 "src/asm/parser.y"
                                          { sym_AddEqu((yyvsp[-2].symName), (yyvsp[0].constValue)); }
#line 3624 "src/asm/parser.c"
    break;

  case 111: /* assignment: "label" "=" const  */
#line 930 "src/asm/parser.y"
                                            { sym_AddVar((yyvsp[-2].symName), (yyvsp[0].constValue)); }
#line 3630 "src/asm/parser.c"
    break;

  case 112: /* assignment: "label" compoundeq const  */
#line 931 "src/asm/parser.y"
                                           { compoundAssignment((yyvsp[-2].symName), (yyvsp[-1].compoundEqual), (yyvsp[0].constValue)); }
#line 3636 "src/asm/parser.c"
    break;

  case 113: /* equs: "label" "EQUS" string  */
#line 934 "src/asm/parser.y"
                                            { sym_AddString((yyvsp[-2].symName), (yyvsp[0].string)); }
#line 3642 "src/asm/parser.c"
    break;

  case 114: /* rb: "label" "RB" rs_uconst  */
#line 937 "src/asm/parser.y"
                                             {
			sym_AddEqu((yyvsp[-2].symName), sym_GetConstantValue("_RS"));
			sym_AddVar("_RS", sym_GetConstantValue("_RS") + (yyvsp[0].constValue));
		}
#line 3651 "src/asm/parser.c"
    break;

  case 115: /* rw: "label" "RW" rs_uconst  */
#line 943 "src/asm/parser.y"
                                             {
			sym_AddEqu((yyvsp[-2].symName), sym_GetConstantValue("_RS"));
			sym_AddVar("_RS", sym_GetConstantValue("_RS") + 2 * (yyvsp[0].constValue));
		}
#line 3660 "src/asm/parser.c"
    break;

  case 116: /* rl: "label" "rl" rs_uconst  */
#line 949 "src/asm/parser.y"
                                             {
			sym_AddEqu((yyvsp[-2].symName), sym_GetConstantValue("_RS"));
			sym_AddVar("_RS", sym_GetConstantValue("_RS") + 4 * (yyvsp[0].constValue));
		}
#line 3669 "src/asm/parser.c"
    break;

  case 117: /* align: "ALIGN" uconst  */
#line 955 "src/asm/parser.y"
                                    {
			if ((yyvsp[0].constValue) > 16)
				error("Alignment must be between 0 and 16, not %u\n", (yyvsp[0].constValue));
			else
				sect_AlignPC((yyvsp[0].constValue), 0);
		}
#line 3680 "src/asm/parser.c"
    break;

  case 118: /* align: "ALIGN" uconst "," uconst  */
#line 961 "src/asm/parser.y"
                                                   {
			if ((yyvsp[-2].constValue) > 16)
				error("Alignment must be between 0 and 16, not %u\n", (yyvsp[-2].constValue));
			else if ((yyvsp[0].constValue) >= 1 << (yyvsp[-2].constValue))
				error("Offset must be between 0 and %u, not %u\n",
					(1 << (yyvsp[-2].constValue)) - 1, (yyvsp[0].constValue));
			else
				sect_AlignPC((yyvsp[-2].constValue), (yyvsp[0].constValue));
		}
#line 3694 "src/asm/parser.c"
    break;

  case 119: /* $@6: %empty  */
#line 972 "src/asm/parser.y"
                            {
			// Parsing 'opt_list' will restore the lexer's normal mode
			lexer_SetMode(LEXER_RAW);
		}
#line 3703 "src/asm/parser.c"
    break;

  case 123: /* opt_list_entry: "string"  */
#line 982 "src/asm/parser.y"
                           { opt_Parse((yyvsp[0].string)); }
#line 3709 "src/asm/parser.c"
    break;

  case 124: /* popo: "POPO"  */
#line 985 "src/asm/parser.y"
                             { opt_Pop(); }
#line 3715 "src/asm/parser.c"
    break;

  case 125: /* pusho: "PUSHO"  */
#line 988 "src/asm/parser.y"
                              { opt_Push(); }
#line 3721 "src/asm/parser.c"
    break;

  case 126: /* pops: "POPS"  */
#line 991 "src/asm/parser.y"
                             { sect_PopSection(); }
#line 3727 "src/asm/parser.c"
    break;

  case 127: /* pushs: "PUSHS"  */
#line 994 "src/asm/parser.y"
                              { sect_PushSection(); }
#line 3733 "src/asm/parser.c"
    break;

  case 128: /* fail: "FAIL" string  */
#line 997 "src/asm/parser.y"
                                    { fatalerror("%s\n", (yyvsp[0].string)); }
#line 3739 "src/asm/parser.c"
    break;

  case 129: /* warn: "WARN" string  */
#line 1000 "src/asm/parser.y"
                                    { warning(WARNING_USER, "%s\n", (yyvsp[0].string)); }
#line 3745 "src/asm/parser.c"
    break;

  case 130: /* assert_type: %empty  */
#line 1003 "src/asm/parser.y"
                         { (yyval.assertType) = ASSERT_ERROR; }
#line 3751 "src/asm/parser.c"
    break;

  case 131: /* assert_type: "WARN" ","  */
#line 1004 "src/asm/parser.y"
                                     { (yyval.assertType) = ASSERT_WARN; }
#line 3757 "src/asm/parser.c"
    break;

  case 132: /* assert_type: "FAIL" ","  */
#line 1005 "src/asm/parser.y"
                                     { (yyval.assertType) = ASSERT_ERROR; }
#line 3763 "src/asm/parser.c"
    break;

  case 133: /* assert_type: "FATAL" ","  */
#line 1006 "src/asm/parser.y"
                                      { (yyval.assertType) = ASSERT_FATAL; }
#line 3769 "src/asm/parser.c"
    break;

  case 134: /* assert: "ASSERT" assert_type relocexpr  */
#line 1009 "src/asm/parser.y"
                                                     {
			if (!rpn_isKnown(&(yyvsp[0].expr))) {
				if (!out_CreateAssert((yyvsp[-1].assertType), &(yyvsp[0].expr), "",
						      sect_GetOutputOffset()))
					error("Assertion creation failed: %s\n",
						strerror(errno));
			} else if ((yyvsp[0].expr).val == 0) {
				failAssert((yyvsp[-1].assertType));
			}
			rpn_Free(&(yyvsp[0].expr));
		}
#line 3785 "src/asm/parser.c"
    break;

  case 135: /* assert: "ASSERT" assert_type relocexpr "," string  */
#line 1020 "src/asm/parser.y"
                                                                    {
			if (!rpn_isKnown(&(yyvsp[-2].expr))) {
				if (!out_CreateAssert((yyvsp[-3].assertType), &(yyvsp[-2].expr), (yyvsp[0].string),
						      sect_GetOutputOffset()))
					error("Assertion creation failed: %s\n",
						strerror(errno));
			} else if ((yyvsp[-2].expr).val == 0) {
				failAssertMsg((yyvsp[-3].assertType), (yyvsp[0].string));
			}
			rpn_Free(&(yyvsp[-2].expr));
		}
#line 3801 "src/asm/parser.c"
    break;

  case 136: /* assert: "STATIC_ASSERT" assert_type const  */
#line 1031 "src/asm/parser.y"
                                                        {
			if ((yyvsp[0].constValue) == 0)
				failAssert((yyvsp[-1].assertType));
		}
#line 3810 "src/asm/parser.c"
    break;

  case 137: /* assert: "STATIC_ASSERT" assert_type const "," string  */
#line 1035 "src/asm/parser.y"
                                                                       {
			if ((yyvsp[-2].constValue) == 0)
				failAssertMsg((yyvsp[-3].assertType), (yyvsp[0].string));
		}
#line 3819 "src/asm/parser.c"
    break;

  case 138: /* shift: "SHIFT"  */
#line 1041 "src/asm/parser.y"
                              { macro_ShiftCurrentArgs(1); }
#line 3825 "src/asm/parser.c"
    break;

  case 139: /* shift: "SHIFT" const  */
#line 1042 "src/asm/parser.y"
                                    { macro_ShiftCurrentArgs((yyvsp[0].constValue)); }
#line 3831 "src/asm/parser.c"
    break;

  case 140: /* load: "LOAD" sectmod string "," sectiontype sectorg sectattrs  */
#line 1045 "src/asm/parser.y"
                                                                                  {
			sect_SetLoadSection((yyvsp[-4].string), (yyvsp[-2].constValue), (yyvsp[-1].constValue), &(yyvsp[0].sectSpec), (yyvsp[-5].sectMod));
		}
#line 3839 "src/asm/parser.c"
    break;

  case 141: /* load: "ENDL"  */
#line 1048 "src/asm/parser.y"
                             { sect_EndLoadSection(); }
#line 3845 "src/asm/parser.c"
    break;

  case 142: /* @7: %empty  */
#line 1051 "src/asm/parser.y"
                                              {
			(yyval.captureTerminated) = lexer_CaptureRept(&captureBody);
		}
#line 3853 "src/asm/parser.c"
    break;

  case 143: /* rept: "REPT" uconst "newline" @7 endofline  */
#line 1053 "src/asm/parser.y"
                            {
			if ((yyvsp[-1].captureTerminated))
				fstk_RunRept((yyvsp[-3].constValue), captureBody.lineNo, captureBody.body,
					     captureBody.size);
		}
#line 3863 "src/asm/parser.c"
    break;

  case 144: /* $@8: %empty  */
#line 1060 "src/asm/parser.y"
                            {
			lexer_ToggleStringExpansion(false);
		}
#line 3871 "src/asm/parser.c"
    break;

  case 145: /* $@9: %empty  */
#line 1062 "src/asm/parser.y"
                       {
			lexer_ToggleStringExpansion(true);
		}
#line 3879 "src/asm/parser.c"
    break;

  case 146: /* @10: %empty  */
#line 1064 "src/asm/parser.y"
                                             {
			(yyval.captureTerminated) = lexer_CaptureRept(&captureBody);
		}
#line 3887 "src/asm/parser.c"
    break;

  case 147: /* for: "FOR" $@8 "identifier" $@9 "," for_args "newline" @10 endofline  */
#line 1066 "src/asm/parser.y"
                            {
			if ((yyvsp[-1].captureTerminated))
				fstk_RunFor((yyvsp[-6].symName), (yyvsp[-3].forArgs).start, (yyvsp[-3].forArgs).stop, (yyvsp[-3].forArgs).step, captureBody.lineNo,
					    captureBody.body, captureBody.size);
		}
#line 3897 "src/asm/parser.c"
    break;

  case 148: /* for_args: const  */
#line 1072 "src/asm/parser.y"
                        {
			(yyval.forArgs).start = 0;
			(yyval.forArgs).stop = (yyvsp[0].constValue);
			(yyval.forArgs).step = 1;
		}
#line 3907 "src/asm/parser.c"
    break;

  case 149: /* for_args: const "," const  */
#line 1077 "src/asm/parser.y"
                                      {
			(yyval.forArgs).start = (yyvsp[-2].constValue);
			(yyval.forArgs).stop = (yyvsp[0].constValue);
			(yyval.forArgs).step = 1;
		}
#line 3917 "src/asm/parser.c"
    break;

  case 150: /* for_args: const "," const "," const  */
#line 1082 "src/asm/parser.y"
                                                    {
			(yyval.forArgs).start = (yyvsp[-4].constValue);
			(yyval.forArgs).stop = (yyvsp[-2].constValue);
			(yyval.forArgs).step = (yyvsp[0].constValue);
		}
#line 3927 "src/asm/parser.c"
    break;

  case 151: /* break: label "BREAK" endofline  */
#line 1089 "src/asm/parser.y"
                                              {
			if (fstk_Break())
				lexer_SetMode(LEXER_SKIP_TO_ENDR);
		}
#line 3936 "src/asm/parser.c"
    break;

  case 152: /* $@11: %empty  */
#line 1095 "src/asm/parser.y"
                              {
			lexer_ToggleStringExpansion(false);
		}
#line 3944 "src/asm/parser.c"
    break;

  case 153: /* $@12: %empty  */
#line 1097 "src/asm/parser.y"
                       {
			lexer_ToggleStringExpansion(true);
		}
#line 3952 "src/asm/parser.c"
    break;

  case 154: /* @13: %empty  */
#line 1099 "src/asm/parser.y"
                            {
			(yyval.captureTerminated) = lexer_CaptureMacroBody(&captureBody);
		}
#line 3960 "src/asm/parser.c"
    break;

  case 155: /* macrodef: "MACRO" $@11 "identifier" $@12 "newline" @13 endofline  */
#line 1101 "src/asm/parser.y"
                            {
			if ((yyvsp[-1].captureTerminated))
				sym_AddMacro((yyvsp[-4].symName), captureBody.lineNo, captureBody.body,
					     captureBody.size);
		}
#line 3970 "src/asm/parser.c"
    break;

  case 156: /* @14: %empty  */
#line 1106 "src/asm/parser.y"
                                                        {
			warning(WARNING_OBSOLETE, "`%s: MACRO` is deprecated; use `MACRO %s`\n", (yyvsp[-3].symName), (yyvsp[-3].symName));
			(yyval.captureTerminated) = lexer_CaptureMacroBody(&captureBody);
		}
#line 3979 "src/asm/parser.c"
    break;

  case 157: /* macrodef: "label" ":" "MACRO" "newline" @14 endofline  */
#line 1109 "src/asm/parser.y"
                            {
			if ((yyvsp[-1].captureTerminated))
				sym_AddMacro((yyvsp[-5].symName), captureBody.lineNo, captureBody.body,
					     captureBody.size);
		}
#line 3989 "src/asm/parser.c"
    break;

  case 158: /* rsset: "RSSET" uconst  */
#line 1116 "src/asm/parser.y"
                                     { sym_AddVar("_RS", (yyvsp[0].constValue)); }
#line 3995 "src/asm/parser.c"
    break;

  case 159: /* rsreset: "RSRESET"  */
#line 1119 "src/asm/parser.y"
                                { sym_AddVar("_RS", 0); }
#line 4001 "src/asm/parser.c"
    break;

  case 160: /* rs_uconst: %empty  */
#line 1122 "src/asm/parser.y"
                         { (yyval.constValue) = 1; }
#line 4007 "src/asm/parser.c"
    break;

  case 162: /* union: "UNION"  */
#line 1126 "src/asm/parser.y"
                              { sect_StartUnion(); }
#line 4013 "src/asm/parser.c"
    break;

  case 163: /* nextu: "NEXTU"  */
#line 1129 "src/asm/parser.y"
                              { sect_NextUnionMember(); }
#line 4019 "src/asm/parser.c"
    break;

  case 164: /* endu: "ENDU"  */
#line 1132 "src/asm/parser.y"
                             { sect_EndUnion(); }
#line 4025 "src/asm/parser.c"
    break;

  case 165: /* ds: "DS" uconst  */
#line 1135 "src/asm/parser.y"
                                  { sect_Skip((yyvsp[0].constValue), true); }
#line 4031 "src/asm/parser.c"
    break;

  case 166: /* ds: "DS" uconst "," ds_args trailing_comma  */
#line 1136 "src/asm/parser.y"
                                                                 {
			sect_RelBytes((yyvsp[-3].constValue), (yyvsp[-1].dsArgs).args, (yyvsp[-1].dsArgs).nbArgs);
			freeDsArgList(&(yyvsp[-1].dsArgs));
		}
#line 4040 "src/asm/parser.c"
    break;

  case 167: /* ds_args: reloc_8bit  */
#line 1142 "src/asm/parser.y"
                             {
			initDsArgList(&(yyval.dsArgs));
			appendDsArgList(&(yyval.dsArgs), &(yyvsp[0].expr));
		}
#line 4049 "src/asm/parser.c"
    break;

  case 168: /* ds_args: ds_args "," reloc_8bit  */
#line 1146 "src/asm/parser.y"
                                             {
			appendDsArgList(&(yyvsp[-2].dsArgs), &(yyvsp[0].expr));
			(yyval.dsArgs) = (yyvsp[-2].dsArgs);
		}
#line 4058 "src/asm/parser.c"
    break;

  case 169: /* db: "DB"  */
#line 1152 "src/asm/parser.y"
                           { sect_Skip(1, false); }
#line 4064 "src/asm/parser.c"
    break;

  case 171: /* dw: "DW"  */
#line 1156 "src/asm/parser.y"
                           { sect_Skip(2, false); }
#line 4070 "src/asm/parser.c"
    break;

  case 173: /* dl: "DL"  */
#line 1160 "src/asm/parser.y"
                           { sect_Skip(4, false); }
#line 4076 "src/asm/parser.c"
    break;

  case 175: /* def_equ: def_id "EQU" const  */
#line 1164 "src/asm/parser.y"
                                         { sym_AddEqu((yyvsp[-2].symName), (yyvsp[0].constValue)); }
#line 4082 "src/asm/parser.c"
    break;

  case 176: /* redef_equ: redef_id "EQU" const  */
#line 1167 "src/asm/parser.y"
                                           { sym_RedefEqu((yyvsp[-2].symName), (yyvsp[0].constValue)); }
#line 4088 "src/asm/parser.c"
    break;

  case 177: /* def_set: def_id "=" const  */
#line 1170 "src/asm/parser.y"
                                           { sym_AddVar((yyvsp[-2].symName), (yyvsp[0].constValue)); }
#line 4094 "src/asm/parser.c"
    break;

  case 178: /* def_set: redef_id "=" const  */
#line 1171 "src/asm/parser.y"
                                             { sym_AddVar((yyvsp[-2].symName), (yyvsp[0].constValue)); }
#line 4100 "src/asm/parser.c"
    break;

  case 179: /* def_set: def_id compoundeq const  */
#line 1172 "src/asm/parser.y"
                                          { compoundAssignment((yyvsp[-2].symName), (yyvsp[-1].compoundEqual), (yyvsp[0].constValue)); }
#line 4106 "src/asm/parser.c"
    break;

  case 180: /* def_set: redef_id compoundeq const  */
#line 1173 "src/asm/parser.y"
                                            { compoundAssignment((yyvsp[-2].symName), (yyvsp[-1].compoundEqual), (yyvsp[0].constValue)); }
#line 4112 "src/asm/parser.c"
    break;

  case 181: /* def_rb: def_id "RB" rs_uconst  */
#line 1176 "src/asm/parser.y"
                                            {
			sym_AddEqu((yyvsp[-2].symName), sym_GetConstantValue("_RS"));
			sym_AddVar("_RS", sym_GetConstantValue("_RS") + (yyvsp[0].constValue));
		}
#line 4121 "src/asm/parser.c"
    break;

  case 182: /* def_rw: def_id "RW" rs_uconst  */
#line 1182 "src/asm/parser.y"
                                            {
			sym_AddEqu((yyvsp[-2].symName), sym_GetConstantValue("_RS"));
			sym_AddVar("_RS", sym_GetConstantValue("_RS") + 2 * (yyvsp[0].constValue));
		}
#line 4130 "src/asm/parser.c"
    break;

  case 183: /* def_rl: def_id "rl" rs_uconst  */
#line 1188 "src/asm/parser.y"
                                            {
			sym_AddEqu((yyvsp[-2].symName), sym_GetConstantValue("_RS"));
			sym_AddVar("_RS", sym_GetConstantValue("_RS") + 4 * (yyvsp[0].constValue));
		}
#line 4139 "src/asm/parser.c"
    break;

  case 184: /* def_equs: def_id "EQUS" string  */
#line 1194 "src/asm/parser.y"
                                           { sym_AddString((yyvsp[-2].symName), (yyvsp[0].string)); }
#line 4145 "src/asm/parser.c"
    break;

  case 185: /* redef_equs: redef_id "EQUS" string  */
#line 1197 "src/asm/parser.y"
                                             { sym_RedefString((yyvsp[-2].symName), (yyvsp[0].string)); }
#line 4151 "src/asm/parser.c"
    break;

  case 186: /* $@15: %empty  */
#line 1200 "src/asm/parser.y"
                              {
			lexer_ToggleStringExpansion(false);
		}
#line 4159 "src/asm/parser.c"
    break;

  case 187: /* purge: "PURGE" $@15 purge_list trailing_comma  */
#line 1202 "src/asm/parser.y"
                                            {
			lexer_ToggleStringExpansion(true);
		}
#line 4167 "src/asm/parser.c"
    break;

  case 190: /* purge_list_entry: scoped_id  */
#line 1211 "src/asm/parser.y"
                             { sym_Purge((yyvsp[0].symName)); }
#line 4173 "src/asm/parser.c"
    break;

  case 194: /* export_list_entry: scoped_id  */
#line 1221 "src/asm/parser.y"
                              { sym_Export((yyvsp[0].symName)); }
#line 4179 "src/asm/parser.c"
    break;

  case 195: /* include: label "INCLUDE" string endofline  */
#line 1224 "src/asm/parser.y"
                                                       {
			fstk_RunInclude((yyvsp[-1].string));
			if (failedOnMissingInclude)
				YYACCEPT;
		}
#line 4189 "src/asm/parser.c"
    break;

  case 196: /* incbin: "INCBIN" string  */
#line 1231 "src/asm/parser.y"
                                      {
			sect_BinaryFile((yyvsp[0].string), 0);
			if (failedOnMissingInclude)
				YYACCEPT;
		}
#line 4199 "src/asm/parser.c"
    break;

  case 197: /* incbin: "INCBIN" string "," const  */
#line 1236 "src/asm/parser.y"
                                                    {
			sect_BinaryFile((yyvsp[-2].string), (yyvsp[0].constValue));
			if (failedOnMissingInclude)
				YYACCEPT;
		}
#line 4209 "src/asm/parser.c"
    break;

  case 198: /* incbin: "INCBIN" string "," const "," const  */
#line 1241 "src/asm/parser.y"
                                                                  {
			sect_BinaryFileSlice((yyvsp[-4].string), (yyvsp[-2].constValue), (yyvsp[0].constValue));
			if (failedOnMissingInclude)
				YYACCEPT;
		}
#line 4219 "src/asm/parser.c"
    break;

  case 199: /* charmap: "CHARMAP" string "," const_8bit  */
#line 1248 "src/asm/parser.y"
                                                          {
			charmap_Add((yyvsp[-2].string), (uint8_t)(yyvsp[0].constValue));
		}
#line 4227 "src/asm/parser.c"
    break;

  case 200: /* newcharmap: "NEWCHARMAP" "identifier"  */
#line 1253 "src/asm/parser.y"
                                        { charmap_New((yyvsp[0].symName), NULL); }
#line 4233 "src/asm/parser.c"
    break;

  case 201: /* newcharmap: "NEWCHARMAP" "identifier" "," "identifier"  */
#line 1254 "src/asm/parser.y"
                                                     { charmap_New((yyvsp[-2].symName), (yyvsp[0].symName)); }
#line 4239 "src/asm/parser.c"
    break;

  case 202: /* setcharmap: "SETCHARMAP" "identifier"  */
#line 1257 "src/asm/parser.y"
                                        { charmap_Set((yyvsp[0].symName)); }
#line 4245 "src/asm/parser.c"
    break;

  case 203: /* pushc: "PUSHC"  */
#line 1260 "src/asm/parser.y"
                              { charmap_Push(); }
#line 4251 "src/asm/parser.c"
    break;

  case 204: /* popc: "POPC"  */
#line 1263 "src/asm/parser.y"
                             { charmap_Pop(); }
#line 4257 "src/asm/parser.c"
    break;

  case 206: /* println: "PRINTLN"  */
#line 1269 "src/asm/parser.y"
                                {
			putchar('\n');
			fflush(stdout);
		}
#line 4266 "src/asm/parser.c"
    break;

  case 207: /* println: "PRINTLN" print_exprs trailing_comma  */
#line 1273 "src/asm/parser.y"
                                                           {
			putchar('\n');
			fflush(stdout);
		}
#line 4275 "src/asm/parser.c"
    break;

  case 210: /* print_expr: const_no_str  */
#line 1283 "src/asm/parser.y"
                               { printf("$%" PRIX32, (yyvsp[0].constValue)); }
#line 4281 "src/asm/parser.c"
    break;

  case 211: /* print_expr: string  */
#line 1284 "src/asm/parser.y"
                         { printf("%s", (yyvsp[0].string)); }
#line 4287 "src/asm/parser.c"
    break;

  case 212: /* const_3bit: const  */
#line 1287 "src/asm/parser.y"
                        {
			int32_t value = (yyvsp[0].constValue);

			if ((value < 0) || (value > 7)) {
				error("Immediate value must be 3-bit\n");
				(yyval.constValue) = 0;
			} else {
				(yyval.constValue) = value & 0x7;
			}
		}
#line 4302 "src/asm/parser.c"
    break;

  case 215: /* constlist_8bit_entry: reloc_8bit_no_str  */
#line 1303 "src/asm/parser.y"
                                         {
			sect_RelByte(&(yyvsp[0].expr), 0);
		}
#line 4310 "src/asm/parser.c"
    break;

  case 216: /* constlist_8bit_entry: string  */
#line 1306 "src/asm/parser.y"
                         {
			uint8_t *output = malloc(strlen((yyvsp[0].string))); // Cannot be larger than that
			size_t length = charmap_Convert((yyvsp[0].string), output);

			sect_AbsByteGroup(output, length);
			free(output);
		}
#line 4322 "src/asm/parser.c"
    break;

  case 219: /* constlist_16bit_entry: reloc_16bit_no_str  */
#line 1319 "src/asm/parser.y"
                                           {
			sect_RelWord(&(yyvsp[0].expr), 0);
		}
#line 4330 "src/asm/parser.c"
    break;

  case 220: /* constlist_16bit_entry: string  */
#line 1322 "src/asm/parser.y"
                         {
			uint8_t *output = malloc(strlen((yyvsp[0].string))); // Cannot be larger than that
			size_t length = charmap_Convert((yyvsp[0].string), output);

			sect_AbsWordGroup(output, length);
			free(output);
		}
#line 4342 "src/asm/parser.c"
    break;

  case 223: /* constlist_32bit_entry: relocexpr_no_str  */
#line 1335 "src/asm/parser.y"
                                         {
			sect_RelLong(&(yyvsp[0].expr), 0);
		}
#line 4350 "src/asm/parser.c"
    break;

  case 224: /* constlist_32bit_entry: string  */
#line 1338 "src/asm/parser.y"
                         {
			// Charmaps cannot increase the length of a string
			uint8_t *output = malloc(strlen((yyvsp[0].string)));
			size_t length = charmap_Convert((yyvsp[0].string), output);

			sect_AbsLongGroup(output, length);
			free(output);
		}
#line 4363 "src/asm/parser.c"
    break;

  case 225: /* reloc_8bit: relocexpr  */
#line 1348 "src/asm/parser.y"
                            {
			rpn_CheckNBit(&(yyvsp[0].expr), 8);
			(yyval.expr) = (yyvsp[0].expr);
		}
#line 4372 "src/asm/parser.c"
    break;

  case 226: /* reloc_8bit_no_str: relocexpr_no_str  */
#line 1354 "src/asm/parser.y"
                                     {
			rpn_CheckNBit(&(yyvsp[0].expr), 8);
			(yyval.expr) = (yyvsp[0].expr);
		}
#line 4381 "src/asm/parser.c"
    break;

  case 227: /* reloc_8bit_offset: "+" relocexpr  */
#line 1360 "src/asm/parser.y"
                                       {
			rpn_CheckNBit(&(yyvsp[0].expr), 8);
			(yyval.expr) = (yyvsp[0].expr);
		}
#line 4390 "src/asm/parser.c"
    break;

  case 228: /* reloc_8bit_offset: "-" relocexpr  */
#line 1364 "src/asm/parser.y"
                                     {
			rpn_UNNEG(&(yyval.expr), &(yyvsp[0].expr));
			rpn_CheckNBit(&(yyval.expr), 8);
		}
#line 4399 "src/asm/parser.c"
    break;

  case 229: /* reloc_16bit: relocexpr  */
#line 1370 "src/asm/parser.y"
                            {
			rpn_CheckNBit(&(yyvsp[0].expr), 16);
			(yyval.expr) = (yyvsp[0].expr);
		}
#line 4408 "src/asm/parser.c"
    break;

  case 230: /* reloc_16bit_no_str: relocexpr_no_str  */
#line 1376 "src/asm/parser.y"
                                      {
			rpn_CheckNBit(&(yyvsp[0].expr), 16);
			(yyval.expr) = (yyvsp[0].expr);
		}
#line 4417 "src/asm/parser.c"
    break;

  case 232: /* relocexpr: string  */
#line 1384 "src/asm/parser.y"
                         {
			// Charmaps cannot increase the length of a string
			uint8_t *output = malloc(strlen((yyvsp[0].string)));
			uint32_t length = charmap_Convert((yyvsp[0].string), output);
			uint32_t r = str2int2(output, length);

			free(output);
			rpn_Number(&(yyval.expr), r);
		}
#line 4431 "src/asm/parser.c"
    break;

  case 233: /* relocexpr_no_str: scoped_anon_id  */
#line 1395 "src/asm/parser.y"
                                  { rpn_Symbol(&(yyval.expr), (yyvsp[0].symName)); }
#line 4437 "src/asm/parser.c"
    break;

  case 234: /* relocexpr_no_str: "number"  */
#line 1396 "src/asm/parser.y"
                           { rpn_Number(&(yyval.expr), (yyvsp[0].constValue)); }
#line 4443 "src/asm/parser.c"
    break;

  case 235: /* relocexpr_no_str: "!" relocexpr  */
#line 1397 "src/asm/parser.y"
                                                    {
			rpn_LOGNOT(&(yyval.expr), &(yyvsp[0].expr));
		}
#line 4451 "src/asm/parser.c"
    break;

  case 236: /* relocexpr_no_str: relocexpr "||" relocexpr  */
#line 1400 "src/asm/parser.y"
                                                   {
			rpn_BinaryOp(RPN_LOGOR, &(yyval.expr), &(yyvsp[-2].expr), &(yyvsp[0].expr));
		}
#line 4459 "src/asm/parser.c"
    break;

  case 237: /* relocexpr_no_str: relocexpr "&&" relocexpr  */
#line 1403 "src/asm/parser.y"
                                                    {
			rpn_BinaryOp(RPN_LOGAND, &(yyval.expr), &(yyvsp[-2].expr), &(yyvsp[0].expr));
		}
#line 4467 "src/asm/parser.c"
    break;

  case 238: /* relocexpr_no_str: relocexpr "==" relocexpr  */
#line 1406 "src/asm/parser.y"
                                                    {
			rpn_BinaryOp(RPN_LOGEQ, &(yyval.expr), &(yyvsp[-2].expr), &(yyvsp[0].expr));
		}
#line 4475 "src/asm/parser.c"
    break;

  case 239: /* relocexpr_no_str: relocexpr ">" relocexpr  */
#line 1409 "src/asm/parser.y"
                                                   {
			rpn_BinaryOp(RPN_LOGGT, &(yyval.expr), &(yyvsp[-2].expr), &(yyvsp[0].expr));
		}
#line 4483 "src/asm/parser.c"
    break;

  case 240: /* relocexpr_no_str: relocexpr "<" relocexpr  */
#line 1412 "src/asm/parser.y"
                                                   {
			rpn_BinaryOp(RPN_LOGLT, &(yyval.expr), &(yyvsp[-2].expr), &(yyvsp[0].expr));
		}
#line 4491 "src/asm/parser.c"
    break;

  case 241: /* relocexpr_no_str: relocexpr ">=" relocexpr  */
#line 1415 "src/asm/parser.y"
                                                   {
			rpn_BinaryOp(RPN_LOGGE, &(yyval.expr), &(yyvsp[-2].expr), &(yyvsp[0].expr));
		}
#line 4499 "src/asm/parser.c"
    break;

  case 242: /* relocexpr_no_str: relocexpr "<=" relocexpr  */
#line 1418 "src/asm/parser.y"
                                                   {
			rpn_BinaryOp(RPN_LOGLE, &(yyval.expr), &(yyvsp[-2].expr), &(yyvsp[0].expr));
		}
#line 4507 "src/asm/parser.c"
    break;

  case 243: /* relocexpr_no_str: relocexpr "!=" relocexpr  */
#line 1421 "src/asm/parser.y"
                                                   {
			rpn_BinaryOp(RPN_LOGNE, &(yyval.expr), &(yyvsp[-2].expr), &(yyvsp[0].expr));
		}
#line 4515 "src/asm/parser.c"
    break;

  case 244: /* relocexpr_no_str: relocexpr "+" relocexpr  */
#line 1424 "src/asm/parser.y"
                                               {
			rpn_BinaryOp(RPN_ADD, &(yyval.expr), &(yyvsp[-2].expr), &(yyvsp[0].expr));
		}
#line 4523 "src/asm/parser.c"
    break;

  case 245: /* relocexpr_no_str: relocexpr "-" relocexpr  */
#line 1427 "src/asm/parser.y"
                                               {
			rpn_BinaryOp(RPN_SUB, &(yyval.expr), &(yyvsp[-2].expr), &(yyvsp[0].expr));
		}
#line 4531 "src/asm/parser.c"
    break;

  case 246: /* relocexpr_no_str: relocexpr "^" relocexpr  */
#line 1430 "src/asm/parser.y"
                                               {
			rpn_BinaryOp(RPN_XOR, &(yyval.expr), &(yyvsp[-2].expr), &(yyvsp[0].expr));
		}
#line 4539 "src/asm/parser.c"
    break;

  case 247: /* relocexpr_no_str: relocexpr "|" relocexpr  */
#line 1433 "src/asm/parser.y"
                                              {
			rpn_BinaryOp(RPN_OR, &(yyval.expr), &(yyvsp[-2].expr), &(yyvsp[0].expr));
		}
#line 4547 "src/asm/parser.c"
    break;

  case 248: /* relocexpr_no_str: relocexpr "&" relocexpr  */
#line 1436 "src/asm/parser.y"
                                               {
			rpn_BinaryOp(RPN_AND, &(yyval.expr), &(yyvsp[-2].expr), &(yyvsp[0].expr));
		}
#line 4555 "src/asm/parser.c"
    break;

  case 249: /* relocexpr_no_str: relocexpr "<<" relocexpr  */
#line 1439 "src/asm/parser.y"
                                               {
			rpn_BinaryOp(RPN_SHL, &(yyval.expr), &(yyvsp[-2].expr), &(yyvsp[0].expr));
		}
#line 4563 "src/asm/parser.c"
    break;

  case 250: /* relocexpr_no_str: relocexpr ">>" relocexpr  */
#line 1442 "src/asm/parser.y"
                                               {
			rpn_BinaryOp(RPN_SHR, &(yyval.expr), &(yyvsp[-2].expr), &(yyvsp[0].expr));
		}
#line 4571 "src/asm/parser.c"
    break;

  case 251: /* relocexpr_no_str: relocexpr ">>>" relocexpr  */
#line 1445 "src/asm/parser.y"
                                                {
			rpn_BinaryOp(RPN_USHR, &(yyval.expr), &(yyvsp[-2].expr), &(yyvsp[0].expr));
		}
#line 4579 "src/asm/parser.c"
    break;

  case 252: /* relocexpr_no_str: relocexpr "*" relocexpr  */
#line 1448 "src/asm/parser.y"
                                               {
			rpn_BinaryOp(RPN_MUL, &(yyval.expr), &(yyvsp[-2].expr), &(yyvsp[0].expr));
		}
#line 4587 "src/asm/parser.c"
    break;

  case 253: /* relocexpr_no_str: relocexpr "/" relocexpr  */
#line 1451 "src/asm/parser.y"
                                               {
			rpn_BinaryOp(RPN_DIV, &(yyval.expr), &(yyvsp[-2].expr), &(yyvsp[0].expr));
		}
#line 4595 "src/asm/parser.c"
    break;

  case 254: /* relocexpr_no_str: relocexpr "%" relocexpr  */
#line 1454 "src/asm/parser.y"
                                               {
			rpn_BinaryOp(RPN_MOD, &(yyval.expr), &(yyvsp[-2].expr), &(yyvsp[0].expr));
		}
#line 4603 "src/asm/parser.c"
    break;

  case 255: /* relocexpr_no_str: relocexpr "**" relocexpr  */
#line 1457 "src/asm/parser.y"
                                               {
			rpn_BinaryOp(RPN_EXP, &(yyval.expr), &(yyvsp[-2].expr), &(yyvsp[0].expr));
		}
#line 4611 "src/asm/parser.c"
    break;

  case 256: /* relocexpr_no_str: "+" relocexpr  */
#line 1460 "src/asm/parser.y"
                                               { (yyval.expr) = (yyvsp[0].expr); }
#line 4617 "src/asm/parser.c"
    break;

  case 257: /* relocexpr_no_str: "-" relocexpr  */
#line 1461 "src/asm/parser.y"
                                               { rpn_UNNEG(&(yyval.expr), &(yyvsp[0].expr)); }
#line 4623 "src/asm/parser.c"
    break;

  case 258: /* relocexpr_no_str: "~" relocexpr  */
#line 1462 "src/asm/parser.y"
                                               { rpn_UNNOT(&(yyval.expr), &(yyvsp[0].expr)); }
#line 4629 "src/asm/parser.c"
    break;

  case 259: /* relocexpr_no_str: "HIGH" "(" relocexpr ")"  */
#line 1463 "src/asm/parser.y"
                                                        { rpn_HIGH(&(yyval.expr), &(yyvsp[-1].expr)); }
#line 4635 "src/asm/parser.c"
    break;

  case 260: /* relocexpr_no_str: "LOW" "(" relocexpr ")"  */
#line 1464 "src/asm/parser.y"
                                                       { rpn_LOW(&(yyval.expr), &(yyvsp[-1].expr)); }
#line 4641 "src/asm/parser.c"
    break;

  case 261: /* relocexpr_no_str: "ISCONST" "(" relocexpr ")"  */
#line 1465 "src/asm/parser.y"
                                                           { rpn_ISCONST(&(yyval.expr), &(yyvsp[-1].expr)); }
#line 4647 "src/asm/parser.c"
    break;

  case 262: /* relocexpr_no_str: "BANK" "(" scoped_anon_id ")"  */
#line 1466 "src/asm/parser.y"
                                                             {
			// '@' is also a T_ID; it is handled here
			rpn_BankSymbol(&(yyval.expr), (yyvsp[-1].symName));
		}
#line 4656 "src/asm/parser.c"
    break;

  case 263: /* relocexpr_no_str: "BANK" "(" string ")"  */
#line 1470 "src/asm/parser.y"
                                                     { rpn_BankSection(&(yyval.expr), (yyvsp[-1].string)); }
#line 4662 "src/asm/parser.c"
    break;

  case 264: /* relocexpr_no_str: "SIZEOF" "(" string ")"  */
#line 1471 "src/asm/parser.y"
                                                       { rpn_SizeOfSection(&(yyval.expr), (yyvsp[-1].string)); }
#line 4668 "src/asm/parser.c"
    break;

  case 265: /* relocexpr_no_str: "STARTOF" "(" string ")"  */
#line 1472 "src/asm/parser.y"
                                                        { rpn_StartOfSection(&(yyval.expr), (yyvsp[-1].string)); }
#line 4674 "src/asm/parser.c"
    break;

  case 266: /* $@16: %empty  */
#line 1473 "src/asm/parser.y"
                           {
			lexer_ToggleStringExpansion(false);
		}
#line 4682 "src/asm/parser.c"
    break;

  case 267: /* relocexpr_no_str: "DEF" $@16 "(" scoped_anon_id ")"  */
#line 1475 "src/asm/parser.y"
                                                   {
			rpn_Number(&(yyval.expr), sym_FindScopedValidSymbol((yyvsp[-1].symName)) != NULL);

			lexer_ToggleStringExpansion(true);
		}
#line 4692 "src/asm/parser.c"
    break;

  case 268: /* relocexpr_no_str: "ROUND" "(" const opt_q_arg ")"  */
#line 1480 "src/asm/parser.y"
                                                               {
			rpn_Number(&(yyval.expr), fix_Round((yyvsp[-2].constValue), (yyvsp[-1].constValue)));
		}
#line 4700 "src/asm/parser.c"
    break;

  case 269: /* relocexpr_no_str: "CEIL" "(" const opt_q_arg ")"  */
#line 1483 "src/asm/parser.y"
                                                              {
			rpn_Number(&(yyval.expr), fix_Ceil((yyvsp[-2].constValue), (yyvsp[-1].constValue)));
		}
#line 4708 "src/asm/parser.c"
    break;

  case 270: /* relocexpr_no_str: "FLOOR" "(" const opt_q_arg ")"  */
#line 1486 "src/asm/parser.y"
                                                               {
			rpn_Number(&(yyval.expr), fix_Floor((yyvsp[-2].constValue), (yyvsp[-1].constValue)));
		}
#line 4716 "src/asm/parser.c"
    break;

  case 271: /* relocexpr_no_str: "FDIV" "(" const "," const opt_q_arg ")"  */
#line 1489 "src/asm/parser.y"
                                                                            {
			rpn_Number(&(yyval.expr), fix_Div((yyvsp[-4].constValue), (yyvsp[-2].constValue), (yyvsp[-1].constValue)));
		}
#line 4724 "src/asm/parser.c"
    break;

  case 272: /* relocexpr_no_str: "FMUL" "(" const "," const opt_q_arg ")"  */
#line 1492 "src/asm/parser.y"
                                                                            {
			rpn_Number(&(yyval.expr), fix_Mul((yyvsp[-4].constValue), (yyvsp[-2].constValue), (yyvsp[-1].constValue)));
		}
#line 4732 "src/asm/parser.c"
    break;

  case 273: /* relocexpr_no_str: "FMOD" "(" const "," const opt_q_arg ")"  */
#line 1495 "src/asm/parser.y"
                                                                            {
			rpn_Number(&(yyval.expr), fix_Mod((yyvsp[-4].constValue), (yyvsp[-2].constValue), (yyvsp[-1].constValue)));
		}
#line 4740 "src/asm/parser.c"
    break;

  case 274: /* relocexpr_no_str: "POW" "(" const "," const opt_q_arg ")"  */
#line 1498 "src/asm/parser.y"
                                                                           {
			rpn_Number(&(yyval.expr), fix_Pow((yyvsp[-4].constValue), (yyvsp[-2].constValue), (yyvsp[-1].constValue)));
		}
#line 4748 "src/asm/parser.c"
    break;

  case 275: /* relocexpr_no_str: "LOG" "(" const "," const opt_q_arg ")"  */
#line 1501 "src/asm/parser.y"
                                                                           {
			rpn_Number(&(yyval.expr), fix_Log((yyvsp[-4].constValue), (yyvsp[-2].constValue), (yyvsp[-1].constValue)));
		}
#line 4756 "src/asm/parser.c"
    break;

  case 276: /* relocexpr_no_str: "SIN" "(" const opt_q_arg ")"  */
#line 1504 "src/asm/parser.y"
                                                             {
			rpn_Number(&(yyval.expr), fix_Sin((yyvsp[-2].constValue), (yyvsp[-1].constValue)));
		}
#line 4764 "src/asm/parser.c"
    break;

  case 277: /* relocexpr_no_str: "COS" "(" const opt_q_arg ")"  */
#line 1507 "src/asm/parser.y"
                                                             {
			rpn_Number(&(yyval.expr), fix_Cos((yyvsp[-2].constValue), (yyvsp[-1].constValue)));
		}
#line 4772 "src/asm/parser.c"
    break;

  case 278: /* relocexpr_no_str: "TAN" "(" const opt_q_arg ")"  */
#line 1510 "src/asm/parser.y"
                                                             {
			rpn_Number(&(yyval.expr), fix_Tan((yyvsp[-2].constValue), (yyvsp[-1].constValue)));
		}
#line 4780 "src/asm/parser.c"
    break;

  case 279: /* relocexpr_no_str: "ASIN" "(" const opt_q_arg ")"  */
#line 1513 "src/asm/parser.y"
                                                              {
			rpn_Number(&(yyval.expr), fix_ASin((yyvsp[-2].constValue), (yyvsp[-1].constValue)));
		}
#line 4788 "src/asm/parser.c"
    break;

  case 280: /* relocexpr_no_str: "ACOS" "(" const opt_q_arg ")"  */
#line 1516 "src/asm/parser.y"
                                                              {
			rpn_Number(&(yyval.expr), fix_ACos((yyvsp[-2].constValue), (yyvsp[-1].constValue)));
		}
#line 4796 "src/asm/parser.c"
    break;

  case 281: /* relocexpr_no_str: "ATAN" "(" const opt_q_arg ")"  */
#line 1519 "src/asm/parser.y"
                                                              {
			rpn_Number(&(yyval.expr), fix_ATan((yyvsp[-2].constValue), (yyvsp[-1].constValue)));
		}
#line 4804 "src/asm/parser.c"
    break;

  case 282: /* relocexpr_no_str: "ATAN2" "(" const "," const opt_q_arg ")"  */
#line 1522 "src/asm/parser.y"
                                                                             {
			rpn_Number(&(yyval.expr), fix_ATan2((yyvsp[-4].constValue), (yyvsp[-2].constValue), (yyvsp[-1].constValue)));
		}
#line 4812 "src/asm/parser.c"
    break;

  case 283: /* relocexpr_no_str: "STRCMP" "(" string "," string ")"  */
#line 1525 "src/asm/parser.y"
                                                                      {
			rpn_Number(&(yyval.expr), strcmp((yyvsp[-3].string), (yyvsp[-1].string)));
		}
#line 4820 "src/asm/parser.c"
    break;

  case 284: /* relocexpr_no_str: "STRIN" "(" string "," string ")"  */
#line 1528 "src/asm/parser.y"
                                                                     {
			char const *p = strstr((yyvsp[-3].string), (yyvsp[-1].string));

			rpn_Number(&(yyval.expr), p ? p - (yyvsp[-3].string) + 1 : 0);
		}
#line 4830 "src/asm/parser.c"
    break;

  case 285: /* relocexpr_no_str: "STRRIN" "(" string "," string ")"  */
#line 1533 "src/asm/parser.y"
                                                                      {
			char const *p = strrstr((yyvsp[-3].string), (yyvsp[-1].string));

			rpn_Number(&(yyval.expr), p ? p - (yyvsp[-3].string) + 1 : 0);
		}
#line 4840 "src/asm/parser.c"
    break;

  case 286: /* relocexpr_no_str: "STRLEN" "(" string ")"  */
#line 1538 "src/asm/parser.y"
                                                       {
			rpn_Number(&(yyval.expr), strlenUTF8((yyvsp[-1].string)));
		}
#line 4848 "src/asm/parser.c"
    break;

  case 287: /* relocexpr_no_str: "CHARLEN" "(" string ")"  */
#line 1541 "src/asm/parser.y"
                                                        {
			rpn_Number(&(yyval.expr), charlenUTF8((yyvsp[-1].string)));
		}
#line 4856 "src/asm/parser.c"
    break;

  case 288: /* relocexpr_no_str: "(" relocexpr ")"  */
#line 1544 "src/asm/parser.y"
                                              { (yyval.expr) = (yyvsp[-1].expr); }
#line 4862 "src/asm/parser.c"
    break;

  case 289: /* uconst: const  */
#line 1547 "src/asm/parser.y"
                        {
			(yyval.constValue) = (yyvsp[0].constValue);
			if ((yyval.constValue) < 0)
				fatalerror("Constant must not be negative: %d\n", (yyvsp[0].constValue));
		}
#line 4872 "src/asm/parser.c"
    break;

  case 290: /* const: relocexpr  */
#line 1554 "src/asm/parser.y"
                            { (yyval.constValue) = rpn_GetConstVal(&(yyvsp[0].expr)); }
#line 4878 "src/asm/parser.c"
    break;

  case 291: /* const_no_str: relocexpr_no_str  */
#line 1557 "src/asm/parser.y"
                                   { (yyval.constValue) = rpn_GetConstVal(&(yyvsp[0].expr)); }
#line 4884 "src/asm/parser.c"
    break;

  case 292: /* const_8bit: reloc_8bit  */
#line 1560 "src/asm/parser.y"
                             { (yyval.constValue) = rpn_GetConstVal(&(yyvsp[0].expr)); }
#line 4890 "src/asm/parser.c"
    break;

  case 293: /* opt_q_arg: %empty  */
#line 1563 "src/asm/parser.y"
                         { (yyval.constValue) = fix_Precision(); }
#line 4896 "src/asm/parser.c"
    break;

  case 294: /* opt_q_arg: "," const  */
#line 1564 "src/asm/parser.y"
                                {
			if ((yyvsp[0].constValue) >= 1 && (yyvsp[0].constValue) <= 31) {
				(yyval.constValue) = (yyvsp[0].constValue);
			} else {
				error("Fixed-point precision must be between 1 and 31\n");
				(yyval.constValue) = fix_Precision();
			}
		}
#line 4909 "src/asm/parser.c"
    break;

  case 296: /* string: "STRSUB" "(" string "," const "," uconst ")"  */
#line 1575 "src/asm/parser.y"
                                                                                    {
			size_t len = strlenUTF8((yyvsp[-5].string));
			uint32_t pos = adjustNegativePos((yyvsp[-3].constValue), len, "STRSUB");

			strsubUTF8((yyval.string), sizeof((yyval.string)), (yyvsp[-5].string), pos, (yyvsp[-1].constValue));
		}
#line 4920 "src/asm/parser.c"
    break;

  case 297: /* string: "STRSUB" "(" string "," const ")"  */
#line 1581 "src/asm/parser.y"
                                                                     {
			size_t len = strlenUTF8((yyvsp[-3].string));
			uint32_t pos = adjustNegativePos((yyvsp[-1].constValue), len, "STRSUB");

			strsubUTF8((yyval.string), sizeof((yyval.string)), (yyvsp[-3].string), pos, pos > len ? 0 : len + 1 - pos);
		}
#line 4931 "src/asm/parser.c"
    break;

  case 298: /* string: "CHARSUB" "(" string "," const ")"  */
#line 1587 "src/asm/parser.y"
                                                                      {
			size_t len = charlenUTF8((yyvsp[-3].string));
			uint32_t pos = adjustNegativePos((yyvsp[-1].constValue), len, "CHARSUB");

			charsubUTF8((yyval.string), (yyvsp[-3].string), pos);
		}
#line 4942 "src/asm/parser.c"
    break;

  case 299: /* string: "STRCAT" "(" ")"  */
#line 1593 "src/asm/parser.y"
                                                {
			(yyval.string)[0] = '\0';
		}
#line 4950 "src/asm/parser.c"
    break;

  case 300: /* string: "STRCAT" "(" strcat_args ")"  */
#line 1596 "src/asm/parser.y"
                                                            {
			strcpy((yyval.string), (yyvsp[-1].string));
		}
#line 4958 "src/asm/parser.c"
    break;

  case 301: /* string: "STRUPR" "(" string ")"  */
#line 1599 "src/asm/parser.y"
                                                       {
			upperstring((yyval.string), (yyvsp[-1].string));
		}
#line 4966 "src/asm/parser.c"
    break;

  case 302: /* string: "STRLWR" "(" string ")"  */
#line 1602 "src/asm/parser.y"
                                                       {
			lowerstring((yyval.string), (yyvsp[-1].string));
		}
#line 4974 "src/asm/parser.c"
    break;

  case 303: /* string: "STRRPL" "(" string "," string "," string ")"  */
#line 1605 "src/asm/parser.y"
                                                                                     {
			strrpl((yyval.string), sizeof((yyval.string)), (yyvsp[-5].string), (yyvsp[-3].string), (yyvsp[-1].string));
		}
#line 4982 "src/asm/parser.c"
    break;

  case 304: /* string: "STRFMT" "(" strfmt_args ")"  */
#line 1608 "src/asm/parser.y"
                                                            {
			strfmt((yyval.string), sizeof((yyval.string)), (yyvsp[-1].strfmtArgs).format, (yyvsp[-1].strfmtArgs).nbArgs, (yyvsp[-1].strfmtArgs).args);
			freeStrFmtArgList(&(yyvsp[-1].strfmtArgs));
		}
#line 4991 "src/asm/parser.c"
    break;

  case 305: /* string: "SECTION" "(" scoped_anon_id ")"  */
#line 1612 "src/asm/parser.y"
                                                                 {
			struct Symbol *sym = sym_FindScopedValidSymbol((yyvsp[-1].symName));

			if (!sym)
				fatalerror("Unknown symbol \"%s\"\n", (yyvsp[-1].symName));
			struct Section const *section = sym_GetSection(sym);

			if (!section)
				fatalerror("\"%s\" does not belong to any section\n", sym->name);
			// Section names are capped by rgbasm's maximum string length,
			// so this currently can't overflow.
			strcpy((yyval.string), section->name);
		}
#line 5009 "src/asm/parser.c"
    break;

  case 307: /* strcat_args: strcat_args "," string  */
#line 1628 "src/asm/parser.y"
                                             {
			int ret = snprintf((yyval.string), sizeof((yyval.string)), "%s%s", (yyvsp[-2].string), (yyvsp[0].string));

			if (ret == -1)
				fatalerror("snprintf error in STRCAT: %s\n", strerror(errno));
			else if ((unsigned int)ret >= sizeof((yyval.string)))
				warning(WARNING_LONG_STR, "STRCAT: String too long '%s%s'\n",
					(yyvsp[-2].string), (yyvsp[0].string));
		}
#line 5023 "src/asm/parser.c"
    break;

  case 308: /* strfmt_args: string strfmt_va_args  */
#line 1639 "src/asm/parser.y"
                                        {
			(yyval.strfmtArgs).format = strdup((yyvsp[-1].string));
			(yyval.strfmtArgs).capacity = (yyvsp[0].strfmtArgs).capacity;
			(yyval.strfmtArgs).nbArgs = (yyvsp[0].strfmtArgs).nbArgs;
			(yyval.strfmtArgs).args = (yyvsp[0].strfmtArgs).args;
		}
#line 5034 "src/asm/parser.c"
    break;

  case 309: /* strfmt_va_args: %empty  */
#line 1647 "src/asm/parser.y"
                         {
			initStrFmtArgList(&(yyval.strfmtArgs));
		}
#line 5042 "src/asm/parser.c"
    break;

  case 310: /* strfmt_va_args: strfmt_va_args "," const_no_str  */
#line 1650 "src/asm/parser.y"
                                                      {
			size_t i = nextStrFmtArgListIndex(&(yyvsp[-2].strfmtArgs));

			(yyvsp[-2].strfmtArgs).args[i].number = (yyvsp[0].constValue);
			(yyvsp[-2].strfmtArgs).args[i].isNumeric = true;
			(yyval.strfmtArgs) = (yyvsp[-2].strfmtArgs);
		}
#line 5054 "src/asm/parser.c"
    break;

  case 311: /* strfmt_va_args: strfmt_va_args "," string  */
#line 1657 "src/asm/parser.y"
                                                {
			size_t i = nextStrFmtArgListIndex(&(yyvsp[-2].strfmtArgs));

			(yyvsp[-2].strfmtArgs).args[i].string = strdup((yyvsp[0].string));
			(yyvsp[-2].strfmtArgs).args[i].isNumeric = false;
			(yyval.strfmtArgs) = (yyvsp[-2].strfmtArgs);
		}
#line 5066 "src/asm/parser.c"
    break;

  case 312: /* section: "SECTION" sectmod string "," sectiontype sectorg sectattrs  */
#line 1666 "src/asm/parser.y"
                                                                                     {
			sect_NewSection((yyvsp[-4].string), (yyvsp[-2].constValue), (yyvsp[-1].constValue), &(yyvsp[0].sectSpec), (yyvsp[-5].sectMod));
		}
#line 5074 "src/asm/parser.c"
    break;

  case 313: /* sectmod: %empty  */
#line 1671 "src/asm/parser.y"
                         { (yyval.sectMod) = SECTION_NORMAL; }
#line 5080 "src/asm/parser.c"
    break;

  case 314: /* sectmod: "UNION"  */
#line 1672 "src/asm/parser.y"
                              { (yyval.sectMod) = SECTION_UNION; }
#line 5086 "src/asm/parser.c"
    break;

  case 315: /* sectmod: "FRAGMENT"  */
#line 1673 "src/asm/parser.y"
                                 { (yyval.sectMod) = SECTION_FRAGMENT; }
#line 5092 "src/asm/parser.c"
    break;

  case 316: /* sectiontype: "WRAM0"  */
#line 1676 "src/asm/parser.y"
                               { (yyval.constValue) = SECTTYPE_WRAM0; }
#line 5098 "src/asm/parser.c"
    break;

  case 317: /* sectiontype: "VRAM"  */
#line 1677 "src/asm/parser.y"
                              { (yyval.constValue) = SECTTYPE_VRAM; }
#line 5104 "src/asm/parser.c"
    break;

  case 318: /* sectiontype: "ROMX"  */
#line 1678 "src/asm/parser.y"
                              { (yyval.constValue) = SECTTYPE_ROMX; }
#line 5110 "src/asm/parser.c"
    break;

  case 319: /* sectiontype: "ROM0"  */
#line 1679 "src/asm/parser.y"
                              { (yyval.constValue) = SECTTYPE_ROM0; }
#line 5116 "src/asm/parser.c"
    break;

  case 320: /* sectiontype: "HRAM"  */
#line 1680 "src/asm/parser.y"
                              { (yyval.constValue) = SECTTYPE_HRAM; }
#line 5122 "src/asm/parser.c"
    break;

  case 321: /* sectiontype: "WRAMX"  */
#line 1681 "src/asm/parser.y"
                               { (yyval.constValue) = SECTTYPE_WRAMX; }
#line 5128 "src/asm/parser.c"
    break;

  case 322: /* sectiontype: "SRAM"  */
#line 1682 "src/asm/parser.y"
                              { (yyval.constValue) = SECTTYPE_SRAM; }
#line 5134 "src/asm/parser.c"
    break;

  case 323: /* sectiontype: "OAM"  */
#line 1683 "src/asm/parser.y"
                             { (yyval.constValue) = SECTTYPE_OAM; }
#line 5140 "src/asm/parser.c"
    break;

  case 324: /* sectorg: %empty  */
#line 1686 "src/asm/parser.y"
                         { (yyval.constValue) = -1; }
#line 5146 "src/asm/parser.c"
    break;

  case 325: /* sectorg: "[" uconst "]"  */
#line 1687 "src/asm/parser.y"
                                           {
			if ((yyvsp[-1].constValue) < 0 || (yyvsp[-1].constValue) >= 0x10000) {
				error("Address $%x is not 16-bit\n", (yyvsp[-1].constValue));
				(yyval.constValue) = -1;
			} else {
				(yyval.constValue) = (yyvsp[-1].constValue);
			}
		}
#line 5159 "src/asm/parser.c"
    break;

  case 326: /* sectattrs: %empty  */
#line 1697 "src/asm/parser.y"
                         {
			(yyval.sectSpec).alignment = 0;
			(yyval.sectSpec).alignOfs = 0;
			(yyval.sectSpec).bank = -1;
		}
#line 5169 "src/asm/parser.c"
    break;

  case 327: /* sectattrs: sectattrs "," "ALIGN" "[" uconst "]"  */
#line 1702 "src/asm/parser.y"
                                                                        {
			(yyval.sectSpec).alignment = (yyvsp[-1].constValue);
		}
#line 5177 "src/asm/parser.c"
    break;

  case 328: /* sectattrs: sectattrs "," "ALIGN" "[" uconst "," uconst "]"  */
#line 1705 "src/asm/parser.y"
                                                                                       {
			(yyval.sectSpec).alignment = (yyvsp[-3].constValue);
			(yyval.sectSpec).alignOfs = (yyvsp[-1].constValue);
		}
#line 5186 "src/asm/parser.c"
    break;

  case 329: /* sectattrs: sectattrs "," "BANK" "[" uconst "]"  */
#line 1709 "src/asm/parser.y"
                                                                       {
			// We cannot check the validity of this now
			(yyval.sectSpec).bank = (yyvsp[-1].constValue);
		}
#line 5195 "src/asm/parser.c"
    break;

  case 376: /* z80_adc: "adc" op_a_n  */
#line 1764 "src/asm/parser.y"
                                   {
			sect_AbsByte(0xCE);
			sect_RelByte(&(yyvsp[0].expr), 1);
		}
#line 5204 "src/asm/parser.c"
    break;

  case 377: /* z80_adc: "adc" op_a_r  */
#line 1768 "src/asm/parser.y"
                                   { sect_AbsByte(0x88 | (yyvsp[0].constValue)); }
#line 5210 "src/asm/parser.c"
    break;

  case 378: /* z80_add: "add" op_a_n  */
#line 1771 "src/asm/parser.y"
                                   {
			sect_AbsByte(0xC6);
			sect_RelByte(&(yyvsp[0].expr), 1);
		}
#line 5219 "src/asm/parser.c"
    break;

  case 379: /* z80_add: "add" op_a_r  */
#line 1775 "src/asm/parser.y"
                                   { sect_AbsByte(0x80 | (yyvsp[0].constValue)); }
#line 5225 "src/asm/parser.c"
    break;

  case 380: /* z80_add: "add" "hl" "," reg_ss  */
#line 1776 "src/asm/parser.y"
                                                     { sect_AbsByte(0x09 | ((yyvsp[0].constValue) << 4)); }
#line 5231 "src/asm/parser.c"
    break;

  case 381: /* z80_add: "add" "sp" "," reloc_8bit  */
#line 1777 "src/asm/parser.y"
                                                         {
			sect_AbsByte(0xE8);
			sect_RelByte(&(yyvsp[0].expr), 1);
		}
#line 5240 "src/asm/parser.c"
    break;

  case 382: /* z80_and: "and" op_a_n  */
#line 1784 "src/asm/parser.y"
                                   {
			sect_AbsByte(0xE6);
			sect_RelByte(&(yyvsp[0].expr), 1);
		}
#line 5249 "src/asm/parser.c"
    break;

  case 383: /* z80_and: "and" op_a_r  */
#line 1788 "src/asm/parser.y"
                                   { sect_AbsByte(0xA0 | (yyvsp[0].constValue)); }
#line 5255 "src/asm/parser.c"
    break;

  case 384: /* z80_bit: "bit" const_3bit "," reg_r  */
#line 1791 "src/asm/parser.y"
                                                     {
			sect_AbsByte(0xCB);
			sect_AbsByte(0x40 | ((yyvsp[-2].constValue) << 3) | (yyvsp[0].constValue));
		}
#line 5264 "src/asm/parser.c"
    break;

  case 385: /* z80_call: "call" reloc_16bit  */
#line 1797 "src/asm/parser.y"
                                         {
			sect_AbsByte(0xCD);
			sect_RelWord(&(yyvsp[0].expr), 1);
		}
#line 5273 "src/asm/parser.c"
    break;

  case 386: /* z80_call: "call" ccode_expr "," reloc_16bit  */
#line 1801 "src/asm/parser.y"
                                                            {
			sect_AbsByte(0xC4 | ((yyvsp[-2].constValue) << 3));
			sect_RelWord(&(yyvsp[0].expr), 1);
		}
#line 5282 "src/asm/parser.c"
    break;

  case 387: /* z80_ccf: "ccf"  */
#line 1807 "src/asm/parser.y"
                            { sect_AbsByte(0x3F); }
#line 5288 "src/asm/parser.c"
    break;

  case 388: /* z80_cp: "cp" op_a_n  */
#line 1810 "src/asm/parser.y"
                                  {
			sect_AbsByte(0xFE);
			sect_RelByte(&(yyvsp[0].expr), 1);
		}
#line 5297 "src/asm/parser.c"
    break;

  case 389: /* z80_cp: "cp" op_a_r  */
#line 1814 "src/asm/parser.y"
                                  { sect_AbsByte(0xB8 | (yyvsp[0].constValue)); }
#line 5303 "src/asm/parser.c"
    break;

  case 390: /* z80_cpl: "cpl"  */
#line 1817 "src/asm/parser.y"
                            { sect_AbsByte(0x2F); }
#line 5309 "src/asm/parser.c"
    break;

  case 391: /* z80_daa: "daa"  */
#line 1820 "src/asm/parser.y"
                            { sect_AbsByte(0x27); }
#line 5315 "src/asm/parser.c"
    break;

  case 392: /* z80_dec: "dec" reg_r  */
#line 1823 "src/asm/parser.y"
                                  { sect_AbsByte(0x05 | ((yyvsp[0].constValue) << 3)); }
#line 5321 "src/asm/parser.c"
    break;

  case 393: /* z80_dec: "dec" reg_ss  */
#line 1824 "src/asm/parser.y"
                                   { sect_AbsByte(0x0B | ((yyvsp[0].constValue) << 4)); }
#line 5327 "src/asm/parser.c"
    break;

  case 394: /* z80_di: "di"  */
#line 1827 "src/asm/parser.y"
                           { sect_AbsByte(0xF3); }
#line 5333 "src/asm/parser.c"
    break;

  case 395: /* z80_ei: "ei"  */
#line 1830 "src/asm/parser.y"
                           { sect_AbsByte(0xFB); }
#line 5339 "src/asm/parser.c"
    break;

  case 396: /* z80_halt: "halt"  */
#line 1833 "src/asm/parser.y"
                             {
			sect_AbsByte(0x76);
			if (haltnop) {
				if (warnOnHaltNop) {
					warnOnHaltNop = false;
					warning(WARNING_OBSOLETE, "`nop` after `halt` will stop being the default; pass `-H` to opt into it\n");
				}
				sect_AbsByte(0x00);
			}
		}
#line 5354 "src/asm/parser.c"
    break;

  case 397: /* z80_inc: "inc" reg_r  */
#line 1845 "src/asm/parser.y"
                                  { sect_AbsByte(0x04 | ((yyvsp[0].constValue) << 3)); }
#line 5360 "src/asm/parser.c"
    break;

  case 398: /* z80_inc: "inc" reg_ss  */
#line 1846 "src/asm/parser.y"
                                   { sect_AbsByte(0x03 | ((yyvsp[0].constValue) << 4)); }
#line 5366 "src/asm/parser.c"
    break;

  case 399: /* z80_jp: "jp" reloc_16bit  */
#line 1849 "src/asm/parser.y"
                                       {
			sect_AbsByte(0xC3);
			sect_RelWord(&(yyvsp[0].expr), 1);
		}
#line 5375 "src/asm/parser.c"
    break;

  case 400: /* z80_jp: "jp" ccode_expr "," reloc_16bit  */
#line 1853 "src/asm/parser.y"
                                                          {
			sect_AbsByte(0xC2 | ((yyvsp[-2].constValue) << 3));
			sect_RelWord(&(yyvsp[0].expr), 1);
		}
#line 5384 "src/asm/parser.c"
    break;

  case 401: /* z80_jp: "jp" "hl"  */
#line 1857 "src/asm/parser.y"
                                     {
			sect_AbsByte(0xE9);
		}
#line 5392 "src/asm/parser.c"
    break;

  case 402: /* z80_jr: "jr" reloc_16bit  */
#line 1862 "src/asm/parser.y"
                                       {
			sect_AbsByte(0x18);
			sect_PCRelByte(&(yyvsp[0].expr), 1);
		}
#line 5401 "src/asm/parser.c"
    break;

  case 403: /* z80_jr: "jr" ccode_expr "," reloc_16bit  */
#line 1866 "src/asm/parser.y"
                                                          {
			sect_AbsByte(0x20 | ((yyvsp[-2].constValue) << 3));
			sect_PCRelByte(&(yyvsp[0].expr), 1);
		}
#line 5410 "src/asm/parser.c"
    break;

  case 404: /* z80_ldi: "ldi" "[" "hl" "]" "," T_MODE_A  */
#line 1872 "src/asm/parser.y"
                                                                         {
			sect_AbsByte(0x02 | (2 << 4));
		}
#line 5418 "src/asm/parser.c"
    break;

  case 405: /* z80_ldi: "ldi" T_MODE_A "," "[" "hl" "]"  */
#line 1875 "src/asm/parser.y"
                                                                         {
			sect_AbsByte(0x0A | (2 << 4));
		}
#line 5426 "src/asm/parser.c"
    break;

  case 406: /* z80_ldd: "ldd" "[" "hl" "]" "," T_MODE_A  */
#line 1880 "src/asm/parser.y"
                                                                         {
			sect_AbsByte(0x02 | (3 << 4));
		}
#line 5434 "src/asm/parser.c"
    break;

  case 407: /* z80_ldd: "ldd" T_MODE_A "," "[" "hl" "]"  */
#line 1883 "src/asm/parser.y"
                                                                         {
			sect_AbsByte(0x0A | (3 << 4));
		}
#line 5442 "src/asm/parser.c"
    break;

  case 408: /* z80_ldio: "ldh" T_MODE_A "," op_mem_ind  */
#line 1888 "src/asm/parser.y"
                                                        {
			rpn_CheckHRAM(&(yyvsp[0].expr), &(yyvsp[0].expr));

			sect_AbsByte(0xF0);
			sect_RelByte(&(yyvsp[0].expr), 1);
		}
#line 5453 "src/asm/parser.c"
    break;

  case 409: /* z80_ldio: "ldh" op_mem_ind "," T_MODE_A  */
#line 1894 "src/asm/parser.y"
                                                        {
			rpn_CheckHRAM(&(yyvsp[-2].expr), &(yyvsp[-2].expr));

			sect_AbsByte(0xE0);
			sect_RelByte(&(yyvsp[-2].expr), 1);
		}
#line 5464 "src/asm/parser.c"
    break;

  case 410: /* z80_ldio: "ldh" T_MODE_A "," c_ind  */
#line 1900 "src/asm/parser.y"
                                                   {
			sect_AbsByte(0xF2);
		}
#line 5472 "src/asm/parser.c"
    break;

  case 411: /* z80_ldio: "ldh" c_ind "," T_MODE_A  */
#line 1903 "src/asm/parser.y"
                                                   {
			sect_AbsByte(0xE2);
		}
#line 5480 "src/asm/parser.c"
    break;

  case 413: /* c_ind: "[" relocexpr "+" T_MODE_C "]"  */
#line 1909 "src/asm/parser.y"
                                                                {
			if (!rpn_isKnown(&(yyvsp[-3].expr)) || (yyvsp[-3].expr).val != 0xff00)
				error("Expected constant expression equal to $FF00 for \"$ff00+c\"\n");
		}
#line 5489 "src/asm/parser.c"
    break;

  case 422: /* z80_ld_hl: "ld" "hl" "," "sp" reloc_8bit_offset  */
#line 1925 "src/asm/parser.y"
                                                                         {
			sect_AbsByte(0xF8);
			sect_RelByte(&(yyvsp[0].expr), 1);
		}
#line 5498 "src/asm/parser.c"
    break;

  case 423: /* z80_ld_hl: "ld" "hl" "," reloc_16bit  */
#line 1929 "src/asm/parser.y"
                                                         {
			sect_AbsByte(0x01 | (REG_HL << 4));
			sect_RelWord(&(yyvsp[0].expr), 1);
		}
#line 5507 "src/asm/parser.c"
    break;

  case 424: /* z80_ld_sp: "ld" "sp" "," "hl"  */
#line 1935 "src/asm/parser.y"
                                                       { sect_AbsByte(0xF9); }
#line 5513 "src/asm/parser.c"
    break;

  case 425: /* z80_ld_sp: "ld" "sp" "," reloc_16bit  */
#line 1936 "src/asm/parser.y"
                                                         {
			sect_AbsByte(0x01 | (REG_SP << 4));
			sect_RelWord(&(yyvsp[0].expr), 1);
		}
#line 5522 "src/asm/parser.c"
    break;

  case 426: /* z80_ld_mem: "ld" op_mem_ind "," "sp"  */
#line 1942 "src/asm/parser.y"
                                                        {
			sect_AbsByte(0x08);
			sect_RelWord(&(yyvsp[-2].expr), 1);
		}
#line 5531 "src/asm/parser.c"
    break;

  case 427: /* z80_ld_mem: "ld" op_mem_ind "," T_MODE_A  */
#line 1946 "src/asm/parser.y"
                                                       {
			if (optimizeLoads && rpn_isKnown(&(yyvsp[-2].expr))
			 && (yyvsp[-2].expr).val >= 0xFF00) {
				if (warnOnLdOpt) {
					warnOnLdOpt = false;
					warning(WARNING_OBSOLETE, "ld optimization will stop being the default; pass `-l` to opt into it\n");
				}
				sect_AbsByte(0xE0);
				sect_AbsByte((yyvsp[-2].expr).val & 0xFF);
				rpn_Free(&(yyvsp[-2].expr));
			} else {
				sect_AbsByte(0xEA);
				sect_RelWord(&(yyvsp[-2].expr), 1);
			}
		}
#line 5551 "src/asm/parser.c"
    break;

  case 428: /* z80_ld_cind: "ld" c_ind "," T_MODE_A  */
#line 1963 "src/asm/parser.y"
                                                  {
			sect_AbsByte(0xE2);
		}
#line 5559 "src/asm/parser.c"
    break;

  case 429: /* z80_ld_rr: "ld" reg_rr "," T_MODE_A  */
#line 1968 "src/asm/parser.y"
                                                   {
			sect_AbsByte(0x02 | ((yyvsp[-2].constValue) << 4));
		}
#line 5567 "src/asm/parser.c"
    break;

  case 430: /* z80_ld_r: "ld" reg_r "," reloc_8bit  */
#line 1973 "src/asm/parser.y"
                                                    {
			sect_AbsByte(0x06 | ((yyvsp[-2].constValue) << 3));
			sect_RelByte(&(yyvsp[0].expr), 1);
		}
#line 5576 "src/asm/parser.c"
    break;

  case 431: /* z80_ld_r: "ld" reg_r "," reg_r  */
#line 1977 "src/asm/parser.y"
                                               {
			if (((yyvsp[-2].constValue) == REG_HL_IND) && ((yyvsp[0].constValue) == REG_HL_IND))
				error("LD [HL],[HL] not a valid instruction\n");
			else
				sect_AbsByte(0x40 | ((yyvsp[-2].constValue) << 3) | (yyvsp[0].constValue));
		}
#line 5587 "src/asm/parser.c"
    break;

  case 432: /* z80_ld_a: "ld" reg_r "," c_ind  */
#line 1985 "src/asm/parser.y"
                                               {
			if ((yyvsp[-2].constValue) == REG_A)
				sect_AbsByte(0xF2);
			else
				error("Destination operand must be A\n");
		}
#line 5598 "src/asm/parser.c"
    break;

  case 433: /* z80_ld_a: "ld" reg_r "," reg_rr  */
#line 1991 "src/asm/parser.y"
                                                {
			if ((yyvsp[-2].constValue) == REG_A)
				sect_AbsByte(0x0A | ((yyvsp[0].constValue) << 4));
			else
				error("Destination operand must be A\n");
		}
#line 5609 "src/asm/parser.c"
    break;

  case 434: /* z80_ld_a: "ld" reg_r "," op_mem_ind  */
#line 1997 "src/asm/parser.y"
                                                    {
			if ((yyvsp[-2].constValue) == REG_A) {
				if (optimizeLoads && rpn_isKnown(&(yyvsp[0].expr))
				 && (yyvsp[0].expr).val >= 0xFF00) {
					if (warnOnLdOpt) {
						warnOnLdOpt = false;
						warning(WARNING_OBSOLETE, "ld optimization will stop being the default; pass `-l` to opt into it\n");
					}
					sect_AbsByte(0xF0);
					sect_AbsByte((yyvsp[0].expr).val & 0xFF);
					rpn_Free(&(yyvsp[0].expr));
				} else {
					sect_AbsByte(0xFA);
					sect_RelWord(&(yyvsp[0].expr), 1);
				}
			} else {
				error("Destination operand must be A\n");
				rpn_Free(&(yyvsp[0].expr));
			}
		}
#line 5634 "src/asm/parser.c"
    break;

  case 435: /* z80_ld_ss: "ld" "bc" "," reloc_16bit  */
#line 2019 "src/asm/parser.y"
                                                         {
			sect_AbsByte(0x01 | (REG_BC << 4));
			sect_RelWord(&(yyvsp[0].expr), 1);
		}
#line 5643 "src/asm/parser.c"
    break;

  case 436: /* z80_ld_ss: "ld" "de" "," reloc_16bit  */
#line 2023 "src/asm/parser.y"
                                                         {
			sect_AbsByte(0x01 | (REG_DE << 4));
			sect_RelWord(&(yyvsp[0].expr), 1);
		}
#line 5652 "src/asm/parser.c"
    break;

  case 437: /* z80_nop: "nop"  */
#line 2031 "src/asm/parser.y"
                            { sect_AbsByte(0x00); }
#line 5658 "src/asm/parser.c"
    break;

  case 438: /* z80_or: "or" op_a_n  */
#line 2034 "src/asm/parser.y"
                                  {
			sect_AbsByte(0xF6);
			sect_RelByte(&(yyvsp[0].expr), 1);
		}
#line 5667 "src/asm/parser.c"
    break;

  case 439: /* z80_or: "or" op_a_r  */
#line 2038 "src/asm/parser.y"
                                  { sect_AbsByte(0xB0 | (yyvsp[0].constValue)); }
#line 5673 "src/asm/parser.c"
    break;

  case 440: /* z80_pop: "pop" reg_tt  */
#line 2041 "src/asm/parser.y"
                                   { sect_AbsByte(0xC1 | ((yyvsp[0].constValue) << 4)); }
#line 5679 "src/asm/parser.c"
    break;

  case 441: /* z80_push: "push" reg_tt  */
#line 2044 "src/asm/parser.y"
                                    { sect_AbsByte(0xC5 | ((yyvsp[0].constValue) << 4)); }
#line 5685 "src/asm/parser.c"
    break;

  case 442: /* z80_res: "res" const_3bit "," reg_r  */
#line 2047 "src/asm/parser.y"
                                                     {
			sect_AbsByte(0xCB);
			sect_AbsByte(0x80 | ((yyvsp[-2].constValue) << 3) | (yyvsp[0].constValue));
		}
#line 5694 "src/asm/parser.c"
    break;

  case 443: /* z80_ret: "ret"  */
#line 2053 "src/asm/parser.y"
                            { sect_AbsByte(0xC9); }
#line 5700 "src/asm/parser.c"
    break;

  case 444: /* z80_ret: "ret" ccode_expr  */
#line 2054 "src/asm/parser.y"
                                       { sect_AbsByte(0xC0 | ((yyvsp[0].constValue) << 3)); }
#line 5706 "src/asm/parser.c"
    break;

  case 445: /* z80_reti: "reti"  */
#line 2057 "src/asm/parser.y"
                             { sect_AbsByte(0xD9); }
#line 5712 "src/asm/parser.c"
    break;

  case 446: /* z80_rl: "rl" reg_r  */
#line 2060 "src/asm/parser.y"
                                 {
			sect_AbsByte(0xCB);
			sect_AbsByte(0x10 | (yyvsp[0].constValue));
		}
#line 5721 "src/asm/parser.c"
    break;

  case 447: /* z80_rla: "rla"  */
#line 2066 "src/asm/parser.y"
                            { sect_AbsByte(0x17); }
#line 5727 "src/asm/parser.c"
    break;

  case 448: /* z80_rlc: "rlc" reg_r  */
#line 2069 "src/asm/parser.y"
                                  {
			sect_AbsByte(0xCB);
			sect_AbsByte(0x00 | (yyvsp[0].constValue));
		}
#line 5736 "src/asm/parser.c"
    break;

  case 449: /* z80_rlca: "rlca"  */
#line 2075 "src/asm/parser.y"
                             { sect_AbsByte(0x07); }
#line 5742 "src/asm/parser.c"
    break;

  case 450: /* z80_rr: "rr" reg_r  */
#line 2078 "src/asm/parser.y"
                                 {
			sect_AbsByte(0xCB);
			sect_AbsByte(0x18 | (yyvsp[0].constValue));
		}
#line 5751 "src/asm/parser.c"
    break;

  case 451: /* z80_rra: "rra"  */
#line 2084 "src/asm/parser.y"
                            { sect_AbsByte(0x1F); }
#line 5757 "src/asm/parser.c"
    break;

  case 452: /* z80_rrc: "rrc" reg_r  */
#line 2087 "src/asm/parser.y"
                                  {
			sect_AbsByte(0xCB);
			sect_AbsByte(0x08 | (yyvsp[0].constValue));
		}
#line 5766 "src/asm/parser.c"
    break;

  case 453: /* z80_rrca: "rrca"  */
#line 2093 "src/asm/parser.y"
                             { sect_AbsByte(0x0F); }
#line 5772 "src/asm/parser.c"
    break;

  case 454: /* z80_rst: "rst" reloc_8bit  */
#line 2096 "src/asm/parser.y"
                                       {
			rpn_CheckRST(&(yyvsp[0].expr), &(yyvsp[0].expr));
			if (!rpn_isKnown(&(yyvsp[0].expr)))
				sect_RelByte(&(yyvsp[0].expr), 0);
			else
				sect_AbsByte(0xC7 | (yyvsp[0].expr).val);
			rpn_Free(&(yyvsp[0].expr));
		}
#line 5785 "src/asm/parser.c"
    break;

  case 455: /* z80_sbc: "sbc" op_a_n  */
#line 2106 "src/asm/parser.y"
                                   {
			sect_AbsByte(0xDE);
			sect_RelByte(&(yyvsp[0].expr), 1);
		}
#line 5794 "src/asm/parser.c"
    break;

  case 456: /* z80_sbc: "sbc" op_a_r  */
#line 2110 "src/asm/parser.y"
                                   { sect_AbsByte(0x98 | (yyvsp[0].constValue)); }
#line 5800 "src/asm/parser.c"
    break;

  case 457: /* z80_scf: "scf"  */
#line 2113 "src/asm/parser.y"
                            { sect_AbsByte(0x37); }
#line 5806 "src/asm/parser.c"
    break;

  case 458: /* z80_set: "set" const_3bit "," reg_r  */
#line 2116 "src/asm/parser.y"
                                                     {
			sect_AbsByte(0xCB);
			sect_AbsByte(0xC0 | ((yyvsp[-2].constValue) << 3) | (yyvsp[0].constValue));
		}
#line 5815 "src/asm/parser.c"
    break;

  case 459: /* z80_sla: "sla" reg_r  */
#line 2122 "src/asm/parser.y"
                                  {
			sect_AbsByte(0xCB);
			sect_AbsByte(0x20 | (yyvsp[0].constValue));
		}
#line 5824 "src/asm/parser.c"
    break;

  case 460: /* z80_sra: "sra" reg_r  */
#line 2128 "src/asm/parser.y"
                                  {
			sect_AbsByte(0xCB);
			sect_AbsByte(0x28 | (yyvsp[0].constValue));
		}
#line 5833 "src/asm/parser.c"
    break;

  case 461: /* z80_srl: "srl" reg_r  */
#line 2134 "src/asm/parser.y"
                                  {
			sect_AbsByte(0xCB);
			sect_AbsByte(0x38 | (yyvsp[0].constValue));
		}
#line 5842 "src/asm/parser.c"
    break;

  case 462: /* z80_stop: "stop"  */
#line 2140 "src/asm/parser.y"
                             {
			sect_AbsByte(0x10);
			sect_AbsByte(0x00);
		}
#line 5851 "src/asm/parser.c"
    break;

  case 463: /* z80_stop: "stop" reloc_8bit  */
#line 2144 "src/asm/parser.y"
                                        {
			sect_AbsByte(0x10);
			sect_RelByte(&(yyvsp[0].expr), 1);
		}
#line 5860 "src/asm/parser.c"
    break;

  case 464: /* z80_sub: "sub" op_a_n  */
#line 2150 "src/asm/parser.y"
                                   {
			sect_AbsByte(0xD6);
			sect_RelByte(&(yyvsp[0].expr), 1);
		}
#line 5869 "src/asm/parser.c"
    break;

  case 465: /* z80_sub: "sub" op_a_r  */
#line 2154 "src/asm/parser.y"
                                   { sect_AbsByte(0x90 | (yyvsp[0].constValue)); }
#line 5875 "src/asm/parser.c"
    break;

  case 466: /* z80_swap: "swap" reg_r  */
#line 2157 "src/asm/parser.y"
                                   {
			sect_AbsByte(0xCB);
			sect_AbsByte(0x30 | (yyvsp[0].constValue));
		}
#line 5884 "src/asm/parser.c"
    break;

  case 467: /* z80_xor: "xor" op_a_n  */
#line 2163 "src/asm/parser.y"
                                   {
			sect_AbsByte(0xEE);
			sect_RelByte(&(yyvsp[0].expr), 1);
		}
#line 5893 "src/asm/parser.c"
    break;

  case 468: /* z80_xor: "xor" op_a_r  */
#line 2167 "src/asm/parser.y"
                                   { sect_AbsByte(0xA8 | (yyvsp[0].constValue)); }
#line 5899 "src/asm/parser.c"
    break;

  case 469: /* op_mem_ind: "[" reloc_16bit "]"  */
#line 2170 "src/asm/parser.y"
                                                { (yyval.expr) = (yyvsp[-1].expr); }
#line 5905 "src/asm/parser.c"
    break;

  case 471: /* op_a_r: T_MODE_A "," reg_r  */
#line 2174 "src/asm/parser.y"
                                         { (yyval.constValue) = (yyvsp[0].constValue); }
#line 5911 "src/asm/parser.c"
    break;

  case 473: /* op_a_n: T_MODE_A "," reloc_8bit  */
#line 2178 "src/asm/parser.y"
                                              { (yyval.expr) = (yyvsp[0].expr); }
#line 5917 "src/asm/parser.c"
    break;

  case 489: /* ccode_expr: "!" ccode_expr  */
#line 2210 "src/asm/parser.y"
                                           {
			(yyval.constValue) = (yyvsp[0].constValue) ^ 1;
		}
#line 5925 "src/asm/parser.c"
    break;

  case 490: /* ccode: "nz"  */
#line 2215 "src/asm/parser.y"
                          { (yyval.constValue) = CC_NZ; }
#line 5931 "src/asm/parser.c"
    break;

  case 491: /* ccode: "z"  */
#line 2216 "src/asm/parser.y"
                         { (yyval.constValue) = CC_Z; }
#line 5937 "src/asm/parser.c"
    break;

  case 492: /* ccode: "nc"  */
#line 2217 "src/asm/parser.y"
                          { (yyval.constValue) = CC_NC; }
#line 5943 "src/asm/parser.c"
    break;

  case 493: /* ccode: "c"  */
#line 2218 "src/asm/parser.y"
                            { (yyval.constValue) = CC_C; }
#line 5949 "src/asm/parser.c"
    break;

  case 494: /* reg_r: T_MODE_B  */
#line 2221 "src/asm/parser.y"
                           { (yyval.constValue) = REG_B; }
#line 5955 "src/asm/parser.c"
    break;

  case 495: /* reg_r: T_MODE_C  */
#line 2222 "src/asm/parser.y"
                           { (yyval.constValue) = REG_C; }
#line 5961 "src/asm/parser.c"
    break;

  case 496: /* reg_r: T_MODE_D  */
#line 2223 "src/asm/parser.y"
                           { (yyval.constValue) = REG_D; }
#line 5967 "src/asm/parser.c"
    break;

  case 497: /* reg_r: T_MODE_E  */
#line 2224 "src/asm/parser.y"
                           { (yyval.constValue) = REG_E; }
#line 5973 "src/asm/parser.c"
    break;

  case 498: /* reg_r: T_MODE_H  */
#line 2225 "src/asm/parser.y"
                           { (yyval.constValue) = REG_H; }
#line 5979 "src/asm/parser.c"
    break;

  case 499: /* reg_r: T_MODE_L  */
#line 2226 "src/asm/parser.y"
                           { (yyval.constValue) = REG_L; }
#line 5985 "src/asm/parser.c"
    break;

  case 500: /* reg_r: "[" "hl" "]"  */
#line 2227 "src/asm/parser.y"
                                              { (yyval.constValue) = REG_HL_IND; }
#line 5991 "src/asm/parser.c"
    break;

  case 501: /* reg_r: T_MODE_A  */
#line 2228 "src/asm/parser.y"
                           { (yyval.constValue) = REG_A; }
#line 5997 "src/asm/parser.c"
    break;

  case 502: /* reg_tt: "bc"  */
#line 2231 "src/asm/parser.y"
                            { (yyval.constValue) = REG_BC; }
#line 6003 "src/asm/parser.c"
    break;

  case 503: /* reg_tt: "de"  */
#line 2232 "src/asm/parser.y"
                            { (yyval.constValue) = REG_DE; }
#line 6009 "src/asm/parser.c"
    break;

  case 504: /* reg_tt: "hl"  */
#line 2233 "src/asm/parser.y"
                            { (yyval.constValue) = REG_HL; }
#line 6015 "src/asm/parser.c"
    break;

  case 505: /* reg_tt: "af"  */
#line 2234 "src/asm/parser.y"
                            { (yyval.constValue) = REG_AF; }
#line 6021 "src/asm/parser.c"
    break;

  case 506: /* reg_ss: "bc"  */
#line 2237 "src/asm/parser.y"
                            { (yyval.constValue) = REG_BC; }
#line 6027 "src/asm/parser.c"
    break;

  case 507: /* reg_ss: "de"  */
#line 2238 "src/asm/parser.y"
                            { (yyval.constValue) = REG_DE; }
#line 6033 "src/asm/parser.c"
    break;

  case 508: /* reg_ss: "hl"  */
#line 2239 "src/asm/parser.y"
                            { (yyval.constValue) = REG_HL; }
#line 6039 "src/asm/parser.c"
    break;

  case 509: /* reg_ss: "sp"  */
#line 2240 "src/asm/parser.y"
                            { (yyval.constValue) = REG_SP; }
#line 6045 "src/asm/parser.c"
    break;

  case 510: /* reg_rr: "[" "bc" "]"  */
#line 2243 "src/asm/parser.y"
                                              { (yyval.constValue) = REG_BC_IND; }
#line 6051 "src/asm/parser.c"
    break;

  case 511: /* reg_rr: "[" "de" "]"  */
#line 2244 "src/asm/parser.y"
                                              { (yyval.constValue) = REG_DE_IND; }
#line 6057 "src/asm/parser.c"
    break;

  case 512: /* reg_rr: hl_ind_inc  */
#line 2245 "src/asm/parser.y"
                             { (yyval.constValue) = REG_HL_INDINC; }
#line 6063 "src/asm/parser.c"
    break;

  case 513: /* reg_rr: hl_ind_dec  */
#line 2246 "src/asm/parser.y"
                             { (yyval.constValue) = REG_HL_INDDEC; }
#line 6069 "src/asm/parser.c"
    break;


#line 6073 "src/asm/parser.c"

        default: break;
      }
    if (yychar_backup != yychar)
      YY_LAC_DISCARD ("yychar change");
  }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", YY_CAST (yysymbol_kind_t, yyr1[yyn]), &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;

  *++yyvsp = yyval;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */
  {
    const int yylhs = yyr1[yyn] - YYNTOKENS;
    const int yyi = yypgoto[yylhs] + *yyssp;
    yystate = (0 <= yyi && yyi <= YYLAST && yycheck[yyi] == *yyssp
               ? yytable[yyi]
               : yydefgoto[yylhs]);
  }

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYSYMBOL_YYEMPTY : YYTRANSLATE (yychar);
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
      {
        yypcontext_t yyctx
          = {yyssp, yyesa, &yyes, &yyes_capacity, yytoken};
        char const *yymsgp = YY_("syntax error");
        int yysyntax_error_status;
        if (yychar != YYEMPTY)
          YY_LAC_ESTABLISH;
        yysyntax_error_status = yysyntax_error (&yymsg_alloc, &yymsg, &yyctx);
        if (yysyntax_error_status == 0)
          yymsgp = yymsg;
        else if (yysyntax_error_status == -1)
          {
            if (yymsg != yymsgbuf)
              YYSTACK_FREE (yymsg);
            yymsg = YY_CAST (char *,
                             YYSTACK_ALLOC (YY_CAST (YYSIZE_T, yymsg_alloc)));
            if (yymsg)
              {
                yysyntax_error_status
                  = yysyntax_error (&yymsg_alloc, &yymsg, &yyctx);
                yymsgp = yymsg;
              }
            else
              {
                yymsg = yymsgbuf;
                yymsg_alloc = sizeof yymsgbuf;
                yysyntax_error_status = YYENOMEM;
              }
          }
        yyerror (yymsgp);
        if (yysyntax_error_status == YYENOMEM)
          YYNOMEM;
      }
    }

  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
         error, discard it.  */

      if (yychar <= T_EOF)
        {
          /* Return failure if at end of input.  */
          if (yychar == T_EOF)
            YYABORT;
        }
      else
        {
          yydestruct ("Error: discarding",
                      yytoken, &yylval);
          yychar = YYEMPTY;
        }
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:
  /* Pacify compilers when the user code never invokes YYERROR and the
     label yyerrorlab therefore never appears in user code.  */
  if (0)
    YYERROR;
  ++yynerrs;

  /* Do not reclaim the symbols of the rule whose action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  /* Pop stack until we find a state that shifts the error token.  */
  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYSYMBOL_YYerror;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYSYMBOL_YYerror)
            {
              yyn = yytable[yyn];
              if (0 < yyn)
                break;
            }
        }

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
        YYABORT;


      yydestruct ("Error: popping",
                  YY_ACCESSING_SYMBOL (yystate), yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  /* If the stack popping above didn't lose the initial context for the
     current lookahead token, the shift below will for sure.  */
  YY_LAC_DISCARD ("error recovery");

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", YY_ACCESSING_SYMBOL (yyn), yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturnlab;


/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturnlab;


/*-----------------------------------------------------------.
| yyexhaustedlab -- YYNOMEM (memory exhaustion) comes here.  |
`-----------------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  goto yyreturnlab;


/*----------------------------------------------------------.
| yyreturnlab -- parsing is finished, clean up and return.  |
`----------------------------------------------------------*/
yyreturnlab:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  YY_ACCESSING_SYMBOL (+*yyssp), yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
  if (yyes != yyesa)
    YYSTACK_FREE (yyes);
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
  return yyresult;
}

#line 2257 "src/asm/parser.y"

