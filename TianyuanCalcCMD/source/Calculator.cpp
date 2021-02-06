//
// Author: Jason Huang(jasonhuang1988@gmail.com) 2021
//

#include "pch.h"

#include "Calculator.h"
#include "Utils.h"

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
std::vector<ResultData> FindAllSumToTarget(
    const std::vector<const UserData*>& inputDataList, const UserData* pTarget)
{
    assert(pTarget);

    std::vector<ResultData> outResults;

    auto targetValue = pTarget->GetData();
    int inputSize    = static_cast<int>(inputDataList.size());

    // Recursive lambda
    std::function<void(int, const std::list<int>&)> sumRecursive =
        [&](int currentInputIndex, const std::list<int>& partialIndexList) -> void {
        std::uint64_t sum = 0;
        for (auto& index : partialIndexList)
            sum += inputDataList[index]->GetData();

        if (sum >= targetValue)
        {
            // Found, so all partial numbers are the result.
            auto& result = outResults.emplace_back();

            result.m_pTarget    = pTarget;
            result.m_difference = sum - targetValue;

            result.m_combination.reserve(partialIndexList.size());
            for (auto& index : partialIndexList)
            {
                result.m_combination.emplace_back(inputDataList[index]);
            }

            return;
        }

        for (auto i = currentInputIndex; i < inputSize; ++i)
        {
            // Copy partial and push current data to the end
            auto partial_rec = partialIndexList;
            partial_rec.emplace_back(i);

            sumRecursive(i + 1, partial_rec);
        }
    };

    std::list<int> tempList;
    sumRecursive(0, tempList);

    return outResults;
}

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
    // 2 ^ 6 = 64 values.
    static constexpr std::uint32_t k_bitsIndexToRemove = MAX_COMBO_SIZE_BITS;

    // To be the same as index of input to handle max size of combination.
    static constexpr std::uint32_t k_bitsListSize = k_bitsIndexToRemove;

    // The rest of int, we use it to store the index of comb list.
    static constexpr std::uint32_t k_bitsAllCombSize = 32 - k_bitsIndexToRemove - k_bitsListSize;

    Combination() :
        parentCombIndex(GetInvalidValue(k_bitsAllCombSize)),
        indexToRemove(GetInvalidValue(k_bitsIndexToRemove)),
        listSize(0),
        remainValue(0.0f),
        hash(0) {};

    Combination(float remainValue, std::uint32_t parentCombIndex, std::uint32_t indexToRemove,
        std::uint32_t listSize, std::size_t hash) :
        remainValue(0.0f),
        parentCombIndex(GetInvalidValue(k_bitsAllCombSize)),
        indexToRemove(GetInvalidValue(k_bitsIndexToRemove)),
        listSize(0),
        hash(0) {};

    float remainValue = 0.0f;

    std::uint32_t parentCombIndex : k_bitsAllCombSize;

    std::uint32_t indexToRemove : k_bitsIndexToRemove;
    std::uint32_t listSize : k_bitsListSize;

    std::size_t hash = 0;
};
// Make sure we use expected size of combination
static_assert(
    sizeof(Combination) == sizeof(float) + 1 * sizeof(std::uint32_t) + sizeof(std::size_t));

// Back tracking approach
bool FindClosestSumToTargetBackTracking(const std::vector<const UserData*>& inputVec,
    const UserData* pTarget, ResultData& outResult, std::string& errorStr,
    std::uint64_t unitScale = 1, bool getAllComb = false)
{
    assert(pTarget);

    auto targetValue = pTarget->GetData();
    auto inputSize   = static_cast<std::uint32_t>(inputVec.size());

    // Use our own copy for sorting the input values.
    auto orderedInputVec = inputVec;
    std::sort(std::execution::par, orderedInputVec.begin(), orderedInputVec.end(),
        [&](const UserData* a, const UserData* b) -> bool { return *a > *b; });

    // Init optimized inputData
    std::vector<float> optimizedInputDataVec;
    optimizedInputDataVec.reserve(inputSize);
    for (auto& input : orderedInputVec)
    {
        auto dInput = static_cast<double>(input->GetData());

        optimizedInputDataVec.emplace_back(static_cast<float>(dInput / unitScale));
    }
    // Init optimized target data
    auto optimizedTargetData = static_cast<float>(static_cast<double>(targetValue) / unitScale);

    // Calculate the total sum of every input.
    std::vector<Combination> combinations;

    // Helper function to compute the hash of a combination
    auto getCombHash = [&](const Combination& comb) -> std::size_t {
        size_t hash       = 0;
        auto* currentComb = &comb;
        for (std::uint32_t i = 0; i < comb.listSize; ++i)
        {

            HashCombine(hash, optimizedInputDataVec[currentComb->indexToRemove]);

            auto nextIndex = currentComb->parentCombIndex;
            if (nextIndex != GetInvalidValue(Combination::k_bitsAllCombSize))
                currentComb = &combinations[nextIndex];
            else
                break;
        }

        return hash;
    };
    // This set is used for store hash values of each comb.
    std::unordered_set<std::size_t> combTable;

    auto& init = combinations.emplace_back();
    for (auto& inputData : optimizedInputDataVec)
    {
        init.remainValue += inputData;
    }
    auto totalValue = init.remainValue;

    // Add the init hash
    combTable.insert(getCombHash(init));

    // Init result by copying the init combination.
    Combination opt = combinations.front();

    // Loop over all input data
    for (std::uint32_t i = 0; i < inputSize; ++i)
    {
        auto combSize  = combinations.size();
        auto inputData = optimizedInputDataVec[i];

        auto currentCombSize = combSize;
        // Keep tracking previous combinations.
        for (std::uint32_t j = 0; j < combSize; ++j)
        {
            // Keep substracting until the result is still larger than the target.
            auto& comb = combinations[j];

            // Combine previous hash of comb with current input data to filter duplicated combs
            auto currentHash = comb.hash;
            HashCombine(currentHash, inputData);
            auto exist = combTable.find(currentHash) != combTable.end();
            if (exist)
                continue;

            auto listSize = comb.listSize;
            auto diff     = comb.remainValue - inputData;

            // Our gloal is to find the subset that is closest and also greater equal to the target.
            if (diff >= optimizedTargetData)
            {
                // If the data is too large throw an error.
                if (currentCombSize + 1 >= GetInvalidValue(Combination::k_bitsAllCombSize))
                {
                    errorStr += "Size of input is too large, try to use fewer inputs!\n";
                    return false;
                }

                // Combine the previous result with current index as another new result.
                auto& newComb       = combinations.emplace_back();
                newComb.remainValue = diff;

                newComb.parentCombIndex = j;
                newComb.indexToRemove   = i;

                newComb.listSize = listSize + 1;

                // Use current hash
                newComb.hash = currentHash;
                combTable.insert(newComb.hash);

                ++currentCombSize;

                // Get the optimal result
                if (newComb.remainValue < opt.remainValue)
                {
                    opt = newComb;
                }
            }
        }
    }

#ifdef M_DEBUG
    std::cout << "Size of combs: " << combinations.size() << std::endl;
#endif // M_DEBUG

    // After get the opt, we config the result.
    outResult.m_pTarget = pTarget;
    outResult.m_sum     = 0;

    // Get the indices to remove vec. the remove vec will store all indices in descending order
    std::vector<std::uint32_t> indicesToRemove(opt.listSize);
    {
        Combination* currentComb = &opt;
        for (std::uint32_t i = 0; i < opt.listSize; ++i)
        {
            assert(currentComb->indexToRemove != GetInvalidValue(Combination::k_bitsIndexToRemove));

            indicesToRemove[i] = currentComb->indexToRemove;

            auto nextIndex = currentComb->parentCombIndex;
            if (nextIndex != GetInvalidValue(Combination::k_bitsAllCombSize))
                currentComb = &combinations[nextIndex];
            else
                break;
        }
    }

    for (std::uint32_t i = 0; i < inputSize; ++i)
    {
        // We skip the indices in opt, and the rest indices is the answer, binary search is valid
        // here as it stores preivious indices in descending order.
        if (std::binary_search(indicesToRemove.begin(), indicesToRemove.end(), i, std::greater()))
            continue;

        auto& data = orderedInputVec[i];
        outResult.m_sum += data->GetData();

        outResult.m_combination.emplace_back(data);
    }

    outResult.m_difference = outResult.m_sum >= targetValue ? outResult.m_sum - targetValue
                                                            : targetValue - outResult.m_sum;
    outResult.m_isExceeded = outResult.m_sum > targetValue;

    return true;
}

// Dynamic programming approach.
void FindClosestSumToTargetDP(const std::vector<const UserData*>& inputVec, const UserData* pTarget,
    ResultData& outResult, bool getAllComb = false)
{
    assert(false);
    // TODO: It seems like the best we could do is using dp table along with back tracking to get
    // sums. So we will not benefit a lot from this approach.

    assert(pTarget);
}

} // namespace Algorithms

namespace Solutions
{
bool SolutionBestOfEachTarget(ResultDataList& resultList, std::string& errorStr,
    const std::vector<const UserData*>& inputVec, const std::vector<const UserData*>& targetVec)
{
    auto& resultVec = resultList.m_list;
    auto targetSize = targetVec.size();

    // Make a copy
    auto currentInputVec = inputVec;

    for (int i = 0; i < targetSize; ++i)
    {
        if (currentInputVec.empty())
            break;

        auto& target = targetVec[i];
        auto& result = resultVec.emplace_back();

        if (!Algorithms::FindClosestSumToTargetBackTracking(
                currentInputVec, target, result, errorStr, resultList.m_unitScale))
            return false;

        auto& combination = result.m_combination;

        // Remove elements from inputVec that has been picked by current result.
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
    // TODO: use FindClosestSumToTargetDP to get all combinations, and pick each one combination of
    // the target,then updating a path to walk to the end of the target.

    return false;
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
        return Solutions::SolutionBestOfEachTarget(resultList, errorStr, inputVec, targetVec);
    case Solution::OverallBest:
        return false;
    case Solution::Test:
    {
        ResultData result;
        Algorithms::FindClosestSumToTargetBackTracking(inputVec, targetVec[0], result, errorStr);
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
