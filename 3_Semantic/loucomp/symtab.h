/****************************************************/
/* File: symtab.h                                   */
/* Symbol table interface for the TINY compiler     */
/* (allows only one symbol table)                   */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/****************************************************/

#ifndef _SYMTAB_H_
#define _SYMTAB_H_

#include "globals.h"
/* Procedure st_insert inserts line numbers and
 * memory locations into the symbol table
 * loc = memory location is inserted only the
 * first time, otherwise ignored
 */


//----For Error handling-----//
//--- NOT USING THESE--------//
void st_insertF( char * name, int lineno, int loc );
int st_lookupF ( char * name );
//---------------------------//







//--- edited for Project 03 ---//
//void st_insert( char * name, int lineno, int loc );
void st_insert( int ScopeIndex, char * name, int lineno, int loc, ExpType type,TreeNode * node ,int isFunc,int functionID);
//int st_lookup ( char * name );
int st_lookup(int ScopeIndex, char * name, int isFunc);
int st_lookup_excluding_parent(int ScopeIndex, char * name, int isFunc);

int st_lookup_for_typecheck(int ScopeIndex, char * name, int isFunc, int functionID);

/* Procedure printSymTab prints a formatted
 * listing of the symbol table contents
 * to the listing file
 */
void printSymTab(FILE * listing);

typedef struct LineListRec
{ int lineno;
  struct LineListRec * next;
} * LineList;

typedef struct BucketListRec
   { char * name;
     LineList lines;
     int memloc ; /* memory location for variable */
     struct BucketListRec * next;
    //----edited ----// for saving node
       TreeNode * node;
       ExpType type;
       int isFunc;
       int funcID;
    //---------------//
   } * BucketList;

typedef struct ScopeListRec{
    char * name;
    BucketList Bucket;
    int NofBuckets;
    int index; //index of hash table;
    int nestedLevel; //for limiting should-be-checked scopes.
    struct ScopeListRec * parent;
    TreeNode * nodeForFunc;
} * ScopeList;

/* the hash table */
ScopeList ScopeTable[500];



#endif
