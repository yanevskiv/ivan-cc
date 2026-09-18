TARGET      := ivan
TARGET_ARCH := x86_64

OUT     := out
BUILD   := build

CRT_OBJ  := $(BUILD)/lib/crt0.o
LIBC_OBJ := $(BUILD)/lib/libc.o

CC      := gcc
CFLAGS  := -std=gnu99 -O2 -Iinclude -Iout -DTARGET_ARCH=$(TARGET_ARCH)
WARN    := -Wall -Wextra
LEX     := flex
YACC    := bison

MAIN_SRCS := src/cc.c src/ld.c src/as.c
ALL_SRCS  := $(shell find src -name '*.c' -not -path 'src/libc/*')
LIB_SRCS  := $(filter-out $(MAIN_SRCS),$(ALL_SRCS))
LIB_OBJS  := $(patsubst src/%.c,$(OUT)/%.o,$(LIB_SRCS))
GEN_OBJS  := $(OUT)/lex.yy.o $(OUT)/parser.tab.o

CC_OBJS := $(OUT)/cc.o $(LIB_OBJS) $(GEN_OBJS)
AS_OBJS := $(OUT)/as.o $(OUT)/util/file.o $(OUT)/util/str.o $(OUT)/util/elf.o \
	$(OUT)/arch/$(TARGET_ARCH)/txt.o $(OUT)/arch/$(TARGET_ARCH)/asm.o \
	$(OUT)/arch/$(TARGET_ARCH)/enc.o
LD_OBJS := $(OUT)/ld.o $(OUT)/util/str.o $(OUT)/util/elf.o $(OUT)/util/link.o $(OUT)/arch/$(TARGET_ARCH)/rel.o

TEST_TOOL  := tools/run_test
TEST_SRCS  := $(sort $(wildcard tests/test*.c))
TEST_NAMES := $(patsubst tests/%.c,%,$(TEST_SRCS))

CC_BIN := $(BUILD)/bin/$(TARGET)-cc
AS_BIN := $(BUILD)/bin/$(TARGET)-as
LD_BIN := $(BUILD)/bin/$(TARGET)-ld

# --- phony recipes ---
all: $(CC_BIN) $(AS_BIN) $(LD_BIN) $(CRT_OBJ) $(LIBC_OBJ)

clean:
	rm -rf $(BUILD) $(OUT)

tests: $(TEST_NAMES)

# --- tool recipes ---
$(CC_BIN): $(CC_OBJS) | $(BUILD)/bin
	$(CC) $(CFLAGS) $(WARN) $^ -o $@

$(AS_BIN): $(AS_OBJS) | $(BUILD)/bin
	$(CC) $(CFLAGS) $(WARN) $^ -o $@

$(LD_BIN): $(LD_OBJS) | $(BUILD)/bin
	$(CC) $(CFLAGS) $(WARN) $^ -o $@

# --- test recipes (one target per test, so `make test05_logical` works) ---
$(TEST_NAMES): %: tests/%.c $(TEST_TOOL) $(CC_BIN) $(CRT_OBJ) $(LIBC_OBJ)
	@$(TEST_TOOL) --compiler $(abspath $(CC_BIN)) $<

# --- front-end generators ---
$(OUT)/parser.tab.c $(OUT)/parser.tab.h: src/ast/parser.y | $(OUT)
	$(YACC) -d -o $(OUT)/parser.tab.c $<

$(OUT)/lex.yy.c: src/ast/lexer.flex $(OUT)/parser.tab.h | $(OUT)
	$(LEX) -o $@ $<

$(OUT)/lex.yy.o: $(OUT)/lex.yy.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OUT)/parser.tab.o: $(OUT)/parser.tab.c
	$(CC) $(CFLAGS) -c $< -o $@

# --- objects (mirrors the src/ tree under out/) ---
$(OUT)/%.o: src/%.c $(OUT)/parser.tab.h | $(OUT)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(WARN) -c $< -o $@

# --- runtime (libc/) recipes ---
$(CRT_OBJ): src/libc/arch/$(TARGET_ARCH)/crt0.s $(AS_BIN) | $(BUILD)/lib
	$(AS_BIN) $< -o $@

$(LIBC_OBJ): src/libc/libc.c $(CC_BIN) | $(BUILD)/lib
	$(CC_BIN) -c $< -o $@

# --- build/ recipes ---
$(OUT):
	mkdir -p $(OUT)

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/bin: | $(BUILD)
	mkdir -p $(BUILD)/bin

$(BUILD)/lib: | $(BUILD)
	mkdir -p $(BUILD)/lib

.PHONY: all clean tests $(TEST_NAMES)
