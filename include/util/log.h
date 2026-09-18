#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <stdlib.h>

// Prints a diagnostic and exits; shared by the lexer, parser and back end.
#define Log_ShowError(...)              \
    do {                                \
        fprintf(stderr, "cc: error: "); \
        fprintf(stderr, __VA_ARGS__);   \
        fprintf(stderr, "\n");          \
        exit(1);                        \
    } while (0)

// Prints a diagnostic naming the source line it came from and exits.
#define Log_ShowErrorAt(line, ...)                       \
    do {                                                 \
        fprintf(stderr, "cc: error: line %d: ", (line)); \
        fprintf(stderr, __VA_ARGS__);                    \
        fprintf(stderr, "\n");                           \
        exit(1);                                         \
    } while (0)

// Prints a warning diagnostic and continues; shared by the lexer, parser and back end.
#define Log_ShowWarning(...)              \
    do {                                  \
        fprintf(stderr, "cc: warning: "); \
        fprintf(stderr, __VA_ARGS__);     \
        fprintf(stderr, "\n");            \
    } while (0)

#endif // LOG_H
