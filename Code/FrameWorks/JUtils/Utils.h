//
// Author: Jason Huang(jasonhuang1988@gmail.com) 2021
//
#pragma once

#include <array>
#include <chrono>
#include <functional>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

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

struct StringFormat
{
private:
    template <typename TStringStream>
    static void FormatStr(TStringStream& ss)
    {
    }
    template <typename TStringStream, typename T>
    static void FormatStr(TStringStream& ss, const T& arg)
    {
        ss << arg;
    }
    template <typename TStringStream, typename THead, typename... TRest>
    static void FormatStr(TStringStream& ss, const THead& headArg, const TRest&... restArgs)
    {
        FormatStr(ss, headArg);
        FormatStr(ss, restArgs...);
    }

public:
    template <typename... TRest>
    static std::string FormatString(const TRest&... args)
    {
        std::stringstream ss;
        FormatStr(ss, args...);
        return ss.str();
    }
};

template <typename TypeFloat, typename TypeInt>
TypeFloat FormatIntToFloat(TypeInt value, TypeInt scale)
{
    return static_cast<TypeFloat>(value) / scale;
}

// Helper to check if char ptr is empty
bool isCharPtrEmpty(const char* str);

// Invalid value of every possible bits
constexpr std::uint32_t GetInvalidValue(std::uint32_t numBits)
{
    return static_cast<std::uint32_t>((1ULL << numBits) - 1);
}

// Helper function to compute and combine hash value
template <typename T>
void HashCombine(std::size_t& seed, const T& value)
{
    seed ^= std::hash<T>()(value) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

template <typename TypeFirstArg, typename... TypeRestArgs>
void HashCombine(std::size_t& seed, const TypeFirstArg& firstArg, const TypeRestArgs&... restArgs)
{
    HashCombine(seed, firstArg);
    HashCombine(seed, restArgs...);
}

template <typename... TyepArgs>
std::size_t ComputeHash(const TyepArgs&... args)
{
    std::size_t seed = 0;
    HashCombine(seed, args...);
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
template<typename T>
T& GetSingletonInstance()
{
    static T s_instance;
    return s_instance;
}

} // namespace JUtils