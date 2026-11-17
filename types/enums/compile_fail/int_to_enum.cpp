// Must NOT compile: unlike C, C++ does not convert an int to an enum
// implicitly, even a plain (unscoped) one.
enum Color { red, green, blue };

int main()
{
    Color c = 1;
    return c;
}
