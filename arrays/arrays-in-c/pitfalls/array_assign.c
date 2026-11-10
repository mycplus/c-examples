/* Deliberately broken: arrays cannot be assigned with =. */
int main(void)
{
    int today[4] = {3, 1, 4, 1};
    int saved[4];
    saved = today;
    return saved[0];
}
