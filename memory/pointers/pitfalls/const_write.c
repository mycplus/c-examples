/* Deliberately broken: writes through a pointer to const. */
int main(void)
{
    int a = 1;
    const int *p = &a;
    *p = 5;
    return 0;
}
