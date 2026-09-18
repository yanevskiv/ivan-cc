#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <stdlib.h>

// Print a diagnostic and exit; shared by the lexer, parser and back end.
#define Log_ShowError(...)              \
    do {                                \
        fprintf(stderr, "cc: error: "); \
        fprintf(stderr, __VA_ARGS__);   \
        fprintf(stderr, "\n");          \
        exit(1);                        \
    } while (0)

// Print a diagnostic naming the source line it came from and exit.
#define Log_ShowErrorAt(line, ...)                       \
    do {                                                 \
        fprintf(stderr, "cc: error: line %d: ", (line)); \
        fprintf(stderr, __VA_ARGS__);                    \
        fprintf(stderr, "\n");                           \
        exit(1);                                         \
    } while (0)

// Print a warning diagnostic and continue; shared by the lexer, parser and back end.
#define Log_ShowWarning(...)              \
    do {                                  \
        fprintf(stderr, "cc: warning: "); \
        fprintf(stderr, __VA_ARGS__);     \
        fprintf(stderr, "\n");            \
    } while (0)

#endif // LOG_H
