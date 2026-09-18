#ifndef SEM_H
#define SEM_H

#include "ast/ast.h"

// Pass state
extern Ast_Func *Sem_Prog;
extern Ast_Type *Sem_TypeCharPtr;

// Lookups over the program being analysed
Ast_Func *Sem_FindFunc(const char *name);
int       Sem_CountNodes(Ast_Node *list);

// Checks the parser cannot make
void Sem_CheckCall(Ast_Node *node);

// Annotation
void Sem_Node(Ast_Node *node);
void Sem_Analyze(Ast_Func *prog);

#endif // SEM_H
