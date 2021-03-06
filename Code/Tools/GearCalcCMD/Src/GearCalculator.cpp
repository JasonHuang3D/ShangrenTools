//
// Author: Jason Huang(jasonhuang1988@gmail.com) 2021
//

#include "JUtils/pch.h"

#include "GearCalculator.h"

#include "JUtils/Utils.h"

using namespace JUtils;
using namespace GearCalc;

namespace
{
namespace GameAlgorithms
{
template <PropertyMask PropMask = PropertyMask::All>
void GetSumOfXianjieIndividualBuffs(const XianJieFileData& xianJieFileData, XianRenPropBuff& out)
{
    // Accumulate touxiang.
    for (const auto& data : xianJieFileData.GetTouXiangDataVec())
        out.IncreaseBy<PropMask>(data.individualBuff);

    // Accumulate xianlv tianfu.
    for (const auto& data : xianJieFileData.GetXianLvDataVec())
        out.IncreaseBy<PropMask>(data.tianfu_individual_buff);
}
template <PropertyMask PropMask = PropertyMask::All>
void GetSumOfXianjieGlobalBuffs(const XianJieFileData& xianJieFileData, XianRenPropBuff& out)
{
    // Accumulate xianlv tianfu.
    for (const auto& data : xianJieFileData.GetXianLvDataVec())
        out.IncreaseBy<PropMask>(data.tianfu_global_buff);
}
template <PropertyMask PropMask = PropertyMask::All>
XianRenPropBuff GetXianRenSelfBuff(const XianRenData& target)
{
    XianRenPropBuff out;
    if (target.pXianzhi != nullptr)
    {
        out.IncreaseBy<PropMask>(target.pXianzhi->individual_buff);
        if (target.pXianzhi->pXianlv != nullptr)
        {
            out.IncreaseBy<PropMask>(target.pXianzhi->pXianlv->fushi_buff);
        }
    }
    return out;
}

} // namespace GameAlgorithms

struct GearSelectorBase
{
    virtual ~GearSelectorBase() {};
    virtual bool Run(std::string& errorStr) = 0;
};

template <Calculator::Solution SolutionType>
struct GearSelector : public GearSelectorBase
{
public:
    using ThisType = GearSelector<SolutionType>;
    static std::unique_ptr<ThisType> CreateInstance(
        const XianJieFileData& xianJieFileData, const XianQiFileData& xianQiFileData)
    {
        return std::make_unique<ThisType>(xianJieFileData, xianQiFileData);
    }

    GearSelector(const XianJieFileData& xianJieFileData, const XianQiFileData& xianQiFileData) :
        m_xianJieFileData(xianJieFileData), m_xianQiFileData(xianQiFileData)
    {
    }

    GearSelector(const GearSelector&) = delete;
    GearSelector& operator=(const GearSelector&) = delete;

    bool Run(std::string& errorStr) override
    {
        // Init global buffs
        XianRenPropBuff xianJie_individual_buff;
        GameAlgorithms::GetSumOfXianjieIndividualBuffs(m_xianJieFileData, xianJie_individual_buff);
        XianRenPropBuff xianJie_global_Buff;
        GameAlgorithms::GetSumOfXianjieGlobalBuffs(m_xianJieFileData, xianJie_global_Buff);

        const auto& calcXianRenVec = m_xianJieFileData.GetCalcXianRenDataVec();
        const auto& xianQiVec      = m_xianQiFileData.GetGearDataVec();
        const auto xianRenVecSize  = calcXianRenVec.size();
        const auto xianQiVecSize   = xianQiVec.size();
        const auto maxEquiptNum    = m_xianQiFileData.GetMaxNumEquip();

        if (maxEquiptNum < 1)
        {
            errorStr += u8"最小可装备数不能小于1!\n";
            assert(false);
            return false;
        }

        // Init xian ren self buff vec
        std::vector<XianRenPropBuff> xianRenSelfBuffVec;
        xianRenSelfBuffVec.reserve(calcXianRenVec.size());
        for (const auto* pXianRenData : calcXianRenVec)
        {
            xianRenSelfBuffVec.emplace_back(GameAlgorithms::GetXianRenSelfBuff(*pXianRenData));
        }

        using CombVecType = std::vector<std::size_t>;
        CombVecType bestComb;

        XianRenProp best_xianren_prop_sum;
        double best_xianren_individual_sum_num = 0.0;
        double best_global_li_nian_sum_num     = 0.0;

        std::cout << u8"需计算仙人数: " << xianRenVecSize << std::endl;
        std::cout << u8"需计算仙器数: " << xianQiVecSize << std::endl;
        std::cout << u8"可装备个数: " << maxEquiptNum << std::endl;

        auto combCallBack = [&](const CombVecType& combIndexVec, std::size_t indexOfComb) -> void {
            if constexpr (SolutionType == Calculator::Solution::BestXianRenSumProp)
            {
                // This type of solution we should calculate all properties.
                constexpr auto kPropMask = PropertyMask::All;

                // Accomulate individual buff of each Xian Qi
                XianRenPropBuff xianQi_individual_buff;
                for (auto& combIndex : combIndexVec)
                {
                    xianQi_individual_buff.IncreaseBy<kPropMask>(
                        xianQiVec[combIndex].individualBuff);
                }
                // Accomulate all individual buffs for Xian Ren.
                auto all_individual_buffs =
                    xianQi_individual_buff.Add<kPropMask>(xianJie_individual_buff);

                // Apply all individual buffs to each Xian Ren
                XianRenProp sumXianRenProp;
                for (int i = 0; i < xianRenVecSize; ++i)
                {
                    const auto* pXianRenData = calcXianRenVec[i];

                    // All buffs.
                    auto allBuffs = all_individual_buffs.Add<kPropMask>(xianRenSelfBuffVec[i]);

                    // Apply all buffs to xian ren.
                    auto xianRenPropCopy = pXianRenData->baseProp;
                    xianRenPropCopy.ApplyBuff<kPropMask>(allBuffs);

                    // Accomulate the sum.
                    sumXianRenProp.IncreaseBy<kPropMask>(xianRenPropCopy);
                }

                // Select the best one
                auto sum = sumXianRenProp.GetSum();
                if (sum > best_xianren_individual_sum_num)
                {
                    bestComb                        = combIndexVec;
                    best_xianren_prop_sum           = sumXianRenProp;
                    best_xianren_individual_sum_num = sum;
                }
            }
            else if constexpr (SolutionType == Calculator::Solution::BestGlobalSumLiNian)
            {
                // This type of solution we should only calculate li and nian.
                constexpr auto kPropMask = PropertyMask::Li | PropertyMask::Nian;

                // Accomulate individual & global buffs of each Xian Qi
                XianRenPropBuff xianQi_individual_buff;
                XianRenPropBuff xianQi_global_buff;
                for (auto& combIndex : combIndexVec)
                {
                    const auto& xianQi = xianQiVec[combIndex];

                    xianQi_individual_buff.IncreaseBy<kPropMask>(xianQi.individualBuff);
                    xianQi_global_buff.IncreaseBy<kPropMask>(xianQi.globalBuff);
                }

                // Accomulate all individual & global buffs.
                auto all_individual_buffs =
                    xianQi_individual_buff.Add<kPropMask>(xianJie_individual_buff);
                auto all_global_buffs = xianQi_global_buff.Add<kPropMask>(xianJie_global_Buff);

                // Apply all individual buffs to each Xian Ren
                XianRenProp sumXianRenProp;
                for (int i = 0; i < xianRenVecSize; ++i)
                {
                    const auto* pXianRenData = calcXianRenVec[i];

                    // All buffs.
                    auto allBuffs = all_individual_buffs.Add<kPropMask>(xianRenSelfBuffVec[i]);

                    // Apply all buffs to xian ren.
                    auto xianRenPropCopy = pXianRenData->baseProp;
                    xianRenPropCopy.ApplyBuff<kPropMask>(allBuffs);

                    // Accomulate the sum.
                    sumXianRenProp.IncreaseBy<kPropMask>(xianRenPropCopy);
                }

                // Apply all global buffs to final sum of each xianren prop
                sumXianRenProp.ApplyBuff<kPropMask>(all_global_buffs);

                // Select the best one
                auto sum = sumXianRenProp.li + sumXianRenProp.nian;
                if (sum > best_global_li_nian_sum_num)
                {
                    bestComb                    = combIndexVec;
                    best_xianren_prop_sum       = sumXianRenProp;
                    best_global_li_nian_sum_num = sum;
                }
            }
            else
            {
                assert(false);
            }
        };

        SelectCombination(xianQiVecSize, maxEquiptNum, combCallBack);

        std::vector<const GearData*> selectedGears;
        selectedGears.reserve(maxEquiptNum);
        for (auto& combIndex : bestComb)
        {
            selectedGears.emplace_back(&xianQiVec[combIndex]);
        }

        std::cout << u8"挑选仙器: " << std::endl;
        XianRenPropBuff xianQi_individual_buff_sum;
        XianRenPropBuff xianQi_global_buff_sum;
        for (const auto* pGear : selectedGears)
        {
            xianQi_individual_buff_sum.IncreaseBy(pGear->individualBuff);
            xianQi_global_buff_sum .IncreaseBy(pGear->globalBuff);
            std::cout << pGear->name << std::endl;
        }

        std::cout << std::endl;

        std::cout << u8"仙器属性总和: \n"
                  << xianQi_individual_buff_sum.ToString(XianRenPropBuff::k_individual_prefix)
                  << xianQi_global_buff_sum.ToString(XianRenPropBuff::k_global_prefix) << std::endl;
        std::cout << u8"仙人属性总和: \n" << best_xianren_prop_sum.ToString() << std::endl;

        return true;
    }

private:
    const XianJieFileData& m_xianJieFileData;
    const XianQiFileData& m_xianQiFileData;
};

} // namespace

namespace GearCalc
{
bool Calculator::Init(const char* xianJieFile, const char* xianQiFile, std::string& errorStr)
{

    if (!XianJieFileData::ReadFromJsonFile(xianJieFile, errorStr, m_xianJieFileData))
    {
        errorStr += FormatString(u8"加载文件: ", xianJieFile, u8" 失败,请检查文件!\n");
        return false;
    }

    if (!XianQiFileData::ReadFromJsonFile(xianQiFile, errorStr, m_xianQiFileData))
    {
        errorStr += FormatString(u8"加载文件: ", xianQiFile, u8" 失败,请检查文件!\n");
        return false;
    }

    m_isInitialized = true;
    return true;
}

bool Calculator::Run(std::string& errorStr, Solution solution)
{
    assert(m_isInitialized);
    m_isInitialized = false;

    std::unique_ptr<GearSelectorBase> pSelector = nullptr;
    switch (solution)
    {
    case Solution::BestXianRenSumProp:
        std::cout << "BestXianRenSumProp" << std::endl;
        pSelector = GearSelector<Solution::BestXianRenSumProp>::CreateInstance(
            m_xianJieFileData, m_xianQiFileData);
        break;
    case Solution::BestGlobalSumLiNian:
        std::cout << "BestGlobalSumLiNian" << std::endl;
        pSelector = GearSelector<Solution::BestGlobalSumLiNian>::CreateInstance(
            m_xianJieFileData, m_xianQiFileData);
        break;
    case Solution::Test:
    {
        const auto& calcXianRenVec = m_xianJieFileData.GetCalcXianRenDataVec();
        const auto xianRenVecSize  = calcXianRenVec.size();

        constexpr const char* selectGears[] = { u8"造化圣塔-福念", u8"造化圣塔-福力",
            u8"造化玄天塔-01", u8"造化玄天塔-02", u8"造化玄天塔-03", u8"造化玄天塔-04",
            u8"造化玄天塔-05" };
        constexpr auto gearSize             = std::size(selectGears);

        std::vector<const GearData*> gearsVec;
        gearsVec.reserve(gearSize);

        const auto& gearsLoopUp = m_xianQiFileData.GetLoopUpGears();
        for (const auto* gearName : selectGears)
        {
            auto it = gearsLoopUp.find(gearName);
            if (it == gearsLoopUp.end())
            {
                assert(false);
                return false;
            }
            gearsVec.emplace_back(it->second);
        }

        std::cout << "Xian ren size: " << xianRenVecSize << std::endl;

        // Init global buffs
        XianRenPropBuff xianJie_individual_buff;
        GameAlgorithms::GetSumOfXianjieIndividualBuffs(m_xianJieFileData, xianJie_individual_buff);
        XianRenPropBuff xianJie_global_Buff;
        GameAlgorithms::GetSumOfXianjieGlobalBuffs(m_xianJieFileData, xianJie_global_Buff);

        // Init xian ren self buff vec
        std::vector<XianRenPropBuff> xianRenSelfBuffVec;
        xianRenSelfBuffVec.reserve(xianRenVecSize);
        for (const auto* pXianRenData : calcXianRenVec)
        {
            xianRenSelfBuffVec.emplace_back(GameAlgorithms::GetXianRenSelfBuff(*pXianRenData));
        }

        constexpr auto propMask = PropertyMask::All;
        // Init All gears buff
        XianRenPropBuff gear_individual_buff;
        XianRenPropBuff gear_global_buff;
        for (const auto& data : gearsVec)
        {
            gear_individual_buff.IncreaseBy<propMask>(data->individualBuff);
            gear_global_buff.IncreaseBy<propMask>(data->globalBuff);
        }
        // Accomulating xian ren
        XianRenProp sumXianRenProp;
        for (int i = 0; i < xianRenVecSize; ++i)
        {
            auto all_xianren_buff = xianJie_individual_buff.Add<propMask>(gear_individual_buff);
            all_xianren_buff.IncreaseBy<propMask>(xianRenSelfBuffVec[i]);

            auto xianRen = *calcXianRenVec[i];
            xianRen.baseProp.ApplyBuff<propMask>(all_xianren_buff);

            std::cout << xianRen.name << ":\n" << xianRen.baseProp.ToString() << std::endl;

            sumXianRenProp.IncreaseBy<propMask>(xianRen.baseProp);
        }

        std::cout << "Xian ren Sum:\n" << sumXianRenProp.ToString() << std::endl;

        auto all_global_buffs = xianJie_global_Buff.Add<propMask>(gear_global_buff);
        sumXianRenProp.ApplyBuff<propMask>(all_global_buffs);

        std::cout << "Zong men Sum:\n" << sumXianRenProp.ToString() << std::endl;

        break;
    }
    default:
        errorStr += "Not implememnted yet\n";
        return false;
    }

    if (pSelector != nullptr)
    {
        if (!pSelector->Run(errorStr))
            return false;
    }
    else
    {
        if (solution != Solution::Test)
        {
            assert(false);
            return false;
        }
    }

    return true;
}
} // namespace GearCalc