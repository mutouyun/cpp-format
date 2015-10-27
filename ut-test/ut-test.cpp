#if defined(_MSC_VER)
#   define _CRT_SECURE_NO_WARNINGS
#endif

#include "printf.hpp"
#include "output.hpp"

#include <assert.h>
#include <string.h>

////////////////////////////////////////////////////////////////

#define TEST_METHOD(TEST_NAME) void test_##TEST_NAME(void)

#define EXPECT_STREQ(EXPECTED, ACTUAL) assert(strcmp(EXPECTED, ACTUAL) == 0)
#define EXPECT_THROW(STATEMENT, EXPECTED_EXCEPTION) \
do {                                                \
    bool should_catch_expected_ = false;            \
    (void)should_catch_expected_;                   \
    try {                                           \
        STATEMENT;                                  \
    } catch (EXPECTED_EXCEPTION const &) {          \
        should_catch_expected_ = true;              \
    } catch (...) {}                                \
    assert(should_catch_expected_);                 \
} while(0)
        
////////////////////////////////////////////////////////////////

TEST_METHOD(printf)
{
    char c = 'A';
    char buf[100];

    format::printf("1234567%s%c\n", " ", c);
    format::printf(std::cout, "1234567%s%c\n", " ", c);
    format::printf([&buf](const std::string& str)
    {
        strcpy(buf, str.c_str());
    }, "1234567%s%c\n", " ", c);
    EXPECT_STREQ("1234567 A\n", buf);

    EXPECT_THROW(format::printf(std::cout, "%s\n"    , 123       ), std::invalid_argument);
    EXPECT_THROW(format::printf(std::cout, "%d, %s\n", 123       ), std::invalid_argument);
    EXPECT_THROW(format::printf(std::cout, "%d\n"    , 123, "123"), std::invalid_argument);
}

////////////////////////////////////////////////////////////////

std::string buf;
void out(std::string&& str)
{
    buf = std::move(str);
    std::cout << buf << std::endl;
}

TEST_METHOD(output)
{
    format::output(out, "Hello, {0}!", "World");
    EXPECT_STREQ("Hello, World!", buf.c_str());

    format::output(out, "{0} {1:.1} {2:04.} {3:04.04}", 123.321, 123.321, 123.321, 123.321);
    EXPECT_STREQ("123.321000 123.3 0123 123.3210", buf.c_str());

    format::output(out, "{0} {0:.1} {0:04.} {0:04.04}", 123.321);
    EXPECT_STREQ("123.321000 123.3 0123 123.3210", buf.c_str());

    format::output(out, "{0}, {1}, {2}, {3}", 0, 1, 2, 3);
    EXPECT_STREQ("0, 1, 2, 3", buf.c_str());

    format::output(out, "{}, {}, {}, {}", 0, 1, 2, 3);
    EXPECT_STREQ("0, 1, 2, 3", buf.c_str());

    format::output(out, "{1}, {}, {0}, {}", 1, 2);
    EXPECT_STREQ("2, 1, 1, 2", buf.c_str());

    format::output(out, "{0}, {3}, {1}, {2}", 0, 1, 2, 3);
    EXPECT_STREQ("0, 3, 1, 2", buf.c_str());
}

TEST_METHOD(space)
{
    format::output(out, "Hello, {0  }!", "World");
    EXPECT_STREQ("Hello, World!", buf.c_str());

    format::output(out, "{ 0 } {0 \t : .1} { 0:  04. } { 0 :04.04}", 123.321);
    EXPECT_STREQ("123.321000 123.3 0123 123.3210", buf.c_str());

    format::output(out, "{0}, {3}{2}{1}", 0, 1, 2, 3);
    EXPECT_STREQ("0, 321", buf.c_str());
}

TEST_METHOD(no_placeholder)
{
    format::output(out, "{}, {}, {}, {}", 0, 1, 2, 3);
    EXPECT_STREQ("0, 1, 2, 3", buf.c_str());

    format::output(out, "{_}, {:}, { }, {\t}, {-}, { \t }, {gdgd}", 0, 1, 2);
    EXPECT_STREQ("0, 0, 0, 1, 0, 2, 0", buf.c_str());

    format::output(out, "{{{}, {}}}, {{{}}}, {}", 0, 1, 2, 3);
    EXPECT_STREQ("{0, 1}, {2}, 3", buf.c_str());

    EXPECT_THROW(format::output(out, "{{}, {}, {{}}, {}", 0, 1, 2, 3), std::invalid_argument);
    EXPECT_THROW(format::output(out, "{}, {}}, {{}}, {}", 0, 1, 2, 3), std::invalid_argument);
    EXPECT_THROW(format::output(out, "{}, {", 0, 1), std::invalid_argument);
    EXPECT_THROW(format::output(out, "{}, {}{}", 0, 1), std::invalid_argument);
    EXPECT_THROW(format::output(out, "{}, {}}{}", 0, 1), std::invalid_argument);
    EXPECT_THROW(format::output(out, "Hello, {1}!", "World"), std::invalid_argument);
    EXPECT_THROW(format::output(out, "Hello, {0}!", "World", 123), std::invalid_argument);
}

TEST_METHOD(default_out)
{
    format::output("Hello, World!\n");
    format::output("Hello, {0}!\n", "World");
    format::output((char*)"Hello, {0}!\n", "World");
    format::output("{}, {}, {}, {}\n", "World", 0, 1, 2);
}

////////////////////////////////////////////////////////////////

class Foo
{
public:
    template <typename T>
    void operator()(format::follower<T>&& out) const
    {
        out("Foo address: 0x{:08x}", 0x12345678/*(size_t)this*/);
    }
};

TEST_METHOD(custom_type)
{
    Foo foo;
    format::output(out, "{}, {}, {}, {}", foo, 1, 2, 3);
    EXPECT_STREQ("Foo address: 0x12345678, 1, 2, 3", buf.c_str());
}

TEST_METHOD(follower)
{
    format::output("Hello, World!").ln();

    Foo foo;
    format::output(out, "1 = {}, ", 1)
                       ("2 = {}, ", 2)
                       ("3 = {}"  , 3).ln()
                       ("foo = {}", foo);
    EXPECT_STREQ("1 = 1, 2 = 2, 3 = 3\nfoo = Foo address: 0x12345678", buf.c_str());

    buf.clear();
    {
        auto flw = format::output(out, "1 = {}", 1).ln();
        flw("2 = {}", 2).ln();
        EXPECT_STREQ("", buf.c_str());
        flw();
        EXPECT_STREQ("1 = 1\n2 = 2\n", buf.c_str());
        flw("3 = {}", 3).ln();
        EXPECT_STREQ("1 = 1\n2 = 2\n", buf.c_str());
        flw.clear();
        flw("4 = {}", 4).ln();
    }
    EXPECT_STREQ("4 = 4\n", buf.c_str());
}

////////////////////////////////////////////////////////////////

class Bar1
{
public:
    explicit operator char*(void) const
    {
        return "I'm Bar1...";
    }
};

class Bar2
{
public:
    operator const char*(void) const
    {
        return "I'm Bar2...";
    }
};

TEST_METHOD(string_object)
{
    std::string str = "Hello, World!";
    format::output(out, "{}", str);
    EXPECT_STREQ("Hello, World!", buf.c_str());

    format::output(out, "{}", Bar1{});
    EXPECT_STREQ("I'm Bar1...", buf.c_str());

    format::output(out, "{}", Bar2{});
    EXPECT_STREQ("I'm Bar2...", buf.c_str());
}

////////////////////////////////////////////////////////////////

int main(void)
{
    test_printf();
    test_output();
    test_space();
    test_no_placeholder();
    test_default_out();
    test_custom_type();
    test_follower();
    test_string_object();
    return 0;
}
