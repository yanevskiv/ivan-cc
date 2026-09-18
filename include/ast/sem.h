#ifndef SEM_H
#define SEM_H

#include "ast/ast.h"

// Annotates every node with its type and rejects what the grammar cannot.
void Sem_Analyze(Ast_Func *prog);

#endif // SEM_H
