//
// Author: Jason Huang(jasonhuang1988@gmail.com) 2021
//
#pragma once

#include "Utils.h"
#include <functional>

namespace JUtils
{

// Use bit mask to represent indices
struct PickIndex
{
    // Index bit mask
    static constexpr std::uint32_t k_maxInputSize = 64;
    static constexpr auto k_inputIndexBitMask =
        ArrayHelper::CreateArrayWithInitValues<std::uint64_t, k_maxInputSize>(
            ArrayHelper::BitMaskGenerator);

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

// Helper to compute selection combinations
struct SelectCombination
{
    // Helper function to get the total size of selection combs
    inline static constexpr std::size_t GetNumOfSelectionComb(
        std::size_t numElelment, std::size_t numSelect)
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
    static std::size_t RunSingleThread(std::size_t numElelment, std::size_t numSelect,
        std::function<void(const std::vector<std::size_t>&, std::size_t)> callBack);

    // Multi thread solution
    static std::size_t RunMultiThread(std::size_t numElelment, std::size_t numSelect,
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
};

// Helper for combination related calculations
struct Combination
{
    struct OutputCombination
    {
        OutputCombination(float sum, float diff, std::uint64_t selectedIndices) :
            sum(sum), diff(diff), selectedIndices(selectedIndices) {};

        OutputCombination(OutputCombination&&) = default;
        OutputCombination& operator=(OutputCombination&&) = default;

        float sum = 0.0f;

        // Sum - target
        float diff                    = std::numeric_limits<float>::max();
        std::uint64_t selectedIndices = 0;
    };
    static_assert(sizeof(OutputCombination) == sizeof(float) * 2 + sizeof(std::uint64_t));

    // Descriptor pass to FindSumToTargetBackTracking
    template <typename TypeData, typename = typename std::enable_if_t<std::is_unsigned_v<TypeData>>>
    struct InputSumToTargetDesc
    {
        InputSumToTargetDesc(const std::vector<TypeData>& inputVec,
            TypeData targetValue,
            std::uint64_t unitScale,
            std::uint64_t* pOutClosestCombIndices = nullptr,
            std::vector<OutputCombination>* pOutAllCombIndicesVec = nullptr,
            float refMinExeedSum                                  = -1.0f) :
            inputVec(inputVec),
            targetValue(targetValue),
            unitScale(unitScale),
            pOutClosestCombIndices(pOutClosestCombIndices),
            pOutAllCombIndicesVec(pOutAllCombIndicesVec),
            refMinExeedSum(refMinExeedSum)
        {
        }

        const std::vector<TypeData>& inputVec;
        TypeData targetValue;
        std::uint64_t unitScale;

        std::uint64_t* pOutClosestCombIndices;
        std::vector<OutputCombination>* pOutAllCombIndicesVec;
        float refMinExeedSum;
    };


    // MaxCombSizeBits is used for max combination size of each calculation. 32 means 2 ^ 32 combinations is
    // allowed in memory.
    template <bool UseHashTable = true, std::uint32_t MaxCombSizeBits = 32, typename TypeData = std::uint64_t>
    static bool FindSumToTargetBackTracking(
        const InputSumToTargetDesc<TypeData>& inputDesc, std::string& errorStr);
};

} // namespace JUtils