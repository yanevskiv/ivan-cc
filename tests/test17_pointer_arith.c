// (Test) Return: 17
// Pointer arithmetic steps whole elements, and a difference counts them.

int main()
{
    int a[4];
    int *p;

    p = a;
    *p = 10;
    *(p + 1) = 20;
    *(p + 2) = 30;
    *(p + 3) = 40;

    if (*(p + 3) != 40) return 1;
    if (*(3 + p) != 40) return 2;
    if ((p + 3) - p != 3) return 3;

    p = p + 2;
    if (*p != 30) return 4;

    p = p - 1;
    if (*p != 20) return 5;

    return *p - 3;
}
