//
// Author: Jason Huang(jasonhuang1988@gmail.com) 2021
//

#include "JUtils/pch.h"

#include "TianyuanCalculator.h"

#include "JUtils/Algorithms.h"
#include "JUtils/Utils.h"

#include <bitset>
#include <cmath>
#include <execution>
#include <list>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

#include "vorbrodt/pool.hpp"

#define USE_REMOVE_DUPLICATES false
#define USE_STD_PAR_FOR_OVERALL_SOLUTION false
namespace
{
using namespace JUtils;
using namespace TianyuanCalc;

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

    // Init input desc
    std::uint64_t closestCombIndices = 0;
    std::vector<std::uint64_t> rawInputVec;
    Combination::InputSumToTargetDesc<std::uint64_t> inputDesc(
        rawInputVec, 0, resultList.m_unitScale, &closestCombIndices);

    for (const auto* pTarget : targetVec)
    {
        if (orderedInputVec.empty())
            break;

        // Calculate combination
        {
            // Reset closet comb indices
            closestCombIndices = 0;

            // Transform to input desc
            rawInputVec.resize(orderedInputVec.size());
            std::transform(orderedInputVec.begin(), orderedInputVec.end(), rawInputVec.begin(),
                [](const UserData* element) { return element->GetFixedData(); });

            // Assign target value
            inputDesc.targetValue = pTarget->GetOriginalData();

            if (!Combination::FindSumToTargetBackTracking(inputDesc, errorStr))
                return false;
        }

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

    const auto originalTargetSize = targetVec.size();
    const auto inputSize          = inputVec.size();
    if (inputSize == 0)
    {
        errorStr += "Input size should not be 0!\n";
        assert(false);
        return false;
    }

    // Find out if sum of all inputs can finish the targets.
    auto optimizedTargetSize = originalTargetSize;
    {
        std::uint64_t inputSum = 0;
        for (auto& input : inputVec)
        {
            inputSum += input->GetFixedData();
        }

        std::uint64_t targetSum = 0;
        for (std::size_t i = 0; i < originalTargetSize; ++i)
        {
            auto& target = targetVec[i];
            targetSum += target->GetFixedData();

            if (inputSum < targetSum)
            {
                optimizedTargetSize = i;
                break;
            }
        }

#ifdef M_DEBUG
        {
            auto str = FormatString("inputSum: ", inputSum, " targetSum: ", targetSum,
                "\noriginalTargetSize: ", originalTargetSize,
                " optimizedTargetSize: ", optimizedTargetSize);
            std::cout << str << std::endl;
        }
#endif // M_DEBUG
    }

    // We only need to calculate futher if optimizedTargetSize is greater than 1
    if (optimizedTargetSize <= 1)
        return true;

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
            int valueDiff = static_cast<int>(resultList.m_unitScale) / 100;
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
    std::vector<std::vector<Combination::OutputCombination>> allCombVec(optimizedTargetSize);
    {
        // Make error handling thread safe
        std::vector<std::string> allErrorStrVec(optimizedTargetSize);
        std::atomic<bool> hasError = false;

        static constexpr bool s_kUseHashTable = !USE_REMOVE_DUPLICATES;

        // Make the raw input vec, as we are using the same one.
        std::vector<std::uint64_t> rawInputVec(orderedInputVec.size());
        std::transform(orderedInputVec.begin(), orderedInputVec.end(), rawInputVec.begin(),
            [](const UserData* element) { return element->GetFixedData(); });

        auto taskFunc = [&](std::uint32_t targetIndex) {
            // Init input desc
            Combination::InputSumToTargetDesc<std::uint64_t> inputDesc(rawInputVec,
                targetVec[targetIndex]->GetOriginalData(), resultList.m_unitScale, nullptr,
                &allCombVec[targetIndex], refMinExeedSum);

            hasError = hasError ||
                !Combination::FindSumToTargetBackTracking<s_kUseHashTable>(
                    inputDesc, allErrorStrVec[targetIndex]);
        };

#if USE_STD_PAR_FOR_OVERALL_SOLUTION
        {
            // Iterate indices rather than vector
            std::vector<std::uint32_t> targetIndices(optimizedTargetSize);
            for (std::uint32_t i = 0; i < optimizedTargetSize; ++i)
                targetIndices[i] = i;
            std::for_each(
                std::execution::par_unseq, targetIndices.begin(), targetIndices.end(), taskFunc);
        }
#else
        {
            vorbrodt::thread_pool threadPool;
            for (std::uint32_t i = 0; i < optimizedTargetSize; ++i)
            {
                threadPool.enqueue_work(taskFunc, i);
            }
        }
#endif

        // Sync the error results
        if (hasError)
        {
            for (int i = 0; i < optimizedTargetSize; ++i)
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
    const auto maxPickedIndices = PickIndex::GetMaxPickedIndices(inputSize);

    if (!allCombVec.empty())
    {
        // All combs Traversal, from combs of first target for parallel execution.
        auto& firstCombVec = allCombVec.front();

        auto taskFunc = [&](auto& index) -> void {
            // Avoid to copy vector, we use a stack vector to store results
            std::vector<std::uint32_t> stackIndexResult(optimizedTargetSize);

            // Invalidate all indices of stackIndexResult
            std::fill(stackIndexResult.begin(), stackIndexResult.end(), kInvalidIndex);

            auto outputPath = [&](std::uint32_t maxNumFinishedTarget, float exeedSum) -> void {
                std::lock_guard lock(recordResultMutex);
                bool needToRecord = maxNumFinishedTarget > refMaxNumFinishedTarget ||
                    (exeedSum <= refMinExeedSum && maxNumFinishedTarget >= refMaxNumFinishedTarget);

                if (needToRecord)
                {
                    refMaxNumFinishedTarget = maxNumFinishedTarget;
                    refMinExeedSum          = exeedSum;

                    bestIndicesResult = stackIndexResult;
                }
#ifdef M_DEBUG
                ++pathSize;
#endif // M_DEBUG
            };

            auto walkRecursion =
                LambdaCombinator([&](auto& selfLambda, std::uint64_t pickedIndices,
                                     std::uint32_t targetIndex, float prevExeed) -> void {
                    // We reached the end of the tree
                    if (pickedIndices == maxPickedIndices || targetIndex >= optimizedTargetSize)
                    {
                        outputPath(targetIndex, prevExeed);
                        return;
                    }
                    else if (targetIndex + 1 > refMaxNumFinishedTarget)
                    {

                        // At this point, we still have unpicked indices and still have targets to
                        // finish. and num finished target might be greater

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
                        // else
                        //{
                        //    auto fSum = static_cast<float>(
                        //        static_cast<double>(sum) / resultList.m_unitScale);
                        //
                        //    // if we can finish this target, update the refs.
                        //    std::lock_guard lock(recordResultMutex);
                        //    refMaxNumFinishedTarget = targetIndex;
                        //
                        //    refMinExeedSum = prevExeed + fSum;
                        //}
                    }
                    else
                    {
                        // At this point, we still have unpicked indices and still have targets to
                        // finish. But num finished target can not be greater.

                        // We check prevExeed to see if we can skip
                        if (prevExeed > refMinExeedSum)
                        {
                            outputPath(targetIndex, prevExeed);
                            return;
                        }
                    }

                    const auto& currentCombVec = allCombVec[targetIndex];
                    // Skip if exeed sum already greater than the sum of previous full path.
                    const auto _refMinExeedSum = refMinExeedSum;
                    auto endCombIndex =
                        std::upper_bound(currentCombVec.begin(), currentCombVec.end(), prevExeed,
                            [&](float prevSum, const Combination::OutputCombination& comb) -> bool {
                                return comb.diff + prevSum > _refMinExeedSum;
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
        };

        // Upper bound returns the right most index of comb that has diff greater than
        // refMinExeedSum, which we don't need to calculate anymore.
        auto endIndexFirstComb = static_cast<std::uint32_t>(firstCombVec.size());

#if USE_STD_PAR_FOR_OVERALL_SOLUTION
        {
            // Gen indices, as we need index in std::for_each
            std::vector<std::uint32_t> firstCombIndices;
            firstCombIndices.reserve(endIndexFirstComb);
            for (std::uint32_t i = 0; i < endIndexFirstComb; ++i)
                firstCombIndices.emplace_back(i);

            std::for_each(
                std::execution::par, firstCombIndices.begin(), firstCombIndices.end(), taskFunc);
        }
#else
        {
            vorbrodt::thread_pool threadPool;
            for (std::uint32_t i = 0; i < endIndexFirstComb; ++i)
            {
                threadPool.enqueue_work(taskFunc, i);
            }
        }
#endif // USE_STD_PAR_FOR_OVERALL_SOLUTION
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
        for (std::uint32_t targetIndex = 0; targetIndex < optimizedTargetSize; ++targetIndex)
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

namespace TianyuanCalc
{
bool Calculator::Init(UnitScale::Values unitScale)
{
    if (!UnitScale::IsValid(unitScale))
        return false;

    return true;
}
bool Calculator::LoadInputData(const char* fileName, std::string& errorStr)
{
    return loadUserData(fileName, errorStr, m_inputDataVec);
}

bool Calculator::LoadTargetData(const char* fileName, std::string& errorStr)
{
    return loadUserData(fileName, errorStr, m_targetDataList);
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
    case Solution::UnorderedTarget:
    {
        // Sort by ascending order.
        std::sort(std::execution::par_unseq, targetVec.begin(), targetVec.end(),
            [&](const UserData* a, const UserData* b) -> bool { return *a < *b; });
        return Solutions::SolutionBestOverral(inputVec, targetVec, resultList, errorStr);
    }
    case Solution::Test:
    {
        return true;
    }
    default:
        return false;
    }
}
bool Calculator::loadUserData(const char* fileName, std::string& errorStr, UserDataList& dataList)
{
    if (!UserDataList::ReadFromFile(fileName, m_unitScale, errorStr, dataList))
        return false;

    if (dataList.GetList().empty())
        return false;

    return true;
}
} // namespace TianyuanCalc
