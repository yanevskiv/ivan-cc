// (Test) Return: 23
// Every relational and equality operator, each weighted so that a wrong
// answer names itself in the return value.

int main()
{
    int r = 0;

    r = r + (1 < 2);
    r = r + (3 > 2) * 2;
    r = r + (2 <= 2) * 4;
    r = r + (3 >= 4) * 8;
    r = r + (5 == 5) * 16;
    r = r + (5 != 5) * 32;

    return r;
}
