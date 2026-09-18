#ifndef ENC_X86_64_H
#define ENC_X86_64_H

#include <stdint.h>
#include <stdio.h>
#include "util/elf.h"
#include "arch/x86_64/asm.h"

// A label defined in the stream, awaiting its symbol-table entry.
typedef struct Enc_x86_64_Label Enc_x86_64_Label;
struct Enc_x86_64_Label {
    const char *al_name;
    Elf_Sec    *al_sec;
    uint64_t    al_off;
};

// A pending rel32 fixup: a site in a section and the symbol name it targets.
typedef struct Enc_x86_64_Fix Enc_x86_64_Fix;
struct Enc_x86_64_Fix {
    Elf_Sec    *af_sec;
    uint64_t    af_off;
    const char *af_name;
    uint32_t    af_type;
};

// Byte output
void Enc_x86_64_Emit8(int byte);
void Enc_x86_64_Emit32(uint32_t val);
void Enc_x86_64_Emit64(uint64_t val);
void Enc_x86_64_EmitRaw(const void *data, int len);

// Recording labels and fixups
void Enc_x86_64_RecordLabel(const char *name);
void Enc_x86_64_RecordGlobl(const char *name);
void Enc_x86_64_RecordFixup(const char *name, uint32_t type);

// REX and ModRM encoding
int  Enc_x86_64_RegHigh(Asm_x86_64_Reg reg);
void Enc_x86_64_EmitRexW(int regHigh, int rmHigh);
void Enc_x86_64_EmitRex(int width, Asm_x86_64_Reg reg, Asm_x86_64_Reg rm);
void Enc_x86_64_EmitModRR(int reg, Asm_x86_64_Reg rm);
void Enc_x86_64_EmitMem(int reg, Asm_x86_64_Reg base, int disp);

// Instruction encoding
void Enc_x86_64_EmitRR(int opcode, Asm_x86_64_Reg src, Asm_x86_64_Reg dst);
void Enc_x86_64_EmitGrpImm(int grp, long imm, Asm_x86_64_Reg dst);
void Enc_x86_64_EmitMovImm(long imm, Asm_x86_64_Reg dst);
void Enc_x86_64_EmitMovImm8(long imm, Asm_x86_64_Reg dst);
void Enc_x86_64_EmitMemForm(int opcode, Asm_x86_64_Reg reg, Asm_x86_64_Reg base, int disp, int width);
void Enc_x86_64_EmitMovsx(const Asm_x86_64_Item *item);
void Enc_x86_64_EmitLeaRip(Asm_x86_64_Reg dst, const char *label);
void Enc_x86_64_EmitGrpUnary(int grp, Asm_x86_64_Reg reg);
void Enc_x86_64_EmitSetcc(int opcode, Asm_x86_64_Reg reg);
void Enc_x86_64_EmitBranch(const Asm_x86_64_Item *item);
void Enc_x86_64_EmitMov(const Asm_x86_64_Item *item);
void Enc_x86_64_EmitInstr(const Asm_x86_64_Item *item);

// Symbols, sections and relocations
int  Enc_x86_64_IsGlobl(const char *name);
void Enc_x86_64_SelectSection(const char *name, uint32_t type, uint64_t flags);
void Enc_x86_64_BuildSymbols(void);
void Enc_x86_64_BuildRelocs(void);

// Encoding the instruction list to a relocatable ELF object
void Enc_x86_64_Reset(void);
void Enc_x86_64_BuildObject(void);
Elf *Enc_x86_64_GetObject(void);
int  Enc_x86_64_Write(FILE *out);

#endif // ENC_X86_64_H
