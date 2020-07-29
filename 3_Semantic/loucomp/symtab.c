/****************************************************/
/* File: symtab.c                                   */
/* Symbol table implementation for the TINY compiler*/
/* (allows only one symbol table)                   */
/* Symbol table is implemented as a chained         */
/* hash table                                       */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/****************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "symtab.h"

/* SIZE is the size of the hash table */
#define SIZE 500

/* SHIFT is the power of two used as multiplier
   in hash function  */
#define SHIFT 4

//------------- NOT USING ----------------------------------//
//------ Just for solving compile error for cgen.c----------//

void st_insertF( char * name, int lineno, int loc )
{ int h = hash(name);
  BucketList l =  ScopeTable[h];
  while ((l != NULL) && (strcmp(name,l->name) != 0))
    l = l->next;
  if (l == NULL) /* variable not yet in table */
  { l = (BucketList) malloc(sizeof(struct BucketListRec));
    l->name = name;
    l->lines = (LineList) malloc(sizeof(struct LineListRec));
    l->lines->lineno = lineno;
    l->memloc = loc;
    l->lines->next = NULL;
    l->next = ScopeTable[h];
    ScopeTable[h] = l; }
  else /* found in table, so just add line number */
  { LineList t = l->lines;
    while (t->next != NULL) t = t->next;
    t->next = (LineList) malloc(sizeof(struct LineListRec));
    t->next->lineno = lineno;
    t->next->next = NULL;
  }
} /* st_insert */

int st_lookupF ( char * name )
{ int h = hash(name);
  BucketList l =  ScopeTable[h];
  while ((l != NULL) && (strcmp(name,l->name) != 0))
    l = l->next;
  if (l == NULL) return -1;
  else return l->memloc;
}
/* the hash function */
int hash ( char * key )
{ int temp = 0;
  int i = 0;
  while (key[i] != '\0')
  { temp = ((temp << SHIFT) + key[i]) % SIZE;
    ++i;
  }
  return temp;
}
//----------------------------------------------------------//
//----------------------------------------------------------//




/* Procedure st_insert inserts line numbers and
 * memory locations into the symbol table
 * loc = memory location is inserted only the
 * first time, otherwise ignored
 */

/*
void st_insert( char * name, int lineno, int loc)
{ int h = hash(name);
  BucketList l =  hashTable[h];
  while ((l != NULL) && (strcmp(name,l->name) != 0))
    l = l->next;
  if (l == NULL) //variable not yet in table
  { l = (BucketList) malloc(sizeof(struct BucketListRec));
    l->name = name;
    l->lines = (LineList) malloc(sizeof(struct LineListRec));
    l->lines->lineno = lineno;
    l->memloc = loc;
    l->lines->next = NULL;
    l->next = hashTable[h];
    hashTable[h] = l; }
  else // found in table, so just add line number
  { LineList t = l->lines;
    while (t->next != NULL) t = t->next;
    t->next = (LineList) malloc(sizeof(struct LineListRec));
    t->next->lineno = lineno;
    t->next->next = NULL;
  }
}*/
/* st_insert */


//inserting the "var" into Symbol Table
void st_insert( int ScopeIndex, char* name, int lineno, int loc, ExpType type, TreeNode * node, int isFunc, int functionID){
    if(loc!=0){
        //case of adding new bucket.
        //when  not added in Symboltable yet
        BucketList l = ScopeTable[ScopeIndex]->Bucket;
        while( l != NULL ){
            if(strcmp(name,l->name) == 0 && l->isFunc == isFunc){
                break;
            }
            l = l->next;
        }
        if(l == NULL){ // if the "var" does not exist in  current Scope make a new Bucket
            l = (BucketList)malloc(sizeof(struct BucketListRec));
            l->name = name;
            l->lines = (LineList) malloc(sizeof(struct BucketListRec));
            l->lines->lineno = lineno;
            l->lines->next = NULL;
            l->memloc = loc;
            l->type = type;
            l->isFunc = isFunc;
            l->funcID = functionID;
            l->node = node;
            l->next = ScopeTable[ScopeIndex]->Bucket;
            ScopeTable[ScopeIndex]->Bucket = l;
            ScopeTable[ScopeIndex]->NofBuckets++;
        }
        else { // if the "var" already exist in current scope, just add line number
            //this will not happen;;
            LineList t = l->lines;
            while (t->next != NULL)
                t = t->next;
            t->next = (LineList) malloc(sizeof(struct LineListRec));
            t->next->lineno = lineno;
            t->next->next = NULL;
        }
    }
    else{
        ScopeList S = ScopeTable[ScopeIndex];
        BucketList l;
        int n=0;
        while(S!=NULL){
            l = S->Bucket;
            while(n < S->NofBuckets){
                if(l!=NULL && strcmp(name, l->name) != 0){
                    l = l->next;
                    n++;
                }
                else if (l!=NULL && strcmp(name, l->name) == 0){
                    LineList t = l->lines;
                    while (t->next != NULL)
                        t = t->next;
                    t->next = (LineList) malloc(sizeof(struct LineListRec));
                    t->next->lineno = lineno;
                    t->next->next = NULL;
                    //fprintf(listing,"line is well added\n");
                    return;
                }
            }
            S = S->parent;
            n=0;
        }
    }
}


/* Function st_lookup returns the memory 
 * location of a variable or -1 if not found
 */
/*
int st_lookup ( char * name )
{ int h = hash(name);
  BucketList l =  hashTable[h];
  while ((l != NULL) && (strcmp(name,l->name) != 0))
    l = l->next;
  if (l == NULL) return -1;
  else return l->memloc;
}*/

// look up all the scope from current to all ancestors to find whether "var" exists or not
// when using or calling variable;
int st_lookup(int ScopeIndex, char * name, int isFunc){
    ScopeList S = ScopeTable[ScopeIndex]; //set current scope to start lookup
    BucketList l;
    int n = 0;
    while(S != NULL){
        l = S->Bucket;
        while(n < S->NofBuckets){
            
            if(l!=NULL && strcmp(name, l->name) == 0 && l->isFunc == isFunc){
                return 1;
            }
            else{
                l= l->next;
                n++;
            }
        }
        S = S->parent;
        n = 0;
    }
    return -1;//variable is not declared
}

//lookup only current scope
//when variable declaration or parameter adding
int st_lookup_excluding_parent(int ScopeIndex, char * name, int isFunc){
    ScopeList S = ScopeTable[ScopeIndex];
    BucketList l = S->Bucket;
    if(l == NULL)
        return -1;
    while( l != NULL ){
        if(strcmp(name, l->name)==0 && l->isFunc == isFunc){
            // found it!
            return 1;
        }
        else{
            l = l->next;
        }
    }
   return -1;// variable is not declared
}

int st_lookup_for_typecheck(int ScopeIndex, char * name, int isFunc, int functionID){
    ScopeList S = ScopeTable[ScopeIndex]; //set current scope to start lookup
    BucketList l;
    int n = 0;
    while(S != NULL){
        l = S->Bucket;
        while(n < S->NofBuckets){
            
            if(l!=NULL && strcmp(name, l->name) == 0 && l->isFunc == isFunc&& l->funcID == functionID){
                return 1;
            }
            else{
                l= l->next;
                n++;
            }
        }
        S = S->parent;
        n = 0;
    }
    return -1;//variable is not declared
}


/* Procedure printSymTab prints a formatted
 * listing of the symbol table contents
 * to the listing file
 */
void printSymTab(FILE * listing)
{ int i; BucketList B;
  fprintf(listing,"Scope Name & Number   Symbol Name   Location   Line Numbers\n");
  fprintf(listing,"-------------------  -------------  --------   -----------------------\n");
  for (i=0;i<500;++i)
  { if (ScopeTable[i] != NULL)
    { BucketList l = ScopeTable[i]->Bucket;
        
      while (l != NULL)
      {fprintf(listing, "%10s %6d ",ScopeTable[i]->name, ScopeTable[i]->index);
          LineList t = l->lines;
        fprintf(listing,"%14s ",l->name);
        fprintf(listing,"%8d     ",l->memloc);
        while (t != NULL)
        { fprintf(listing,"%4d",t->lineno);
          t = t->next;
        }
        fprintf(listing,"\n");
        l = l->next;
      }
    }
  }
    
    
    fprintf(listing,"\n\n [2].<Scope Table>\n\n");
    fprintf(listing,"Scope Name & Number  Parant name & No.  nested Level\n");
    fprintf(listing,"-------------------  -----------------  ------------\n");
    for(i=0;i<500;i++){
        if(ScopeTable[i] != NULL){
            fprintf(listing, "%10s %6d", ScopeTable[i]->name, ScopeTable[i]->index);
            if(ScopeTable[i]->parent!=NULL){
                fprintf(listing,"   %10s %6d", ScopeTable[i]->parent->name, ScopeTable[i]->parent->index);
            }
            else{
                fprintf(listing,"          (NULL)    ");
            }
            fprintf(listing," %8d\n", ScopeTable[i]->nestedLevel);
            
        }
    }
    
    
    fprintf(listing,"\n\n [3].<Function and Global variables>\n\n");
    fprintf(listing,"    ID name     ID type      DATAType\n");
    fprintf(listing,"------------   ---------    ------------\n");
    B = ScopeTable[0]->Bucket;
    i=0;
    while( B != NULL ){
        fprintf(listing, "%10s ", B->name);
        
        if(B->memloc < 0)
            fprintf(listing, "     Function ");
        else if(B->node->nodekind == ExpK && B->node->kind.exp == FuncK)
            fprintf(listing, "     Function ");
        else
            fprintf(listing, "     Variable ");
        
        if(B->type == Integer)
            fprintf(listing, "    Integer\n");
        else if(B->type == IntegerArray)
            fprintf(listing, "   IntegerArray\n");
        else //if(B->type == Void)
            fprintf(listing, "      Void\n");
        B = B->next;
    }
    fprintf(listing,"\n");
    
    
    fprintf(listing,"\n [4].<Function Parameter and local variables>\n\n");
    fprintf(listing," Scope Name&no.     nestedLevel    ID name        DataType\n");
    fprintf(listing,"-----------------   -----------    -----------    ---------\n");
    for(i=1;i<500;i++){
        if(ScopeTable[i] != NULL){
            B = ScopeTable[i]->Bucket;
            while(B!=NULL){
                fprintf(listing, "%8s %6d", ScopeTable[i]->name, ScopeTable[i]->index);
                fprintf(listing, "%13d ", ScopeTable[i]->nestedLevel);
                fprintf(listing, "%15s", B->name);
                
                if(B->type == Integer)
                    fprintf(listing, "       Integer\n");
                else if(B->type == IntegerArray)
                    fprintf(listing, "      IntegerArray\n");
                else //if(B->type == Void)
                    fprintf(listing, "         Void\n");
                
                B = B->next;
            }
            fprintf(listing,"\n");
        }
    }
} /* printSymTab */
