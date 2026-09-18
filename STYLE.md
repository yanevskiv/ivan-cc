# C Style

## Naming

- Every module (roughly: one `.c`/`.h` pair) picks a short `PascalCase`
  prefix and uses it everywhere: `Elf_`, `Link_`, `Str_`, `Ast_`.
- Public functions are `Prefix_VerbNoun`, e.g. `Elf_BufReserve`,
  `Link_PlaceSections`. The verb comes first; avoid `Prefix_NounVerb`.
- Types are `Prefix_Noun` (`Link_Options`, `Ast_Node`). Struct tags match
  their typedef name.
- Struct/union fields get a short lowercase tag derived from the type name,
  underscore, then the field name: `Link_Options` fields start `lo_`,
  `Ast_Node` fields start `an_`. This makes a field unambiguous when read
  out of context (in a debugger, a grep, an error message).
- Macros and enum constants are `SCREAMING_SNAKE_CASE`, prefixed with the
  module or type name: `LINK_MAX_PLACE`, `AST_NODE_KIND_ADD`.
- Local variables and parameters are short, lowercase, no prefix.

## Layout

- Indent with 4 spaces. No tabs, except where the tool requires them
  (Makefile recipe lines).
- Function definitions put the opening `{` on its own line. Control-flow
  statements (`if`, `for`, `while`, `switch`) put `{` on the same line as
  the keyword.
- Always brace a controlled block, even a single statement.
- Every `switch` case is a braced block written `case X: { ... } break;`,
  with the `break` after the closing brace. No one-line cases, and an empty
  body still gets its `{ }` with `// empty` inside, so a reader can tell a
  deliberate no-op from an unfinished one. Labels that share a body stack
  above it, and the brace opens on the last one.
- Negation is written `! x`, not `!x` — a bare `!` reads too easily as a
  typo or gets lost before a long expression.
- Casts are written `(type) expr` with a space, not `(type)expr`.
- Related consecutive declarations align their names/types/initializers in
  columns when it doesn't hurt readability (see any struct definition or a
  short run of assignments). Don't force alignment across unrelated lines.
- Keep lines within roughly 100 columns. A function declaration or
  definition is the exception: it stays on one line however long it gets,
  never wrapped across two.

## Structure

- Don't make functions `static`. Every function is declared in the module's
  header, so the header reads as a complete overview of what the `.c` file
  is and does. A reader should not have to open the `.c` to find out what is
  in it.
- File-scope variables are the opposite: keep them `static` and out of the
  header, unless another module genuinely needs one (`Ast_Program`).
- A `.c` file defines things in the same order its header declares them, so
  the two can be read side by side.
- A `.c` file is laid out in this order: includes, defines, enums, structs,
  global variables, static global variables, function definitions. Defines,
  enums and structs belong in the header where they can be, so most `.c`
  files start at the variables.

## Comments

Comments are load-bearing, not decorative. Every comment should tell the
reader something the code cannot: intent, a non-obvious invariant, a reason
a workaround exists, or what a name abbreviates. A comment that only
restates the following line in English is worse than no comment — delete it.

What gets a comment:

- **Every function** gets exactly one comment directly above it, describing
  what it does (and, if not obvious from the signature, why it exists or
  what it assumes). One line normally suffices; wrap to a second only when
  genuinely necessary.
- **Every macro, typedef, struct, and enum** gets one comment above it for
  the same reason. Struct fields and enum constants that aren't
  self-explanatory get a short trailing `// comment` instead of one above.
- **Section dividers** are allowed to group related declarations in a
  header, or related phases inside a long function (`// Phase: ...`), when
  the grouping itself is information.
- A short inline comment is allowed at a specific line where the code does
  something surprising (a non-obvious flag, a deliberate deviation, a
  workaround for an external constraint) — never to narrate normal control
  flow.

What does not get a comment:

- Do not describe *what* a line of code does when the code already says so
  in plain identifiers. Name things well instead of commenting them.
- Do not leave commented-out code, TODOs-as-narration, or changelog-style
  notes ("added X for the Y fix") — that belongs in commit messages, not
  source.
- Do not add a comment just because a block "looks like it needs one" —
  build configuration, glue code and boilerplate are commented only where a
  choice in them is non-obvious, never by default.

If you can't point to what a comment tells the reader beyond the code
itself, delete it.
