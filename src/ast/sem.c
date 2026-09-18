#include <string.h>

#include "common.h"
#include "ast/sem.h"

// The program being analysed, for resolving calls against its definitions.
Ast_Func *Sem_Prog;

// The type of a string literal, built once and shared.
Ast_Type *Sem_TypeCharPtr;

// Returns the function of that name defined in this program, or NULL.
Ast_Func *Sem_FindFunc(const char *name)
{
    for (Ast_Func *func = Sem_Prog; func; func = func->af_next) {
        if (strcmp(func->af_name, name) == 0) {
            return func;
        }
    }
    return NULL;
}

// Returns the length of a node list.
int Sem_CountNodes(Ast_Node *list)
{
    int count = 0;
    for (Ast_Node *node = list; node; node = node->an_next) {
        count++;
    }
    return count;
}

// Checks a call against the callee's definition, if this program has one.
void Sem_CheckCall(Ast_Node *node)
{
    Ast_Func *func = Sem_FindFunc(node->an_funcname);
    if (! func) {
        return;
    }
    int given = Sem_CountNodes(node->an_args);
    if (given != func->af_nparams) {
        Show_ErrorAt(node->an_line, "wrong number of arguments to '%s': got %d, expected %d",
                     node->an_funcname, given, func->af_nparams);
    }
}

// Annotates a node and everything below it, depth first.
void Sem_Node(Ast_Node *node)
{
    if (! node) {
        return;
    }

    Sem_Node(node->an_lhs);
    Sem_Node(node->an_rhs);
    Sem_Node(node->an_cond);
    Sem_Node(node->an_then);
    Sem_Node(node->an_els);
    Sem_Node(node->an_init);
    Sem_Node(node->an_inc);
    Sem_Node(node->an_body);
    Sem_Node(node->an_args);
    Sem_Node(node->an_next);

    switch (node->an_kind) {
        case AST_NODE_KIND_NUM:
        case AST_NODE_KIND_ADD:
        case AST_NODE_KIND_SUB:
        case AST_NODE_KIND_MUL:
        case AST_NODE_KIND_DIV:
        case AST_NODE_KIND_MOD:
        case AST_NODE_KIND_NEG:
        case AST_NODE_KIND_NOT:
        case AST_NODE_KIND_EQ:
        case AST_NODE_KIND_NE:
        case AST_NODE_KIND_LT:
        case AST_NODE_KIND_LE:
        case AST_NODE_KIND_AND:
        case AST_NODE_KIND_OR:
            node->an_type = &Ast_TypeInt;
            break;

        case AST_NODE_KIND_STR:
            node->an_type = Sem_TypeCharPtr;
            break;

        case AST_NODE_KIND_VAR:
            node->an_type = node->an_var->av_type;
            break;

        case AST_NODE_KIND_ASSIGN:
            if (node->an_lhs->an_kind != AST_NODE_KIND_VAR) {
                Show_ErrorAt(node->an_line, "expression is not assignable");
            }
            node->an_type = node->an_lhs->an_type;
            break;

        case AST_NODE_KIND_CALL:
            Sem_CheckCall(node);
            node->an_type = &Ast_TypeInt;
            break;

        case AST_NODE_KIND_RETURN:
        case AST_NODE_KIND_IF:
        case AST_NODE_KIND_FOR:
        case AST_NODE_KIND_BLOCK:
        case AST_NODE_KIND_EXPR_STMT:
        case AST_NODE_KIND_NOP:
            break;
    }
}


// Annotates every node with its type and rejects what the grammar cannot.
void Sem_Analyze(Ast_Func *prog)
{
    Sem_Prog        = prog;
    Sem_TypeCharPtr = Ast_NewPointer(&Ast_TypeChar);

    for (Ast_Func *func = prog; func; func = func->af_next) {
        Sem_Node(func->af_body);
    }
}
