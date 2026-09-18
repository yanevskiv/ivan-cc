// (Test) Return: 60
// Subscripting reads and writes elements, an array's name is its address, and
// an array parameter is really a pointer.

int third(int a[])
{
    return a[2];
}

int main()
{
    char s[3];
    int a[5];
    int i;

    for (i = 0; i < 5; i = i + 1) {
        a[i] = i * 10;
    }

    if (a[0] != 0) return 1;
    if (a[4] != 40) return 2;
    if (*a != 0) return 3;
    if (*(a + 2) != 20) return 4;

    s[0] = 65;
    s[1] = 66;
    s[2] = 0;
    if (s[1] != 66) return 5;
    if (a[4] != 40) return 6;
    if (third(a) != 20) return 7;

    a[2] = a[2] + 5;

    return a[4] + a[2] - 5;
}
