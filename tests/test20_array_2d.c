// (Test) Return: 66
// A two-dimensional array is an array of arrays, indexed row then column.

int main()
{
    int a[3][4];
    int sum;
    int i;
    int j;

    for (i = 0; i < 3; i = i + 1) {
        for (j = 0; j < 4; j = j + 1) {
            a[i][j] = i * 4 + j;
        }
    }

    if (a[0][0] != 0) return 1;
    if (a[2][3] != 11) return 2;
    if (*(*(a + 1) + 2) != 6) return 3;

    sum = 0;
    for (i = 0; i < 3; i = i + 1) {
        for (j = 0; j < 4; j = j + 1) {
            sum = sum + a[i][j];
        }
    }

    return sum;
}
