// (Test) Return: 12
// Walking a literal through a char * finds its length, as printf must.

int length(char *s)
{
    char *p;

    p = s;
    while (*p) {
        p = p + 1;
    }
    return p - s;
}

int main()
{
    char *s;

    s = "Hello world!";

    if (*s != 'H') return 1;
    if (*(s + 4) != 'o') return 2;
    if (s[11] != '!') return 3;
    if (s[12] != 0) return 4;

    return length(s);
}
