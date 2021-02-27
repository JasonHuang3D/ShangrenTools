//
// Author: Jason Huang(jasonhuang1988@gmail.com) 2021
//
#pragma once

#include <string>
#include <type_traits>
#include <vector>

namespace GearCalc
{
struct UnitScale
{
    enum Values : std::uint64_t
    {
        k_10K  = 10000,
        k_100M = 100000000
    };

    template <typename T>
    static constexpr char* GetUnitStr(T value)
    {
        switch (value)
        {
        case k_10K:
            return u8"万";
        case k_100M:
            return u8"亿";
        default:
            return "";
        }
    }

    template <typename T>
    static constexpr bool IsValid(T value)
    {
        switch (value)
        {
        case k_10K:
        case k_100M:
            return true;
        default:
            return false;
        }
    }
};

// No polymorphism is required
struct IndividualBuff
{
    std::uint32_t geLi_add = 0;
    std::uint32_t geNian_add = 0;
    std::uint32_t geFu_add = 0;
    double geLi_percent = 0.0;
    double geNian_percent = 0.0;
    double geFu_percent = 0.0;

};
struct GlobalBuff
{
    std::uint32_t zongLi_add = 0;
    std::uint32_t zongNian_add = 0;
    std::uint32_t zongFu_add = 0;
    double zongLi_percent = 0.0;
    double zongNian_percent = 0.0;
    double zongFu_percent = 0.0;
};
struct ChanyeBuff
{
    std::uint32_t chanJing_add = 0;
    std::uint32_t chanNeng_add = 0;
    double chanjing_percent = 0.0;
    double chanNeng_percent = 0.0;
};

struct GearData
{
    GearData() {};
    // Enbale move
    GearData(GearData&&) noexcept = default;
    GearData& operator=(GearData&&) = default;
    // Disable copy
    GearData(const GearData&) = delete;
    GearData& operator=(const GearData&) = delete;

public:
    std::string name;

    IndividualBuff individualBuff;
    GlobalBuff globalBuff;
    ChanyeBuff chanyeBuff;
};

class GearFileData
{
public:
    static bool ReadFromJsonFile(const char* fileName, std::string& errorStr, GearFileData& outGearFileData);

    GearFileData() {};
    // Enbale move
    GearFileData(GearFileData&&) noexcept = default;
    GearFileData& operator=(GearFileData&&) = default;
    // Disable copy
    GearFileData(const GearFileData&) = delete;
    GearFileData& operator=(const GearFileData&) = delete;
private:
    std::uint32_t m_maxNumEquip = 0;
    std::vector<GearData> m_gearsDataVec;
};


} // namespace GearCalc