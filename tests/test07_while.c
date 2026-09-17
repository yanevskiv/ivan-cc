// (Test) Return: 45
// A while loop that runs, and one whose condition is false on entry.

int main()
{
    int i = 0;
    int s = 0;

    while (i < 10) {
        s = s + i;
        i = i + 1;
    }

    while (0) {
        s = s + 1000;
    }

    return s;
}
