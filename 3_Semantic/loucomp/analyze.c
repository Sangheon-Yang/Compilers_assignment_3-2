/****************************************************/
/* File: analyze.c                                  */
/* Semantic analyzer implementation                 */
/* for the TINY compiler                            */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/****************************************************/
#include <string.h>
#include "globals.h"
#include "symtab.h"
#include "analyze.h"

/* counter for variable memory locations */
static int location = 1; // location starts from 1; Not ZEro;

//for calculating the number of Totally generated Scopes
static int TotalScopes = 0;

//for calculating current scope's nested Level
static int ScopeLevel = 0;

//and in order to access to ScopeTable by index nubmber
//and use it as a parameter to st_insert, st_lookup function
static int CurScopeIndex = 0;

//currentScope
static ScopeList CurrentScope;

// flague for distinguishing FuncK to handle generating newScope
static int isInFuncKpre = 0; // 0 or 1
static int funcCompound = 0; // 0 or 1

// counter of Compound Stmt excluding func Decl
//representing the number of compound stmt & scopelevel
static int just_compound = 0; // 0 or positive integer;

//for return check in type check
static int isWellReturned = 0;
static int ReturnTypeErr = 0;
static int CheckedScopeID = 0;
static int functionID = 0;
static int isDuplicatedError = 0;


// initializing fuction
// add Input() and Output() Function Buckets into ScopeTable (Global Scope)
static void initialize(){
    BucketList input, output;
    
    ScopeTable[0] = (ScopeList)malloc(sizeof(struct ScopeListRec));
    ScopeTable[0]->name = "global";
    ScopeTable[0]->index = 0;
    ScopeTable[0]->NofBuckets = 0;
    ScopeTable[0]->nestedLevel = 0;
    ScopeTable[0]->Bucket = NULL;
    ScopeTable[0]->parent = NULL;
    
    input = (BucketList)malloc(sizeof(struct BucketListRec));
    input->name = "input";
    input->lines = (LineList)malloc(sizeof(struct LineListRec));
    input->lines->lineno = -1;
    input->lines->next = NULL;
    input->memloc = -1;
    input->isFunc = 1;
    input->funcID = 0;
    input->type = Integer;
    input->node = NULL;
    ScopeTable[0]->Bucket = input;
    ScopeTable[0]->NofBuckets = 1;
    
    output = (BucketList)malloc(sizeof(struct BucketListRec));
    output->name = "output";
    output->lines = (LineList) malloc(sizeof(struct LineListRec));
    output->lines->lineno = -1;
    output->lines->next = NULL;
    output->memloc = -2;
    output->isFunc = 1;
    output->funcID = -1;
    output->type = Void;
    output->node = NULL;
    
    output->next = ScopeTable[0]->Bucket;
    ScopeTable[0]->Bucket = output;
    ScopeTable[0]->NofBuckets = 2;
    
    CurrentScope = ScopeTable[0];
    fprintf(listing, "\n>> Input() function & Output() function are added into symbol table <<\n\n");
}


/* Procedure traverse is a generic recursive 
 * syntax tree traversal routine:
 * it applies preProc in preorder and postProc 
 * in postorder to tree pointed to by t
 */
static void traverse( TreeNode * t,
               void (* preProc) (TreeNode *),
               void (* postProc) (TreeNode *) )
{ if (t != NULL)
  { preProc(t);
    { int i;
      for (i=0; i < MAXCHILDREN; i++)
        traverse(t->child[i],preProc,postProc);
    }
    postProc(t);
    traverse(t->sibling,preProc,postProc);
  }
}

/* nullProc is a do-nothing procedure to 
 * generate preorder-only or postorder-only
 * traversals from traverse
 */


//Post function used in traverse when Building Symtab.
static void nullProc(TreeNode * t){
    if(isDuplicatedError && t->nodekind== ExpK &&t->kind.exp == FuncK){
        isDuplicatedError = 0;
    }
    else if(just_compound > 0 && t->nodekind == StmtK && t->kind.stmt == CompndK){
        //when the count of compund stmts are more than 0;
        ScopeLevel--;
        CurrentScope = CurrentScope->parent;
        CurScopeIndex = CurrentScope->index;
        //fprintf(listing, "nullproc scope handling escaping compund stmt\n");
        //fprintf(listing, "Current Scope level : %d\n", ScopeLevel);
        just_compound--;
    }
    else if (isInFuncKpre==1 && funcCompound==1 && just_compound==0 && t->nodekind == StmtK &&t->kind.stmt == CompndK){
        //case of funcK's child[1] compstmt;
        funcCompound = 0;
    }
    else if( isInFuncKpre==1 && funcCompound==0 && just_compound==0 &&t->nodekind == ExpK && t->kind.exp == FuncK){
        //case of FuncK
        ScopeLevel--;
        CurrentScope = CurrentScope->parent;
        CurScopeIndex = CurrentScope->index;
        isInFuncKpre= 0;
        //fprintf(listing, "nullproc scope handling for escaping function Decl\n");
        //fprintf(listing, "Current Scope level : %d\n", ScopeLevel);
    }
}

// detect Semantinc Error while building Symtab;
// ex: void type var declaration, etc.
static void SemanticError(TreeNode * t, char * message)
{ fprintf(listing,"Error(while building SymTab) at line %d: %s\n",t->lineno,message);
  Error = TRUE;
}

// ---- edited -----------//
// adding buckets to ScopeTable;
static void insertNode( TreeNode * t){
    switch(t->nodekind){
        case StmtK:
            switch (t->kind.stmt){
                case CompndK: //Scope is changing here
                    if(isDuplicatedError){
                        SemanticError(t, "Duplaicated Function Declaration: function body is not added to symbol table");
                        return;
                    }
                    if(isInFuncKpre == 1 && funcCompound == 0){//if in funcDecl
                        funcCompound = 1; //just flague on;
                    }
                    else{//just Compund without func Decl
                        //add new scope
                        just_compound++;
                        TotalScopes++;
                        ScopeLevel++;
                        CurScopeIndex = TotalScopes;
                        ScopeTable[CurScopeIndex] = (ScopeList)malloc(sizeof(struct ScopeListRec));
                        ScopeTable[CurScopeIndex]->name = NULL;
                        ScopeTable[CurScopeIndex]->index = CurScopeIndex;
                        ScopeTable[CurScopeIndex]->nodeForFunc = CurrentScope->nodeForFunc;
                        ScopeTable[CurScopeIndex]->nestedLevel = ScopeLevel;
                        ScopeTable[CurScopeIndex]->Bucket = NULL;
                        ScopeTable[CurScopeIndex]->NofBuckets = 0;
                        ScopeTable[CurScopeIndex]->parent = CurrentScope;
                        CurrentScope = ScopeTable[CurScopeIndex];
                        //CurrentScope->returnType = CurrentScope->parent->returnType;
                    }
                    break;
                case CallK:
                    if(isDuplicatedError){
                        SemanticError(t, "Duplicated Function Declaration: function call line # is not added to symbol table");
                        return;
                    }
                    if(st_lookup(CurScopeIndex, t->attr.name, 1) == 1){//called function is declared
                        //insert line no.
                        st_insert(CurScopeIndex, t->attr.name, t->lineno, 0, t->type, t, 1,-1);
                    }
                    else{//called function is not declared
                        SemanticError(t, "Calling Undeclared Function that does not exist in Symbol Table\n");
                    }
                    break;
                case ReturnK:
                    break;
                case IterK:
                    break;
                case SelectK:
                    break;
                case AssignK:
                    break;
                default:
                    break;
            }
            break;
            
        case ExpK:
            switch (t->kind.exp){
                case VarK:
                    if(isDuplicatedError){
                        SemanticError(t, "Duplicated Function Declaration: Variable is not added to symbol table");
                        return;
                    }
                    /*
                    if(t->type == Void){
                        // void type variable error
                         SemanticError(t, "Declaration of [Void] type Variable");
                    }*/
                    if(st_lookup_excluding_parent(CurScopeIndex, t->attr.name, 0) == 1){
                        //variable with the same name already exists in Declaring scope
                        //error "duplicated Declaration of variables with same name"
                        SemanticError(t, "Duplicated Declaration of Variables with the Same Name; It will not be added into Symbol Table!");
                    }
                    else{
                        //insert into symtab
                        st_insert(CurScopeIndex, t->attr.name, t->lineno, location++, t->type, t,0,-1);
                    }
                    break;
                    
                case ArrVarK:
                    if(isDuplicatedError){
                        SemanticError(t, "Duplicated Function Declaration: Variable is not added to symbol table");
                        return;
                    }
                    /*
                    if(t->type == Void){
                        //voidArray Type variable error
                        SemanticError(t, "Declaration of [Void] type Variable");
                    }*/
                    if(st_lookup_excluding_parent(CurScopeIndex, t->attr.name,0) == 1){
                        //variable with the same name already exists in Declaring scope
                        //error "duplicated Declaration of variables with same name"
                        SemanticError(t, "Duplicated Declaration of Variables with the Same Name; It will not be added into Symbol Table!");
                    }
                    else{
                        //insert into symtab
                        st_insert(CurScopeIndex, t->attr.name, t->lineno, location++, t->type, t,0,-1);
                    }
                    break;
                    
                case FuncK:
                    if(st_lookup_excluding_parent(CurScopeIndex, t->attr.name,1)==1){
                        //Function with the same name already exists in Declaring scope
                        //error "duplicated Declaration of function with same name"
                        isDuplicatedError = 1;
                        functionID++;
                        SemanticError(t, "Duplicated Declaration of Function with the Same Name; It will not be added into Symbol Table!");
                    }
                    else{ //can declare;
                        //insert into symtab;
                        functionID++;
                        st_insert(CurScopeIndex, t->attr.name, t->lineno, location++, t->type, t, 1, functionID);
                        //add new scope
                        TotalScopes++;
                        CurScopeIndex = TotalScopes;
                        ScopeLevel++;
                        ScopeTable[CurScopeIndex] = (ScopeList)malloc(sizeof(struct ScopeListRec));
                        ScopeTable[CurScopeIndex]->name = t->attr.name;
                        ScopeTable[CurScopeIndex]->index = CurScopeIndex;
                        ScopeTable[CurScopeIndex]->nodeForFunc = t;
                        ScopeTable[CurScopeIndex]->nestedLevel = ScopeLevel;
                        ScopeTable[CurScopeIndex]->Bucket = NULL;
                        ScopeTable[CurScopeIndex]->NofBuckets = 0;
                        ScopeTable[CurScopeIndex]->parent = CurrentScope;
                        CurrentScope = ScopeTable[CurScopeIndex];
                        //flague on;
                        isInFuncKpre = 1;
                        funcCompound = 0;
                    }
                    break;
                    
                case ParamK:
                    if(isDuplicatedError){
                        SemanticError(t, "Duplicated Function Declaration: Parameter is not added to symbol table");
                        return;
                    }
                    if(t->type == Void && t->attr.name == NULL){
                        // do nothing (void) OK!
                    }
                    /*else if (t->type == Void && t->attr.name != NULL){
                        // void type parameter error
                        SemanticError(t, "Parameter's type should not be [Void]");
                    }*/
                    else if ( st_lookup_excluding_parent(CurScopeIndex, t->attr.name, 0) == 1 ){
                        //error, duplicated parameter name;
                        SemanticError(t, "Duplicated Parameter with the Same Name; It will not be added into Symbol Table, so this Parameter will be disregarded");
                    }
                    else if ( st_lookup_excluding_parent(CurScopeIndex, t->attr.name, 0) == -1 ){
                        //insert into symtab;
                        st_insert(CurScopeIndex, t->attr.name, t->lineno, location++, t->type, t,0,-1);
                    }
                    break;
                    
                case ArrParamK:
                    if(isDuplicatedError){
                        SemanticError(t, "Duplicated Function Declaration: Parameter is not added to symbol table");
                        return;
                    }
                    /*if(t->type == Void){
                        //error void[] type parameter;
                        SemanticError(t, "Parameter's type should not be [Void]");
                    }*/
                    if (st_lookup_excluding_parent(CurScopeIndex, t->attr.name,0)==1){
                         //error, duplicated parameter name;
                        SemanticError(t, "Duplicated Parameter with the Same Name; It will not be added into Symbol Table, so this Parameter will be disregarded");
                    }
                    else if(st_lookup_excluding_parent(CurScopeIndex, t->attr.name,0)!=1){
                        //insert into symtab
                        st_insert(CurScopeIndex, t->attr.name, t->lineno, location++, t->type, t,0,-1);
                    }
                    break;
                
                case IdK:
                    if(isDuplicatedError){
                        SemanticError(t, "Duplicated Function Declaration: line # is not added to symbol table");
                        return;
                    }
                    if( st_lookup(CurScopeIndex, t->attr.name,0) == -1){
                        //Does not exists in Symtab;
                        //error Semantic Error: undeclared variable;
                        SemanticError(t, "Using Undeclared Variable that does not exist in Symbol Table");
                    } else{
                        //insert line no.
                        st_insert(CurScopeIndex, t->attr.name, t->lineno, 0, t->type, t,0,-1);
                    }
                    break;

                case RelOpK:
                    break;
                case OpK:
                    break;
                case ConstK:
                    break;
                default:
                    break;
            }
            break;
            
        default:
            break;
    }
}

//original version
/*
static void insertNode( TreeNode * t)
{ switch (t->nodekind)
  { case StmtK:
      switch (t->kind.stmt)
      { case AssignK:
        case ReadK:
          if (st_lookup(t->attr.name) == -1)
           //not yet in table, so treat as new definition
            st_insert(t->attr.name,t->lineno,location++);
          else
          // already in table, so ignore location,
          //   add line number of use only
            st_insert(t->attr.name,t->lineno,0);
          break;
        default:
          break;
      }
      break;
    case ExpK:
      switch (t->kind.exp)
      { case IdK:
          if (st_lookup(t->attr.name) == -1)
          // not yet in table, so treat as new definition
            st_insert(t->attr.name,t->lineno,location++);
          else
          //already in table, so ignore location,
            // add line number of use only
            st_insert(t->attr.name,t->lineno,0);
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }
}
*/

/* Function buildSymtab constructs the symbol 
 * table by preorder traversal of the syntax tree
 */
void buildSymtab(TreeNode * syntaxTree)
{
    initialize(); // adding Input(), Output() functions to Global Scope.
    traverse(syntaxTree,insertNode,nullProc);
    fprintf(listing, "\nBuilding Symbol Table is done!!\n");
  if (TraceAnalyze)
  { fprintf(listing,"\n\n [1].<Symbol table> \n\n");
    printSymTab(listing);
  }
}

static void typeError(TreeNode * t, char * message)
{ fprintf(listing,"Error(while type-checking) at line %d: %s\n",t->lineno,message);
  Error = TRUE;
}

/* Procedure checkNode performs
 * type checking at a single tree node
 */
/*
static void checkNode(TreeNode * t)
{ switch (t->nodekind)
  { case ExpK:
      switch (t->kind.exp)
      { case OpK:
          if ((t->child[0]->type != Integer) ||
              (t->child[1]->type != Integer))
            typeError(t,"Op applied to non-integer");
          if ((t->attr.op == EQ) || (t->attr.op == LT))
            t->type = Boolean;
          else
            t->type = Integer;
          break;
        case ConstK:
        case IdK:
          t->type = Integer;
          break;
        default:
          break;
      }
      break;
    case StmtK:
      switch (t->kind.stmt)
      { case IfK:
          if (t->child[0]->type == Integer)
            typeError(t->child[0],"if test is not Boolean");
          break;
        case AssignK:
          if (t->child[0]->type != Integer)
            typeError(t->child[0],"assignment of non-integer value");
          break;
        case WriteK:
          if (t->child[0]->type != Integer)
            typeError(t->child[0],"write of non-integer value");
          break;
        case RepeatK:
          if (t->child[1]->type == Integer)
            typeError(t->child[1],"repeat test is not Boolean");
          break;
        default:
          break;
      }
      break;
    default:
      break;

  }
}*/

static void nullProcForCheck(TreeNode * t){
    if(isDuplicatedError == 0){
        
    if(t->nodekind == ExpK && t->kind.exp == FuncK){//when entering function Declaration
        functionID++;
        if(st_lookup_for_typecheck(CurrentScope->index, t->attr.name, 1,functionID) == 1){//exist
            ScopeList S; int i;
            isInFuncKpre = 1;
            ScopeLevel++;
            CheckedScopeID++;
            ReturnTypeErr = 0;
            if(t->type == Void){
                isWellReturned = 1;
            }
            //Change Current scope to the Scope of Declaring FuncK.
            /*
             for(i = 0; i<= TotalScopes;i++){
                if(ScopeTable[i]->name != NULL && strcmp(ScopeTable[i]->name, t->attr.name) == 0){
                    CurrentScope = ScopeTable[i];
                    break;
                }
            }*/
            CurrentScope = ScopeTable[CheckedScopeID];
            //fprintf(listing,"scope down to function scope\n" );
        }
        else{// Not Declared or Duplicated Declaration
            isDuplicatedError = 1;
        }
    }
    else if(isInFuncKpre==1 && funcCompound!=1 &&  t->nodekind == StmtK &&t->kind.stmt == CompndK){
        //when entering functions compound Stmt
        // No Change for SCope; just Flague on;
        funcCompound = 1;
    }
    else if(isInFuncKpre == 1 && funcCompound == 1 && t->nodekind == StmtK &&t->kind.stmt == CompndK){
        // just compound Statement
        // change scope
        //ScopeList S;
        
        CheckedScopeID++;
        //여길 고쳐야함
        CurrentScope = ScopeTable[CheckedScopeID];
        
        
        ScopeLevel++;
        just_compound++;
        //fprintf(listing,"scope down to just compound scope\n" );
    }
        
    }
    else{
        if(t->nodekind == ExpK && t->kind.exp == FuncK)
            isDuplicatedError = 0;
    }
}


static void checkNode(TreeNode * t){
    switch (t->nodekind){
        case ExpK:
            switch(t->kind.exp){
                case VarK:
                    if(isDuplicatedError){
                        t->type = TypeError;
                        typeError(t,"Duplicated Declaration of function: Variable is Disregarded!");
                        return;
                    }
                    if(t->type == Void)
                        typeError(t,"Declaration of [Void] type Variable is Invaild");
                    break;
                    
                case ArrVarK:
                    if(isDuplicatedError){
                        t->type = TypeError;
                        typeError(t,"Duplicated Declaration of function: Variable is Disregarded!");
                        return;
                    }
                    if(t->type == VoidArray)
                         typeError(t,"Declaration of [VoidArray] type Variable is Invaild");
                    break;
                    
                case FuncK:
                    if(isDuplicatedError){
                        t->type = TypeError;
                        isDuplicatedError = 0;
                        
                        typeError(t,"Duplicated Declaration of function!!!");
                        return;
                    }
                    if(isWellReturned == 1 && ReturnTypeErr != 1){
                        isWellReturned = 0;
                        ReturnTypeErr = 0;
                        isInFuncKpre = funcCompound = 0;
                        CurrentScope = CurrentScope->parent;
                        ScopeLevel--;
                    }
                    else{
                        isWellReturned = 0;
                        ReturnTypeErr = 0;
                        ScopeLevel--;
                        CurrentScope = CurrentScope->parent;
                        
                        typeError(t, "Return-Statement is [Missing] or [NOT properly stated] in this function");
                    }
                    //fprintf(listing, "scope up form function scope\n");
                    break;
                    
                case ParamK:
                    if(isDuplicatedError){
                        t->type = TypeError;
                        typeError(t,"Duplicated Declaration of function: Parameter is Disregarded!");
                        return;
                    }
                    if(t->type == Void){
                        if(t->sibling != NULL){
                            typeError(t,"[Void] Type Parameter is Invalid");
                        }
                    }
                    break;
                    
                case ArrParamK:
                    if(isDuplicatedError){
                        t->type = TypeError;
                        typeError(t,"Duplicated Declaration of function: Parameter is Disregarded!");
                        return;
                    }
                    if(t->type == VoidArray)
                        typeError(t,"[VoidArray] Type Parameter is Invalid!");
                    break;
                    
                case RelOpK:
                    if(isDuplicatedError){
                        t->type = TypeError;
                        typeError(t,"Duplicated Declaration of function: Expression is Disregarded !");
                        return;
                    }
                    if(t->child[0]->type==Integer && t->child[1]->type==Integer){
                        t->type = Boolean;
                    }
                    else{
                        t->type = TypeError;
                        typeError(t,"Operand of RelOp should be [Interger] type");
                    }
                    break;
                    
                case OpK:
                    if(isDuplicatedError){
                        t->type = TypeError;
                        typeError(t,"Duplicated Declaration of function: Expression is Disregarded !");
                        return;
                    }
                    if(t->child[0]->type==Integer && t->child[1]->type==Integer){
                        t->type = Integer;
                    }
                    else{
                        t->type = TypeError;
                        typeError(t,"Operand of Op should be [Interger] type");
                    }
                    break;
                    
                case ConstK:
                    if(isDuplicatedError){
                        t->type = TypeError;
                        typeError(t,"Duplicated Declaration of function: Expression is Disregarded !");
                        return;
                    }
                    t->type = Integer;
                    break;
                    
                case IdK://lookup sympad;
                    
                    if(isDuplicatedError){
                        t->type = TypeError;
                        typeError(t,"Duplicated Declaration of function: Expression is Disregarded !");
                        return;
                    }
                    
                {
                    ScopeList S = CurrentScope;
                    BucketList l;
                    int wasFunc = 0;
                    int i=0;
                    while(S!=NULL){
                        l=S->Bucket;
                        while(i < S->NofBuckets  &&  l != NULL){
                            if(l!=NULL && strcmp(t->attr.name,l->name)!=0){
                                l=l->next;
                            }
                            
                            else if(l!=NULL && strcmp(t->attr.name,l->name)==0&& l->isFunc==1){
                                wasFunc = 1;
                                l=l->next;
                            }
                            
                            else if(l!=NULL && strcmp(t->attr.name, l->name) == 0 && l->isFunc == 0){
                                /*if(l->node->nodekind == ExpK && l->node->kind.exp == FuncK){//funciton name을 찾은경우
                                    // search more
                                    wasFunc = 1;
                                }*/
                                if(l->type == IntegerArray && t->child[0]==NULL){
                                    t->type = IntegerArray;
                                    return;
                                }
                                else if(l->type == IntegerArray && t->child[0]!=NULL){
                                    t->type = Integer;
                                    return;
                                }
                                else if(l->type == Integer && t->child[0]!=NULL){
                                    t->type = TypeError;
                                    typeError(t, "Using [Integer] Type Variable as [IntegerArray]; Accessing by index is Invalid");
                                    return;
                                }
                                else{
                                    t->type = l->type;
                                    return;
                                }
                            }
                            i++;
                        }
                        S = S->parent;
                        i=0;
                    }
                    if(wasFunc){
                        t->type = TypeError;
                        typeError(t, "Using Function name as a variable is Invalid!!");
                        return;
                    }
                    t->type = TypeError;
                    typeError(t, "Using Undeclared Variable");
                }
                    
                    break;
            }
            break;
        case StmtK:
            switch(t->kind.stmt){
                case AssignK://
                    if(isDuplicatedError){
                        t->type = TypeError;
                        typeError(t,"Duplicated Declaration of function: Expression is Disregarded !");
                        return;
                    }
                    if (t->child[0]->type != Integer || t->child[1]->type != Integer)
                        typeError(t->child[0],"Type Conflict in Assignment; LHS's type and RHS's type should be all Integer");
                    break;
                    
                case CompndK:
                    if(isDuplicatedError){
                        t->type = TypeError;
                        typeError(t,"Duplicated Declaration of function: function is Disregarded !");
                        return;
                    }
                    if(just_compound > 0){
                        ScopeLevel--;
                        CurrentScope = CurrentScope->parent;
                        //fprintf(listing, "scope up from juct copmstmt\n");
                        just_compound--;
                    }
                    else if(isInFuncKpre==1 && funcCompound==1){
                        funcCompound=0;
                    }
                    break;
                    
                case SelectK://
                    if(isDuplicatedError){
                        t->type = TypeError;
                        typeError(t,"Duplicated Declaration of function: Expression is Disregarded !");
                        return;
                    }
                    if(t->child[0]->type != Boolean)
                        typeError(t->child[0],"Invalid Expression, Conditional of If() should be [Boolean] Type");
                    break;
                    
                case IterK://
                    if(isDuplicatedError){
                        t->type = TypeError;
                        typeError(t,"Duplicated Declaration of function: Expression is Disregarded !");
                        return;
                    }
                    if(t->child[0]->type != Boolean)
                        typeError(t->child[0],"Invalid Expression, Conditional of While() should be [Boolean] Type");
                    break;
                    
                case ReturnK://함수의 리턴타입이랑 맞는지 확인해야함.
                    if(isDuplicatedError){
                        t->type = TypeError;
                        typeError(t,"Duplicated Declaration of function: Expression is Disregarded !");
                        return;
                    }
                    if(CurrentScope->nodeForFunc->type == Void){ // should be return;
                        if(t->child[0] != NULL){
                            typeError(t, "Invalid Return Type!, Return Type should be [Void]");
                            ReturnTypeErr = 1;
                        }
                        else{
                            isWellReturned = 1;
                        }
                    }
                    else if(CurrentScope->nodeForFunc->type == Integer){ //should return somthing;
                        if(t->child[0]==NULL){
                            typeError(t, "Invalid Return Type!, Return Type should be [Interger]");
                            ReturnTypeErr = 1;
                        }
                        else if(t->child[0]->type != Integer){
                            typeError(t, "Invalid Return Type!, Return Type should be [Interger]");
                            ReturnTypeErr = 1;
                        }
                        else{
                            isWellReturned = 1;
                        }
                    }
                    break;
            
                case CallK://
                    if(isDuplicatedError){
                        t->type = TypeError;
                        typeError(t,"Duplicated Declaration of function: Expression is Disregarded!");
                        return;
                    }
                    
                    if(t->child[0] == NULL){//void parameter//
                        BucketList l = ScopeTable[0]->Bucket;
                        while(l != NULL){
                            if(strcmp(l->name,t->attr.name) == 0 && l->isFunc == 1)
                                break;
                            else
                                l=l->next;
                        }
                        if(l!=NULL){//found!
                            t->type = l->type;
                            if(l->memloc == -1 ){//case of input fuc
                                //No error
                                return;
                            }
                            else if(l->memloc == -2){//case of calling output without parameter Error
                                typeError(t, "Need Proper Parameter for Function Call");
                            }
                            else if(l->node->child[0]->type == Void && l->node->child[0]->sibling == NULL){
                                //good NO ERror
                                return;
                            }
                            else{
                                typeError(t, "Need Proper Parameter for Function Call");
                            }
                        }
                        else{
                            t->type = TypeError;
                            typeError(t, "Calling Undeclared Function");
                        }
                    }
                    else{//parameter list 존재하는 경우//
                        TreeNode * tempnode;
                        TreeNode * params;
                        BucketList l = ScopeTable[0]->Bucket;
                        while(l != NULL){
                            if(strcmp(l->name,t->attr.name) == 0 && l->isFunc == 1)
                                break;
                            else
                                l=l->next;
                        }
                        if(l!=NULL){//found!
                            t->type = l->type;
                            if(l->memloc == -1 ){//case of calling input with parameter
                                //error
                                typeError(t, "Need Proper Parameter for Function Call");
                            }
                            else if(l->memloc == -2){//case of calling output without parameter Error
                                //No error
                                if(t->child[0]->type == Integer && t->child[0]->sibling == NULL){
                                    //no Error
                                    return;
                                }
                                else{
                                    typeError(t, "Need Proper Parameter for Function Call");
                                }
                            }
                            else if(l->node->child[0]->type==Void && l->node->child[0]->sibling==NULL){
                                typeError(t, "Need Proper Parameter for Function Call");
                            }
                            else{
                                tempnode = l->node->child[0];//decl param
                                params = t->child[0];//call param
                                while(tempnode!=NULL && params!=NULL){
                                    if(tempnode->type == params->type){
                                        tempnode = tempnode->sibling;
                                        params = params->sibling;
                                    }
                                    else{
                                        typeError(t, "Need Proper Parameter for Function Call");
                                        return;
                                    }
                                }
                                if(tempnode==NULL && params==NULL){
                                    return;
                                }
                                else{
                                    typeError(t, "Need Proper Parameter for Function Call");
                                }
                            }
                        }
                        else {
                            t->type = TypeError;
                            typeError(t, "Calling Undeclared Function");
                        }
                    }
                    
                    break;
            }
            break;
        default:
            break;
    }
}
    
    

/* Procedure typeCheck performs type checking 
 * by a postorder syntax tree traversal
 */
void typeCheck(TreeNode * syntaxTree)
{
    functionID = 0;
    isDuplicatedError =0;
    funcCompound = 0;
    isInFuncKpre = 0;
    just_compound = 0;
    CheckedScopeID = 0;
    traverse(syntaxTree,nullProcForCheck,checkNode);
}
