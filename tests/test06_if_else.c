// (Test) Return: 121
// if, if/else, and a braced branch.

int main()
{
    int a = 7;
    int r = 0;

    if (a > 5) r = 1; else r = 2;

    if (a > 100) r = r + 10; else r = r + 20;

    if (a == 7) {
        r = r + 100;
    }

    return r;
}
