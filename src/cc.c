#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "common.h"
#include "ast/ast.h"
#include "ast/sem.h"
#include "util/elf.h"
#include "util/link.h"
#include "util/str.h"
#include "arch/x86_64/gen.h"
#include "arch/x86_64/enc.h"
#include "arch/x86_64/txt.h"

// Permission bits for the executables cc writes (rwxr-xr-x).
#define ELF_MODE 0755

// Default output name for a freestanding executable.
#define DEFAULT_OUTPUT "a.out"

// Target architecture selected when no -march= is given.
#define DEFAULT_ARCH "x86_64"

// Machine-option prefix recognised inside -m (e.g. -march=x86_64).
#define MARCH_PREFIX "arch="

// Input stream read by the generated lexer.
extern FILE *yyin;

// Entry point of the generated parser; fills in Ast_Program.
int yyparse(void);

// Objects the default (linked) output is always merged with.
static const char *const Cc_Runtime[] = { CRT_PATH, LIBC_PATH };

// Show usage information and exits.
static void Cc_ShowUsage(const char *prog)
{
    fprintf(stderr,
        "Usage: %s [options] INPUT.c\n"
        "  -o OUTPUT   write output to OUTPUT (default: " DEFAULT_OUTPUT ")\n"
        "  -S          write assembly text instead of an executable\n"
        "  -c          write a relocatable object (.o) instead of an executable\n"
        "  -march=ARCH target architecture (default: " DEFAULT_ARCH ")\n",
        prog);
    exit(1);
}

// Writes the program as AT&T assembly text.
static void Cc_x86_64_WriteText(FILE *out, Ast_Func *prog)
{
    Gen_x86_64_BuildProgram(prog);
    Txt_x86_64_Att_Write(out);
}

// Writes the program as a relocatable object, references left undefined.
static void Cc_x86_64_WriteObject(FILE *out, Ast_Func *prog)
{
    Gen_x86_64_BuildProgram(prog);
    Enc_x86_64_BuildObject();
    Enc_x86_64_Write(out);
}

// Writes the program linked against the runtime as a static executable.
static void Cc_x86_64_WriteExec(FILE *out, Ast_Func *prog)
{
    Gen_x86_64_BuildProgram(prog);
    Enc_x86_64_BuildObject();

    Elf *obj = Enc_x86_64_GetObject();
    Link_Options opts = { .lo_entry = "_start" };
    Link_MergeFiles(obj, Cc_Runtime, (int) (sizeof Cc_Runtime / sizeof Cc_Runtime[0]));
    Link_Exec(obj, &opts);

    Enc_x86_64_Write(out);
}

// Main function
int main(int argc, char **argv)
{
    const char *output = NULL;
    const char *arch = DEFAULT_ARCH;
    int emit_text = 0;
    int emit_obj = 0;

    static struct option longopts[] = {
        { 0, 0, 0, 0 }
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "o:cESgI:D:U:l:L:W:f:m:O::", longopts, NULL)) != -1) {
        switch (opt) {
            case 'o': {
                output = optarg;
            } break;
            case 'S': {
                emit_text = 1;
            } break;
            case 'c': {
                emit_obj = 1;
            } break;
            case 'm': {
                // -m carries machine options; only -march=ARCH is recognised.
                if (Str_StartsWith(optarg, MARCH_PREFIX)) {
                    arch = optarg + strlen(MARCH_PREFIX);
                }
            } break;
            case 'E': case 'g':
            case 'I': case 'D': case 'U': case 'l':
            case 'L': case 'W': case 'f':
            case 'O': {
                // Recognised compiler flag with no effect here; ignore it.
            } break;
        }
    }

    if (! Str_Equals(arch, DEFAULT_ARCH)) {
        Show_Error("unsupported architecture '%s' (only " DEFAULT_ARCH " is supported)", arch);
    }

    if (optind >= argc) {
        Cc_ShowUsage(argv[0]);
    }
    const char *input = argv[optind];

    char *outbuf = NULL;
    if (! output) {
        if (emit_text) {
            output = outbuf = Str_ChangeOrAppendExt(input, ".s");
        } else if (emit_obj) {
            output = outbuf = Str_ChangeOrAppendExt(input, ".o");
        } else {
            output = outbuf = strdup(DEFAULT_OUTPUT);
        }
    }

    int result = 0;

    // Front end: build the AST
    yyin = fopen(input, "r");
    if (! yyin) {
        perror(input);
        result = 1;
        goto cleanup;
    }
    yyparse();
    Sem_Analyze(Ast_Program);
    fclose(yyin);

    // Back end: emit assembly text or a freestanding executable
    FILE *out = fopen(output, emit_text ? "w" : "wb");
    if (! out) {
        perror(output);
        result = 1;
        goto cleanup;
    }
    if (emit_text) {
        Cc_x86_64_WriteText(out, Ast_Program);
    } else if (emit_obj) {
        Cc_x86_64_WriteObject(out, Ast_Program);
    } else {
        Cc_x86_64_WriteExec(out, Ast_Program);
    }
    fclose(out);

    // Only the freestanding executable is made runnable; .s and .o are not.
    if (! emit_text && ! emit_obj) {
        chmod(output, ELF_MODE);
    }

cleanup:
    Str_Free(outbuf);
    return result;
}
