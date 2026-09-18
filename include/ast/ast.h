#ifndef AST_H
#define AST_H

// Maximum number of distinct string literals in one translation unit.
#define MAX_STRINGS 1024

// The kind of a type.
typedef enum Ast_TypeKind Ast_TypeKind;
enum Ast_TypeKind {
    AST_TYPE_KIND_VOID,
    AST_TYPE_KIND_CHAR,
    AST_TYPE_KIND_INT,
    AST_TYPE_KIND_PTR,
    AST_TYPE_KIND_ARRAY
};

// The target ABI's sizes in bytes; C does not define void's.
typedef enum Ast_TypeSize Ast_TypeSize;
enum Ast_TypeSize {
    AST_TYPE_SIZE_VOID = 1,
    AST_TYPE_SIZE_CHAR = 1,
    AST_TYPE_SIZE_INT  = 4,
    AST_TYPE_SIZE_PTR  = 8
};

// The target ABI's alignments in bytes.
typedef enum Ast_TypeAlign Ast_TypeAlign;
enum Ast_TypeAlign {
    AST_TYPE_ALIGN_VOID = 1,
    AST_TYPE_ALIGN_CHAR = 1,
    AST_TYPE_ALIGN_INT  = 4,
    AST_TYPE_ALIGN_PTR  = 8
};

// A C type: a primitive, or a pointer or array built over another one.
typedef struct Ast_Type Ast_Type;
struct Ast_Type {
    Ast_TypeKind at_kind;
    int          at_size;  // bytes an object of this type occupies
    int          at_align; // address multiple an object must sit on
    Ast_Type    *at_base;  // pointee for PTR, element type for ARRAY
    int          at_len;   // element count for ARRAY
};

// The primitive types, shared by every declaration that names one.
extern Ast_Type Ast_TypeVoid;
extern Ast_Type Ast_TypeChar;
extern Ast_Type Ast_TypeInt;

// An interned string literal, kept with its length because it may embed a NUL.
typedef struct Ast_Str Ast_Str;
struct Ast_Str {
    char *as_data; // decoded bytes, also NUL-terminated so it can be printed
    int   as_len;  // number of bytes before that terminator
};

// The kind of an AST node.
typedef enum Ast_NodeKind Ast_NodeKind;
enum Ast_NodeKind {
    AST_NODE_KIND_NUM,       // integer literal
    AST_NODE_KIND_STR,       // string literal
    AST_NODE_KIND_VAR,       // a reference to a local variable
    AST_NODE_KIND_ADD,       // lhs + rhs
    AST_NODE_KIND_SUB,       // lhs - rhs
    AST_NODE_KIND_MUL,       // lhs * rhs
    AST_NODE_KIND_DIV,       // lhs / rhs
    AST_NODE_KIND_MOD,       // lhs % rhs
    AST_NODE_KIND_NEG,       // -lhs
    AST_NODE_KIND_NOT,       // !lhs
    AST_NODE_KIND_ADDR,      // &lhs
    AST_NODE_KIND_DEREF,     // *lhs
    AST_NODE_KIND_CAST,      // (type) lhs
    AST_NODE_KIND_SIZEOF,    // sizeof lhs, folded to a constant by the Sem_ pass
    AST_NODE_KIND_EQ,        // lhs == rhs
    AST_NODE_KIND_NE,        // lhs != rhs
    AST_NODE_KIND_LT,        // lhs <  rhs   (> is LT reversed)
    AST_NODE_KIND_LE,        // lhs <= rhs   (>= is LE reversed)
    AST_NODE_KIND_AND,       // lhs && rhs
    AST_NODE_KIND_OR,        // lhs || rhs
    AST_NODE_KIND_ASSIGN,    // lhs = rhs
    AST_NODE_KIND_CALL,      // function call
    AST_NODE_KIND_RETURN,    // return lhs;
    AST_NODE_KIND_IF,        // if (cond) then; else els;
    AST_NODE_KIND_FOR,       // for (init; cond; inc) body;
    AST_NODE_KIND_BLOCK,     // { ... }
    AST_NODE_KIND_EXPR_STMT, // expression used as a statement
    AST_NODE_KIND_NOP        // empty statement / bare declaration
};

// A local variable or function parameter.
typedef struct Ast_Var Ast_Var;
struct Ast_Var {
    Ast_Var *av_next;       // chains every local in a function
    Ast_Var *av_param_next; // chains parameters in declaration order
    char     *av_name;      // identifier as written in the source
    Ast_Type *av_type;      // declared type
    int      av_line;       // source line the declaration appeared on
    int      av_offset;     // offset from %rbp, filled in by the back end
};

// A node in the abstract syntax tree.
typedef struct Ast_Node Ast_Node;
struct Ast_Node {
    Ast_NodeKind an_kind;     // which kind of node this is
    Ast_Type    *an_type;     // type of the value, filled in by the Sem_ pass
    int          an_line;     // source line the construct started on
    Ast_Node    *an_next;     // next node in a statement / argument list
    Ast_Node    *an_lhs;      // generic left operand
    Ast_Node    *an_rhs;      // generic right operand
    Ast_Node    *an_cond;     // condition of AST_NODE_KIND_IF / AST_NODE_KIND_FOR
    Ast_Node    *an_then;     // then branch of AST_NODE_KIND_IF
    Ast_Node    *an_els;      // else branch of AST_NODE_KIND_IF
    Ast_Node    *an_init;     // initialiser of AST_NODE_KIND_FOR
    Ast_Node    *an_inc;      // increment of AST_NODE_KIND_FOR
    Ast_Node    *an_body;     // statement list for AST_NODE_KIND_BLOCK / FOR body
    char        *an_funcname; // callee name for AST_NODE_KIND_CALL
    Ast_Node    *an_args;     // argument list for AST_NODE_KIND_CALL
    long         an_val;      // integer value for AST_NODE_KIND_NUM
    int          an_str_idx;  // string table slot for AST_NODE_KIND_STR
    Ast_Var     *an_var;      // referenced variable for AST_NODE_KIND_VAR
};

// A function definition.
typedef struct Ast_Func Ast_Func;
struct Ast_Func {
    Ast_Func *af_next;       // next function in the program
    char     *af_name;       // function name
    Ast_Node *af_body;       // function body (AST_NODE_KIND_BLOCK)
    Ast_Var  *af_params;     // parameters, in declaration order
    int       af_nparams;    // number of parameters
    Ast_Var  *af_locals;     // every local, including parameters
    int       af_stack_size; // frame size, filled in by the code generator
};

// The finished program, produced by the parser.
extern Ast_Func *Ast_Program;

// Type construction
Ast_Type *Ast_NewPointer(Ast_Type *base);
Ast_Type *Ast_NewArray(Ast_Type *base, int len);

// Node construction
Ast_Node *Ast_NewNode(Ast_NodeKind kind, int line);
Ast_Node *Ast_NewBinary(Ast_NodeKind kind, Ast_Node *lhs, Ast_Node *rhs, int line);
Ast_Node *Ast_NewUnary(Ast_NodeKind kind, Ast_Node *lhs, int line);
Ast_Node *Ast_NewNum(long val, int line);
Ast_Node *Ast_NewVarNode(Ast_Var *var, int line);

// Variable scopes
void     Ast_BeginScope(void);
Ast_Var *Ast_FindVar(const char *name);
Ast_Var *Ast_DeclareVar(const char *name, Ast_Type *type, int line);
Ast_Var *Ast_CurrentLocals(void);

// String literal interning
int      Ast_AddString(char *s, int len);
int      Ast_StringCount(void);
Ast_Str *Ast_StringAt(int idx);

#endif // AST_H
