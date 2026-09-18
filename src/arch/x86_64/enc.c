#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "util/elf.h"
#include "arch/x86_64/asm.h"
#include "arch/x86_64/enc.h"
#include "arch/x86_64/rel.h"

// The object being encoded.
static Elf *Enc_x86_64_Out;

// The section that following writes append to.
static Elf_Sec *Enc_x86_64_Cur;

// Labels collected while encoding.
static Enc_x86_64_Label *Enc_x86_64_Labels;
static size_t Enc_x86_64_NumLabels;
static size_t Enc_x86_64_CapLabels;

// Names declared via .globl while encoding.
static const char **Enc_x86_64_Globls;
static size_t Enc_x86_64_NumGlobls;
static size_t Enc_x86_64_CapGlobls;

// Pending rel32 fixups collected while encoding.
static Enc_x86_64_Fix *Enc_x86_64_Fixes;
static size_t Enc_x86_64_NumFixes;
static size_t Enc_x86_64_CapFixes;

// Appends one byte to the current section.
void Enc_x86_64_Emit8(int byte)
{
    Elf_BufByte(Elf_SectionData(Enc_x86_64_Cur), (uint8_t) byte);
}

// Appends a little-endian 32-bit value to the current section.
void Enc_x86_64_Emit32(uint32_t val)
{
    Elf_BufU32(Elf_SectionData(Enc_x86_64_Cur), val);
}

// Appends a little-endian 64-bit value to the current section.
void Enc_x86_64_Emit64(uint64_t val)
{
    Elf_BufU64(Elf_SectionData(Enc_x86_64_Cur), val);
}

// Appends a run of raw bytes to the current section.
void Enc_x86_64_EmitRaw(const void *data, int len)
{
    Elf_BufData(Elf_SectionData(Enc_x86_64_Cur), data, (size_t) len);
}

// Records a label at the current position in the current section.
void Enc_x86_64_RecordLabel(const char *name)
{
    if (Enc_x86_64_NumLabels == Enc_x86_64_CapLabels) {
        Enc_x86_64_CapLabels = Enc_x86_64_CapLabels ? Enc_x86_64_CapLabels * 2 : 64;
        Enc_x86_64_Labels = realloc(Enc_x86_64_Labels, Enc_x86_64_CapLabels * sizeof(*Enc_x86_64_Labels));
    }
    Enc_x86_64_Labels[Enc_x86_64_NumLabels++] = (Enc_x86_64_Label) {
        .al_name = name,
        .al_sec  = Enc_x86_64_Cur,
        .al_off  = Elf_SectionData(Enc_x86_64_Cur)->eb_len
    };
}

// Records that name appeared in a .globl directive.
void Enc_x86_64_RecordGlobl(const char *name)
{
    if (Enc_x86_64_NumGlobls == Enc_x86_64_CapGlobls) {
        Enc_x86_64_CapGlobls = Enc_x86_64_CapGlobls ? Enc_x86_64_CapGlobls * 2 : 64;
        Enc_x86_64_Globls = realloc(Enc_x86_64_Globls, Enc_x86_64_CapGlobls * sizeof(*Enc_x86_64_Globls));
    }
    Enc_x86_64_Globls[Enc_x86_64_NumGlobls++] = name;
}

// Records a rel32 fixup at the current site; the caller writes the placeholder bytes.
void Enc_x86_64_RecordFixup(const char *name, uint32_t type)
{
    if (Enc_x86_64_NumFixes == Enc_x86_64_CapFixes) {
        Enc_x86_64_CapFixes = Enc_x86_64_CapFixes ? Enc_x86_64_CapFixes * 2 : 64;
        Enc_x86_64_Fixes = realloc(Enc_x86_64_Fixes, Enc_x86_64_CapFixes * sizeof(*Enc_x86_64_Fixes));
    }
    Enc_x86_64_Fixes[Enc_x86_64_NumFixes++] = (Enc_x86_64_Fix) {
        .af_sec  = Enc_x86_64_Cur,
        .af_off  = Elf_SectionData(Enc_x86_64_Cur)->eb_len,
        .af_name = name,
        .af_type = type
    };
}

// Returns the high bit of a register number, extending ModRM.reg or .rm.
int Enc_x86_64_RegHigh(Asm_x86_64_Reg reg)
{
    return reg >> ENC_X86_64_REG_SHIFT;
}

// Emits a REX.W prefix with the given reg- and rm-field extension bits.
void Enc_x86_64_EmitRexW(int regHigh, int rmHigh)
{
    Enc_x86_64_Emit8(ENC_X86_64_REX_BASE | ENC_X86_64_REX_W | (regHigh ? ENC_X86_64_REX_R : 0) | (rmHigh ? ENC_X86_64_REX_B : 0));
}

// Emits a register-direct ModRM byte pairing reg with rm.
void Enc_x86_64_EmitModRR(int reg, Asm_x86_64_Reg rm)
{
    Enc_x86_64_Emit8((ENC_X86_64_MOD_DIRECT << ENC_X86_64_MOD_SHIFT) | ((reg & ENC_X86_64_REG_MASK) << ENC_X86_64_REG_SHIFT) | (rm & ENC_X86_64_REG_MASK));
}

// Emits the ModRM, optional SIB and displacement for disp(%base).
void Enc_x86_64_EmitMem(int reg, Asm_x86_64_Reg base, int disp)
{
    int rm  = base & ENC_X86_64_REG_MASK;
    int mod;
    if (disp == 0 && rm != (ASM_X86_64_REG_RBP & ENC_X86_64_REG_MASK)) {
        mod = ENC_X86_64_MOD_INDIRECT;
    } else if (disp >= INT8_MIN && disp <= INT8_MAX) {
        mod = ENC_X86_64_MOD_DISP8;
    } else {
        mod = ENC_X86_64_MOD_DISP32;
    }

    Enc_x86_64_Emit8((mod << ENC_X86_64_MOD_SHIFT) | ((reg & ENC_X86_64_REG_MASK) << ENC_X86_64_REG_SHIFT) | rm);
    if (rm == (ASM_X86_64_REG_RSP & ENC_X86_64_REG_MASK)) {
        Enc_x86_64_Emit8(ENC_X86_64_SIB_BASE_RSP);
    }
    if (mod == ENC_X86_64_MOD_DISP8) {
        Enc_x86_64_Emit8(disp & 0xFF);
    } else if (mod == ENC_X86_64_MOD_DISP32) {
        Enc_x86_64_Emit32((unsigned int) disp);
    }
}

// Emits `<opcode> %src, %dst` for a register-to-register operation.
void Enc_x86_64_EmitRR(int opcode, Asm_x86_64_Reg src, Asm_x86_64_Reg dst)
{
    Enc_x86_64_EmitRexW(Enc_x86_64_RegHigh(src), Enc_x86_64_RegHigh(dst));
    Enc_x86_64_Emit8(opcode);
    Enc_x86_64_EmitModRR(src, dst);
}

// Emits a group-1 `<grp> $imm, %dst` with a 32-bit immediate.
void Enc_x86_64_EmitGrpImm(int grp, long imm, Asm_x86_64_Reg dst)
{
    Enc_x86_64_EmitRexW(0, Enc_x86_64_RegHigh(dst));
    Enc_x86_64_Emit8(ENC_X86_64_OPCODE_GRP1_RM_IMM32);
    Enc_x86_64_EmitModRR(grp, dst);
    Enc_x86_64_Emit32((unsigned int) imm);
}

// Emits `mov $imm, %dst` into a 64-bit register.
void Enc_x86_64_EmitMovImm(long imm, Asm_x86_64_Reg dst)
{
    if (imm >= INT32_MIN && imm <= INT32_MAX) {
        Enc_x86_64_EmitRexW(0, Enc_x86_64_RegHigh(dst));
        Enc_x86_64_Emit8(ENC_X86_64_OPCODE_MOV_RM_IMM32);
        Enc_x86_64_EmitModRR(0, dst);
        Enc_x86_64_Emit32((unsigned int) imm);
    } else {
        Enc_x86_64_EmitRexW(0, Enc_x86_64_RegHigh(dst));
        Enc_x86_64_Emit8(ENC_X86_64_OPCODE_MOV_R_IMM64 + (dst & ENC_X86_64_REG_MASK));
        Enc_x86_64_Emit64((unsigned long long) imm);
    }
}

// Emits a REX prefix when the operand width or the registers chosen require one.
void Enc_x86_64_EmitRex(Asm_x86_64_Width width, Asm_x86_64_Reg reg, Asm_x86_64_Reg rm)
{
    int bits = (width == ASM_X86_64_WIDTH_64 ? ENC_X86_64_REX_W : 0)
             | (Enc_x86_64_RegHigh(reg) ? ENC_X86_64_REX_R : 0)
             | (Enc_x86_64_RegHigh(rm) ? ENC_X86_64_REX_B : 0);

    // %spl, %bpl, %sil and %dil are only reachable through a REX prefix.
    int lowbyte = width == ASM_X86_64_WIDTH_8 && reg >= ASM_X86_64_REG_RSP && reg < ASM_X86_64_REG_R8;

    if (bits || lowbyte) {
        Enc_x86_64_Emit8(ENC_X86_64_REX_BASE | bits);
    }
}

// Emits `mov $imm, %dst` into an 8-bit register.
void Enc_x86_64_EmitMovImm8(long imm, Asm_x86_64_Reg dst)
{
    if (dst >= ASM_X86_64_REG_R8) {
        Enc_x86_64_Emit8(ENC_X86_64_REX_BASE | ENC_X86_64_REX_B);
    } else if (dst >= ASM_X86_64_REG_RSP) {
        Enc_x86_64_Emit8(ENC_X86_64_REX_BASE);
    }
    Enc_x86_64_Emit8(ENC_X86_64_OPCODE_MOV_R8_IMM8 + (dst & ENC_X86_64_REG_MASK));
    Enc_x86_64_Emit8(imm & 0xFF);
}

// Emits `<opcode> disp(%base), %reg` (or the reverse for a store) at width bits.
void Enc_x86_64_EmitMemForm(int opcode, Asm_x86_64_Reg reg, Asm_x86_64_Reg base, int disp, Asm_x86_64_Width width)
{
    Enc_x86_64_EmitRex(width, reg, base);
    Enc_x86_64_Emit8(opcode);
    Enc_x86_64_EmitMem(reg, base, disp);
}

// Emits a sign-extending `movs<w>q` from a register or from disp(%base).
void Enc_x86_64_EmitMovsx(const Asm_x86_64_Item *item)
{
    Asm_x86_64_Reg dst = item->ai_dst.ao_reg;
    Asm_x86_64_Reg src = item->ai_src.ao_reg;

    Enc_x86_64_EmitRexW(Enc_x86_64_RegHigh(dst), Enc_x86_64_RegHigh(src));
    if (item->ai_src.ao_width == ASM_X86_64_WIDTH_8) {
        Enc_x86_64_Emit8(ENC_X86_64_OPCODE_ESCAPE);
        Enc_x86_64_Emit8(ENC_X86_64_OPCODE2_MOVSX_R_RM8);
    } else {
        Enc_x86_64_Emit8(ENC_X86_64_OPCODE_MOVSXD_R_RM32);
    }
    if (item->ai_src.ao_kind == ASM_X86_64_OPERAND_REG) {
        Enc_x86_64_EmitModRR(dst, src);
    } else {
        Enc_x86_64_EmitMem(dst, src, item->ai_src.ao_disp);
    }
}

// Emits `lea label(%rip), %dst` with a rel32 fixup to label.
void Enc_x86_64_EmitLeaRip(Asm_x86_64_Reg dst, const char *label)
{
    Enc_x86_64_EmitRexW(Enc_x86_64_RegHigh(dst), 0);
    Enc_x86_64_Emit8(ENC_X86_64_OPCODE_LEA_R_M);
    Enc_x86_64_Emit8((ENC_X86_64_MOD_INDIRECT << ENC_X86_64_MOD_SHIFT) | ((dst & ENC_X86_64_REG_MASK) << ENC_X86_64_REG_SHIFT) | ENC_X86_64_RM_RIP);
    Enc_x86_64_RecordFixup(label, R_X86_64_PC32);
    Enc_x86_64_Emit32(0);
}

// Emits a group-3 unary instruction `<grp> %reg`.
void Enc_x86_64_EmitGrpUnary(int grp, Asm_x86_64_Reg reg)
{
    Enc_x86_64_EmitRexW(0, Enc_x86_64_RegHigh(reg));
    Enc_x86_64_Emit8(ENC_X86_64_OPCODE_GRP3_RM);
    Enc_x86_64_EmitModRR(grp, reg);
}

// Emits a `setcc %reg` byte-setting instruction.
void Enc_x86_64_EmitSetcc(int opcode, Asm_x86_64_Reg reg)
{
    if (reg >= ASM_X86_64_REG_R8) {
        Enc_x86_64_Emit8(ENC_X86_64_REX_BASE | ENC_X86_64_REX_B);
    } else if (reg >= ASM_X86_64_REG_RSP) {
        Enc_x86_64_Emit8(ENC_X86_64_REX_BASE);
    }
    Enc_x86_64_Emit8(ENC_X86_64_OPCODE_ESCAPE);
    Enc_x86_64_Emit8(opcode);
    Enc_x86_64_EmitModRR(0, reg);
}

// Emits a rel32 control-transfer instruction with a fixup to its target.
void Enc_x86_64_EmitBranch(const Asm_x86_64_Item *item)
{
    switch (item->ai_op) {
        case ASM_X86_64_OP_JMP: {
            Enc_x86_64_Emit8(ENC_X86_64_OPCODE_JMP_REL32);
        } break;
        case ASM_X86_64_OP_CALL: {
            Enc_x86_64_Emit8(ENC_X86_64_OPCODE_CALL_REL32);
        } break;
        case ASM_X86_64_OP_JE: {
            Enc_x86_64_Emit8(ENC_X86_64_OPCODE_ESCAPE);
            Enc_x86_64_Emit8(ENC_X86_64_OPCODE2_JE_REL32);
        } break;
        case ASM_X86_64_OP_JNE: {
            Enc_x86_64_Emit8(ENC_X86_64_OPCODE_ESCAPE);
            Enc_x86_64_Emit8(ENC_X86_64_OPCODE2_JNE_REL32);
        } break;
        default: {
            // empty
        } break;
    }
    // A call may bind through the PLT; jmp/jcc are plain PC-relative.
    uint32_t type = item->ai_op == ASM_X86_64_OP_CALL ? R_X86_64_PLT32
                                                      : R_X86_64_PC32;
    Enc_x86_64_RecordFixup(item->ai_dst.ao_label, type);
    Enc_x86_64_Emit32(0);
}

// Emits a `mov` in whichever of its forms the operands select.
void Enc_x86_64_EmitMov(const Asm_x86_64_Item *item)
{
    Asm_x86_64_Reg dst = item->ai_dst.ao_reg;
    Asm_x86_64_Reg src = item->ai_src.ao_reg;

    switch (item->ai_src.ao_kind) {
        case ASM_X86_64_OPERAND_IMM: {
            if (item->ai_dst.ao_width == ASM_X86_64_WIDTH_8) {
                Enc_x86_64_EmitMovImm8(item->ai_src.ao_imm, dst);
            } else {
                Enc_x86_64_EmitMovImm(item->ai_src.ao_imm, dst);
            }
        } break;
        case ASM_X86_64_OPERAND_MEM: {
            Enc_x86_64_EmitMemForm(ENC_X86_64_OPCODE_MOV_R_RM, dst, item->ai_src.ao_reg, item->ai_src.ao_disp, ASM_X86_64_WIDTH_64);
        } break;
        case ASM_X86_64_OPERAND_REG: {
            if (item->ai_dst.ao_kind == ASM_X86_64_OPERAND_MEM) {
                Asm_x86_64_Width width = item->ai_src.ao_width;
                int opcode = width == ASM_X86_64_WIDTH_8 ? ENC_X86_64_OPCODE_MOV_RM8_R8 : ENC_X86_64_OPCODE_MOV_RM_R;
                Enc_x86_64_EmitMemForm(opcode, src, item->ai_dst.ao_reg, item->ai_dst.ao_disp, width);
            } else {
                Enc_x86_64_EmitRR(ENC_X86_64_OPCODE_MOV_RM_R, src, dst);
            }
        } break;
        default: {
            // empty
        } break;
    }
}

// Encodes one instruction item into the current section.
void Enc_x86_64_EmitInstr(const Asm_x86_64_Item *item)
{
    Asm_x86_64_Reg dst = item->ai_dst.ao_reg;
    Asm_x86_64_Reg src = item->ai_src.ao_reg;
    int imm = item->ai_src.ao_kind == ASM_X86_64_OPERAND_IMM;

    switch (item->ai_op) {
        case ASM_X86_64_OP_MOVSX: {
            Enc_x86_64_EmitMovsx(item);
        } break;
        case ASM_X86_64_OP_ADD: {
            if (imm) {
                Enc_x86_64_EmitGrpImm(ENC_X86_64_GRP_ADD, item->ai_src.ao_imm, dst);
            } else {
                Enc_x86_64_EmitRR(ENC_X86_64_OPCODE_ADD_RM_R, src, dst);
            }
        } break;
        case ASM_X86_64_OP_SUB: {
            if (imm) {
                Enc_x86_64_EmitGrpImm(ENC_X86_64_GRP_SUB, item->ai_src.ao_imm, dst);
            } else {
                Enc_x86_64_EmitRR(ENC_X86_64_OPCODE_SUB_RM_R, src, dst);
            }
        } break;
        case ASM_X86_64_OP_CMP: {
            if (imm) {
                Enc_x86_64_EmitGrpImm(ENC_X86_64_GRP_CMP, item->ai_src.ao_imm, dst);
            } else {
                Enc_x86_64_EmitRR(ENC_X86_64_OPCODE_CMP_RM_R, src, dst);
            }
        } break;
        case ASM_X86_64_OP_IMUL: {
            Enc_x86_64_EmitRexW(Enc_x86_64_RegHigh(dst), Enc_x86_64_RegHigh(src));
            Enc_x86_64_Emit8(ENC_X86_64_OPCODE_ESCAPE);
            Enc_x86_64_Emit8(ENC_X86_64_OPCODE2_IMUL_R_RM);
            Enc_x86_64_EmitModRR(dst, src);
        } break;
        case ASM_X86_64_OP_MOV: {
            Enc_x86_64_EmitMov(item);
        } break;
        case ASM_X86_64_OP_LEA: {
            if (item->ai_src.ao_kind == ASM_X86_64_OPERAND_RIP) {
                Enc_x86_64_EmitLeaRip(dst, item->ai_src.ao_label);
            } else {
                Enc_x86_64_EmitMemForm(ENC_X86_64_OPCODE_LEA_R_M, dst, item->ai_src.ao_reg, item->ai_src.ao_disp, ASM_X86_64_WIDTH_64);
            }
        } break;
        case ASM_X86_64_OP_IDIV: {
            Enc_x86_64_EmitGrpUnary(ENC_X86_64_GRP_IDIV, dst);
        } break;
        case ASM_X86_64_OP_NEG: {
            Enc_x86_64_EmitGrpUnary(ENC_X86_64_GRP_NEG, dst);
        } break;
        case ASM_X86_64_OP_CQO: {
            Enc_x86_64_Emit8(ENC_X86_64_REX_BASE | ENC_X86_64_REX_W);
            Enc_x86_64_Emit8(ENC_X86_64_OPCODE_CQO);
        } break;
        case ASM_X86_64_OP_SETE: {
            Enc_x86_64_EmitSetcc(ENC_X86_64_OPCODE2_SETE, dst);
        } break;
        case ASM_X86_64_OP_SETNE: {
            Enc_x86_64_EmitSetcc(ENC_X86_64_OPCODE2_SETNE, dst);
        } break;
        case ASM_X86_64_OP_SETL: {
            Enc_x86_64_EmitSetcc(ENC_X86_64_OPCODE2_SETL, dst);
        } break;
        case ASM_X86_64_OP_SETLE: {
            Enc_x86_64_EmitSetcc(ENC_X86_64_OPCODE2_SETLE, dst);
        } break;
        case ASM_X86_64_OP_MOVZB: {
            Enc_x86_64_EmitRexW(Enc_x86_64_RegHigh(dst), Enc_x86_64_RegHigh(src));
            Enc_x86_64_Emit8(ENC_X86_64_OPCODE_ESCAPE);
            Enc_x86_64_Emit8(ENC_X86_64_OPCODE2_MOVZX_R_RM8);
            Enc_x86_64_EmitModRR(dst, src);
        } break;
        case ASM_X86_64_OP_PUSH: {
            if (dst >= ASM_X86_64_REG_R8) {
                Enc_x86_64_Emit8(ENC_X86_64_REX_BASE | ENC_X86_64_REX_B);
            }
            Enc_x86_64_Emit8(ENC_X86_64_OPCODE_PUSH_R + (dst & ENC_X86_64_REG_MASK));
        } break;
        case ASM_X86_64_OP_POP: {
            if (dst >= ASM_X86_64_REG_R8) {
                Enc_x86_64_Emit8(ENC_X86_64_REX_BASE | ENC_X86_64_REX_B);
            }
            Enc_x86_64_Emit8(ENC_X86_64_OPCODE_POP_R + (dst & ENC_X86_64_REG_MASK));
        } break;
        case ASM_X86_64_OP_JMP:
        case ASM_X86_64_OP_JE:
        case ASM_X86_64_OP_JNE:
        case ASM_X86_64_OP_CALL: {
            Enc_x86_64_EmitBranch(item);
        } break;
        case ASM_X86_64_OP_RET: {
            Enc_x86_64_Emit8(ENC_X86_64_OPCODE_RET);
        } break;
        case ASM_X86_64_OP_SYSCALL: {
            Enc_x86_64_Emit8(ENC_X86_64_OPCODE_ESCAPE);
            Enc_x86_64_Emit8(ENC_X86_64_OPCODE2_SYSCALL);
        } break;
    }
}

// True if name was declared via .globl.
int Enc_x86_64_IsGlobl(const char *name)
{
    for (size_t i = 0; i < Enc_x86_64_NumGlobls; i++) {
        if (strcmp(Enc_x86_64_Globls[i], name) == 0) {
            return 1;
        }
    }
    return 0;
}

// Switches the current section to the named one, creating it on first use.
void Enc_x86_64_SelectSection(const char *name, uint32_t type, uint64_t flags)
{
    Enc_x86_64_Cur = Elf_SectionGet(Enc_x86_64_Out, name, type, flags);
}

// Creates a symbol for every label, then an undefined symbol for each unresolved target.
void Enc_x86_64_BuildSymbols(void)
{
    for (size_t i = 0; i < Enc_x86_64_NumLabels; i++) {
        Enc_x86_64_Label *l = &Enc_x86_64_Labels[i];
        int global = Enc_x86_64_IsGlobl(l->al_name);
        uint8_t bind = global ? ELF_BIND_GLOBAL : ELF_BIND_LOCAL;
        uint8_t type = ELF_TYPE_NOTYPE;
        if (global) {
            type = (l->al_sec->sec_flags & ELF_SHF_EXECINSTR) ? ELF_TYPE_FUNC
                                                              : ELF_TYPE_OBJECT;
        }
        Elf_SymbolAdd(Enc_x86_64_Out, l->al_name, l->al_sec, l->al_off, bind, type);
    }
    for (size_t i = 0; i < Enc_x86_64_NumFixes; i++) {
        const char *name = Enc_x86_64_Fixes[i].af_name;
        if (! Elf_SymbolFind(Enc_x86_64_Out, name)) {
            Elf_SymbolAdd(Enc_x86_64_Out, name, NULL, 0, ELF_BIND_GLOBAL, ELF_TYPE_NOTYPE);
        }
    }
}

// Turns each collected fixup into a relocation against its resolved symbol.
void Enc_x86_64_BuildRelocs(void)
{
    for (size_t i = 0; i < Enc_x86_64_NumFixes; i++) {
        Enc_x86_64_Fix *f   = &Enc_x86_64_Fixes[i];
        Elf_Sym        *sym = Elf_SymbolFind(Enc_x86_64_Out, f->af_name);
        Elf_RelaAdd(f->af_sec, f->af_off, sym, f->af_type, ENC_X86_64_REL32_ADDEND);
    }
}

// Discards any previous object and starts a fresh one.
void Enc_x86_64_Reset(void)
{
    if (Enc_x86_64_Out) {
        Elf_Free(Enc_x86_64_Out);
    }
    Enc_x86_64_Out       = Elf_New(ELF_ET_REL, ELF_EM_X86_64);
    Enc_x86_64_Cur       = NULL;
    Enc_x86_64_NumLabels = 0;
    Enc_x86_64_NumGlobls = 0;
    Enc_x86_64_NumFixes  = 0;
}

// Encodes the instruction list into a fresh relocatable object.
void Enc_x86_64_BuildObject(void)
{
    Enc_x86_64_Reset();
    Enc_x86_64_SelectSection(".text", ELF_SHT_PROGBITS, ELF_SHF_ALLOC | ELF_SHF_EXECINSTR);

    for (Asm_x86_64_Item *item = Asm_x86_64_Items(); item; item = item->ai_next) {
        switch (item->ai_kind) {
            case ASM_X86_64_ITEM_INSTR: {
                Enc_x86_64_EmitInstr(item);
            } break;
            case ASM_X86_64_ITEM_LABEL: {
                Enc_x86_64_RecordLabel(item->ai_label);
            } break;
            case ASM_X86_64_ITEM_SECTION: {
                Enc_x86_64_SelectSection(item->ai_secname, item->ai_sectype, item->ai_secflags);
            } break;
            case ASM_X86_64_ITEM_BYTES: {
                Enc_x86_64_EmitRaw(item->ai_bytes, item->ai_nbytes);
            } break;
            case ASM_X86_64_ITEM_GLOBL: {
                Enc_x86_64_RecordGlobl(item->ai_label);
            } break;
            case ASM_X86_64_ITEM_DIRECTIVE: {
                // raw assembler text, not represented in the encoded image
            } break;
        }
    }

    Enc_x86_64_BuildSymbols();
    Enc_x86_64_BuildRelocs();
}

// Returns the object the last Enc_x86_64_BuildObject() encoded.
Elf *Enc_x86_64_GetObject(void)
{
    return Enc_x86_64_Out;
}

// Writes the encoded object to out, returning nonzero on failure.
int Enc_x86_64_Write(FILE *out)
{
    return Elf_WriteFile(Enc_x86_64_Out, out);
}
