/* Must NOT compile: enumeration constants share one namespace with
   variables and functions, so two enums in the same scope cannot both
   have a constant called NONE. */
enum alignment { NONE, LEFT, RIGHT };
enum border { NONE, THIN, THICK };

int main(void)
{
    return LEFT;
}
