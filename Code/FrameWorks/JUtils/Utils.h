//
// Author: Jason Huang(jasonhuang1988@gmail.com) 2021
//
#pragma once

#include <array>
#include <chrono>
#include <functional>
#include <iomanip>
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

template <typename... TRest>
static std::string FormatString(TRest&&... args)
{
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2);
    (ss << ... << std::forward<TRest>(args));
    return ss.str();
}

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

struct PickIndex
{
    // Index bit mask
    static constexpr std::uint32_t k_maxInputSize = 64;
    static constexpr auto k_inputIndexBitMask =
        ArrayHelper::CreateArrayWithInitValues<std::uint64_t, k_maxInputSize>(ArrayHelper::BitMaskGenerator);

    // Config max picked table by removing the bits from left most to match number of inputs
    template <typename T, typename = typename std::enable_if_t<std::is_unsigned_v<T>>>
    static constexpr std::uint64_t GetMaxPickedIndices(T inputSize)
    {
        assert(k_maxInputSize > inputSize && inputSize > 0);

        auto out = std::numeric_limits<std::uint64_t>::max();

        auto numBitsToRemove = k_maxInputSize - inputSize;
        out >>= numBitsToRemove;
        return out;
    }
};
namespace SelectCombination
{
// Helper function to get the total size of selection combs
inline constexpr std::size_t GetNumOfSelectionComb(std::size_t numElelment, std::size_t numSelect)
{
    if (numSelect > numElelment)
        return 0;
    if (numSelect * 2 > numElelment)
        numSelect = numElelment - numSelect;
    if (numSelect == 0)
        return 1;

    std::size_t result = numElelment;
    for (std::size_t i = 2; i <= numSelect; ++i)
    {
        result *= (numElelment - i + 1);
        result /= i;
    }
    return result;
}

// Single thread solution
std::size_t RunSingleThread(std::size_t numElelment, std::size_t numSelect,
    std::function<void(const std::vector<std::size_t>&, std::size_t)> callBack);

// Multi thread solution
std::size_t RunMultiThread(std::size_t numElelment, std::size_t numSelect,
    std::function<void(const std::vector<std::size_t>&, std::size_t)> callBack);


struct SelectGroupComb
{
    template <typename TypeCallBack>
    SelectGroupComb(std::uint32_t numInputs, std::uint32_t numGroups, std::uint32_t numPerGroup,
        TypeCallBack&& callBack) :
        m_numInputs(numInputs),
        m_numGroups(numGroups),
        m_numPerGroup(numPerGroup),
        m_maxSelectInputMask(PickIndex::GetMaxPickedIndices(numInputs)),
        m_callBack(std::forward<TypeCallBack>(callBack))
    {
    }

    void Run(bool useMultiThread = true);
private:


    const std::uint32_t m_numInputs;
    const std::uint32_t m_numGroups;
    const std::uint32_t m_numPerGroup;
    const std::uint64_t m_maxSelectInputMask;

    std::function<void(const std::vector<std::uint64_t>&)> m_callBack;
};
} // namespace SelectCombination




} // namespace JUtils