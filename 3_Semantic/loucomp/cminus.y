/****************************************************/
/* File: tiny.y                                     */
/* The TINY Yacc/Bison specification file           */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/****************************************************/
%{
#define YYPARSER /* distinguishes Yacc output from other code files */

#include "globals.h"
#include "util.h"
#include "scan.h"
#include "parse.h"

#define YYSTYPE TreeNode *
static char * savedName; /* for use in assignments */
static int savedNumber;
static int savedLineNo;  /* ditto */
static TreeNode * savedTree; /* stores syntax tree for later return */
static int yylex(void); // added 11/2/11 to ensure no conflict with lex

%}

%token IF ELSE WHILE RETURN INT VOID THEN END REPEAT UNTIL READ WRITE
%token ID NUM 
%token ASSIGN EQ NE LT LE GT GE PLUS MINUS TIMES OVER 
%token LPAREN RPAREN LBRACE RBRACE LCURLY RCURLY SEMI COMMA
%token ERROR 

%nonassoc LOWER_THAN_ELSE
%nonassoc ELSE


%% /* Grammar for CMINUS */
program				: declaration_list { savedTree = $1;}
		 			;

declaration_list	: declaration_list declaration 
				 		{ 
							YYSTYPE t = $1;
							if(t != NULL){
								while(t->sibling != NULL){
									t = t->sibling;
								}
								t->sibling = $2;
								$$ = $1;
							}
							else { $$ = $2;}
				 		}
				 	| declaration { $$ = $1; }
					;

declaration			: var_declaration { $$ = $1; }
			  		| fun_declaration { $$ = $1; }
					;

var_declaration		: INT saveid SEMI {/*single int var*/
							$$ = newExpNode(VarK);
							$$->lineno = lineno;
							$$->attr.name = savedName;
							$$->type = Integer; 
						}
				 	| INT saveid LBRACE savenum RBRACE SEMI {/*int array var*/
							$$ = newExpNode(ArrVarK);
							$$->lineno = lineno;
							$$->attr.name = savedName;
							$$->isArr = savedNumber;
							$$->type = IntegerArray; 
						}
					| VOID saveid SEMI {/*single void var*/
							$$ = newExpNode(VarK);
							$$->lineno = lineno;
							$$->attr.name = savedName;
							$$->type = Void; 
						}
				 	| VOID saveid LBRACE savenum RBRACE SEMI {/*void array var*/
							$$ = newExpNode(ArrVarK);
							$$->lineno = lineno;
							$$->attr.name = savedName;
							$$->isArr = savedNumber;
							$$->type = VoidArray;
						}
					;

saveid 				: ID { /*ID saving name string*/
		   					savedName = copyString(tokenString);
							savedLineNo = lineno; 
						}
		   			;

savenum				: NUM { /*NUM saving constant number*/
		   					savedNumber = atoi(tokenString);
							savedLineNo = lineno; 
						}
					;

fun_declaration		: INT saveid { /*int func(){} declaration */
				 			$$ = newExpNode(FuncK);
							$$->lineno = lineno;
							$$->attr.name = savedName;
							$$->type = Integer;
				 		} LPAREN params RPAREN compound_stmt {
							$$ = $3;
							$$->child[0] = $5;
							$$->child[1] = $7;
						}
					| VOID saveid { /*void func(){} declaration */
				 			$$ = newExpNode(FuncK);
							$$->lineno = lineno;
							$$->attr.name = savedName;
							$$->type = Void;
				 		} LPAREN params RPAREN compound_stmt {
							$$ = $3;
							$$->child[0] = $5;
							$$->child[1] = $7;
						}
				 	;

params				: param_list {$$ = $1;}/*one or more parameters*/
		  			| VOID {/*No Parameter (void)*/
							$$ = newExpNode(ParamK);
							$$->type = Void;
							$$->isParam = TRUE;
						}
					;

param_list			: param_list COMMA param {
			 				YYSTYPE t = $1;
							if(t != NULL){
								while(t->sibling != NULL){
									t = t->sibling;
								}
								t->sibling = $3;
								$$ = $1;
							}
							else{
								$$ = $3;
							}
						}
			 		| param {$$ = $1;}
					;

param				: INT saveid { /*int var as a parameter*/
		 					$$ = newExpNode(ParamK);
							$$->attr.name = savedName;
							$$->isParam = TRUE;
							$$->type = Integer;
						}
		 			| INT saveid LBRACE RBRACE { /*int var[] as a parameter*/
							$$ = newExpNode(ArrParamK);
							$$->attr.name = savedName;
							$$->isParam =TRUE;
							$$->type = IntegerArray;
						}
					| VOID saveid { /*void var as a parameter*/
		 					$$ = newExpNode(ParamK);
							$$->attr.name = savedName;
							$$->isParam = TRUE;
							$$->type = Void;
						}
		 			| VOID saveid LBRACE RBRACE {/*void var[] as a parameter*/
							$$ = newExpNode(ArrParamK);
							$$->attr.name = savedName;
							$$->isParam =TRUE;
							$$->type = VoidArray;
						}
					;

compound_stmt		: LCURLY local_declarations statement_list RCURLY {
			   				$$ = newStmtNode(CompndK);
							$$->child[0] = $2;
							$$->child[1] = $3;
			   			}
			   		;

local_declarations 	: local_declarations var_declaration {
							YYSTYPE t = $1;
							if(t != NULL){
								while(t->sibling != NULL){
									t = t->sibling;
								}
								t->sibling = $2;
								$$ = $1;
							}
							else{
								$$ = $2;
							}
						}
					| {$$ = NULL;}
					;

statement_list 		: statement_list statement {
				 			YYSTYPE t = $1;
							if(t != NULL){
								while(t->sibling != NULL){
									t = t->sibling;
								}
								t->sibling = $2;
								$$ = $1;
							}
							else{
								$$ = $2;
							}
				 		}
				 	| {$$ = NULL;}
					;

statement			: expression_stmt {$$ = $1;}
					| compound_stmt {$$ = $1;}
					| selection_stmt {$$ = $1;}
					| iteration_stmt {$$ = $1;}
					| return_stmt {$$ = $1;}
					;

expression_stmt		: expression SEMI {$$ = $1;}
				 	| SEMI {$$ = NULL;}
					;

selection_stmt		: IF LPAREN expression RPAREN statement %prec LOWER_THAN_ELSE{
							/*using %prec LOWER_THAN_ELSE to solve dangling else problem*/
							$$= newStmtNode(SelectK);
							$$->child[0] = $3;
							$$->child[1] = $5;
							$$->child[2] = NULL;
						}
					
					| IF LPAREN expression RPAREN statement ELSE statement {
							$$= newStmtNode(SelectK);
							$$->child[0] = $3;
							$$->child[1] = $5;
							$$->child[2] = $7;
						}
					;

iteration_stmt		: WHILE LPAREN expression RPAREN statement {
							$$= newStmtNode(IterK);
							$$->child[0] = $3;
							$$->child[1] = $5;
						}
					;

return_stmt			: RETURN SEMI {
			  				$$ = newStmtNode(ReturnK);
							$$->child[0] = NULL;
			  			}
			  		| RETURN expression SEMI {
							$$ = newStmtNode(ReturnK);
							$$->child[0] = $2;
						}
					;

expression			: var ASSIGN expression {
			 				$$ = newStmtNode(AssignK);
							$$->child[0] = $1;
							$$->child[1] = $3;
			 			}
			 		| simple_expression {$$ = $1;}
					;

var					: saveid { 
							$$ = newExpNode(IdK);
							$$->attr.name = savedName;
						}
						
					| saveid {
							$$ = newExpNode(IdK);
							$$->attr.name = savedName;
						} LBRACE expression RBRACE {
							$$ = $2;
							$$->child[0] = $4;
						}
					;

simple_expression	: additive_expression LE additive_expression {
							$$ = newExpNode(RelOpK);
							$$->child[0] = $1;
							$$->child[1] = $3;
							$$->attr.op = LE;
						}
				  	| additive_expression LT additive_expression {
							$$ = newExpNode(RelOpK);
							$$->child[0] = $1;
							$$->child[1] = $3;
							$$->attr.op = LT;
						}
					| additive_expression GT additive_expression {
							$$ = newExpNode(RelOpK);
							$$->child[0] = $1;
							$$->child[1] = $3;
							$$->attr.op = GT;
						}
					| additive_expression GE additive_expression {
							$$ = newExpNode(RelOpK);
							$$->child[0] = $1;
							$$->child[1] = $3;
							$$->attr.op = GE;
						}
					| additive_expression EQ additive_expression {
							$$ = newExpNode(RelOpK);
							$$->child[0] = $1;
							$$->child[1] = $3;
							$$->attr.op = EQ;
						}
					| additive_expression NE additive_expression {
							$$ = newExpNode(RelOpK);
							$$->child[0] = $1;
							$$->child[1] = $3;
							$$->attr.op = NE;
						}
				  	| additive_expression { $$ = $1; }
					;

additive_expression	: additive_expression PLUS term {
							$$ = newExpNode(OpK);
							$$->child[0] = $1;
							$$->child[1] = $3;
							$$->attr.op = PLUS;
						}
					| additive_expression MINUS term {
							$$ = newExpNode(OpK);
							$$->child[0] = $1;
							$$->child[1] = $3;
							$$->attr.op = MINUS;
						}
					| term {$$ = $1;}
					;

term				: term TIMES factor {
							$$ = newExpNode(OpK);
							$$->child[0] = $1;
							$$->child[1] = $3;
							$$->attr.op = TIMES;
						}
					| term OVER factor {
							$$ = newExpNode(OpK);
							$$->child[0] = $1;
							$$->child[1] = $3;
							$$->attr.op = OVER;
						}
					| factor {$$ = $1;}
					;

factor				: LPAREN expression RPAREN {$$ = $2;}
		  			| var {$$ = $1;}
					| call {$$ = $1;}
					| NUM {$$ = newExpNode(ConstK);
						$$->attr.val = atoi(tokenString);}
						
					;

call				: saveid {
							$$ = newStmtNode(CallK);
							$$->attr.name = savedName;
						} LPAREN args RPAREN {
							$$ = $2;
							$$->child[0] = $4;
						}
					;

args				: arg_list {$$ = $1;}
					| {$$ = NULL;}
					;

arg_list			: arg_list COMMA expression 
		   				{
							YYSTYPE t = $1;
							if(t!=NULL){
								while(t->sibling != NULL){
									t = t->sibling;
								}
								t->sibling = $3;
								$$ = $1;
							}
							else{
								$$ = $3;
							}
							
		   				}
		   			| expression {$$ = $1;}
					;

%%

int yyerror(char * message)
{ fprintf(listing,"Syntax error at line %d: %s\n",lineno,message);
  fprintf(listing,"Current token: ");
  printToken(yychar,tokenString);
  Error = TRUE;
  return 0;
}

/* yylex calls getToken to make Yacc/Bison output
 * compatible with ealier versions of the TINY scanner
 */
static int yylex(void)
{ return getToken(); }

TreeNode * parse(void)
{ yyparse();
  return savedTree;
}

