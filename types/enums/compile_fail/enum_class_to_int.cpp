// Must NOT compile: a scoped enum (enum class) does not convert to int
// implicitly. Use static_cast<int>(Signal::green) when the number is needed.
enum class Signal { red, amber, green };

int main()
{
    int n = Signal::green;
    return n;
}
