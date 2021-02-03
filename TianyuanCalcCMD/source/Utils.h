//
// Author: Jason Huang(jasonhuang1988@gmail.com) 2021
//
#pragma once

#include <string>
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

template <typename TypeFloat, typename TypeInt>
TypeFloat FormatIntToFloat(TypeInt value, TypeInt scale)
{
    return static_cast<TypeFloat>(value) / scale;
}

} // namespace JUtils