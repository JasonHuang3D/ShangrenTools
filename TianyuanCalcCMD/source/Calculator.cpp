//
// Author: Jason Huang(jasonhuang1988@gmail.com) 2021
//

#include "pch.h"

#include "Calculator.h"
#include "Utils.h"

#include <bitset>
#include <cmath>
#include <execution>
#include <functional>
#include <list>
#include <unordered_set>

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

    auto targetValue = pTarget->GetData();
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
        currentSum += inputData->GetData();

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
                auto toSub = inputVec[leftIndex]->GetData() + inputVec[rightIndex]->GetData();
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
        outResult.m_sum += data->GetData();
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

struct Combination
{
    Combination() {}
    Combination(float remainValue, std::uint64_t bitFlag, std::size_t hash) :
        remainValue(remainValue), bitFlag(bitFlag), hash(hash)
    {
    }

    float remainValue = 0.0f;

    // Bit flag represent the index of input vector that need to be removed
    std::uint64_t bitFlag = 0;
    std::size_t hash      = 0;
};

// Create our index bit mask at compile time
constexpr auto k_inputIndexBitMask =
    ArrayHelper::CreateArrayWithInitValues<std::uint64_t, MAX_INPUT_SIZE>(
        ArrayHelper::BitMaskGenerator);

// Back tracking approach
bool FindSumToTargetBackTracking(const std::vector<const UserData*>& inputVec,
    const UserData* pTarget, std::uint64_t unitScale, std::string& errorStr,
    std::vector<Combination>& outCombVec, std::uint32_t* pOutClosestCombIndex = nullptr)
{
    assert(pTarget);

    auto targetValue = pTarget->GetData();
    auto inputSize   = static_cast<std::uint32_t>(inputVec.size());
    outCombVec.clear();

    // Init optimized inputData
    std::vector<float> optimizedInputDataVec;
    optimizedInputDataVec.reserve(inputSize);
    for (auto& input : inputVec)
    {
        auto dInput = static_cast<double>(input->GetData());

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

    auto& init = outCombVec.emplace_back();
    for (auto& inputData : optimizedInputDataVec)
    {
        init.remainValue += inputData;
    }

    // Add the init hash
    combTable.insert(init.hash);

    // Init closest sum index of outCombVec
    bool needsToOutPutClosestComb  = pOutClosestCombIndex != nullptr;
    std::uint32_t closestCombIndex = 0;
    if (needsToOutPutClosestComb)
    {
        *pOutClosestCombIndex = closestCombIndex;
    }

    constexpr auto kInvalidCombSize = GetInvalidValue(MAX_COMB_NUM_BIT);

    // Loop over all input data
    for (std::uint32_t i = 0; i < inputSize; ++i)
    {
        auto combSize      = outCombVec.size();
        auto& inputData    = optimizedInputDataVec[i];
        auto& inputBitMask = k_inputIndexBitMask[i];

        auto currentCombSize = combSize;
        // Keep tracking previous combinations.
        for (std::uint32_t j = 0; j < combSize; ++j)
        {
            // Keep substracting until the result is still larger than the target.
            auto& comb = outCombVec[j];

            // Filter out same input value to be added with the same combination.
            auto currentHash = comb.hash;
            HashCombine(currentHash, inputData);
            auto exist = combTable.find(currentHash) != combTable.end();
            if (exist)
                continue;

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
                auto& newComb = outCombVec.emplace_back(diff, combFlag | inputBitMask, currentHash);

                // Insert current hash
                combTable.insert(newComb.hash);

                // Get the optimal result
                if (needsToOutPutClosestComb)
                {
                    auto& closest = outCombVec[closestCombIndex];
                    if (newComb.remainValue < closest.remainValue)
                    {
                        // newComb index is the end of outCombVec
                        closestCombIndex = static_cast<std::uint32_t>(currentCombSize);
                    }
                }

                ++currentCombSize;
            }
        }
    }

#ifdef M_DEBUG
    std::cout << "Size of combs: " << outCombVec.size() << std::endl;
#endif // M_DEBUG

    // Out put closest comb
    if (needsToOutPutClosestComb)
    {
        if (closestCombIndex >= outCombVec.size())
        {
            errorStr += "closestCombIndex is out of range of outCombVec\n";
            assert(false);
            return false;
        }
        *pOutClosestCombIndex = closestCombIndex;
    }

    return true;
}

} // namespace Algorithms

namespace Solutions
{
bool SolutionBestOfEachTarget(const std::vector<const UserData*>& inputVec,
    const std::vector<const UserData*>& targetVec, ResultDataList& resultList,
    std::string& errorStr)
{
    auto& resultVec = resultList.m_list;
    // Make a copy
    auto currentInputVec = inputVec;
    // Sort the input data by descending  order
    std::sort(std::execution::par, currentInputVec.begin(), currentInputVec.end(),
        [&](const UserData* a, const UserData* b) -> bool { return *a > *b; });

    std::vector<Algorithms::Combination> allSumsOfTarget;

    for (auto& target : targetVec)
    {
        if (currentInputVec.empty())
            break;

        auto targetData = target->GetData();

        std::uint32_t closestCombIndex = 0;
        if (!Algorithms::FindSumToTargetBackTracking(currentInputVec, target,
                resultList.m_unitScale, errorStr, allSumsOfTarget, &closestCombIndex))
            return false;

        // After get the opt, we config the result.
        auto& result     = resultVec.emplace_back();
        result.m_pTarget = target;
        result.m_sum     = 0;

        auto& closestComb = allSumsOfTarget[closestCombIndex];

        // Get the indices of input to be added to result. The opt stores the indices bit flag that
        // needs to be removed.
        auto selectedIndices  = ~closestComb.bitFlag;
        auto currentInputSize = currentInputVec.size();
        for (std::uint32_t i = 0; i < currentInputSize; ++i)
        {
            // Starting from lowest bit to see if it is on.
            if (selectedIndices & 1)
            {
                auto& data = currentInputVec[i];
                result.m_sum += data->GetData();
                result.m_combination.emplace_back(data);
            }

            // Remove the lowest bit
            selectedIndices >>= 1;
        }

        // Config the difference
        result.m_difference =
            result.m_sum >= targetData ? result.m_sum - targetData : targetData - result.m_sum;

        result.m_isExceeded = result.m_sum > targetData;

        // Remove elements from inputVec that has been picked by current result.
        auto& combination = result.m_combination;
        currentInputVec.erase(std::remove_if(currentInputVec.begin(), currentInputVec.end(),
                                  [&](auto& inputData) {
                                      return std::find(combination.begin(), combination.end(),
                                                 inputData) != combination.end();
                                  }),
            currentInputVec.end());
    }

    if (!resultVec.empty())
    {
        auto& lastResult = resultVec.back();
        if (lastResult.isFinished())
        {
            // Copy reamin inputs
            resultList.m_remainInputs = currentInputVec;
        }
        else
        {
            // Make sure we have used all input data.
            if (!currentInputVec.empty())
            {
                errorStr += "Current input vec should be empty now!\n";
                assert(false);
                return false;
            }
        }
    }

    // Check if every target's calculation is finished.
    for (int i = 0; i < resultVec.size(); ++i)
    {
        auto& result = resultVec[i];

        // The last one can be unfinished.
        if (i < resultVec.size() - 1)
        {
            if (!result.isFinished())
            {
                errorStr += "Every target's calculation needs to be finished!\n";
                assert(false);
                return false;
            }
        }
    }

    // Sum all results
    resultList.m_combiSum  = 0;
    resultList.m_remainSum = 0;
    resultList.m_exeedSum  = 0;
    for (auto& result : resultList.m_list)
    {
        resultList.m_combiSum += result.m_sum;

        if (result.m_isExceeded)
            resultList.m_exeedSum += result.m_difference;
        else
            resultList.m_remainSum += result.m_difference;
    }

    return true;
}

bool SolutionBestOverral(ResultDataList& resultList, std::string& errorStr,
    const std::vector<const UserData*>& inputVec, const std::vector<const UserData*>& targetVec)
{
    auto& resultVec = resultList.m_list;
    auto targetSize = targetVec.size();

    for (int i = 0; i < targetSize; ++i)
    {
        auto& target = targetVec[i];
        auto& result = resultVec.emplace_back();

        auto& combination = result.m_combination;
    }

    return true;
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
    auto& resultVec = resultList.m_list;
    resultVec.clear();
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
        return false;
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
