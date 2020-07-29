/****************************************************/
/* File: tiny.l                                     */
/* Lex specification for TINY                       */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/****************************************************/

%{
#include "globals.h"
#include "util.h"
#include "scan.h"
/* lexeme of identifier or reserved word */
char tokenString[MAXTOKENLEN+1];
%}

digit       [0-9]
number      {digit}+
letter      [a-zA-Z]
identifier  {letter}+
newline     \n
whitespace  [ \t]+

%%

"if"            {return IF;}
"then"          {return THEN;}
"else"          {return ELSE;}
"while"			{return WHILE;}
"return"		{return RETURN;}
"int"			{return INT;}
"void"			{return VOID;}
"end"           {return END;}
"repeat"        {return REPEAT;}
"until"         {return UNTIL;}
"read"          {return READ;}
"write"         {return WRITE;}
"="      		{return ASSIGN;}
"=="            {return EQ;}
"!="			{return NE;}
"<="			{return LE;}
">="			{return GE;}
">"				{return GT;}
"<"             {return LT;}
"+"             {return PLUS;}
"-"             {return MINUS;}
"*"             {return TIMES;}
"/"             {return OVER;}
"("             {return LPAREN;}
")"             {return RPAREN;}
"["				{return LBRACE;}
"]"				{return RBRACE;}
"{"				{return LCURLY;}
"}"				{return RCURLY;}
";"             {return SEMI;}
","				{return COMMA;}
{number}        {return NUM;}
{identifier}    {return ID;}
{newline}       {lineno++;}
{whitespace}    {/* skip whitespace */}
"/*"           	{
					char c = ' ';
					while(1){
						while(c !='*'){
							c = input();
							if(c == EOF)
								break;
							if(c == '\n')
								lineno++;
						}
						//shoud be c == "*"
						c = input();
						if(c == EOF)
							break;
						if(c == '\n')
							lineno++;
						if(c == '/')
							break;
					}   
				}
.               {return ERROR;}

%%

TokenType getToken(void)
{ static int firstTime = TRUE;
  TokenType currentToken;
  if (firstTime)
  { firstTime = FALSE;
    lineno++;
    yyin = source;
    yyout = listing;
  }
  currentToken = yylex();
  strncpy(tokenString,yytext,MAXTOKENLEN);
  if (TraceScan) {
    fprintf(listing,"\t%d: ",lineno);
    printToken(currentToken,tokenString);
  }
  return currentToken;
}

