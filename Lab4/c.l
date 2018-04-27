%e  1019
%p  2807
%n  371
%k  284
%a  1213
%o  1117

O   [0-7]
D   [0-9]
NZ  [1-9]
L   [a-zA-Z_]
A   [a-zA-Z_0-9]
H   [a-fA-F0-9]
HP  (0[xX])
E   ([Ee][+-]?{D}+)
P   ([Pp][+-]?{D}+)
FS  (f|F|l|L)
IS  (((u|U)(l|L|ll|LL)?)|((l|L|ll|LL)(u|U)?))
CP  (u|U|L)
SP  (u8|u|U|L)
ES  (\\(['"\?\\abfnrtv]|[0-7]{1,3}|x[a-fA-F0-9]+))
WS  [ \t\v\n\f]

%{
#include <stdio.h>
#include <string>
#include "c.tab.hpp"

extern void yyerror(const char *);  /* prints grammar violation message */

extern int sym_type(const char *);  /* returns type from symbol table */

#define sym_type(identifier) IDENTIFIER /* with no symbol table, fake it */

static void comment(void);
static int check_type(int i);
#define YY_DECL extern "C" int yylex()
%}

%option yylineno

%%
"/*"                                    { comment(); }
"//".*                                    { /* consume //-comment */ }

"auto"					{ return(AUTO); }
"break"					{ return(BREAK); }
"case"					{ return(CASE); }
"char"					{ return(CHAR); }
"const"					{ return(CONST); }
"continue"				{ return(CONTINUE); }
"default"				{ return(DEFAULT); }
"do"					{ return(DO); }
"double"				{ return(DOUBLE); }
"else"					{ return(ELSE); }
"enum"					{ return(ENUM); }
"extern"				{ return(EXTERN); }
"float"					{ return(FLOAT); }
"for"					{ return(FOR); }
"goto"					{ return(GOTO); }
"if"					{ return(IF); }
"inline"				{ return(INLINE); }
"int"					{ return(INT); }
"long"					{ return(LONG); }
"register"				{ return(REGISTER); }
"restrict"				{ return(RESTRICT); }
"return"				{ return(RETURN); }
"short"					{ return(SHORT); }
"signed"				{ return(SIGNED); }
"sizeof"				{ return(SIZEOF); }
"static"				{ return(STATIC); }
"struct"				{ return(STRUCT); }
"switch"				{ return(SWITCH); }
"typedef"				{ return(TYPEDEF); }
"union"					{ return(UNION); }
"unsigned"				{ return(UNSIGNED); }
"void"					{ return(VOID); }
"volatile"				{ return(VOLATILE); }
"while"					{ return(WHILE); }
"_Alignas"                              { return ALIGNAS; }
"_Alignof"                              { return ALIGNOF; }
"_Atomic"                               { return ATOMIC; }
"_Bool"                                 { return BOOL; }
"_Complex"                              { return COMPLEX; }
"_Generic"                              { return GENERIC; }
"_Imaginary"                            { return IMAGINARY; }
"_Noreturn"                             { return NORETURN; }
"_Static_assert"                        { return STATIC_ASSERT; }
"_Thread_local"                         { return THREAD_LOCAL; }
"__func__"                              { return FUNC_NAME; }

{L}{A}*					{ return check_type(0); }

{HP}{H}+{IS}?				{ yylval.ival = atoi(yytext); return I_CONSTANT; }
{NZ}{D}*{IS}?				{ yylval.ival = atoi(yytext); return I_CONSTANT; }
"0"{O}*{IS}?				{ yylval.ival = atoi(yytext); return I_CONSTANT; }
{CP}?"'"([^'\\\n]|{ES})+"'"		{  yylval.ival = atoi(yytext); return I_CONSTANT; }

{D}+{E}{FS}?				{  yylval.fval = std::stof(yytext); return F_CONSTANT; }
{D}*"."{D}+{E}?{FS}?			{ yylval.fval = std::stof(yytext); return F_CONSTANT; }
{D}+"."{E}?{FS}?			{ yylval.fval = std::stof(yytext); return F_CONSTANT; }
{HP}{H}+{P}{FS}?			{ yylval.fval = std::stof(yytext); return F_CONSTANT; }
{HP}{H}*"."{H}+{P}{FS}?			{ yylval.fval = std::stof(yytext); return F_CONSTANT; }
{HP}{H}+"."{P}{FS}?			{ yylval.fval = std::stof(yytext); return F_CONSTANT; }

({SP}?\"([^"\\\n]|{ES})*\"{WS}*)+	{ return check_type(1); /*return S_CONSTANT*/ }

"..."					{ return ELLIPSIS; }
">>="					{ return RIGHT_ASSIGN; }
"<<="					{ return LEFT_ASSIGN; }
"+="					{ return ADD_ASSIGN; }
"-="					{ return SUB_ASSIGN; }
"*="					{ return MUL_ASSIGN; }
"/="					{ return DIV_ASSIGN; }
"%="					{ return MOD_ASSIGN; }
"&="					{ return AND_ASSIGN; }
"^="					{ return XOR_ASSIGN; }
"|="					{ return OR_ASSIGN; }
">>"					{ return RIGHT_OP; }
"<<"					{ return LEFT_OP; }
"++"					{ return INC_OP; }
"--"					{ return DEC_OP; }
"->"					{ return PTR_OP; }
"&&"					{ return AND_OP; }
"||"					{ return OR_OP; }
"<="					{ return LE_OP; }
">="					{ return GE_OP; }
"=="					{ return EQ_OP; }
"!="					{ return NE_OP; }
";"					{ return ';'; }
("{"|"<%")				{ return '{'; }
("}"|"%>")				{ return '}'; }
","					{ return ','; }
":"					{ return ':'; }
"="					{ return '='; }
"("					{ return '('; }
")"					{ return ')'; }
("["|"<:")				{ return '['; }
("]"|":>")				{ return ']'; }
"."					{ return '.'; }
"&"					{ return '&'; }
"!"					{ return '!'; }
"~"					{ return '~'; }
"-"					{ return '-'; }
"+"					{ return '+'; }
"*"					{ return '*'; }
"/"					{ return '/'; }
"%"					{ return '%'; }
"<"					{ return '<'; }
">"					{ return '>'; }
"^"					{ return '^'; }
"|"					{ return '|'; }
"?"					{ return '?'; }

{WS}+					{ /* whitespace separates tokens */ }
.					{ /* discard bad characters */ }

%%

int yywrap(void)        /* called at end of input */
{
    return 1;           /* terminate now */
}

static void comment(void)
{
    int c;

    while ((c = yyinput()) != 0)
        if (c == '*')
        {
            while ((c = yyinput()) == '*')
                ;

            if (c == '/')
                return;

            if (c == 0)
                break;
        }
    yyerror("Unterminated Comment");
}

static int check_type(int type)
{
    switch (type)
    {
    case 1:              /* previously defined */
        {
            // allocate sval and copy yytext to it
            yylval.sval = new char[yyleng];
            int i = 0;
            for (; i < yyleng; i++) {
                yylval.sval[i] = yytext[i];
            }
            yylval.sval[i] = '\0';
            return STRING_LITERAL;
        }
    default:                         /* includes undefined */
        {
        // allocate sval and copy yytext to it
            yylval.sval = new char[yyleng];
            int i = 0;
            for (; i < yyleng; i++) {
                yylval.sval[i] = yytext[i];
            }
            yylval.sval[i] = '\0';
            return IDENTIFIER;
        }
    }
}
