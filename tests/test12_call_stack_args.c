// (Test) Return: 204
// Eight arguments, so two travel on the stack rather than in registers.
// Each is weighted, so a misordered or dropped argument changes the result.

int weigh(int a, int b, int c, int d, int e, int f, int g, int h)
{
    return a + b * 2 + c * 3 + d * 4 + e * 5 + f * 6 + g * 7 + h * 8;
}

int main()
{
    return weigh(1, 2, 3, 4, 5, 6, 7, 8);
}
