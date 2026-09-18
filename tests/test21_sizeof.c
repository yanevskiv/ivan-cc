// (Test) Return: 60
// sizeof reports the size of a named type, or of what an expression yields.
// A string literal is an array of char, so its size counts the NUL.

int main()
{
    char c;
    int n;
    int *p;
    int a[10];

    if (sizeof(char) != 1) return 1;
    if (sizeof(int) != 4) return 2;
    if (sizeof(int *) != 8) return 3;
    if (sizeof(char *) != 8) return 4;

    if (sizeof(c) != 1) return 5;
    if (sizeof(n) != 4) return 6;
    if (sizeof(p) != 8) return 7;
    if (sizeof(a) != 40) return 8;
    if (sizeof(a[0]) != 4) return 9;

    p = a;
    if (sizeof(*p) != 4) return 10;
    if (sizeof *p != 4) return 11;

    if (sizeof("abc") != 4) return 12;
    if (sizeof("") != 1) return 13;

    return sizeof(a) + sizeof(int) * 5;
}
