#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "ast/ast.h"

// The finished program, filled in by the parser.
Ast_Func *Ast_Program;

// The primitive types.
Ast_Type Ast_TypeVoid = { AST_TYPE_KIND_VOID, AST_TYPE_SIZE_VOID, AST_TYPE_ALIGN_VOID, NULL, 0 };
Ast_Type Ast_TypeChar = { AST_TYPE_KIND_CHAR, AST_TYPE_SIZE_CHAR, AST_TYPE_ALIGN_CHAR, NULL, 0 };
Ast_Type Ast_TypeInt  = { AST_TYPE_KIND_INT,  AST_TYPE_SIZE_INT,  AST_TYPE_ALIGN_INT,  NULL, 0 };

// Table of interned string literals, indexed by AST_NODE_KIND_STR slot.
static Ast_Str Ast_Strings[MAX_STRINGS];

// Number of entries currently used in Ast_Strings.
static int Ast_NumStrings;

// Locals of the function currently being parsed.
static Ast_Var *Ast_Locals;

// Builds the pointer type that points at base.
Ast_Type *Ast_NewPointer(Ast_Type *base)
{
    Ast_Type *type = calloc(1, sizeof(Ast_Type));
    type->at_kind  = AST_TYPE_KIND_PTR;
    type->at_size  = AST_TYPE_SIZE_PTR;
    type->at_align = AST_TYPE_ALIGN_PTR;
    type->at_base  = base;
    return type;
}

// Builds the type of an array of len elements of base.
Ast_Type *Ast_NewArray(Ast_Type *base, int len)
{
    Ast_Type *type = calloc(1, sizeof(Ast_Type));
    type->at_kind  = AST_TYPE_KIND_ARRAY;
    type->at_size  = base->at_size * len;
    type->at_align = base->at_align;
    type->at_base  = base;
    type->at_len   = len;
    return type;
}

// Allocates a zeroed node of the given kind.
Ast_Node *Ast_NewNode(Ast_NodeKind kind, int line)
{
    Ast_Node *node = calloc(1, sizeof(Ast_Node));
    node->an_kind = kind;
    node->an_line = line;
    return node;
}

// Builds a binary-operator node with the given operands.
Ast_Node *Ast_NewBinary(Ast_NodeKind kind, Ast_Node *lhs, Ast_Node *rhs, int line)
{
    Ast_Node *node = Ast_NewNode(kind, line);
    node->an_lhs = lhs;
    node->an_rhs = rhs;
    return node;
}

// Builds a unary-operator node with the given operand.
Ast_Node *Ast_NewUnary(Ast_NodeKind kind, Ast_Node *lhs, int line)
{
    Ast_Node *node = Ast_NewNode(kind, line);
    node->an_lhs = lhs;
    return node;
}

// Builds an integer-literal node.
Ast_Node *Ast_NewNum(long val, int line)
{
    Ast_Node *node = Ast_NewNode(AST_NODE_KIND_NUM, line);
    node->an_val = val;
    return node;
}

// Builds a node that references a local variable.
Ast_Node *Ast_NewVarNode(Ast_Var *var, int line)
{
    Ast_Node *node = Ast_NewNode(AST_NODE_KIND_VAR, line);
    node->an_var = var;
    return node;
}

// Starts a fresh variable scope for a new function.
void Ast_BeginScope(void)
{
    Ast_Locals = NULL;
}

// Looks up a variable by name in the current scope, or NULL.
Ast_Var *Ast_FindVar(const char *name)
{
    for (Ast_Var *var = Ast_Locals; var; var = var->av_next) {
        if (strcmp(var->av_name, name) == 0) {
            return var;
        }
    }
    return NULL;
}

// Declares a variable in the current scope, reusing any existing slot.
Ast_Var *Ast_DeclareVar(const char *name, Ast_Type *type, int line)
{
    Ast_Var *var = Ast_FindVar(name);
    if (var) {
        return var;
    }
    var = calloc(1, sizeof(Ast_Var));
    var->av_name = strdup(name);
    var->av_type = type;
    var->av_line = line;
    var->av_next = Ast_Locals;
    Ast_Locals = var;
    return var;
}

// Returns the list of locals declared in the current scope.
Ast_Var *Ast_CurrentLocals(void)
{
    return Ast_Locals;
}

// Interns a decoded string literal of len bytes and returns its table slot.
int Ast_AddString(char *str, int len)
{
    if (Ast_NumStrings >= MAX_STRINGS) {
        Show_Error("too many string literals (max %d)", MAX_STRINGS);
    }
    Ast_Strings[Ast_NumStrings].as_data = str;
    Ast_Strings[Ast_NumStrings].as_len  = len;
    return Ast_NumStrings++;
}

// Returns the number of interned string literals.
int Ast_StringCount(void)
{
    return Ast_NumStrings;
}

// Returns the interned string literal in the given slot.
Ast_Str *Ast_StringAt(int idx)
{
    return &Ast_Strings[idx];
}
