//
// Author: Jason Huang(jasonhuang1988@gmail.com) 2021
//
#pragma once

#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

namespace GearCalc
{
struct UnitScale
{
    enum Enum : std::uint64_t
    {
        NotValid = 0,

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
    static constexpr Enum GetEnumFromStr(const T& strValue)
    {
        if (strValue == u8"万")
        {
            return k_10K;
        }
        else if(strValue == u8"亿")
        {
            return k_100M;
        }

        return Enum::NotValid;
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
    std::uint32_t geLi_add   = 0;
    std::uint32_t geNian_add = 0;
    std::uint32_t geFu_add   = 0;
    double geLi_percent      = 0.0;
    double geNian_percent    = 0.0;
    double geFu_percent      = 0.0;
};
struct GlobalBuff
{
    std::uint32_t zongLi_add   = 0;
    std::uint32_t zongNian_add = 0;
    std::uint32_t zongFu_add   = 0;
    double zongLi_percent      = 0.0;
    double zongNian_percent    = 0.0;
    double zongFu_percent      = 0.0;
};
struct ChanyeBuff
{
    std::uint32_t chanJing_add = 0;
    std::uint32_t chanNeng_add = 0;
    double chanjing_percent    = 0.0;
    double chanNeng_percent    = 0.0;
};

struct XianrenProp
{
    double li = 0.0;
    double nian = 0.0;
    double fu = 0.0;
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

struct TouXiangData
{
    TouXiangData() {};
    // Enbale move
    TouXiangData(TouXiangData&&) noexcept = default;
    TouXiangData& operator=(TouXiangData&&) = default;
    // Disable copy
    TouXiangData(const TouXiangData&) = delete;
    TouXiangData& operator=(const TouXiangData&) = delete;

public:
    std::string name;

    IndividualBuff individualBuff;
};

struct XianlvData
{
    XianlvData() {};
    // Enbale move
    XianlvData(XianlvData&&) noexcept = default;
    XianlvData& operator=(XianlvData&&) = default;
    // Disable copy
    XianlvData(const XianlvData&) = delete;
    XianlvData& operator=(const XianlvData&) = delete;

public:
    std::string name;

    IndividualBuff fushi_buff;

    IndividualBuff tianfu_individual_buff;
    GlobalBuff tianfu_global_buff;
    ChanyeBuff tianfu_chanye_buff;
};

struct XianZhiData
{
    XianZhiData() {};
    // Enbale move
    XianZhiData(XianZhiData&&) noexcept = default;
    XianZhiData& operator=(XianZhiData&&) = default;
    // Disable copy
    XianZhiData(const XianZhiData&) = delete;
    XianZhiData& operator=(const XianZhiData&) = delete;

public:
    std::string name;

    IndividualBuff individual_buff;

    std::string xianlvName;
    const XianlvData* pXianlv = nullptr;
};

struct XianRenData
{
    XianRenData() {};
    // Enbale move
    XianRenData(XianRenData&&) noexcept = default;
    XianRenData& operator=(XianRenData&&) = default;
    // Disable copy
    XianRenData(const XianRenData&) = delete;
    XianRenData& operator=(const XianRenData&) = delete;
public:
    std::string name;

    XianrenProp baseProp;

    std::string xianZhiName;
    const XianZhiData* pXianzhi = nullptr;
};

class XianqiFileData
{
public:
    static bool ReadFromJsonFile(
        const char* fileName, std::string& errorStr, XianqiFileData& outGearFileData);

    XianqiFileData() {};
    // Enbale move
    XianqiFileData(XianqiFileData&&) noexcept = default;
    XianqiFileData& operator=(XianqiFileData&&) = default;
    // Disable copy
    XianqiFileData(const XianqiFileData&) = delete;
    XianqiFileData& operator=(const XianqiFileData&) = delete;

    void Reset();
private:
    std::vector<GearData> m_gearsDataVec;
    std::uint32_t m_maxNumEquip = 0;
};

class XianjieFileData
{
public:
    static bool ReadFromJsonFile(
        const char* fileName, std::string& errorStr, XianjieFileData& outXianjieFileData);

    XianjieFileData() {};
    // Enbale move
    XianjieFileData(XianjieFileData&&) noexcept = default;
    XianjieFileData& operator=(XianjieFileData&&) = default;
    // Disable copy
    XianjieFileData(const XianjieFileData&) = delete;
    XianjieFileData& operator=(const XianjieFileData&) = delete;

    void Reset();
private:
    std::vector<TouXiangData> m_touxiangDataVec;
    std::vector<XianlvData> m_xianLvDataVec;
    std::vector<XianZhiData> m_xianZhiDataVec;
    std::vector<XianRenData> m_xianrenDataVec;

    std::unordered_map<std::string, const XianlvData* const> m_loopUpXainlv;
    std::unordered_map<std::string, const XianZhiData* const> m_loopUpXainZhi;
    std::unordered_map<std::string, const XianRenData* const> m_loopUpXainRen;

    UnitScale::Enum m_xianrenBaseUnitScale = UnitScale::NotValid;
};

} // namespace GearCalc