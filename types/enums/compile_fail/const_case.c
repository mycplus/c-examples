/* Must NOT compile with warnings as errors: a const int is not an
   integer constant expression in C, so it cannot be a case label.
   (An enumeration constant can; see src/order.c.) */
static const int MAX_RETRIES = 3;

int give_up(int attempts)
{
    switch (attempts) {
    case MAX_RETRIES:
        return 1;
    default:
        return 0;
    }
}

int main(void)
{
    return give_up(1);
}
