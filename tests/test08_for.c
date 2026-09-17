// (Test) Return: 67
// A for loop and a nested one.

int main()
{
    int i = 0;
    int j = 0;
    int s = 0;
    int n = 0;

    for (i = 1; i <= 10; i = i + 1) s = s + i;

    for (i = 0; i < 3; i = i + 1)
        for (j = 0; j < 4; j = j + 1)
            n = n + 1;

    return s + n;
}
