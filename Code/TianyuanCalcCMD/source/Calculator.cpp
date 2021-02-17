//
// Author: Jason Huang(jasonhuang1988@gmail.com) 2021
//

#include "pch.h"

#include "Calculator.h"
#include "Utils.h"

#include <bitset>
#include <cmath>
#include <execution>
#include <list>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

#define USE_REMOVE_DUPLICATES true

namespace
{
using namespace JUtils;

namespace Algorithms
{
// Time Complexity O(N), TODO: check the algorithm carefully. Currently it produce wrong result
// depends on input order.
void FindClosestSumToTargetON(const std::vector<const UserData*>& inputVec, const UserData* pTarget,
    ResultData& outResult, bool forceGreaterEq = false)
{
    assert(pTarget);

    auto targetValue = pTarget->GetOriginalData();
    int inputSize    = static_cast<int>(inputVec.size());

    struct WindowResult
    {
        WindowResult(int leftIndex, int rightIndex, std::uint64_t sum, bool isGreaterEqual) :
            m_leftIndex(leftIndex),
            m_rightIndex(rightIndex),
            m_difference(sum),
            m_isGreaterEqual(isGreaterEqual)
        {
        }
        int m_leftIndex            = 0;
        int m_rightIndex           = 0;
        std::uint64_t m_difference = 0;
        bool m_isGreaterEqual      = false;
        bool isInit                = false;
    };

    std::uint64_t currentSum = 0;
    std::uint64_t prevDiff = std::numeric_limits<std::uint64_t>::max(), currentDiff = 0;
    bool currentSign = false, prevSign = false;

    int leftIndex = 0, rightIndex = 0;

    WindowResult result(leftIndex, rightIndex, std::numeric_limits<std::uint64_t>::max(), false);
    WindowResult resultTemp = result;
    resultTemp.isInit       = true;

    while (leftIndex <= rightIndex && rightIndex < inputSize)
    {
        // Add last element
        auto& inputData = inputVec[rightIndex];
        currentSum += inputData->GetOriginalData();

        // When the current sum exceeds target value
        if (targetValue < currentSum)
        {
            // Calculate current difference
            currentDiff = currentSum - targetValue;
            currentSign = true;

            if (currentDiff < prevDiff || (!prevSign && forceGreaterEq))
            {
                // Store result as current diff is better.
                resultTemp.m_leftIndex      = leftIndex;
                resultTemp.m_rightIndex     = rightIndex;
                resultTemp.m_difference     = currentDiff;
                resultTemp.m_isGreaterEqual = currentSign;

                ++rightIndex;
            }
            else
            {
                // Store result as prev diff is better.
                // prev index is right index -1;
                resultTemp.m_leftIndex      = leftIndex;
                resultTemp.m_rightIndex     = rightIndex - 1;
                resultTemp.m_difference     = prevDiff;
                resultTemp.m_isGreaterEqual = prevSign;

                // In next iteration, left index increases
                // but right index remains the same
                // Update currentSum and leftIndex accordingly
                auto toSub = inputVec[leftIndex]->GetOriginalData() +
                    inputVec[rightIndex]->GetOriginalData();
                assert(currentSum >= toSub);
                currentSum -= toSub;

                ++leftIndex;
            }
        }
        else
        {
            // Calculate current difference
            currentDiff = targetValue - currentSum;
            currentSign = currentDiff == 0;

            // When the current sum is still not enough, we increase the right index to try to add
            // more elemenets.
            resultTemp.m_leftIndex      = leftIndex;
            resultTemp.m_rightIndex     = rightIndex;
            resultTemp.m_difference     = currentDiff;
            resultTemp.m_isGreaterEqual = currentSign;

            ++rightIndex;
        }

        // For every end of each iterations, we compare the result against the temp result to get
        // the optimal result.
        if (resultTemp.m_difference < result.m_difference)
        {
            if (!forceGreaterEq || (forceGreaterEq && resultTemp.m_isGreaterEqual))
            {
                result = resultTemp;
            }
        }

        // Save difference of previous iteration
        prevDiff = currentDiff;
        prevSign = currentSign;
    }

    // After getting the best result window, we config the out put using indices of result.
    outResult.m_pTarget = pTarget;

    bool resultNotFound = !result.isInit;

    int startIndex = result.m_leftIndex, endIndex = result.m_rightIndex;
    if (resultNotFound)
    {
        // If not found, we simply copy the input list if it's not empty.
        startIndex = 0;
        endIndex   = inputSize - 1;
        if (endIndex < 0)
            endIndex = 0;
    }

    // Calculate the sum again, to make sure the result is correct.
    outResult.m_sum = 0;
    for (int i = startIndex; i <= endIndex && inputSize > 0; ++i)
    {
        auto& data = inputVec[i];
        outResult.m_sum += data->GetOriginalData();
        outResult.m_combination.emplace_back(data);
    }

    outResult.m_isExceeded = outResult.m_sum > targetValue;

    if (resultNotFound)
    {
        // Make sure the only reason of result not found is input sum is not enough to the target.
        assert(targetValue > outResult.m_sum && !outResult.m_isExceeded);
        outResult.m_difference = targetValue - outResult.m_sum;
    }
    else
    {
        outResult.m_difference = result.m_difference;

        auto isCorrectSign = (result.m_isGreaterEqual && outResult.m_sum >= targetValue) ||
            (!result.m_isGreaterEqual && outResult.m_sum < targetValue);
        assert(isCorrectSign);

        auto isCorrectValue = outResult.m_isExceeded
            ? outResult.m_sum - result.m_difference == targetValue
            : outResult.m_sum + result.m_difference == targetValue;
        assert(isCorrectValue);
    }
}

// Create our index bit mask at compile time
constexpr auto k_inputIndexBitMask =
    ArrayHelper::CreateArrayWithInitValues<std::uint64_t, MAX_INPUT_SIZE>(
        ArrayHelper::BitMaskGenerator);

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

template <bool UseHashTable>
struct TempCombination
{
};
template <>
struct TempCombination<true>
{
    TempCombination() {}
    TempCombination(float remainValue, std::uint64_t bitFlag, std::size_t hash) :
        remainValue(remainValue), bitFlag(bitFlag), hash(hash)
    {
    }

    float remainValue = 0.0f;

    // Bit flag represent the index of input vector that need to be removed
    std::uint64_t bitFlag = 0;
    std::size_t hash      = 0;
};

template <>
struct TempCombination<false>
{
    TempCombination() {}
    TempCombination(float remainValue, std::uint64_t bitFlag, std::size_t hash) :
        remainValue(remainValue), bitFlag(bitFlag)
    {
    }
    float remainValue = 0.0f;
    // Bit flag represent the index of input vector that need to be removed
    std::uint64_t bitFlag = 0;
};

// Config max picked table by removing the bits from left most to match number of inputs
template <typename T, typename = typename std::enable_if_t<std::is_integral_v<T>>>
constexpr std::uint64_t GetMaxPickedIndices(T inputSize)
{
    assert(inputSize > 0);

    auto out = std::numeric_limits<std::uint64_t>::max();

    auto numBitsToRemove = MAX_INPUT_SIZE - inputSize;
    out >>= numBitsToRemove;
    return out;
}

// Back tracking approach
template <bool UseHashTable = true>
bool FindSumToTargetBackTracking(const std::vector<const UserData*>& inputVec,
    const UserData* pTarget, std::uint64_t unitScale, std::string& errorStr,
    std::uint64_t* pOutClosestCombIndices                 = nullptr,
    std::vector<OutputCombination>* pOutAllCombIndicesVec = nullptr,
    const float* pRefMinExeedSum                          = nullptr)
{
    if (!pTarget)
    {
        errorStr += "Target can not be null\n";
        assert(false);
        return false;
    }

    if (inputVec.empty())
    {
        errorStr += "InputVec can not be empty\n";
        assert(false);
        return false;
    }

    auto targetValue = pTarget->GetOriginalData();
    auto inputSize   = static_cast<std::uint32_t>(inputVec.size());

    // Init closest sum index of combsVec
    bool needsToOutPutClosestComb      = pOutClosestCombIndices != nullptr;
    std::uint64_t outClosetCombIndices = 0;

    // Start the calculation scope
    constexpr auto kInvalidCombSize = GetInvalidValue(MAX_COMB_NUM_BIT);
    {
        // Init optimized inputData
        std::vector<float> optimizedInputDataVec;
        optimizedInputDataVec.reserve(inputSize);
        for (auto& input : inputVec)
        {
            auto dInput = static_cast<double>(input->GetFixedData());
            optimizedInputDataVec.emplace_back(static_cast<float>(dInput / unitScale));
        }
        // Init optimized target data
        auto optimizedTargetData = static_cast<float>(static_cast<double>(targetValue) / unitScale);

        // This set is used for store hash values of each comb. Use our own hash function,
        // as the value we store is already hashed.
        struct Hasher
        {
            std::size_t operator()(const std::size_t& value) const { return value; }
        };
        std::unordered_set<std::size_t, Hasher, std::equal_to<std::size_t>> combTable;

        std::vector<TempCombination<UseHashTable>> combsVec;
        auto& init = combsVec.emplace_back();
        for (auto& inputData : optimizedInputDataVec)
        {
            init.remainValue += inputData;
        }

        if constexpr (UseHashTable)
        {
#ifdef M_DEBUG
            {
                std::stringstream msg;
                msg << "Current pTarget is: " << pTarget->GetDesc() << ", Use Hash table!"
                    << std::endl;
                std::cout << msg.str();
            }
#endif // M_DEBUG

            // Add the init hash
            combTable.insert(init.hash);
        }

        // Init closet index of combsVec
        std::uint32_t closetCombIndexOfVec = 0;

        // Loop over all input data
        for (std::uint32_t i = 0; i < inputSize; ++i)
        {
            auto combSize      = combsVec.size();
            auto& inputData    = optimizedInputDataVec[i];
            auto& inputBitMask = k_inputIndexBitMask[i];

            auto currentCombSize = combSize;
            // Keep tracking previous combinations.
            for (std::uint32_t j = 0; j < combSize; ++j)
            {
                // Keep substracting until the result is still larger than the target.
                auto& comb = combsVec[j];

                // Filter out same input value to be added with the same combination.
                std::size_t currentHash = 0;
                if constexpr (UseHashTable)
                {
                    currentHash = comb.hash;
                    HashCombine(currentHash, inputData);
                    auto exist = combTable.find(currentHash) != combTable.end();
                    if (exist)
                        continue;
                }

                // Compute the difference
                auto diff = comb.remainValue - inputData;

                // Get current flag
                auto combFlag = comb.bitFlag;

                // Our gloal is to find the subset that is closest and also greater equal to the
                // target.
                if (diff >= optimizedTargetData)
                {
                    // If the data is too large throw an error.
                    if (currentCombSize + 1 >= kInvalidCombSize)
                    {
                        errorStr += "Size of input is too large, try to use fewer inputs!\n";
                        return false;
                    }

                    // Combine the previous result with current index as another new result.
                    auto& newComb =
                        combsVec.emplace_back(diff, combFlag | inputBitMask, currentHash);

                    if constexpr (UseHashTable)
                    {
                        // Insert current hash
                        combTable.insert(newComb.hash);
                    }

                    // Get the optimal result
                    if (needsToOutPutClosestComb)
                    {
                        auto& closestComb = combsVec[closetCombIndexOfVec];
                        if (newComb.remainValue < closestComb.remainValue)
                        {
                            // The closet index of vec is the newComb index of combsVec
                            closetCombIndexOfVec = static_cast<std::uint32_t>(currentCombSize);
                        }
                    }

                    ++currentCombSize;
                }
            }
        }

#ifdef M_DEBUG
        {
            std::stringstream msg;
            msg << "Current pTarget is: " << pTarget->GetDesc()
                << " Size of combs: " << combsVec.size() << std::endl;
            std::cout << msg.str();
        }
#endif // M_DEBUG

        auto maxPickedIndices = GetMaxPickedIndices(inputSize);
        // Out put closest comb
        if (needsToOutPutClosestComb)
        {
            auto& closestComb = combsVec[closetCombIndexOfVec];
            // Reverse "remove" to "pick" indices.
            outClosetCombIndices = ~closestComb.bitFlag;
            // Remove the unused bits.
            outClosetCombIndices &= maxPickedIndices;

            *pOutClosestCombIndices = outClosetCombIndices;
        }

        // Out put all combs vec
        if (pOutAllCombIndicesVec)
        {
            auto& outCombVec = *pOutAllCombIndicesVec;
            outCombVec.clear();
            auto size = combsVec.size();
            outCombVec.reserve(size);
            for (auto& comb : combsVec)
            {
                auto indices = ~comb.bitFlag;
                indices &= maxPickedIndices;

                auto diff = comb.remainValue - optimizedTargetData;
                if (diff < 0)
                {
                    // Make sure all indices have been picked when comb sum can not finish the
                    // target.
                    if (indices != maxPickedIndices)
                    {
                        errorStr +=
                            "All indices must be picked, when comb sum is not enough to finish the "
                            "pTarget\n";
                        assert(false);
                        return false;
                    }
                }

                if (pRefMinExeedSum)
                {
                    // Skip the diff that is already greater than ref min exeed.
                    if (diff > *pRefMinExeedSum)
                        continue;
                }

                outCombVec.emplace_back(comb.remainValue, diff, indices);
            }

            // Release the memory
            combsVec.clear();

            // Sort by ascending order of diff
            std::sort(std::execution::par, outCombVec.begin(), outCombVec.end(),
                [](const OutputCombination& a, const OutputCombination& b) -> bool {
                    return a.diff < b.diff;
                });

#ifdef M_DEBUG
            {
                std::stringstream msg;
                msg << "Current pTarget is: " << pTarget->GetDesc()
                    << " Size of OutAllCombIndicesVec: " << outCombVec.size() << std::endl;
                std::cout << msg.str();
            }
#endif // M_DEBUG
        }

    } // End calculation scope

    return true;
}

} // namespace Algorithms

namespace Solutions
{

bool ConfigResultListByResults(ResultDataList& resultList, std::string& errorStr)
{
    const auto& resultVec    = resultList.m_selectedInputs;
    const auto resultVecSize = resultVec.size();
    // Count all finished result and check if every target's calculation is finished.
    resultList.m_numfinished = 0;
    for (int i = 0; i < resultVecSize; ++i)
    {
        auto& result = resultVec[i];

        if (result.isFinished())
        {
            ++resultList.m_numfinished;
        }
        else if (i < resultVecSize - 1)
        {
            // The last one can be unfinished.

            errorStr += "Every pTarget's calculation needs to be finished!\n";
            assert(false);
            return false;
        }
    }

    // Sum all results
    resultList.m_combiSum  = 0;
    resultList.m_remainSum = 0;
    resultList.m_exeedSum  = 0;
    for (auto& result : resultVec)
    {
        resultList.m_combiSum += result.m_sum;

        if (result.m_isExceeded)
            resultList.m_exeedSum += result.m_difference;
        else
            resultList.m_remainSum += result.m_difference;
    }

    return true;
}

template <typename T>
void ParseIndicesToUserData(
    const std::vector<const UserData*>& inputVec, std::uint64_t indices, T callBack)
{
    const auto currentInputSize = inputVec.size();
    for (std::uint32_t i = 0; i < currentInputSize; ++i)
    {
        // Starting from lowest bit to see if it is on.
        if (indices & 1)
        {
            if (!callBack(inputVec[i]))
                break;
        }
        // Remove the lowest bit
        indices >>= 1;
    }
}
bool ConfigResultData(const std::vector<const UserData*>& inputVec, std::uint64_t selectedIndices,
    const UserData* pTarget, ResultData& result, std::string& errorStr)
{

    if (!pTarget)
    {
        errorStr += "Target can not be null!\n";
        assert(false);
        return false;
    }

    result.m_pTarget = pTarget;
    result.m_sum     = 0;

    ParseIndicesToUserData(inputVec, selectedIndices, [&](const UserData* data) -> bool {
        result.m_sum += data->GetOriginalData();
        result.m_combination.emplace_back(data);
        return true;
    });

    // Config the difference
    {
        const auto targetData = pTarget->GetOriginalData();
        result.m_difference =
            result.m_sum >= targetData ? result.m_sum - targetData : targetData - result.m_sum;

        result.m_isExceeded = result.m_sum > targetData;
    }

    return true;
}

bool SolutionBestOfEachTarget(const std::vector<const UserData*>& inputVec,
    const std::vector<const UserData*>& targetVec, ResultDataList& resultList,
    std::string& errorStr)
{
    auto& resultVec = resultList.m_selectedInputs;
    resultVec.clear();
    // Make a copy
    auto orderedInputVec = inputVec;
    // Sort the input data by descending order
    std::sort(std::execution::par, orderedInputVec.begin(), orderedInputVec.end(),
        [&](const UserData* a, const UserData* b) -> bool { return *a > *b; });

    for (const auto* pTarget : targetVec)
    {
        if (orderedInputVec.empty())
            break;

        std::uint64_t closestCombIndices = 0;
        if (!Algorithms::FindSumToTargetBackTracking(
                orderedInputVec, pTarget, resultList.m_unitScale, errorStr, &closestCombIndices))
            return false;

        // After get the opt, we config the result.
        auto& result = resultVec.emplace_back();
        ConfigResultData(orderedInputVec, closestCombIndices, pTarget, result, errorStr);

        // Remove elements from inputVec that has been picked by current result.
        const auto& combination = result.m_combination;
        orderedInputVec.erase(
            std::remove_if(std::execution::par, orderedInputVec.begin(), orderedInputVec.end(),
                [&](auto& inputData) {
                    return std::find(combination.begin(), combination.end(), inputData) !=
                        combination.end();
                }),
            orderedInputVec.end());
    }

    resultList.m_remainInputs.clear();
    if (!resultVec.empty())
    {
        const auto& lastResult = resultVec.back();
        if (lastResult.isFinished())
        {
            // Copy reamin inputs
            resultList.m_remainInputs = orderedInputVec;
        }
        else
        {
            // Make sure we have used all input data.
            if (!orderedInputVec.empty())
            {
                errorStr += "Current input vec should be empty now!\n";
                assert(false);
                return false;
            }
        }
    }

    return ConfigResultListByResults(resultList, errorStr);
}

// TODO: Need research to find out a better algorithm. Such as A* path finding or simple Dijkstra.
bool SolutionBestOverral(const std::vector<const UserData*>& inputVec,
    const std::vector<const UserData*>& targetVec, ResultDataList& resultList,
    std::string& errorStr)
{
    // Get the referenced solution result.
    std::uint32_t refMaxNumFinishedTarget = 0;
    float refMinExeedSum                  = std::numeric_limits<float>::max();
    {
        if (!SolutionBestOfEachTarget(inputVec, targetVec, resultList, errorStr))
            return false;

        // We only need to calculate futher if m_numfinished of resultList is greater than 1
        if (resultList.m_numfinished <= 1)
            return true;

        refMaxNumFinishedTarget = resultList.m_numfinished;
        refMinExeedSum =
            static_cast<float>(static_cast<double>(resultList.m_exeedSum) / resultList.m_unitScale);
    }

    const auto targetSize = targetVec.size();
    const auto inputSize  = inputVec.size();
    if (inputSize == 0)
    {
        errorStr += "Input size should not be 0!\n";
        assert(false);
        return false;
    }

    // Make a copy
    auto orderedInputVec = inputVec;
    {
        // Sort the input data by descending order
        std::sort(std::execution::par_unseq, orderedInputVec.begin(), orderedInputVec.end(),
            [&](const UserData* a, const UserData* b) -> bool { return *a > *b; });
#if USE_REMOVE_DUPLICATES
        // Adjacent find duplicated inputs and adding a small offset to duplicated inputs to avoid
        // upper_bound collisions.
        {
            std::size_t currentIndex = 0, nextIndex = 1,
                        prevDuplicatedIndex = std::numeric_limits<std::size_t>::max();
            struct DuplicateRange
            {
                DuplicateRange(std::size_t startIndex, std::size_t endIndex) :
                    startIndex(startIndex), endIndex(endIndex)
                {
                }
                std::size_t startIndex = 0;
                std::size_t endIndex   = 0;
            };
            std::vector<DuplicateRange> duplicateVec;
            for (; nextIndex < inputSize; ++nextIndex, ++currentIndex)
            {
                auto& currentData = orderedInputVec[currentIndex];
                auto& nextData    = orderedInputVec[nextIndex];

                if (currentData->GetOriginalData() == nextData->GetOriginalData())
                {
                    if (currentIndex == prevDuplicatedIndex)
                    {
                        if (!duplicateVec.empty())
                        {
                            auto& lastRange    = duplicateVec.back();
                            lastRange.endIndex = nextIndex;
                        }
                    }
                    else
                    {
                        duplicateVec.emplace_back(currentIndex, nextIndex);
                    }
                    prevDuplicatedIndex = nextIndex;
                }
            }

            // Small offset to add to the input to avoid duplicates.
            int valueDiff = static_cast<int>(resultList.m_unitScale) / 1000;
            for (auto& duplicate : duplicateVec)
            {
                auto sizeOfRange = duplicate.endIndex - duplicate.startIndex;
                for (int i = 0; i < sizeOfRange; ++i)
                {
                    auto* pInput     = orderedInputVec[i + duplicate.startIndex];
                    pInput->m_offset = (i + 1) * valueDiff;
                }
            }
        }
#endif
    }

    // Firstly, get all possible combinations of each target.
    std::vector<std::vector<Algorithms::OutputCombination>> allCombVec(targetSize);
    {
        // Make error handling thread safe
        std::vector<std::string> allErrorStrVec(targetSize);
        std::atomic<bool> hasError = false;

        // Iterate indices rather than vector
        std::vector<int> targetIndices(targetSize);
        for (int i = 0; i < targetSize; ++i)
            targetIndices[i] = i;

        static constexpr bool s_kUseHashTable = !USE_REMOVE_DUPLICATES;

        // Parallel for each
        std::for_each(std::execution::par_unseq, targetIndices.begin(), targetIndices.end(),
            [&](int targetIndex) {
                hasError = hasError ||
                    !Algorithms::FindSumToTargetBackTracking<s_kUseHashTable>(orderedInputVec,
                        targetVec[targetIndex], resultList.m_unitScale, allErrorStrVec[targetIndex],
                        nullptr, &allCombVec[targetIndex], &refMinExeedSum);
            });

        // Sync the error results
        if (hasError)
        {
            for (int i = 0; i < targetSize; ++i)
            {
                auto& currentError = allErrorStrVec[i];
                if (currentError.empty())
                    continue;
                errorStr += allErrorStrVec[i];
            }
            return false;
        }
    }

    // Secondly, walk through all combs to find the best result
#ifdef M_DEBUG
    std::atomic<std::size_t> pathSize = 0;
#endif // M_DEBUG

    std::vector<std::uint32_t> bestIndicesResult;
    constexpr auto kInvalidIndex = GetInvalidValue(sizeof(std::uint32_t) * 8);
    std::mutex recordResultMutex;

    // Config max picked table by removing the bits from left most to match number of inputs
    const auto maxPickedIndices = Algorithms::GetMaxPickedIndices(inputSize);

    if (!allCombVec.empty())
    {
        // All combs Traversal, from combs of first target for parallel execution.
        auto& firstCombVec = allCombVec.front();

        // Upper bound returns the right most index of comb that has diff greater than
        // refMinExeedSum, which we don't need to calculate anymore.
        auto endIndexFirstComb =
            std::upper_bound(firstCombVec.begin(), firstCombVec.end(), refMinExeedSum,
                [](float minSum, const Algorithms::OutputCombination& comb) -> bool {
                    return comb.diff > minSum;
                }) -
            firstCombVec.begin();

        // Gen indices, as we need index in std::for_each
        std::vector<std::uint32_t> firstCombIndices;
        firstCombIndices.reserve(endIndexFirstComb);
        for (auto i = 0; i < endIndexFirstComb; ++i)
            firstCombIndices.emplace_back(i);

        std::for_each(std::execution::par, firstCombIndices.begin(), firstCombIndices.end(),
            [&](auto& index) -> void {
                // Avoid to copy vector, we use a stack vector to store results
                std::vector<std::uint32_t> stackIndexResult(targetSize);

                // Invalidate all indices of stackIndexResult
                std::fill(stackIndexResult.begin(), stackIndexResult.end(), kInvalidIndex);

                auto outputPath = [&](std::uint32_t maxNumFinishedTarget, float exeedSum) -> void {
                    bool needToRecord = maxNumFinishedTarget > refMaxNumFinishedTarget ||
                        (exeedSum < refMinExeedSum &&
                            maxNumFinishedTarget >= refMaxNumFinishedTarget);

                    if (needToRecord)
                    {
                        std::lock_guard lock(recordResultMutex);
                        refMaxNumFinishedTarget = maxNumFinishedTarget;
                        refMinExeedSum          = exeedSum;

                        bestIndicesResult = stackIndexResult;
                    }
#ifdef M_DEBUG
                    ++pathSize;
#endif // M_DEBUG
                };

                auto walkRecursion = LambdaCombinator([&](auto& selfLambda,
                                                          std::uint64_t pickedIndices,
                                                          std::uint32_t targetIndex,
                                                          float prevExeed) -> void {
                    // We reached the end of the tree
                    if (pickedIndices == maxPickedIndices || targetIndex >= targetSize)
                    {
                        outputPath(targetIndex, prevExeed);
                        return;
                    }
                    else if (targetIndex + 1 > refMaxNumFinishedTarget)
                    {

                        // At this point, we still have unpicked indices and still have targets to
                        // finish.

                        // We make a prediction to see remianing indices are enough to finish this
                        // target.
                        auto remianIndices            = pickedIndices ^ maxPickedIndices;
                        std::uint64_t sum             = 0;
                        bool canFinish                = false;
                        const auto& currentTargetData = targetVec[targetIndex]->GetOriginalData();
                        ParseIndicesToUserData(
                            orderedInputVec, remianIndices, [&](const UserData* pUserData) {
                                sum += pUserData->GetFixedData();
                                if (sum >= currentTargetData)
                                {
                                    canFinish = true;
                                    return false;
                                }
                                return true;
                            });

                        // if the prediction we have made can not finish the target, then we just
                        // end this path.
                        if (!canFinish)
                        {
                            outputPath(targetIndex, prevExeed);
                            return;
                        }
                        else
                        {
                            auto fSum = static_cast<float>(
                                static_cast<double>(sum) / resultList.m_unitScale);

                            // if we can finish this target, update the refs.
                            std::lock_guard lock(recordResultMutex);
                            refMaxNumFinishedTarget = targetIndex;

                            refMinExeedSum = prevExeed + fSum;
                        }
                    }

                    const auto& currentCombVec = allCombVec[targetIndex];
                    // Skip if exeed sum already greater than the sum of previous full path.
                    auto endCombIndex =
                        std::upper_bound(currentCombVec.begin(), currentCombVec.end(), prevExeed,
                            [&](float prevSum, const Algorithms::OutputCombination& comb) -> bool {
                                return comb.diff + prevSum > refMinExeedSum;
                            }) -
                        currentCombVec.begin();

                    bool picked = false;
                    for (std::uint32_t i = 0; i < endCombIndex; ++i)
                    {
                        auto& currentComb = currentCombVec[i];

                        // Skip already picked indices and comb that can not finish the target
                        if ((pickedIndices & currentComb.selectedIndices) != 0 ||
                            currentComb.diff < 0.0f)
                            continue;

                        // Save current result.
                        stackIndexResult[targetIndex] = i;
                        picked                        = true;

                        selfLambda(pickedIndices | currentComb.selectedIndices, targetIndex + 1,
                            prevExeed + currentComb.diff);

                        // Reset current result.
                        stackIndexResult[targetIndex] = kInvalidIndex;
                    }

                    if (!picked)
                    {
                        outputPath(targetIndex, prevExeed);
                    }
                });

                // Pick the indices of each comb from first comb vec.
                auto& comb = firstCombVec[index];
                if (comb.diff < refMinExeedSum)
                {
                    stackIndexResult[0] = index;
                    walkRecursion(comb.selectedIndices, 1, comb.diff);
                }
            });
    }

#ifdef M_DEBUG
    std::cout << "Total number of path: " << pathSize << std::endl;
#endif // M_DEBUG

    // If did not find any path that is better then ref solution we out put the ref result.
    if (bestIndicesResult.empty())
        return true;

    // Finally, we output the results to resultList.
    {
        auto& resultVec = resultList.m_selectedInputs;
        resultVec.clear();
        resultList.m_remainInputs.clear();

        // Config all finished targets.
        std::uint64_t allPickedIndices = 0;
        for (std::uint32_t targetIndex = 0; targetIndex < targetSize; ++targetIndex)
        {
            const auto& currentCombsVec = allCombVec[targetIndex];
            const auto& pCurrentTarget  = targetVec[targetIndex];
            const auto indexOfCombsVec  = bestIndicesResult[targetIndex];

            // We reached the end of the finished target.
            if (targetIndex >= refMaxNumFinishedTarget)
            {
                // Check if we made anything wrong with recursion.
                if (indexOfCombsVec != kInvalidIndex)
                {
                    errorStr += "Invalid bestIndicesResult\n";
                    assert(false);
                    return false;
                }

                // Check if there is any input remians.
                if (allPickedIndices != maxPickedIndices)
                {
                    // Put all remian indices to a new result
                    std::uint64_t remainIndices = allPickedIndices ^ maxPickedIndices;
                    auto& result                = resultVec.emplace_back();
                    ConfigResultData(
                        orderedInputVec, remainIndices, pCurrentTarget, result, errorStr);

                    allPickedIndices |= remainIndices;
                }

                break;
            }

            // Now the index of combs vec should be valid
            if (indexOfCombsVec == kInvalidIndex)
            {
                errorStr += "Invalid indexOfCombsVec\n";
                assert(false);
                return false;
            }

            const auto& currentSelectedComb = currentCombsVec[indexOfCombsVec];
            auto currentSelectedIndices     = currentSelectedComb.selectedIndices;

            // Append selected indices.
            allPickedIndices |= currentSelectedIndices;
            auto& result = resultVec.emplace_back();
            ConfigResultData(
                orderedInputVec, currentSelectedIndices, pCurrentTarget, result, errorStr);
        }

        // Still has reamins, that means all targets are finished.
        if (allPickedIndices != maxPickedIndices)
        {
            std::uint64_t remainIndices = allPickedIndices ^ maxPickedIndices;

            ParseIndicesToUserData(
                orderedInputVec, remainIndices, [&](const UserData* pData) -> bool {
                    resultList.m_remainInputs.emplace_back(pData);
                    return true;
                });
        }
    }

    return ConfigResultListByResults(resultList, errorStr);
}
} // namespace Solutions
} // namespace

namespace JUtils
{
bool Calculator::Init(UnitScale::Values unitScale)
{
    if (!UnitScale::IsValid(unitScale))
        return false;

    return true;
}
bool JUtils::Calculator::LoadInputData(const char* fileName)
{
    return loadUserData(fileName, m_inputDataVec);
}

bool Calculator::LoadTargetData(const char* fileName)
{
    return loadUserData(fileName, m_targetDataList);
}

bool Calculator::Run(ResultDataList& resultList, std::string& errorStr, Solution solution)
{
    // Init result list
    resultList.m_unitScale = m_unitScale;

    // Init input vector of pointers, avoid extra copies
    std::vector<const UserData*> inputVec;
    inputVec.reserve(m_inputDataVec.GetList().size());
    for (auto& data : m_inputDataVec.GetList())
        inputVec.emplace_back(&data);

    // Init target lists of pointers
    std::vector<const UserData*> targetVec;
    targetVec.reserve(m_targetDataList.GetList().size());
    for (auto& data : m_targetDataList.GetList())
        targetVec.emplace_back(&data);

    switch (solution)
    {
    case Solution::BestOfEachTarget:
        return Solutions::SolutionBestOfEachTarget(inputVec, targetVec, resultList, errorStr);
    case Solution::OverallBest:
        return Solutions::SolutionBestOverral(inputVec, targetVec, resultList, errorStr);
    case Solution::Test:
    {
        return true;
    }
    default:
        return false;
    }
}
bool Calculator::loadUserData(const char* fileName, UserDataList& dataList)
{
    if (!UserDataList::ReadFromFile(fileName, m_unitScale, dataList))
        return false;

    if (dataList.GetList().empty())
        return false;

    return true;
}
} // namespace JUtils
