// (Test) Return: 29
// Several locals coexisting, reassignment, and a chained assignment.

int main()
{
    int a = 1;
    int b = 2;
    int c = 3;
    int d = 0;
    int e = 0;

    d = a + b + c;
    e = d;
    a = b = 7;

    return a + b + c + d + e;
}
