//
// Author: Jason Huang(jasonhuang1988@gmail.com) 2021
//
#pragma once

#include <array>
#include <cmath>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "JUtils/Utils.h"

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
        else if (strValue == u8"亿")
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

enum class XianRenPropertyMask : std::uint32_t
{
    None = 0,

    Li   = 1 << 0,
    Nian = 1 << 1,
    Fu   = 1 << 2,

    All = Li | Nian | Fu
};
DEFINE_FLAG_ENUM_OPERATORS(XianRenPropertyMask);

struct ChanyeFieldCategory
{
    enum Enum : std::uint8_t
    {
        ChanJing = 0,
        ChanNeng,

        NumCategory,
        None
    };
};
enum class ChanyePropertyMask : std::uint8_t
{
    None = 0,

    ChanJing = 1 << 0,
    ChanNeng = 1 << 1,

    All = ChanJing | ChanNeng
};
DEFINE_FLAG_ENUM_OPERATORS(ChanyePropertyMask);

template <ChanyeFieldCategory::Enum ChanyeFieldCat>
constexpr ChanyePropertyMask ChanyeFieldCatToMask()
{
    if constexpr (ChanyeFieldCat == ChanyeFieldCategory::ChanJing)
        return ChanyePropertyMask::ChanJing;
    else if constexpr (ChanyeFieldCat == ChanyeFieldCategory::ChanNeng)
        return ChanyePropertyMask::ChanNeng;
    else
    {
        static_assert(false, "Invalid ChanyeFieldCat provided!");
        return ChanyePropertyMask::None;
    }
};

struct XianRenPropBuff
{
    static constexpr auto k_individual_prefix = u8"各";
    static constexpr auto k_global_prefix     = u8"总";

    template <XianRenPropertyMask PropMask = XianRenPropertyMask::All>
    void IncreaseBy(const XianRenPropBuff& right)
    {
        if constexpr ((PropMask & XianRenPropertyMask::Li) != XianRenPropertyMask::None)
        {
            li_add += right.li_add;
            li_percent += right.li_percent;
        }
        if constexpr ((PropMask & XianRenPropertyMask::Nian) != XianRenPropertyMask::None)
        {
            nian_add += right.nian_add;
            nian_percent += right.nian_percent;
        }

        if constexpr ((PropMask & XianRenPropertyMask::Fu) != XianRenPropertyMask::None)
        {
            fu_add += right.fu_add;
            fu_percent += right.fu_percent;
        }
    }
    template <XianRenPropertyMask PropMask = XianRenPropertyMask::All>
    XianRenPropBuff Add(const XianRenPropBuff& right)
    {
        XianRenPropBuff out(*this);
        out.IncreaseBy<PropMask>(right);
        return out;
    }

    void Reset();
    std::string ToString(const char* prefix = k_individual_prefix) const;

    std::uint32_t li_add   = 0;
    std::uint32_t nian_add = 0;
    std::uint32_t fu_add   = 0;
    double li_percent      = 0.0;
    double nian_percent    = 0.0;
    double fu_percent      = 0.0;
};

struct ChanyePropBuff
{
    template <ChanyePropertyMask PropMask = ChanyePropertyMask::All>
    void IncreaseBy(const ChanyePropBuff& right)
    {
        if constexpr ((PropMask & ChanyePropertyMask::ChanJing) != ChanyePropertyMask::None)
        {
            chanJing_add += right.chanJing_add;
            chanJing_percent += right.chanJing_percent;
        }
        if constexpr ((PropMask & ChanyePropertyMask::ChanNeng) != ChanyePropertyMask::None)
        {
            chanNeng_add += right.chanNeng_add;
            chanNeng_percent += right.chanNeng_percent;
        }
    }
    template <ChanyePropertyMask PropMask = ChanyePropertyMask::All>
    ChanyePropBuff Add(const ChanyePropBuff& right) const
    {
        ChanyePropBuff out(*this);
        out.IncreaseBy<PropMask>(right);
        return out;
    }

    void Reset();
    std::string ToString() const;

    std::uint32_t chanJing_add = 0;
    std::uint32_t chanNeng_add = 0;
    double chanJing_percent    = 0.0;
    double chanNeng_percent    = 0.0;
};

struct XianRenProp
{
    template <XianRenPropertyMask PropMask = XianRenPropertyMask::All>
    void AddBuff(const XianRenPropBuff& buff)
    {
        if constexpr ((PropMask & XianRenPropertyMask::Li) != XianRenPropertyMask::None)
        {
            li += buff.li_add;
        }
        if constexpr ((PropMask & XianRenPropertyMask::Nian) != XianRenPropertyMask::None)
        {
            nian += buff.nian_add;
        }
        if constexpr ((PropMask & XianRenPropertyMask::Fu) != XianRenPropertyMask::None)
        {
            fu += buff.fu_add;
        }
    }
    template <XianRenPropertyMask PropMask = XianRenPropertyMask::All>
    void MultiplyBuff(const XianRenPropBuff& buff)
    {
        if constexpr ((PropMask & XianRenPropertyMask::Li) != XianRenPropertyMask::None)
        {
            li += std::trunc(li * buff.li_percent * 0.01);
        }
        if constexpr ((PropMask & XianRenPropertyMask::Nian) != XianRenPropertyMask::None)
        {
            nian += std::trunc(nian * buff.nian_percent * 0.01);
        }
        if constexpr ((PropMask & XianRenPropertyMask::Fu) != XianRenPropertyMask::None)
        {
            fu += std::trunc(fu * buff.fu_percent * 0.01);
        }
    }
    template <XianRenPropertyMask PropMask = XianRenPropertyMask::All>
    void ApplyBuff(const XianRenPropBuff& buff)
    {
        // Multiply and then add
        MultiplyBuff<PropMask>(buff);
        AddBuff<PropMask>(buff);
    }
    template <XianRenPropertyMask PropMask = XianRenPropertyMask::All>
    void IncreaseBy(const XianRenProp& right)
    {
        if constexpr ((PropMask & XianRenPropertyMask::Li) != XianRenPropertyMask::None)
        {
            li += right.li;
        }
        if constexpr ((PropMask & XianRenPropertyMask::Nian) != XianRenPropertyMask::None)
        {
            nian += right.nian;
        }
        if constexpr ((PropMask & XianRenPropertyMask::Fu) != XianRenPropertyMask::None)
        {
            fu += right.fu;
        }
    }
    template <XianRenPropertyMask PropMask = XianRenPropertyMask::All>
    double GetSum() const
    {
        double out = 0.0;
        if constexpr ((PropMask & XianRenPropertyMask::Li) != XianRenPropertyMask::None)
        {
            out += li;
        }
        if constexpr ((PropMask & XianRenPropertyMask::Nian) != XianRenPropertyMask::None)
        {
            out += nian;
        }

        if constexpr ((PropMask & XianRenPropertyMask::Fu) != XianRenPropertyMask::None)
        {
            out += fu;
        }

        return out;
    }

    void Reset();
    // Apply unit scale
    XianRenProp& operator*=(UnitScale::Enum unitScale);
    std::string ToString() const;

public:
    double li   = 0.0;
    double nian = 0.0;
    double fu   = 0.0;
};

template <ChanyeFieldCategory::Enum ChanyeCat,
    typename = std::enable_if_t<ChanyeCat == ChanyeFieldCategory::ChanJing ||
        ChanyeCat == ChanyeFieldCategory::ChanNeng>>
struct ChanyeProp
{
    void AddBuff(const ChanyePropBuff& buff)
    {
        if constexpr (ChanyeCat == ChanyeFieldCategory::ChanJing)
        {
            output += buff.chanJing_add;
        }
        else
        {
            output += buff.chanNeng_add;
        }
    }
    void MultiplyBuff(const ChanyePropBuff& buff)
    {
        if constexpr (ChanyeCat == ChanyeFieldCategory::ChanJing)
        {
            output += std::trunc(output * buff.chanJing_percent * 0.01);
        }
        else
        {
            output += std::trunc(output * buff.chanNeng_percent * 0.01);
        }
    }
    void ApplyBuff(const ChanyePropBuff& buff)
    {
        // Multiply and then add
        MultiplyBuff(buff);
        AddBuff(buff);
    }
    ChanyeProp<ChanyeCat>& operator+=(const ChanyeProp<ChanyeCat>& right)
    {
        output += right.output;
        return *this;
    }

    std::string ToString()
    {
        if constexpr (ChanyeCat == ChanyeFieldCategory::ChanJing)
        {
            return FormatString(u8"产晶: ", output, "\n");
        }
        else
        {
            return FormatString(u8"产能: ", output, "\n");
        }
    }

public:
    double output = 0.0;
};
struct GearData
{
    std::string name;

    XianRenPropBuff individualBuff;
    XianRenPropBuff globalBuff;
    ChanyePropBuff chanyeBuff;
};

struct TouXiangData
{
    std::string name;

    XianRenPropBuff individualBuff;
};

struct XianlvData
{
    std::string name;

    XianRenPropBuff fushi_buff;

    XianRenPropBuff tianfu_individual_buff;
    XianRenPropBuff tianfu_global_buff;
    ChanyePropBuff tianfu_chanye_buff;
};

struct XianZhiData
{
    std::string name;

    XianRenPropBuff individual_buff;

    std::string xianlvName;
    const XianlvData* pXianlv = nullptr;
};

struct XianRenData
{
    std::string name;

    XianRenProp baseProp;

    std::string xianZhiName;
    const XianZhiData* pXianzhi = nullptr;
};

// Chanye Map
struct ChanyeFieldInfo
{
    ChanyeFieldInfo() {};

    template <typename TypeFunction>
    ChanyeFieldInfo(std::uint32_t index, TypeFunction&& opFunction) :
        index(index), opFunction(std::forward<TypeFunction>(opFunction))
    {
    }
    std::uint32_t index = JUtils::GetInvalidValue(32);
    std::function<double(const XianRenProp&)> opFunction;
};

struct ChanyeFieldData
{
    std::string name;

    // Chanye level buff + Zaohua buff
    double selfBuff = 0.0;

    ChanyeFieldInfo chanyeInfo;
};

class XianQiFileData
{
public:
    static bool ReadFromJsonFile(
        const char* fileName, std::string& errorStr, XianQiFileData& outXianQiFileData);

    XianQiFileData() {};
    // Enbale move
    XianQiFileData(XianQiFileData&&) noexcept = default;
    XianQiFileData& operator=(XianQiFileData&&) = default;
    // Disable copy
    XianQiFileData(const XianQiFileData&) = delete;
    XianQiFileData& operator=(const XianQiFileData&) = delete;

    void Reset();

    //const std::vector<GearData>& GetGearDataVec() const { return m_gearsDataVec; }
    std::unordered_map<std::string, const GearData* const> const GetLoopUpGears()
    {
        return m_loopUpGears;
    }

    const std::vector<const GearData*>& GetCalcGearsVec() const { return m_calcGearsVec; }
    std::uint32_t GetMaxNumEquip() const { return m_maxNumEquip; }

private:
    std::vector<GearData> m_gearsDataVec;
    std::unordered_map<std::string, const GearData* const> m_loopUpGears;

    std::uint32_t m_maxNumEquip = 0;

    std::vector<const GearData*> m_calcGearsVec;
};

class XianJieFileData
{
public:
    static bool ReadFromJsonFile(
        const char* fileName, std::string& errorStr, XianJieFileData& outXianJieFileData);

    XianJieFileData() {};
    // Enbale move
    XianJieFileData(XianJieFileData&&) noexcept = default;
    XianJieFileData& operator=(XianJieFileData&&) = default;
    // Disable copy
    XianJieFileData(const XianJieFileData&) = delete;
    XianJieFileData& operator=(const XianJieFileData&) = delete;

    void Reset();

    const std::vector<TouXiangData>& GetTouXiangDataVec() const { return m_touXiangDataVec; }
    const std::vector<XianlvData>& GetXianLvDataVec() const { return m_xianLvDataVec; }
    const std::vector<XianZhiData>& GetXianZhiDataVec() const { return m_xianZhiDataVec; }
    const std::vector<XianRenData>& GetXianRenDataVec() const { return m_xianRenDataVec; }
    const std::vector<const XianRenData*>& GetCalcXianRenDataVec() const
    {
        return m_calcXianRenDataVec;
    }

    template <ChanyeFieldCategory::Enum ChanyeCat,
        typename = std::enable_if_t<ChanyeCat == ChanyeFieldCategory::ChanJing ||
            ChanyeCat == ChanyeFieldCategory::ChanNeng>>
    const std::vector<ChanyeFieldData>& GetChanYeFieldDataVec() const
    {
        if constexpr (ChanyeCat == ChanyeFieldCategory::ChanJing)
            return m_chanJingFiledVec;
        else
            return m_chanNengFiledVec;
    }

private:
    std::vector<TouXiangData> m_touXiangDataVec;
    std::vector<XianlvData> m_xianLvDataVec;
    std::vector<XianZhiData> m_xianZhiDataVec;
    std::vector<XianRenData> m_xianRenDataVec;
    std::vector<ChanyeFieldData> m_chanJingFiledVec;
    std::vector<ChanyeFieldData> m_chanNengFiledVec;
    double m_chanyeInterval = 0.0;

    std::unordered_map<std::string, const XianlvData* const> m_loopUpXainLv;
    std::unordered_map<std::string, const XianZhiData* const> m_loopUpXainZhi;
    std::unordered_map<std::string, const XianRenData* const> m_loopUpXainRen;
    std::vector<const XianRenData*> m_calcXianRenDataVec;

    UnitScale::Enum m_unitScale = UnitScale::NotValid;
};

} // namespace GearCalc