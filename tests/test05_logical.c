// (Test) Return: 5
// && and ||, including the short circuit: the guarded division by zero
// raises SIGFPE if either operator evaluates its right side needlessly.

int main()
{
    int zero = 0;
    int r = 0;

    r = r + (1 && 1);
    r = r + (1 && 0) * 2;
    r = r + (0 || 1) * 4;
    r = r + (0 || 0) * 8;

    if (zero && 1 / zero) {
        return 100;
    }

    if (!zero || 1 / zero) {
        return r;
    }

    return 101;
}
