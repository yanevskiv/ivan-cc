// (Test) Return: 21
// & yields an address a callee can write through, and each object has its own.

int store(int *p, int v)
{
    *p = v;
    return 0;
}

int main()
{
    int a;
    int b;

    a = 1;
    b = 2;

    if (&a == &b) return 1;

    store(&a, 20);
    store(&b, 1);

    if (a != 20) return 2;
    if (b != 1) return 3;

    return a + b;
}
