// (Test) Return: 25
// Compound statements nest, and an empty one is legal.

int main()
{
    int r = 1;

    {
        r = r + 1;
        {
            r = r * 10;
        }
    }

    { }

    if (1) {
        r = r + 5;
    }

    return r;
}
