#ifndef SEM_H
#define SEM_H

#include "ast/ast.h"

// Lookups over the program being analysed
Ast_Func *Sem_FindFunc(const char *name);
int       Sem_CountNodes(Ast_Node *list);

// Type and expression queries
int       Sem_IsPointer(const Ast_Type *type);
int       Sem_IsLvalue(const Ast_Node *node);
Ast_Type *Sem_Decay(Ast_Type *type);

// Checks the parser cannot make
void      Sem_CheckCall(Ast_Node *node);

// Annotation
Ast_Node *Sem_ScaleBy(Ast_Node *node, int size);
void      Sem_Arith(Ast_Node *node);
void      Sem_Node(Ast_Node *node);
void      Sem_Analyze(Ast_Func *prog);

#endif // SEM_H
