#include <string.h>

#include "util/log.h"
#include "ast/sem.h"

// The program being analysed, for resolving calls against its definitions.
static Ast_Func *Sem_Prog;

// The type of a string literal, built once and shared.
static Ast_Type *Sem_TypeCharPtr;

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

// Returns whether values of this type address memory: a pointer, or an array.
int Sem_IsPointer(const Ast_Type *type)
{
    return type->at_kind == AST_TYPE_KIND_PTR || type->at_kind == AST_TYPE_KIND_ARRAY;
}

// Returns whether a node names an object, so that it can be assigned or addressed.
int Sem_IsLvalue(const Ast_Node *node)
{
    return node->an_kind == AST_NODE_KIND_VAR || node->an_kind == AST_NODE_KIND_DEREF;
}

// Returns the type an expression of this type yields; an array yields a pointer.
Ast_Type *Sem_Decay(Ast_Type *type)
{
    if (type->at_kind == AST_TYPE_KIND_ARRAY) {
        return Ast_NewPointer(type->at_base);
    }
    return type;
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
        Log_ShowErrorAt(node->an_line, "wrong number of arguments to '%s': got %d, expected %d", node->an_funcname, given, func->af_nparams);
    }
}

// Wraps node in a multiplication by size, so it steps whole elements.
Ast_Node *Sem_ScaleBy(Ast_Node *node, int size)
{
    Ast_Node *num = Ast_NewNum(size, node->an_line);
    num->an_type = &Ast_TypeInt;

    Ast_Node *mul = Ast_NewBinary(AST_NODE_KIND_MUL, node, num, node->an_line);
    mul->an_type = &Ast_TypeInt;
    return mul;
}

// Types + and -, scaling an integer operand against a pointer and reducing p - q.
void Sem_Arith(Ast_Node *node)
{
    Ast_Type *lhs = node->an_lhs->an_type;
    Ast_Type *rhs = node->an_rhs->an_type;

    if (! Sem_IsPointer(lhs) && ! Sem_IsPointer(rhs)) {
        node->an_type = &Ast_TypeInt;
        return;
    }

    if (Sem_IsPointer(lhs) && Sem_IsPointer(rhs)) {
        if (node->an_kind != AST_NODE_KIND_SUB) {
            Log_ShowErrorAt(node->an_line, "cannot add two pointers");
        }

        // A pointer difference counts elements, not the bytes between them.
        Ast_Node *diff = Ast_NewBinary(AST_NODE_KIND_SUB, node->an_lhs, node->an_rhs, node->an_line);
        diff->an_type = &Ast_TypeInt;

        node->an_kind = AST_NODE_KIND_DIV;
        node->an_lhs  = diff;
        node->an_rhs  = Ast_NewNum(lhs->at_base->at_size, node->an_line);
        node->an_rhs->an_type = &Ast_TypeInt;
        node->an_type = &Ast_TypeInt;
        return;
    }

    if (Sem_IsPointer(rhs)) {
        if (node->an_kind != AST_NODE_KIND_ADD) {
            Log_ShowErrorAt(node->an_line, "cannot subtract a pointer from an integer");
        }
        node->an_lhs  = Sem_ScaleBy(node->an_lhs, rhs->at_base->at_size);
        node->an_type = Sem_Decay(rhs);
        return;
    }

    node->an_rhs  = Sem_ScaleBy(node->an_rhs, lhs->at_base->at_size);
    node->an_type = Sem_Decay(lhs);
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
        case AST_NODE_KIND_OR: {
            node->an_type = &Ast_TypeInt;
        } break;

        case AST_NODE_KIND_ADD:
        case AST_NODE_KIND_SUB: {
            Sem_Arith(node);
        } break;

        case AST_NODE_KIND_STR: {
            node->an_type = Sem_TypeCharPtr;
        } break;

        case AST_NODE_KIND_ADDR: {
            if (! Sem_IsLvalue(node->an_lhs)) {
                Log_ShowErrorAt(node->an_line, "cannot take the address of this expression");
            }
            node->an_type = Ast_NewPointer(node->an_lhs->an_type);
        } break;

        case AST_NODE_KIND_DEREF: {
            if (! Sem_IsPointer(node->an_lhs->an_type)) {
                Log_ShowErrorAt(node->an_line, "indirection requires a pointer operand");
            }
            if (node->an_lhs->an_type->at_base->at_kind == AST_TYPE_KIND_VOID) {
                Log_ShowErrorAt(node->an_line, "cannot dereference a pointer to void");
            }
            node->an_type = node->an_lhs->an_type->at_base;
        } break;

        case AST_NODE_KIND_CAST: {
            // the parser already set an_type from the type it names
        } break;

        case AST_NODE_KIND_SIZEOF: {
            node->an_kind = AST_NODE_KIND_NUM;
            node->an_val  = node->an_lhs->an_type->at_size;
            node->an_lhs  = NULL;
            node->an_type = &Ast_TypeInt;
        } break;

        case AST_NODE_KIND_VAR: {
            node->an_type = node->an_var->av_type;
        } break;

        case AST_NODE_KIND_ASSIGN: {
            if (! Sem_IsLvalue(node->an_lhs)) {
                Log_ShowErrorAt(node->an_line, "expression is not assignable");
            }
            if (node->an_lhs->an_type->at_kind == AST_TYPE_KIND_ARRAY) {
                Log_ShowErrorAt(node->an_line, "cannot assign to an array");
            }
            node->an_type = node->an_lhs->an_type;
        } break;

        case AST_NODE_KIND_CALL: {
            Sem_CheckCall(node);
            node->an_type = &Ast_TypeInt;
        } break;

        case AST_NODE_KIND_RETURN:
        case AST_NODE_KIND_IF:
        case AST_NODE_KIND_FOR:
        case AST_NODE_KIND_BLOCK:
        case AST_NODE_KIND_EXPR_STMT:
        case AST_NODE_KIND_NOP: {
            // empty
        } break;
    }
}

// Annotates every node with its type and rejects what the grammar cannot.
void Sem_Analyze(Ast_Func *prog)
{
    Sem_Prog = prog;
    Sem_TypeCharPtr = Ast_NewPointer(&Ast_TypeChar);

    for (Ast_Func *func = prog; func; func = func->af_next) {
        Sem_Node(func->af_body);
    }
}
