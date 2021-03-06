//
// Author: Jason Huang(jasonhuang1988@gmail.com) 2021
//

#include "pch.h"

#include "Utils.h"

namespace JUtils
{
Timer::Timer()
{
    Reset();
}
void Timer::Reset()
{
    m_t0 = m_clock.now();
}
double Timer::DurationInSec()
{
    return Elapsed() * 0.001;
}
double Timer::DurationInMsec()
{
    return Elapsed();
}
double Timer::Elapsed() const
{
    std::chrono::time_point<std::chrono::high_resolution_clock> t1 = m_clock.now();
    std::chrono::milliseconds dt = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - m_t0);
    return static_cast<double>(dt.count());
}


// Helper to check if char ptr is empty
bool isCharPtrEmpty(const char* str)
{
    if (str == nullptr)
        return true;
    if (std::strlen(str) == 0 || !str[0])
        return true;
    return false;
}


std::size_t GetNumOfSelectionComb(std::size_t numElelment, std::size_t numSelect)
{
    if (numSelect > numElelment) return 0;
    if (numSelect * 2 > numElelment) numSelect = numElelment - numSelect;
    if (numSelect == 0) return 1;

    std::size_t result = numElelment;
    for (std::size_t i = 2; i <= numSelect; ++i) {
        result *= (numElelment - i + 1);
        result /= i;
    }
    return result;
}

} // namespace JUtils