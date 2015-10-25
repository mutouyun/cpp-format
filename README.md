# A C++ Formatting Library
C# / Rust style formatting in C++.  
The codes has been compiled in:  
 - MSVC-2015  
 - g++-4.9.1(-std=c++1y)  
 - clang-3.5.0-4(-std=c++1y)

# License
Codes covered by the MIT License.
# Tutorial
For using it, you only need to include printf.hpp or output.hpp.  
Some examples:
```cpp
/*
 * printf format string syntax.
*/

char c = 'A';
format::printf("1234567%s%c\n", " ", c);
format::printf(std::cout, "1234567%s%c\n", " ", c);

char buf[100];
format::printf([&buf](const std::string& str)
{
    strcpy(buf, str.c_str());
}, "1234567%s%c\n", " ", c);

/*
 * C# / Rust style formatting.
*/

format::output("Hello, World!\n");
format::output("Hello, {0}!\n", "World");
format::output("{}, {}, {}, {}\n", "World", 0, 1, 2);
format::output("{1}, {}, {0}, {}", 1, 2); // 2, 1, 1, 2

/*
 * Using format specifier to change your output, just likes std::printf.
 * See: http://www.cplusplus.com/reference/cstdio/printf/
*/

// 123.321000 123.3 0123 123.3210
format::output("{0} {1:.1} {2:04.} {3:04.04}", 123.321, 123.321, 123.321, 123.321);
format::output("{0} {0:.1} {0:04.} {0:04.04}", 123.321);
// address: 0x12345678
format::output("address: 0x{:08x}", 0x12345678);

/*
 * You could customize your output to redirect the result to any place.
*/

std::string buf;
auto out = [&buf](std::string&& str)
{
    buf = std::move(str);
};
format::output(out, "{0}, {1}, {2}, {3}", 0, 1, 2, 3); // buf = "0, 1, 2, 3"

/*
 * It will ignore needless spaces.
*/

// 123.321000 123.3 0123 123.3210
format::output(out, "{ 0 } {0 \t : .1} { 0:  04. } { 0 :04.04}", 123.321);

/*
 * You could output '{' & '}' for using "{{" & "}}".
*/

// {0, 1}, {2}, 3
format::output(out, "{{{}, {}}}, {{{}}}, {}", 0, 1, 2, 3);

/*
 * You could output your user-defined type like this:
*/

class Foo
{
public:
    template <typename T>
    void operator()(format::follower<T>&& out) const
    {
        out("Foo address: 0x{:08x}", 0x12345678/*(size_t)this*/);
    }
} foo;
// Foo address: 0x12345678, 1, 2, 3
format::output("{}, {}, {}, {}", foo, 1, 2, 3);
// 1 = 1, 2 = 2, 3 = 3\nfoo = Foo address: 0x12345678
format::output("1 = {}, ", 1)
              ("2 = {}, ", 2)
              ("3 = {}"  , 3).ln()
              ("foo = {}", foo);

/*
 * Any invalid input will throw an exception.
*/
EXPECT_THROW(format::printf("%s\n"    , 123)                , std::invalid_argument);
EXPECT_THROW(format::printf("%d, %s\n", 123)                , std::invalid_argument);
EXPECT_THROW(format::printf("%d\n"    , 123, "123")         , std::invalid_argument);
EXPECT_THROW(format::output("{{}, {}, {{}}, {}", 0, 1, 2, 3), std::invalid_argument);
EXPECT_THROW(format::output("{}, {}}, {{}}, {}", 0, 1, 2, 3), std::invalid_argument);
EXPECT_THROW(format::output("{}, {", 0, 1)                  , std::invalid_argument);
EXPECT_THROW(format::output("{}, {}{}", 0, 1)               , std::invalid_argument);
EXPECT_THROW(format::output("{}, {}}{}", 0, 1)              , std::invalid_argument);
EXPECT_THROW(format::output("Hello, {1}!", "World")         , std::invalid_argument);
EXPECT_THROW(format::output("Hello, {0}!", "World", 123)    , std::invalid_argument);
```
