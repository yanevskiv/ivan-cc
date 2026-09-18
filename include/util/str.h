#ifndef STR_H
#define STR_H

#include <stdarg.h>

// A list of owned strings, as produced by Str_Split.
typedef struct Str_List Str_List;
struct Str_List {
    char **sl_items;
    int    sl_count;
};

// String utility functions
char *Str_Format(const char *fmt, ...);
char *Str_VFormat(const char *fmt, va_list ap);
char *Str_ChangeOrAppendExt(const char *input, const char *suffix);
int Str_Equals(const char *a, const char *b);
int Str_StartsWith(const char *str, const char *prefix);
char *Str_Trim(char *str);
void Str_Free(char *str);

// Splitting a string into an owned list of pieces
Str_List Str_Split(const char *str, const char *sep);
void Str_ListFree(Str_List *list);

// C literal escape decoding
char *Str_Unescape(const char *p, int len, int *out_len);

// POSIX extended-regex matching over a whole string
int Str_RegexMatch(const char *str, const char *pattern);
int Str_RegexExtract(const char *str, const char *pattern, char **groups, int ngroups);

#endif // STR_H
