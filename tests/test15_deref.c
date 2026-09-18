// (Test) Return: 42
// Reading and writing through a pointer reaches the object it points at.

int main()
{
    int x;
    int *p;

    x = 7;
    p = &x;

    if (*p != 7) return 1;

    *p = 42;
    if (x != 42) return 2;

    x = x + 1;
    if (*p != 43) return 3;

    return *p - 1;
}
