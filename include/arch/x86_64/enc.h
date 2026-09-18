#ifndef ENC_X86_64_H
#define ENC_X86_64_H

#include <stdint.h>
#include <stdio.h>
#include "util/elf.h"
#include "arch/x86_64/asm.h"

// A rel32 fixup targets its exact site, so it carries an addend of -4.
#define ENC_X86_64_REL32_ADDEND (-4)

// Field layout of the ModRM byte: mod at bit 6, reg at bit 3, r/m at bit 0.
#define ENC_X86_64_MOD_SHIFT 6
#define ENC_X86_64_REG_SHIFT 3
#define ENC_X86_64_REG_MASK  7

// REX prefix bits: 64-bit operand (W) and high-register extensions (R/B).
typedef enum Enc_x86_64_Rex Enc_x86_64_Rex;
enum Enc_x86_64_Rex {
    ENC_X86_64_REX_BASE = 0x40,
    ENC_X86_64_REX_W    = 0x08,
    ENC_X86_64_REX_R    = 0x04,
    ENC_X86_64_REX_B    = 0x01
};

// ModRM mod field, which says how the r/m operand is addressed. It occupies the
// top two bits of the byte, above a 3-bit reg field and a 3-bit r/m field.
typedef enum Enc_x86_64_Mod Enc_x86_64_Mod;
enum Enc_x86_64_Mod {
    ENC_X86_64_MOD_INDIRECT = 0, // (%rm)
    ENC_X86_64_MOD_DISP8    = 1, // disp8(%rm)
    ENC_X86_64_MOD_DISP32   = 2, // disp32(%rm)
    ENC_X86_64_MOD_DIRECT   = 3  // %rm
};

// r/m encodings that name something other than a register.
typedef enum Enc_x86_64_Rm Enc_x86_64_Rm;
enum Enc_x86_64_Rm {
    ENC_X86_64_RM_RIP = 5 // rip-relative, when mod is INDIRECT
};

// SIB byte selecting %rsp as base with no index.
typedef enum Enc_x86_64_Sib Enc_x86_64_Sib;
enum Enc_x86_64_Sib {
    ENC_X86_64_SIB_BASE_RSP = 0x24
};

// Opcode extensions carried in the ModRM reg field by the group opcodes.
typedef enum Enc_x86_64_Grp Enc_x86_64_Grp;
enum Enc_x86_64_Grp {
    ENC_X86_64_GRP_ADD  = 0,
    ENC_X86_64_GRP_NEG  = 3,
    ENC_X86_64_GRP_SUB  = 5,
    ENC_X86_64_GRP_CMP  = 7,
    ENC_X86_64_GRP_IDIV = 7
};

// Primary opcode bytes, named <mnemonic>_<dst>_<src> as the Intel tables list them.
typedef enum Enc_x86_64_Opcode Enc_x86_64_Opcode;
enum Enc_x86_64_Opcode {
    ENC_X86_64_OPCODE_ADD_RM_R      = 0x01,
    ENC_X86_64_OPCODE_SUB_RM_R      = 0x29,
    ENC_X86_64_OPCODE_CMP_RM_R      = 0x39,
    ENC_X86_64_OPCODE_PUSH_R        = 0x50, // + the low 3 bits of the register
    ENC_X86_64_OPCODE_POP_R         = 0x58, // + the low 3 bits of the register
    ENC_X86_64_OPCODE_MOVSXD_R_RM32 = 0x63,
    ENC_X86_64_OPCODE_MOV_RM8_R8    = 0x88,
    ENC_X86_64_OPCODE_MOV_RM_R      = 0x89,
    ENC_X86_64_OPCODE_MOV_R_RM      = 0x8B,
    ENC_X86_64_OPCODE_LEA_R_M       = 0x8D,
    ENC_X86_64_OPCODE_CQO           = 0x99,
    ENC_X86_64_OPCODE_MOV_R8_IMM8   = 0xB0, // + the low 3 bits of the register
    ENC_X86_64_OPCODE_MOV_R_IMM64   = 0xB8, // + the low 3 bits of the register
    ENC_X86_64_OPCODE_RET           = 0xC3,
    ENC_X86_64_OPCODE_MOV_RM_IMM32  = 0xC7,
    ENC_X86_64_OPCODE_CALL_REL32    = 0xE8,
    ENC_X86_64_OPCODE_JMP_REL32     = 0xE9,
    ENC_X86_64_OPCODE_GRP1_RM_IMM32 = 0x81, // add/sub/cmp, selected by Enc_x86_64_Grp
    ENC_X86_64_OPCODE_GRP3_RM       = 0xF7, // neg/idiv, selected by Enc_x86_64_Grp
    ENC_X86_64_OPCODE_ESCAPE        = 0x0F  // introduces a two-byte opcode
};

// Second bytes of the two-byte opcodes, each following ENC_X86_64_OPCODE_ESCAPE.
typedef enum Enc_x86_64_Opcode2 Enc_x86_64_Opcode2;
enum Enc_x86_64_Opcode2 {
    ENC_X86_64_OPCODE2_SYSCALL     = 0x05,
    ENC_X86_64_OPCODE2_JE_REL32    = 0x84,
    ENC_X86_64_OPCODE2_JNE_REL32   = 0x85,
    ENC_X86_64_OPCODE2_SETE        = 0x94,
    ENC_X86_64_OPCODE2_SETNE       = 0x95,
    ENC_X86_64_OPCODE2_SETL        = 0x9C,
    ENC_X86_64_OPCODE2_SETLE       = 0x9E,
    ENC_X86_64_OPCODE2_IMUL_R_RM   = 0xAF,
    ENC_X86_64_OPCODE2_MOVZX_R_RM8 = 0xB6,
    ENC_X86_64_OPCODE2_MOVSX_R_RM8 = 0xBE
};

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
void Enc_x86_64_EmitRex(Asm_x86_64_Width width, Asm_x86_64_Reg reg, Asm_x86_64_Reg rm);
void Enc_x86_64_EmitModRR(int reg, Asm_x86_64_Reg rm);
void Enc_x86_64_EmitMem(int reg, Asm_x86_64_Reg base, int disp);

// Instruction encoding
void Enc_x86_64_EmitRR(int opcode, Asm_x86_64_Reg src, Asm_x86_64_Reg dst);
void Enc_x86_64_EmitGrpImm(int grp, long imm, Asm_x86_64_Reg dst);
void Enc_x86_64_EmitMovImm(long imm, Asm_x86_64_Reg dst);
void Enc_x86_64_EmitMovImm8(long imm, Asm_x86_64_Reg dst);
void Enc_x86_64_EmitMemForm(int opcode, Asm_x86_64_Reg reg, Asm_x86_64_Reg base, int disp, Asm_x86_64_Width width);
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
