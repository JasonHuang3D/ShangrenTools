//
// Author: Jason Huang(jasonhuang1988@gmail.com) 2021
//
#pragma once

#include <array>
#include <chrono>
#include <functional>
#include <iomanip>
#include <locale>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

template <typename EnumType, typename = typename std::enable_if_t<std::is_enum_v<EnumType>>>
using TUnderLyingEnum = typename std::underlying_type<EnumType>::type;
#define DEFINE_FLAG_ENUM_OPERATORS(ENUMTYPE)                                                       \
    inline ENUMTYPE& operator|=(ENUMTYPE& a, ENUMTYPE b)                                           \
    {                                                                                              \
        return reinterpret_cast<ENUMTYPE&>(reinterpret_cast<TUnderLyingEnum<ENUMTYPE>&>(a) |=      \
            static_cast<TUnderLyingEnum<ENUMTYPE>>(b));                                            \
    }                                                                                              \
    inline ENUMTYPE& operator&=(ENUMTYPE& a, ENUMTYPE b)                                           \
    {                                                                                              \
        return reinterpret_cast<ENUMTYPE&>(reinterpret_cast<TUnderLyingEnum<ENUMTYPE>&>(a) &=      \
            static_cast<TUnderLyingEnum<ENUMTYPE>>(b));                                            \
    }                                                                                              \
    inline ENUMTYPE& operator^=(ENUMTYPE& a, ENUMTYPE b)                                           \
    {                                                                                              \
        return reinterpret_cast<ENUMTYPE&>(reinterpret_cast<TUnderLyingEnum<ENUMTYPE>&>(a) ^=      \
            static_cast<TUnderLyingEnum<ENUMTYPE>>(b));                                            \
    }                                                                                              \
    inline constexpr ENUMTYPE operator|(ENUMTYPE a, ENUMTYPE b)                                    \
    {                                                                                              \
        return static_cast<ENUMTYPE>(static_cast<TUnderLyingEnum<ENUMTYPE>>(a) |                   \
            static_cast<TUnderLyingEnum<ENUMTYPE>>(b));                                            \
    }                                                                                              \
    inline constexpr ENUMTYPE operator&(ENUMTYPE a, ENUMTYPE b)                                    \
    {                                                                                              \
        return static_cast<ENUMTYPE>(static_cast<TUnderLyingEnum<ENUMTYPE>>(a) &                   \
            static_cast<TUnderLyingEnum<ENUMTYPE>>(b));                                            \
    }                                                                                              \
    inline constexpr ENUMTYPE operator^(ENUMTYPE a, ENUMTYPE b)                                    \
    {                                                                                              \
        return static_cast<ENUMTYPE>(static_cast<TUnderLyingEnum<ENUMTYPE>>(a) ^                   \
            static_cast<TUnderLyingEnum<ENUMTYPE>>(b));                                            \
    }                                                                                              \
    inline constexpr ENUMTYPE operator~(ENUMTYPE a)                                                \
    {                                                                                              \
        return static_cast<ENUMTYPE>(~static_cast<TUnderLyingEnum<ENUMTYPE>>(a));                  \
    }

namespace JUtils
{

// Accepts std::string std::wstring, etc.
template <typename T>
std::vector<T> TockenizeString(const T& str, const T& delimiters)
{
    std::vector<T> v;
    typename T::size_type start = 0;
    auto pos                    = str.find_first_of(delimiters, start);

    while (pos != T::npos)
    {
        if (pos != start) // ignore empty tokens
            v.emplace_back(str, start, pos - start);
        start = pos + 1;
        pos   = str.find_first_of(delimiters, start);
    }

    if (start < str.length())                             // ignore trailing delimiter
        v.emplace_back(str, start, str.length() - start); // add what's left of the string

    return v;
}

// Remove the Byte Order Mark from string if there is any.
inline void RemoveBomFromString(std::string& x)
{
    if (x.size() < 3)
        return;
    if (x[0] == '\xEF' && x[1] == '\xBB' && x[2] == '\xBF')
        x.erase(0, 3);
}

template <std::streamsize Precision = 2, std::uint8_t NumDigitsToGroup = 4, typename TypeValue,
    typename TypeExpected = TypeValue, typename = std::enable_if_t<std::is_arithmetic_v<TypeValue>>>
std::string FormatNumber(TypeValue value)
{
    class CommaFacet : public std::numpunct<char>
    {
    protected:
        char do_thousands_sep() const override { return ','; }
        std::string do_grouping() const override
        {
            static constexpr auto kGourp = { static_cast<char>(NumDigitsToGroup) };
            return kGourp;
        }
    };
    // this creates a new locale based on the current application default
    // (which is either the one given on startup, but can be overriden with
    // std::locale::global) - then extends it with an extra facet that
    // controls numeric output.
    // Note: std::locale will delete facet ptr when desstruct.
    static std::locale s_commaLocale(std::locale(), new CommaFacet);

    std::stringstream ss;
    ss.imbue(s_commaLocale);

    if constexpr (std::is_floating_point_v<TypeExpected>)
    {
        ss << std::fixed << std::setprecision(Precision);
    }

    if constexpr (std::is_same_v<TypeExpected, TypeValue>)
    {
        ss << value;
    }
    else
    {
        ss << static_cast<TypeExpected>(value);
    }
    return ss.str();
}
// Helper to format float to int with commas
template <typename TypeInt, std::uint8_t NumDigitsToGroup = 4, typename TypeFloat,
    typename = std::enable_if_t<std::is_floating_point_v<TypeFloat>>>
std::string FormatFloatToInt(TypeFloat value)
{
    return FormatNumber<0, NumDigitsToGroup, TypeFloat, TypeInt>(value);
}

template <typename TypeFloat, typename TypeInt>
TypeFloat FormatIntToFloat(TypeInt value, TypeInt scale)
{
    return static_cast<TypeFloat>(value) / scale;
}

template <std::streamsize Precision = 2, typename... TRest>
static std::string FormatString(TRest&&... args)
{
    std::stringstream ss;
    ss << std::fixed << std::setprecision(Precision);
    (ss << ... << std::forward<TRest>(args));
    return ss.str();
}

// Helper to check if char ptr is empty
bool isCharPtrEmpty(const char* str);

// Invalid value of every possible bits
constexpr std::uint32_t GetInvalidValue(std::uint32_t numBits)
{
    return static_cast<std::uint32_t>((1ULL << numBits) - 1);
}

// Helper function to compute and combine hash value
template <typename TypeFirst, typename... TypeRest>
void HashCombine(std::size_t& seed, const TypeFirst& v, TypeRest&&... rest)
{
    seed ^= std::hash<TypeFirst>()(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    (HashCombine(seed, std::forward<TypeRest>(rest)), ...);
}
template <typename... TyepArgs>
std::size_t ComputeHash(TyepArgs&&... args)
{
    std::size_t seed = 0;
    HashCombine(seed, std::forward<TyepArgs>(args)...);
    return seed;
}

// Get binary form of the given integer
template <typename T, typename = typename std::enable_if_t<std::is_integral_v<T>>>
std::string GetBinaryStrFromInt(T value)
{
    return std::bitset<sizeof(T) * 8>(value).to_string();
}

struct ArrayHelper
{
    // A generator for bit mask
    static constexpr std::uint64_t BitMaskGenerator(std::size_t index) { return 1ull << index; };

    // A generator for index
    static constexpr std::uint64_t IndexGenerator(std::size_t index) { return index; };

    // Helper function to create an array with init values at compile time
    template <typename T, std::size_t N>
    static constexpr std::array<T, N> CreateArrayWithInitValues(T (*generator)(std::size_t))
    {
        return FillArray(generator, std::make_index_sequence<N>());
    }

private:
    template <typename T, std::size_t... I>
    static constexpr std::array<T, sizeof...(I)> FillArray(
        T (*generator)(std::size_t), std::index_sequence<I...>)
    {
        return std::array<T, sizeof...(I)> { generator(I)... };
    }
};

// Helper to create recursive lambda
template <typename T>
struct LambdaCombinator
{
    LambdaCombinator(T lambda) : lambda(lambda) {}

    template <class... TArgs>
    decltype(auto) operator()(TArgs&&... args) const
    {
        // We pass ourselves to lambda, then the arguments.
        return lambda(*this, std::forward<TArgs>(args)...);
    }

    T lambda;
};
// C++17 Class Type Decution Guide
template <class T>
LambdaCombinator(T) -> LambdaCombinator<T>;

class Timer
{
public:
    Timer();
    // Starts/resets the high resolution timer.
    void Reset();

    double DurationInSec();
    double DurationInMsec();

private:
    double Elapsed() const;
    std::chrono::high_resolution_clock m_clock;
    std::chrono::time_point<std::chrono::high_resolution_clock> m_t0;
};

// Helper to get pointer type to a class member
template <typename TypeMemberPtr>
struct GetMemberPointerType
{
private:
    template <typename C, typename T>
    static T get_type(T C::*v) {};

public:
    typedef decltype(get_type(static_cast<TypeMemberPtr>(nullptr))) Type;
};

// Simple helper function for singleton types.
template <typename T>
T& GetSingletonInstance()
{
    static T s_instance;
    return s_instance;
}

// Helper function for unsigned int division, e.g. 5 / 3 = 2;
template <typename T, typename = typename std::enable_if_t<std::is_unsigned_v<T>>>
constexpr std::uint32_t CeilUintDivision(T x, T y)
{
    if (x == 0)
        return 0;
    return 1 + ((x - 1) / y);
}
} // namespace JUtils