//
// Author: Jason Huang(jasonhuang1988@gmail.com) 2021
//

#include "JUtils/pch.h"

#include "GearCalculator.h"

#include "JUtils/Utils.h"
#include "JUtils/Algorithms.h"

#include <execution>
#include <mutex>
#include <unordered_set>

#include "vorbrodt/pool.hpp"

using namespace JUtils;
using namespace GearCalc;

namespace
{
namespace GameAlgorithms
{
template <XianRenPropertyMask PropMask = XianRenPropertyMask::All>
void GetSumOfXianjieIndividualBuffs(const XianJieFileData& xianJieFileData, XianRenPropBuff& out)
{
    // Accumulate touxiang.
    for (const auto& data : xianJieFileData.GetTouXiangDataVec())
        out.IncreaseBy<PropMask>(data.individualBuff);

    // Accumulate xianlv tianfu.
    for (const auto& data : xianJieFileData.GetXianLvDataVec())
        out.IncreaseBy<PropMask>(data.tianfu_individual_buff);
}
template <XianRenPropertyMask PropMask = XianRenPropertyMask::All>
void GetSumOfXianjieGlobalBuffs(const XianJieFileData& xianJieFileData, XianRenPropBuff& out)
{
    // Accumulate xianlv tianfu.
    for (const auto& data : xianJieFileData.GetXianLvDataVec())
        out.IncreaseBy<PropMask>(data.tianfu_global_buff);
}
template <ChanyePropertyMask PropMask = ChanyePropertyMask::All>
void GetSumOfXianjieChanyeBuffs(const XianJieFileData& xianJieFileData, ChanyePropBuff& out)
{
    // Accumulate xianlv tianfu.
    for (const auto& data : xianJieFileData.GetXianLvDataVec())
        out.IncreaseBy<PropMask>(data.tianfu_chanye_buff);
}
template <XianRenPropertyMask PropMask = XianRenPropertyMask::All>
void AddXianRenSelfBuff(const XianRenData& target, XianRenPropBuff& out)
{
    if (target.pXianzhi != nullptr)
    {
        out.IncreaseBy<PropMask>(target.pXianzhi->individual_buff);
        if (target.pXianzhi->pXianlv != nullptr)
        {
            out.IncreaseBy<PropMask>(target.pXianzhi->pXianlv->fushi_buff);
        }
    }
}

template <XianRenPropertyMask PropMask = XianRenPropertyMask::All, typename TypeXianRenElement,
    typename XianAccessor>
std::vector<XianRenPropBuff> GetXianRenStaicBuffVec(const XianJieFileData& xianJieFileData,
    const std::vector<TypeXianRenElement>& xianRenVec, XianAccessor&& xianrenAccessor)
{
    const auto xianRenVecSize = xianRenVec.size();

    // Init out, xianZhi + fushi + global(touxiang, xianlv tianfu)
    std::vector<XianRenPropBuff> out(xianRenVecSize);

    // Init global buff(touxiang, xianlv tianfu)
    XianRenPropBuff xianJie_individual_buff;
    GetSumOfXianjieIndividualBuffs<PropMask>(xianJieFileData, xianJie_individual_buff);

    for (std::uint32_t i = 0; i < xianRenVecSize; ++i)
    {
        const auto& xianRenElement = xianRenVec[i];
        const XianRenData& xianRenConstRef =
            std::forward<XianAccessor>(xianrenAccessor)(xianRenElement);

        auto& xianRenSelfBuff = out[i];

        AddXianRenSelfBuff<PropMask>(xianRenConstRef, xianRenSelfBuff);

        // Add global buffs to each one
        xianRenSelfBuff.IncreaseBy<PropMask>(xianJie_individual_buff);
    }

    return out;
}

template <ChanyeFieldCategory::Enum ChanyeCat,
    typename = std::enable_if_t<ChanyeCat == ChanyeFieldCategory::ChanJing ||
        ChanyeCat == ChanyeFieldCategory::ChanNeng>>
std::vector<ChanyePropBuff> GetChanyeStaicBuffVec(
    const XianJieFileData& xianJieFileData, const std::vector<ChanyeFieldData>& chanyeFieldDataVec)
{

    const auto chanyeFiledSize = chanyeFieldDataVec.size();

    static constexpr auto kChanyePropMask = ChanyeFieldCatToMask<ChanyeCat>();

    // Init global buff
    ChanyePropBuff xianJie_chanye_Buff;
    GetSumOfXianjieChanyeBuffs<kChanyePropMask>(xianJieFileData, xianJie_chanye_Buff);

    std::vector<ChanyePropBuff> out(chanyeFiledSize);

    for (std::uint32_t i = 0; i < chanyeFiledSize; ++i)
    {
        auto& chanyePropBuff = out[i];

        // Init with loaded zaohua percent + chanye level percent
        if constexpr (ChanyeCat == ChanyeFieldCategory::ChanJing)
            chanyePropBuff.chanJing_percent = chanyeFieldDataVec[i].selfBuff;
        else
            chanyePropBuff.chanNeng_percent = chanyeFieldDataVec[i].selfBuff;

        // Add global buffs to each one
        chanyePropBuff.IncreaseBy<kChanyePropMask>(xianJie_chanye_Buff);
    }

    return out;
}

template <ChanyeFieldCategory::Enum ChanyeCat,
    typename = std::enable_if_t<ChanyeCat == ChanyeFieldCategory::ChanJing ||
        ChanyeCat == ChanyeFieldCategory::ChanNeng>>
void GetChanyeProp(const XianRenProp& xianRenPropSum, const ChanyeFieldData& chanyeData,
    const ChanyePropBuff& allChanyeBuffs, ChanyeProp<ChanyeCat>& out)
{
    // Base output
    out.output = chanyeData.CalcBaseOutput(xianRenPropSum);
    // Apply chanye buffs
    out.ApplyBuff(allChanyeBuffs);
}
} // namespace GameAlgorithms

void PrintLargeSpace()
{
    std::cout << "=========================================" << std::endl << std::endl;
}
void PrintSmallSpace()
{
    std::cout << "-----------------------------------------" << std::endl;
}

struct SolutionSelectorBase
{
    template <typename ThisType,
        typename = typename std::enable_if_t<std::is_base_of_v<SolutionSelectorBase, ThisType>>>
    static std::unique_ptr<ThisType> CreateInstance(
        const XianJieFileData& xianJieFileData, const XianQiFileData& xianQiFileData)
    {
        return std::make_unique<ThisType>(xianJieFileData, xianQiFileData);
    }

    SolutionSelectorBase(
        const XianJieFileData& xianJieFileData, const XianQiFileData& xianQiFileData) :
        m_xianJieFileData(xianJieFileData), m_xianQiFileData(xianQiFileData)
    {
    }

    virtual ~SolutionSelectorBase() {};
    virtual bool Run(std::string& errorStr) = 0;

protected:
    const XianJieFileData& m_xianJieFileData;
    const XianQiFileData& m_xianQiFileData;
};

template <Calculator::Solution SolutionType,
    typename = std::enable_if_t<SolutionType == Calculator::Solution::BestXianRenSumProp ||
        SolutionType == Calculator::Solution::BestGlobalSumLiNian>>
struct XianRenGearSelector : public SolutionSelectorBase
{
public:
    XianRenGearSelector(
        const XianJieFileData& xianJieFileData, const XianQiFileData& xianQiFileData) :
        SolutionSelectorBase(xianJieFileData, xianQiFileData)
    {
    }

    bool Run(std::string& errorStr) override
    {
        const auto& xianRenVec = m_xianJieFileData.GetCalcXianRenDataVec();

        const auto& xianQiVec     = m_xianQiFileData.GetCalcGearsVec();
        const auto xianRenVecSize = xianRenVec.size();
        const auto xianQiVecSize  = xianQiVec.size();
        const auto maxEquiptNum   = m_xianQiFileData.GetMaxNumEquip();

        // Config prop mask
        static constexpr auto kXianRenPropMask = []() -> XianRenPropertyMask {
            if constexpr (SolutionType == Calculator::Solution::BestXianRenSumProp)
                return XianRenPropertyMask::All;
            else
                return XianRenPropertyMask::Li | XianRenPropertyMask::Nian;
        }();

        if (maxEquiptNum < 1)
        {
            errorStr += u8"最小可装备数不能小于1!\n";
            assert(false);
            return false;
        }
        const auto expectedCombSize =
            SelectCombination::GetNumOfSelectionComb(xianQiVecSize, maxEquiptNum);
        std::cout << u8"需计算仙人数: " << xianRenVecSize << std::endl;
        std::cout << u8"需计算仙器数: " << xianQiVecSize << std::endl;
        std::cout << u8"可装备个数: " << maxEquiptNum << std::endl;
        std::cout << u8"需计算: " << FormatNumber(expectedCombSize) << u8" 种可能性" << std::endl;
        PrintLargeSpace();

        if (expectedCombSize == 0)
        {
            errorStr += u8"无需计算!\n";
            return false;
        }

        // Init xian ren static buff vec (global + self )
        const std::vector<XianRenPropBuff> xianRenStaticBuffVec =
            GameAlgorithms::GetXianRenStaicBuffVec<kXianRenPropMask>(m_xianJieFileData, xianRenVec,
                [](auto& element) -> const XianRenData& { return *element; });

        // Init xian ren self buff vec
        XianRenPropBuff xianJie_global_Buff;
        GameAlgorithms::GetSumOfXianjieGlobalBuffs(m_xianJieFileData, xianJie_global_Buff);

        // Best ones
        using CombVecType = std::vector<std::size_t>;
        CombVecType bestComb;
        XianRenProp best_xianren_prop_sum;
        double best_xianren_individual_sum_num = 0.0;
        double best_global_li_nian_sum_num     = 0.0;
        std::vector<XianRenProp> bestXianRenFinalPropVec;

        std::vector<XianRenProp> xianRenFinalPropStack(xianRenVecSize);
        auto combCallBack = [&](const CombVecType& combIndexVec, std::size_t indexOfComb) -> void {
            if constexpr (SolutionType == Calculator::Solution::BestXianRenSumProp)
            {

                // Accomulate individual buff of each Xian Qi
                XianRenPropBuff xianQi_individual_buff;
                for (auto& combIndex : combIndexVec)
                {
                    xianQi_individual_buff.IncreaseBy<kXianRenPropMask>(
                        xianQiVec[combIndex]->individualBuff);
                }

                // Apply all individual buffs to each Xian Ren
                XianRenProp sumXianRenProp;
                for (int i = 0; i < xianRenVecSize; ++i)
                {
                    const auto* pXianRenData = xianRenVec[i];

                    // All buffs.
                    auto allBuffs =
                        xianQi_individual_buff.Add<kXianRenPropMask>(xianRenStaticBuffVec[i]);

                    // Apply all buffs to xian ren.
                    auto& xianRenPropCopy = xianRenFinalPropStack[i];
                    xianRenPropCopy       = pXianRenData->baseProp;
                    xianRenPropCopy.ApplyBuff<kXianRenPropMask>(allBuffs);

                    // Accomulate the sum.
                    sumXianRenProp.IncreaseBy<kXianRenPropMask>(xianRenPropCopy);
                }

                // Select the best one
                {
                    auto sum = sumXianRenProp.GetSum();
                    if (sum > best_xianren_individual_sum_num)
                    {
                        bestComb                        = combIndexVec;
                        best_xianren_prop_sum           = sumXianRenProp;
                        best_xianren_individual_sum_num = sum;

                        bestXianRenFinalPropVec = xianRenFinalPropStack;
                    }
                }
            }
            else if constexpr (SolutionType == Calculator::Solution::BestGlobalSumLiNian)
            {
                // Accomulate individual & global buffs of each Xian Qi
                XianRenPropBuff xianQi_individual_buff;
                XianRenPropBuff all_global_buffs;
                for (auto& combIndex : combIndexVec)
                {
                    const auto& xianQi = xianQiVec[combIndex];

                    xianQi_individual_buff.IncreaseBy<kXianRenPropMask>(xianQi->individualBuff);
                    all_global_buffs.IncreaseBy<kXianRenPropMask>(xianQi->globalBuff);
                }
                all_global_buffs.IncreaseBy<kXianRenPropMask>(xianJie_global_Buff);

                // Apply all individual buffs to each Xian Ren
                XianRenProp sumXianRenProp;
                for (int i = 0; i < xianRenVecSize; ++i)
                {
                    const auto* pXianRenData = xianRenVec[i];

                    // All buffs.
                    auto allBuffs =
                        xianQi_individual_buff.Add<kXianRenPropMask>(xianRenStaticBuffVec[i]);

                    // Apply all buffs to xian ren.
                    auto& xianRenPropCopy = xianRenFinalPropStack[i];
                    xianRenPropCopy       = pXianRenData->baseProp;
                    xianRenPropCopy.ApplyBuff<kXianRenPropMask>(allBuffs);

                    // Accomulate the sum.
                    sumXianRenProp.IncreaseBy<kXianRenPropMask>(xianRenPropCopy);
                }

                // Apply all global buffs to final sum of each xianren prop
                sumXianRenProp.ApplyBuff<kXianRenPropMask>(all_global_buffs);

                // Select the best one
                {
                    auto sum = sumXianRenProp.li + sumXianRenProp.nian;
                    if (sum > best_global_li_nian_sum_num)
                    {
                        bestComb                    = combIndexVec;
                        best_xianren_prop_sum       = sumXianRenProp;
                        best_global_li_nian_sum_num = sum;

                        bestXianRenFinalPropVec = xianRenFinalPropStack;
                    }
                }
            }
            else
            {
                static_assert(false);
            }
        };

        // Run selection combination.
        Timer timer;
        auto numCombs =
            SelectCombination::RunSingleThread(xianQiVecSize, maxEquiptNum, combCallBack);

        std::cout << u8"仙器挑选耗时: " << timer.DurationInSec() << u8"秒" << std::endl;
        std::cout << u8"共计算组合数: " << numCombs << std::endl;
        PrintLargeSpace();

        if (numCombs != expectedCombSize)
        {
            errorStr +=
                FormatString(u8"预计计算: ", expectedCombSize, u8", 实际计算: ", numCombs, "\n");
            assert(false);
            return false;
        }

        for (int i = 0; i < xianRenVecSize; ++i)
        {
            if (i != 0)
                PrintSmallSpace();
            std::cout << u8"仙人" << i + 1 << ": " << std::quoted(xianRenVec[i]->name) << std::endl;
            std::cout << bestXianRenFinalPropVec[i].ToString();
        }

        PrintLargeSpace();

        std::vector<const GearData*> selectedGears;
        selectedGears.reserve(maxEquiptNum);
        for (auto& combIndex : bestComb)
        {
            selectedGears.emplace_back(xianQiVec[combIndex]);
        }
        std::cout << u8"挑选仙器: " << std::endl;
        PrintSmallSpace();
        XianRenPropBuff xianQi_individual_buff_sum;
        XianRenPropBuff xianQi_global_buff_sum;
        for (const auto* pGear : selectedGears)
        {
            xianQi_individual_buff_sum.IncreaseBy(pGear->individualBuff);
            xianQi_global_buff_sum.IncreaseBy(pGear->globalBuff);
            std::cout << pGear->name << std::endl;
        }

        PrintLargeSpace();

        std::cout << u8"仙器属性总和:" << std::endl;
        PrintSmallSpace();
        std::cout << xianQi_individual_buff_sum.ToString(XianRenPropBuff::k_individual_prefix);
        PrintSmallSpace();
        std::cout << xianQi_global_buff_sum.ToString(XianRenPropBuff::k_global_prefix);

        PrintLargeSpace();

        std::cout << u8"仙人属性总和:" << std::endl;
        PrintSmallSpace();
        std::cout << best_xianren_prop_sum.ToString() << std::endl;

        PrintLargeSpace();

        return true;
    }
};

template <Calculator::Solution SolutionType,
    typename = std::enable_if_t<SolutionType == Calculator::Solution::BestChanJing ||
        SolutionType == Calculator::Solution::BestChanNeng>>
struct ChanYeSelector : public SolutionSelectorBase
{
    ChanYeSelector(const XianJieFileData& xianJieFileData, const XianQiFileData& xianQiFileData) :
        SolutionSelectorBase(xianJieFileData, xianQiFileData)
    {
    }

    bool Run(std::string& errorStr) override
    {
        // Config Chanye prop type.
        static constexpr auto kChanyeCat = []() constexpr->ChanyeFieldCategory::Enum
        {
            if constexpr (SolutionType == Calculator::Solution::BestChanJing)
                return ChanyeFieldCategory::ChanJing;
            else
                return ChanyeFieldCategory::ChanNeng;
        }
        ();
        using ChanyePropType = ChanyeProp<kChanyeCat>;

        // Config prop mask.
        static constexpr auto kChanyePropMask  = ChanyeFieldCatToMask<kChanyeCat>();
        static constexpr auto kXianRenPropMask = XianRenPropertyMask::All;

        // Chanye related refs and consts.
        const std::vector<ChanyeFieldData>& chanyeVec =
            m_xianJieFileData.GetChanYeFieldDataVec<kChanyeCat>();
        const auto chanyeVecSize = chanyeVec.size();

        // XianRen related refs and consts.
        const auto& xianRenVec    = m_xianJieFileData.GetCalcXianRenDataVec();
        const auto xianRenVecSize = xianRenVec.size();

        // XianQi related refs and consts.
        const auto& xianQiVec    = m_xianQiFileData.GetCalcGearsVec();
        const auto xianQiVecSize = xianQiVec.size();
        const auto maxEquiptNum  = m_xianQiFileData.GetMaxNumEquip();

        const auto expectedCombSize =
            SelectCombination::GetNumOfSelectionComb(xianQiVecSize, maxEquiptNum);
        if (maxEquiptNum < 1)
        {
            errorStr += u8"最小可装备数不能小于1!\n";
            assert(false);
            return false;
        }

        std::cout << u8"需计算仙人数: " << xianRenVecSize << std::endl;
        std::cout << u8"需计算仙器数: " << xianQiVecSize << std::endl;
        std::cout << u8"可装备个数: " << maxEquiptNum << std::endl;
        std::cout << u8"需计算: " << FormatNumber(expectedCombSize) << u8" 种可能性" << std::endl;
        PrintLargeSpace();

        if (expectedCombSize == 0)
        {
            errorStr += u8"无需计算!\n";
            return false;
        }

        // Init chan ye self buff vec
        const std::vector<ChanyePropBuff> chanyeFieldStaticBuffVec =
            GameAlgorithms::GetChanyeStaicBuffVec<kChanyeCat>(m_xianJieFileData, chanyeVec);

        // Init xian ren static buff vec
        const std::vector<XianRenPropBuff> xianRenStaticBuffVec =
            GameAlgorithms::GetXianRenStaicBuffVec<kXianRenPropMask>(m_xianJieFileData, xianRenVec,
                [](auto& element) -> const XianRenData& { return *element; });

        // Select Xian Ren for each Chanye field
        struct SelectedChanyeXianRen
        {
            SelectedChanyeXianRen(
                std::uint32_t xianRenIndexInXinrenVec, std::uint32_t chanyeIndexInChanyeVec) :
                xianRenIndexInXianRenVec(xianRenIndexInXinrenVec),
                chanyeIndexInChanyeVec(chanyeIndexInChanyeVec)
            {
            }

            std::uint32_t xianRenIndexInXianRenVec = GetInvalidValue(32);
            std::uint32_t chanyeIndexInChanyeVec   = GetInvalidValue(32);
        };
        std::vector<SelectedChanyeXianRen> selectedXianRenVec;
        {

            struct ChanyeWeight
            {
                ChanyeWeight(std::uint32_t xianRenIndex, std::uint32_t chanyeFieldIndex,
                    double outputValue) :
                    xianRenIndex(xianRenIndex),
                    chanyeFieldIndex(chanyeFieldIndex),
                    outputValue(outputValue) {};

                std::uint32_t xianRenIndex     = GetInvalidValue(32);
                std::uint32_t chanyeFieldIndex = GetInvalidValue(32);

                double outputValue = 0.0;
            };

            // We store all possible weights for each chanye field
            std::vector<ChanyeWeight> allChanyeWeightVec;
            const auto allChanyeWeightSize = chanyeVecSize * xianRenVecSize;
            allChanyeWeightVec.reserve(allChanyeWeightSize);
            ChanyePropType chanyePropStack;
            for (std::uint32_t chanyeIndex = 0; chanyeIndex < chanyeVecSize; ++chanyeIndex)
            {
                const auto& chanyeData       = chanyeVec[chanyeIndex];
                const auto& chanyeStaticBuff = chanyeFieldStaticBuffVec[chanyeIndex];

                for (std::uint32_t xianRenIndex = 0; xianRenIndex < xianRenVecSize; ++xianRenIndex)
                {
                    const auto* pXianRen = xianRenVec[xianRenIndex];

                    // Calculate xianren prop without gears
                    auto xianRenPropCopy = pXianRen->baseProp;
                    xianRenPropCopy.ApplyBuff<kXianRenPropMask>(xianRenStaticBuffVec[xianRenIndex]);

                    // Calculate chanye output without gears
                    GameAlgorithms::GetChanyeProp(
                        xianRenPropCopy, chanyeData, chanyeStaticBuff, chanyePropStack);

                    allChanyeWeightVec.emplace_back(
                        xianRenIndex, chanyeIndex, chanyePropStack.output);
                }
            }

            // Sort allChanyeWeightVec by descending order, so start with highest output
            std::sort(std::execution::par_unseq, allChanyeWeightVec.begin(),
                allChanyeWeightVec.end(), [](const ChanyeWeight& a, const ChanyeWeight& b) -> bool {
                    return a.outputValue > b.outputValue;
                });

            // Pick the Xian ren from the weight vec
            std::uint64_t pickedIndices = 0;
            const auto kMaxPickMask     = PickIndex::GetMaxPickedIndices(xianRenVecSize);
            std::vector<std::uint32_t> chanyeXianRenCountVec(chanyeVecSize);

            for (const auto& weight : allChanyeWeightVec)
            {
                const auto& indexBitMask = PickIndex::k_inputIndexBitMask[weight.xianRenIndex];

                // Skip the index which has been picked
                if (pickedIndices & indexBitMask)
                    continue;

                const auto& chanyeFieldData = chanyeVec[weight.chanyeFieldIndex];
                auto& chanyeNumCount        = chanyeXianRenCountVec[weight.chanyeFieldIndex];
                // Still have room for more Xianren
                if (chanyeNumCount < chanyeFieldData.numXianRen)
                {
                    selectedXianRenVec.emplace_back(weight.xianRenIndex, weight.chanyeFieldIndex);

                    pickedIndices |= indexBitMask;
                    ++chanyeNumCount;
                }
            }

            std::cout << u8"不考虑装备的影响下, 选择以下仙人, 按产出由大到小排列:" << std::endl;
            PrintSmallSpace();

            for (std::uint32_t i = 0; i < selectedXianRenVec.size(); ++i)
            {
                const auto& selectedXianRen = selectedXianRenVec[i];
                std::cout << u8"仙人" << i + 1 << ": "
                          << std::quoted(xianRenVec[selectedXianRen.xianRenIndexInXianRenVec]->name)
                          << u8", 所属产业: "
                          << std::quoted(chanyeVec[selectedXianRen.chanyeIndexInChanyeVec].name)
                          << std::endl;
            }
            PrintLargeSpace();
            std::cout << u8"继续计算中, 请耐心等待..." << std::endl;

            // Sort selected xian ren vec by index in chanye vec
            std::sort(std::execution::par_unseq, selectedXianRenVec.begin(),
                selectedXianRenVec.end(),
                [&](const SelectedChanyeXianRen& a, const SelectedChanyeXianRen& b) -> bool {
                    return a.chanyeIndexInChanyeVec < b.chanyeIndexInChanyeVec;
                });
        }

        const auto selectedXianRenSize = selectedXianRenVec.size();
        // Select Xianqi
        std::vector<const GearData*> selectedGears;
        ChanyePropType bestChanyeProp;
        std::vector<XianRenProp> bestXianRenFinalPropVec;
        std::vector<ChanyePropType> bestChanyeFinalOutputVec;
        {

            if (selectedXianRenVec.empty())
            {
                errorStr += u8"产业未能选择任何仙人!\n";
                assert(false);
                return false;
            }

            // Best ones
            using CombVecType = std::vector<std::size_t>;
            CombVecType bestComb;
            std::vector<XianRenProp> xianRenFinalPropStack(xianRenVecSize);
            std::vector<ChanyePropType> chanyeFinalOutputStack(chanyeVecSize);
            auto combCallBack = [&](const CombVecType& combIndexVec,
                                    std::size_t indexOfComb) -> void {
                // Accomulate all individual buff and all chanye buff of each Xian Qi
                // xianjie buffs + xianQi buffs
                XianRenPropBuff xianQi_individual_buffs;
                ChanyePropBuff xianQi_chanye_buff;
                for (auto& combIndex : combIndexVec)
                {
                    const auto& xianQi = xianQiVec[combIndex];

                    xianQi_individual_buffs.IncreaseBy<kXianRenPropMask>(xianQi->individualBuff);
                    xianQi_chanye_buff.IncreaseBy<kChanyePropMask>(xianQi->chanyeBuff);
                }

                // Apply all individual buffs to each Xian Ren
                XianRenProp sumXianRenProp;
                std::uint32_t currentChanyeIndex = selectedXianRenVec[0].chanyeIndexInChanyeVec;
                ChanyePropType sumChanyeProp;

                // Helper function to output each chanye prop
                auto outPutChanye = [&]() -> void {
                    auto allChanyeBuffs =
                        chanyeFieldStaticBuffVec[currentChanyeIndex].Add<kChanyePropMask>(
                            xianQi_chanye_buff);

                    auto& chanyeProp = chanyeFinalOutputStack[currentChanyeIndex];
                    GameAlgorithms::GetChanyeProp(
                        sumXianRenProp, chanyeVec[currentChanyeIndex], allChanyeBuffs, chanyeProp);

                    // Accomulate the chanye prop
                    sumChanyeProp += chanyeProp;
                };

                for (const SelectedChanyeXianRen& selectedXianRen : selectedXianRenVec)
                {
                    // End of one of chanyeField accomulating
                    const auto selectedChanyeIndex = selectedXianRen.chanyeIndexInChanyeVec;
                    if (currentChanyeIndex != selectedChanyeIndex)
                    {
                        // After accomulating prop of each XianRen in a chanye field, we calculate
                        // the final output of current chanye field.
                        outPutChanye();

                        // Reset states
                        currentChanyeIndex = selectedChanyeIndex;
                        sumXianRenProp.Reset();
                    }

                    // All XianRen buffs is now xianQi_individual_buffs + xianRenStaticBuff (xianzhi
                    // + fushi + global)
                    auto allXianRenBuffs = xianQi_individual_buffs.Add<kXianRenPropMask>(
                        xianRenStaticBuffVec[selectedXianRen.xianRenIndexInXianRenVec]);

                    // Apply all buffs to xian ren.
                    auto& xianRenPropCopy =
                        xianRenFinalPropStack[selectedXianRen.xianRenIndexInXianRenVec];

                    xianRenPropCopy =
                        xianRenVec[selectedXianRen.xianRenIndexInXianRenVec]->baseProp;
                    xianRenPropCopy.ApplyBuff<kXianRenPropMask>(allXianRenBuffs);

                    // Accomulate the sum of each Xian Prop of current chanye field.
                    sumXianRenProp.IncreaseBy<kXianRenPropMask>(xianRenPropCopy);
                }

                // Ouput the tailing chanye
                outPutChanye();

                // Select the best one
                {
                    if (sumChanyeProp.output > bestChanyeProp.output)
                    {
                        bestComb       = combIndexVec;
                        bestChanyeProp = sumChanyeProp;

                        bestXianRenFinalPropVec  = xianRenFinalPropStack;
                        bestChanyeFinalOutputVec = chanyeFinalOutputStack;
                    }
                }
            };

            // Run selection combination.
            Timer timer;
            auto numCombs =
                SelectCombination::RunSingleThread(xianQiVecSize, maxEquiptNum, combCallBack);

            if (numCombs != expectedCombSize)
            {
                errorStr += FormatString(
                    u8"预计计算: ", expectedCombSize, u8", 实际计算: ", numCombs, "\n");
                assert(false);
                return false;
            }

            std::cout << u8"仙器挑选耗时: " << timer.DurationInSec() << u8"秒" << std::endl;
            std::cout << u8"共计算组合数: " << numCombs << std::endl;
            PrintSmallSpace();

            selectedGears.reserve(maxEquiptNum);
            for (auto& combIndex : bestComb)
            {
                selectedGears.emplace_back(xianQiVec[combIndex]);
            }
        }

        std::cout << u8"挑选仙器: " << std::endl;
        PrintSmallSpace();
        XianRenPropBuff xianQi_individual_buff_sum;
        ChanyePropBuff xianQi_chanye_buff_sum;
        for (const auto* pGear : selectedGears)
        {
            xianQi_individual_buff_sum.IncreaseBy(pGear->individualBuff);
            xianQi_chanye_buff_sum.IncreaseBy(pGear->chanyeBuff);

            std::cout << pGear->name << std::endl;
        }

        PrintSmallSpace();

        std::cout << u8"仙器属性总和:" << std::endl;
        PrintSmallSpace();
        std::cout << xianQi_individual_buff_sum.ToString(XianRenPropBuff::k_individual_prefix);
        PrintSmallSpace();
        std::cout << xianQi_chanye_buff_sum.ToString() << std::endl;

        PrintLargeSpace();

        {
            std::uint32_t currentChanyeIndex  = selectedXianRenVec[0].chanyeIndexInChanyeVec;
            std::uint32_t currentXianRenIndex = 0;

            auto printChanye = [&]() -> void {
                std::cout << u8"产业: " << std::quoted(chanyeVec[currentChanyeIndex].name)
                          << u8" 总产值: "
                          << FormatFloatToInt<std::uint64_t>(
                                 bestChanyeFinalOutputVec[currentChanyeIndex].output)
                          << std::endl;
            };

            printChanye();
            for (std::uint32_t i = 0; i < selectedXianRenSize; ++i)
            {
                const SelectedChanyeXianRen& selectedXianRen = selectedXianRenVec[i];

                const auto selectedChanyeIndex = selectedXianRen.chanyeIndexInChanyeVec;
                if (currentChanyeIndex != selectedChanyeIndex)
                {
                    currentChanyeIndex  = selectedChanyeIndex;
                    currentXianRenIndex = 0;

                    PrintLargeSpace();
                    printChanye();
                }

                PrintSmallSpace();
                std::cout << u8"仙人" << currentXianRenIndex + 1 << ": "
                          << std::quoted(xianRenVec[selectedXianRen.xianRenIndexInXianRenVec]->name)
                          << std::endl;
                std::cout
                    << bestXianRenFinalPropVec[selectedXianRen.xianRenIndexInXianRenVec].ToString();

                ++currentXianRenIndex;
            }
            PrintLargeSpace();
        }
        std::cout << u8"每轮总收益:" << std::endl << bestChanyeProp.ToString();
        PrintLargeSpace();

        return true;
    }
};

namespace UnitTest
{
void TestSelectComb()
{
    constexpr auto kNumElement = 30;
    constexpr auto kNumSelect  = 7;

    // No need to compute the hash value once again.
    struct Hasher
    {
        std::size_t operator()(const std::size_t& value) const { return value; }
    };

    // Get the results using single thread.
    Timer timer;
    std::unordered_set<std::size_t, Hasher> uniqueCombs;
    {
        auto numCombs = SelectCombination::RunSingleThread(kNumElement, kNumSelect,
            [&](const std::vector<std::size_t>& combIndexVec, std::size_t indexOfComb) -> void {
                std::size_t hash = ComputeHash(indexOfComb);
                for (auto index : combIndexVec)
                {
                    HashCombine(hash, index);
                }

                auto insertedPair = uniqueCombs.emplace(hash);
                if (!insertedPair.second)
                {
                    assert(false);
                    throw std::runtime_error("Duplicated comb found!\n");
                }
            });
        if (numCombs != uniqueCombs.size())
        {
            assert(false);
            throw std::runtime_error("Unexpected comb size!\n");
        }
    }
    std::cout << "Single thread cost: " << timer.DurationInSec() << std::endl;

    timer.Reset();
    // Compare the results using multi thread.
    {
        std::atomic<std::size_t> numCompared = 0;
        auto numCombs = SelectCombination::RunMultiThread(kNumElement, kNumSelect,
            [&](const std::vector<std::size_t>& combIndexVec, std::size_t indexOfComb) -> void {
                std::size_t hash = ComputeHash(indexOfComb);
                for (auto index : combIndexVec)
                {
                    HashCombine(hash, index);
                }

                auto it = uniqueCombs.find(hash);
                if (it == uniqueCombs.end())
                {
                    assert(false);
                    throw std::runtime_error("Unexpected comb found using mulithread!\n");
                }
                ++numCompared;
            });
        if (numCombs != uniqueCombs.size())
        {
            assert(false);
            throw std::runtime_error("Unexpected comb size!\n");
        }

        if (numCompared != uniqueCombs.size())
        {
            assert(false);
            throw std::runtime_error("Multithread solution has different results!\n");
        }
    }
    std::cout << "Multi thread cost: " << timer.DurationInSec() << std::endl;
}

void TestSelectionComb()
{
    constexpr std::uint32_t kMaxInputSize = 64;
    constexpr std::uint32_t kNumInput     = 9;
    constexpr std::uint32_t kNumPerGroup  = 3;

    constexpr auto kMaxSelectInputMask = PickIndex::GetMaxPickedIndices(kNumInput);

    constexpr std::uint32_t expectedNumGroups = CeilUintDivision(kNumInput, kNumPerGroup);
    // size = C{N,k} *  C{N-k * i, k} ...
    constexpr auto exptectedResultSize = [=]() constexpr->std::size_t
    {
        std::size_t out = 1;
        for (std::uint32_t i = 0; i < expectedNumGroups; ++i)
        {
            auto numElements = kNumInput - i * kNumPerGroup;
            auto combSize    = SelectCombination::GetNumOfSelectionComb(numElements, kNumPerGroup);
            out *= combSize == 0 ? 1 : combSize;
        }
        return out;
    }
    ();
    std::atomic<std::size_t> sizeOfReults = 0;
    auto outputResult                     = [&](const std::vector<std::uint64_t>& resultStack) {
        constexpr bool enablePrint = true;
        if constexpr (enablePrint)
        {
            std::stringstream strStream;
            strStream << "Comb: ";
            for (auto comb : resultStack)
            {
                auto index = 0;
                while (comb != 0)
                {
                    if (comb & 1)
                    {
                        strStream << index << " ";
                    }
                    comb >>= 1;
                    ++index;
                }
                strStream << "| ";
            }
            strStream << std::endl;
            std::cout << strStream.str();
        }
        ++sizeOfReults;
    };

    Timer timer;
    SelectCombination::SelectGroupComb selector(
        kNumInput, expectedNumGroups, kNumPerGroup, outputResult);

    timer.Reset();
    sizeOfReults = 0;
    selector.Run(true);
    std::cout << "Multi-thread Time cost: " << timer.DurationInSec() << std::endl;
    if (sizeOfReults != exptectedResultSize)
    {
        assert(false);
        throw std::runtime_error("sizeOfReults should equal to exptectedResultSize\n");
    }

    timer.Reset();
    sizeOfReults = 0;
    selector.Run(false);
    std::cout << "Single- thread Time cost: " << timer.DurationInSec() << std::endl;

    if (sizeOfReults != exptectedResultSize)
    {
        assert(false);
        throw std::runtime_error("sizeOfReults should equal to exptectedResultSize\n");
    }
}
} // namespace UnitTest
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

    std::unique_ptr<SolutionSelectorBase> pSelector = nullptr;

    switch (solution)
    {
    case Solution::BestXianRenSumProp:
        pSelector =
            SolutionSelectorBase::CreateInstance<XianRenGearSelector<Solution::BestXianRenSumProp>>(
                m_xianJieFileData, m_xianQiFileData);
        break;
    case Solution::BestGlobalSumLiNian:
        pSelector = SolutionSelectorBase::CreateInstance<
            XianRenGearSelector<Solution::BestGlobalSumLiNian>>(
            m_xianJieFileData, m_xianQiFileData);
        break;
    case Solution::BestChanJing:
        pSelector = SolutionSelectorBase::CreateInstance<ChanYeSelector<Solution::BestChanJing>>(
            m_xianJieFileData, m_xianQiFileData);
        break;
    case Solution::BestChanNeng:
        pSelector = SolutionSelectorBase::CreateInstance<ChanYeSelector<Solution::BestChanNeng>>(
            m_xianJieFileData, m_xianQiFileData);
        break;
    case Solution::Test:
    {
#ifdef M_DEBUG

        if (0)
        {
            UnitTest::TestSelectionComb();
            UnitTest::TestSelectComb();
        }
#else
        errorStr += u8"测试模式仅供开发阶段使用\n";
        return false;
#endif // M_DEBUG
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