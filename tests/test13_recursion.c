// (Test) Return: 175
// Linear and tree recursion.

int fact(int n)
{
    if (n <= 1) return 1;

    return n * fact(n - 1);
}

int fib(int n)
{
    if (n < 2) return n;

    return fib(n - 1) + fib(n - 2);
}

int main()
{
    return fact(5) + fib(10);
}
