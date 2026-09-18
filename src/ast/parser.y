/* parser.y - grammar for the cc compiler (a small subset of C).
 *
 * Generates build/parser.tab.c and build/parser.tab.h
 * (via `bison -d -o build/parser.tab.c`).
 *
 * The grammar is deliberately flat: operator precedence is resolved through
 * the %left / %right declarations below rather than through a tower of
 * non-terminals, which keeps the grammar short and easy to extend.
 */

%code requires {
    #include "ast/ast.h"
}

%{
#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include "ast/ast.h"

int  yylex(void);
void yyerror(const char *s);

/* Gives a rule the line of its first token, or of the preceding one if empty. */
#define YYLLOC_DEFAULT(cur, rhs, n)  ((cur) = (n) ? YYRHSLOC(rhs, 1) : YYRHSLOC(rhs, 0))

/* State for the function definition currently being parsed. */
static char     *Parser_CurFuncName;
static Ast_Var  *Parser_CurParams;
static Ast_Var  *Parser_CurParamsTail;
static int       Parser_CurNumParams;

/* The program assembled so far, as functions are reduced. */
static Ast_Func *Parser_ProgHead;
static Ast_Func *Parser_ProgTail;

/* Appends a parameter to the function currently being parsed. */
static void Parser_AddParam(Ast_Var *v)
{
    v->av_param_next = NULL;
    if (! Parser_CurParams) {
        Parser_CurParams = Parser_CurParamsTail = v;
    } else {
        Parser_CurParamsTail->av_param_next = v;
        Parser_CurParamsTail = v;
    }
    Parser_CurNumParams++;
}

/* Appends a finished function to the program. */
static void Parser_AddFunction(Ast_Func *fn)
{
    fn->af_next = NULL;
    if (! Parser_ProgHead) {
        Parser_ProgHead = Parser_ProgTail = fn;
    } else {
        Parser_ProgTail->af_next = fn;
        Parser_ProgTail = fn;
    }
    Ast_Program = Parser_ProgHead;
}
%}

%locations
%define api.location.type {int}

%union {
    long      num;
    char     *str;
    Ast_Str   str_lit;
    Ast_Node *node;
    Ast_Type *type;
}

%token <num>     NUM
%token <str>     IDENT
%token <str_lit> STR
%token INT CHAR VOID CONST RETURN IF ELSE FOR WHILE BREAK CONTINUE
%token ADD SUB MUL DIV MOD ASSIGN NOT AMP
%token EQ NE LT GT LE GE AND OR
%token LPAREN RPAREN LSQUARE RSQUARE LBRACE RBRACE SEMI COMMA ELLIPSIS

%type <node> stmt stmt_list compound_stmt decl expr expr_opt args arg_list
%type <type> type_name base
%type <num>  stars

/* Lowest precedence first. */
%nonassoc LOWER_THAN_ELSE
%nonassoc ELSE
%right ASSIGN
%left OR
%left AND
%left EQ NE
%left LT GT LE GE
%left ADD SUB
%left MUL DIV MOD
%right NOT UMINUS

%start translation_unit

%%

/* ---- top level: function definitions and prototypes ---------------- */

translation_unit
    : /* empty */
    | translation_unit external_decl
    ;

external_decl
    : type_name IDENT LPAREN
        {
            Parser_CurFuncName   = $2;
            Parser_CurParams     = NULL;
            Parser_CurParamsTail = NULL;
            Parser_CurNumParams  = 0;
            Ast_BeginScope();
        }
      params RPAREN func_tail
    ;

func_tail
    : compound_stmt
        {
            Ast_Func *fn = calloc(1, sizeof(Ast_Func));
            fn->af_name    = Parser_CurFuncName;
            fn->af_body    = $1;
            fn->af_params  = Parser_CurParams;
            fn->af_nparams = Parser_CurNumParams;
            fn->af_locals  = Ast_CurrentLocals();
            Parser_AddFunction(fn);
        }
    | SEMI  /* a prototype, e.g. `int printf(const char *, ...);` -- discard */
    ;

params
    : /* empty */
    | param_list
    ;

param_list
    : param
    | param_list COMMA param
    ;

param
    : type_name IDENT   { Parser_AddParam(Ast_DeclareVar($2, $1, @2)); }
    | type_name         /* unnamed parameter, e.g. `void` */
    | ELLIPSIS          /* variadic marker, ignored */
    ;

/* ---- types -------------------------------------------------------- */

type_name
    : quals base stars
        { Ast_Type *t = $2;
          for (int i = 0; i < $3; i++) { t = Ast_NewPointer(t); }
          $$ = t; }
    ;

quals
    : /* empty */
    | quals CONST
    ;

base
    : INT                  { $$ = &Ast_TypeInt; }
    | CHAR                 { $$ = &Ast_TypeChar; }
    | VOID                 { $$ = &Ast_TypeVoid; }
    ;

stars
    : /* empty */          { $$ = 0; }
    | stars MUL            { $$ = $1 + 1; }
    ;

/* ---- statements ---------------------------------------------------- */

compound_stmt
    : LBRACE stmt_list RBRACE
        { Ast_Node *n = Ast_NewNode(AST_NODE_KIND_BLOCK, @1); n->an_body = $2; $$ = n; }
    ;

stmt_list
    : /* empty */          { $$ = NULL; }
    | stmt stmt_list       { $1->an_next = $2; $$ = $1; }
    ;

stmt
    : RETURN expr SEMI     { $$ = Ast_NewUnary(AST_NODE_KIND_RETURN, $2, @1); }
    | RETURN SEMI          { $$ = Ast_NewUnary(AST_NODE_KIND_RETURN, NULL, @1); }
    | IF LPAREN expr RPAREN stmt %prec LOWER_THAN_ELSE
        { Ast_Node *n = Ast_NewNode(AST_NODE_KIND_IF, @1);
          n->an_cond = $3; n->an_then = $5; $$ = n; }
    | IF LPAREN expr RPAREN stmt ELSE stmt
        { Ast_Node *n = Ast_NewNode(AST_NODE_KIND_IF, @1);
          n->an_cond = $3; n->an_then = $5; n->an_els = $7; $$ = n; }
    | FOR LPAREN expr_opt SEMI expr_opt SEMI expr_opt RPAREN stmt
        { Ast_Node *n = Ast_NewNode(AST_NODE_KIND_FOR, @1);
          n->an_init = $3; n->an_cond = $5; n->an_inc = $7; n->an_body = $9; $$ = n; }
    | WHILE LPAREN expr RPAREN stmt
        { Ast_Node *n = Ast_NewNode(AST_NODE_KIND_FOR, @1);
          n->an_cond = $3; n->an_body = $5; $$ = n; }
    | compound_stmt        { $$ = $1; }
    | decl SEMI            { $$ = $1; }
    | expr SEMI            { $$ = Ast_NewUnary(AST_NODE_KIND_EXPR_STMT, $1, @1); }
    | SEMI                 { $$ = Ast_NewNode(AST_NODE_KIND_NOP, @1); }
    ;

decl
    : type_name IDENT
        { Ast_DeclareVar($2, $1, @2); $$ = Ast_NewNode(AST_NODE_KIND_NOP, @2); }
    | type_name IDENT ASSIGN expr
        { Ast_Var *v = Ast_DeclareVar($2, $1, @2);
          Ast_Node *n = Ast_NewBinary(AST_NODE_KIND_ASSIGN, Ast_NewVarNode(v, @2), $4, @3);
          $$ = Ast_NewUnary(AST_NODE_KIND_EXPR_STMT, n, @2); }
    ;

expr_opt
    : /* empty */          { $$ = NULL; }
    | expr                 { $$ = $1; }
    ;

/* ---- expressions --------------------------------------------------- */

expr
    : NUM                  { $$ = Ast_NewNum($1, @1); }
    | STR                  { Ast_Node *n = Ast_NewNode(AST_NODE_KIND_STR, @1);
                             n->an_str_idx = Ast_AddString($1.as_data, $1.as_len); $$ = n; }
    | IDENT
        { Ast_Var *v = Ast_FindVar($1);
          if (! v) Show_ErrorAt(@1, "use of undeclared identifier '%s'", $1);
          $$ = Ast_NewVarNode(v, @1); }
    | IDENT LPAREN args RPAREN
        { Ast_Node *n = Ast_NewNode(AST_NODE_KIND_CALL, @1);
          n->an_funcname = $1; n->an_args = $3; $$ = n; }
    | LPAREN expr RPAREN   { $$ = $2; }
    | expr ADD expr        { $$ = Ast_NewBinary(AST_NODE_KIND_ADD, $1, $3, @2); }
    | expr SUB expr        { $$ = Ast_NewBinary(AST_NODE_KIND_SUB, $1, $3, @2); }
    | expr MUL expr        { $$ = Ast_NewBinary(AST_NODE_KIND_MUL, $1, $3, @2); }
    | expr DIV expr        { $$ = Ast_NewBinary(AST_NODE_KIND_DIV, $1, $3, @2); }
    | expr MOD expr        { $$ = Ast_NewBinary(AST_NODE_KIND_MOD, $1, $3, @2); }
    | expr EQ expr         { $$ = Ast_NewBinary(AST_NODE_KIND_EQ, $1, $3, @2); }
    | expr NE expr         { $$ = Ast_NewBinary(AST_NODE_KIND_NE, $1, $3, @2); }
    | expr LT expr         { $$ = Ast_NewBinary(AST_NODE_KIND_LT, $1, $3, @2); }
    | expr GT expr         { $$ = Ast_NewBinary(AST_NODE_KIND_LT, $3, $1, @2); }  /* a>b  == b<a  */
    | expr LE expr         { $$ = Ast_NewBinary(AST_NODE_KIND_LE, $1, $3, @2); }
    | expr GE expr         { $$ = Ast_NewBinary(AST_NODE_KIND_LE, $3, $1, @2); }  /* a>=b == b<=a */
    | expr AND expr        { $$ = Ast_NewBinary(AST_NODE_KIND_AND, $1, $3, @2); }
    | expr OR expr         { $$ = Ast_NewBinary(AST_NODE_KIND_OR, $1, $3, @2); }
    | expr ASSIGN expr     { $$ = Ast_NewBinary(AST_NODE_KIND_ASSIGN, $1, $3, @2); }
    | SUB expr %prec UMINUS { $$ = Ast_NewUnary(AST_NODE_KIND_NEG, $2, @1); }
    | NOT expr %prec UMINUS { $$ = Ast_NewUnary(AST_NODE_KIND_NOT, $2, @1); }
    ;

args
    : /* empty */          { $$ = NULL; }
    | arg_list             { $$ = $1; }
    ;

arg_list
    : expr                 { $$ = $1; }
    | expr COMMA arg_list  { $1->an_next = $3; $$ = $1; }
    ;

%%

void yyerror(const char *s)
{
    fprintf(stderr, "cc: parse error: %s near line %d\n", s, yylloc);
    exit(1);
}
