/*
    cpp-format - Code covered by the MIT License
    Author: mutouyun (http://darkc.at)
*/

#pragma once

#include <typeinfo>     // typeid
#include <type_traits>  // std::is_convertible, std::enable_if, ...
#include <string>       // std::string
#include <stdexcept>    // std::invalid_argument
#include <iostream>     // std::cout
#include <utility>      // std::forward, std::move
#include <cstdint>      // intmax_t, uintmax_t
#include <cstddef>      // size_t, ptrdiff_t
#include <cstdio>       // vsnprintf
#include <cstdarg>      // va_list, va_start, va_end

#if defined(__GNUC__)
#   include <cxxabi.h>  // abi::__cxa_demangle
#endif/*__GNUC__*/

namespace format {

/*
    Type qualifiers & reference removing
*/

template <typename T>
using underlying = typename std::remove_cv<typename std::remove_reference<T>::type>::type;

/*
    Functor & closure checking
*/

namespace detail_format_ {

struct is_functor_checker_
{
    template <typename T> static std::true_type  check(decltype(&T::operator())*);
    template <typename T> static std::false_type check(...);
};

} // namespace detail_format_

template <typename T>
struct is_functor : decltype(detail_format_::is_functor_checker_::check<T>(nullptr)) {};

template <typename T>
using is_closure = std::integral_constant<bool, 
     (is_functor<underlying<T>>::value || std::is_function<typename std::remove_pointer<underlying<T>>::type>::value)>;

/*
    Shift-left checking
*/

namespace detail_format_ {

struct can_shift_left_checker_
{
    template <typename T> static std::true_type  check(typename std::remove_reference<
                                                                decltype(std::declval<T>() << std::declval<std::string>())
                                                                >::type*);
    template <typename T> static std::false_type check(...);
};

} // namespace detail_format_

template <typename T>
struct can_shift_left : decltype(detail_format_::can_shift_left_checker_::check<underlying<T>>(nullptr)) {};

/*
    Casting checking
*/

namespace detail_format_ {

struct can_cast_str_checker_
{
    template <typename T> static std::true_type  check(decltype(static_cast<const char*>(std::declval<T&&>()))*);
    template <typename T> static std::false_type check(...);
};

} // namespace detail_format_

template <typename T>
struct can_cast_str : decltype(detail_format_::can_cast_str_checker_::check<T>(nullptr)) {};

/*
    Define concepts for SFINAE.
*/
template <typename T, typename U>
using Different = std::integral_constant<bool, !std::is_same<underlying<T>, underlying<U>>::value>;

template <typename T>
using OutputPred = std::integral_constant<bool, is_closure<T>::value || can_shift_left<T>::value>;

////////////////////////////////////////////////////////////////
/// Perpare for type-safe printf
////////////////////////////////////////////////////////////////

namespace detail_printf_ {

template <typename T>
std::string type_name(void)
{
#if defined(__GNUC__)
    const char* typeid_name = typeid(T).name();
    char* real_name = abi::__cxa_demangle(typeid_name, nullptr, nullptr, nullptr);
    std::string name;
    if (real_name == nullptr)
    {
        name = typeid_name;
    }
    else
    {
        name = real_name;
        ::free(real_name);
    }
    return name;
#else /*__GNUC__*/
    return { typeid(T).name() };
#endif/*__GNUC__*/
}

inline void enforce(std::string&& what)
{
    throw std::invalid_argument
    {
        "Invalid format: " + what + "."
    };
}

template <typename T, typename U>
inline void enforce(void)
{
    if (!std::is_convertible<T, U>::value)
    {
        enforce(type_name<T>() + " => " + type_name<U>());
    }
}

template <typename T>
void enforce_argument(const char* fmt)
{
    using t_t = typename std::decay<T>::type;
    enum class length_t
    {
        none, h, hh, l, ll, j, z, t, L
    }
    state = length_t::none;
    for (; *fmt; ++fmt)
    {
        switch(*fmt)
        {
        // check specifiers's length
        case 'h':
                if (state == length_t::h) state = length_t::hh;
                else state = length_t::h; break;
        case 'l':
                if (state == length_t::l) state = length_t::ll;
                else state = length_t::l; break;
        case 'j': state = length_t::j; break;
        case 'z': state = length_t::z; break;
        case 't': state = length_t::t; break;
        case 'L': state = length_t::L; break;

        // check specifiers
        case 'd': case 'i':
            switch(state)
            {
            default:
            case length_t::none: enforce<t_t, int>();       break;
            case length_t::h:    enforce<t_t, short>();     break;
            case length_t::hh:   enforce<t_t, char>();      break;
            case length_t::l:    enforce<t_t, long>();      break;
            case length_t::ll:   enforce<t_t, long long>(); break;
            case length_t::j:    enforce<t_t, intmax_t>();  break;
            case length_t::z:    enforce<t_t, size_t>();    break;
            case length_t::t:    enforce<t_t, ptrdiff_t>(); break;
            }
            return;
        case 'u': case 'o': case 'x': case 'X':
            switch(state)
            {
            default:
            case length_t::none: enforce<t_t, unsigned int>();       break;
            case length_t::h:    enforce<t_t, unsigned short>();     break;
            case length_t::hh:   enforce<t_t, unsigned char>();      break;
            case length_t::l:    enforce<t_t, unsigned long>();      break;
            case length_t::ll:   enforce<t_t, unsigned long long>(); break;
            case length_t::j:    enforce<t_t, uintmax_t>();          break;
            case length_t::z:    enforce<t_t, size_t>();             break;
            case length_t::t:    enforce<t_t, ptrdiff_t>();          break;
            }
            return;
        case 'f': case 'F': case 'e': case 'E': case 'g': case 'G': case 'a': case 'A':
            switch(state)
            {
            default:
            case length_t::none: enforce<t_t, double>();      break;
            case length_t::L:    enforce<t_t, long double>(); break;
            }
            return;
        case 'c':
            switch(state)
            {
            default:
            case length_t::none: enforce<t_t, char>();    break;
            case length_t::l:    enforce<t_t, wchar_t>(); break;
            }
            return;
        case 's':
            switch(state)
            {
            default:
            case length_t::none: enforce<t_t, const char*>();    break;
            case length_t::l:    enforce<t_t, const wchar_t*>(); break;
            }
            return;
        case 'p':
            enforce<t_t, void*>();
            return;
        case 'n':
            switch(state)
            {
            default:
            case length_t::none: enforce<t_t, int*>();       break;
            case length_t::h:    enforce<t_t, short*>();     break;
            case length_t::hh:   enforce<t_t, char*>();      break;
            case length_t::l:    enforce<t_t, long*>();      break;
            case length_t::ll:   enforce<t_t, long long*>(); break;
            case length_t::j:    enforce<t_t, intmax_t*>();  break;
            case length_t::z:    enforce<t_t, size_t*>();    break;
            case length_t::t:    enforce<t_t, ptrdiff_t*>(); break;
            }
            return;
        }
    }
    enforce("Has no specifier");
}

void check(const char* fmt)
{
    for (; *fmt; ++fmt)
    {
        if (*fmt != '%' || *++fmt == '%') continue;
        enforce("Bad format");
    }
}

template <typename T1, typename... T>
void check(const char* fmt, T1&& /*a1*/, T&&... args)
{
    for (; *fmt; ++fmt)
    {
        if (*fmt != '%' || *++fmt == '%') continue;
        enforce_argument<T1>(fmt);
        return check(++fmt, args...);
    }
    enforce("Too few format specifiers");
}

template <typename F, typename std::enable_if<is_closure<F>::value, bool>::type = true>
void do_out(F&& out, std::string&& buf)
{
    out(std::move(buf));
}

template <typename F, typename std::enable_if<!is_closure<F>::value, bool>::type = true>
void do_out(F&& out, std::string&& buf)
{
    out << std::move(buf);
}

inline bool is_specifier(char c)
{
    static const char sps[] =
    {
        'd', 'i', 'u', 'o', 'x', 'X', 'f', 'F', 'e',
        'E', 'g', 'G', 'a', 'A', 'c', 's', 'p', 'n'
    };
    for (char s : sps) if (s == c) return true;
    return false;
}

template <typename F>
int impl_(F&& out, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    std::string buf;
    int n = ::vsnprintf(nullptr, 0, fmt, args);
    if (n <= 0) goto exit_output;
    buf.resize(n);
    n = ::vsnprintf(const_cast<char*>(buf.data()), n + 1, fmt, args);
    if (n <= 0) goto exit_output;
    do_out(std::forward<F>(out), std::move(buf));
exit_output:
    va_end(args);
    return n;
}

} // namespace detail_printf_

////////////////////////////////////////////////////////////////
/// Print formatted data to output stream
////////////////////////////////////////////////////////////////

namespace use
{
    auto strout(std::string& buf)
    {
        return [&](std::string&& str)
        {
            buf = std::move(str);
        };
    }
}

template <typename F, typename... T, typename std::enable_if<OutputPred<F>::value, bool>::type = true>
int printf(F&& out, const char* fmt, T&&... args)
{
    if (fmt == nullptr) return 0;
    detail_printf_::check(fmt, std::forward<T>(args)...);
    return detail_printf_::impl_(std::forward<F>(out), fmt, std::forward<T>(args)...);
}

template <typename... T>
int printf(const char* fmt, T&&... args)
{
    return format::printf(std::cout, fmt, std::forward<T>(args)...);
}

} // namespace format
