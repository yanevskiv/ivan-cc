// (Test) Return: 44
// A cast converts its operand to the named type, narrowing where that is smaller.

int main()
{
    int n;
    char c;
    int *p;

    n = 300;
    if ((char) n != 44) return 1;
    if (n != 300) return 2;

    n = -1;
    if ((char) n != -1) return 3;

    c = 65;
    if ((int) c != 65) return 4;

    n = 1000;
    p = (int *) &n;
    if (*p != 1000) return 5;

    return (char) 300;
}
