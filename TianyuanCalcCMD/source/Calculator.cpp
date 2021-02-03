//
// Author: Jason Huang(jasonhuang1988@gmail.com) 2021
//

#include "pch.h"

#include "Calculator.h"

#include <cmath>
#include <execution>
#include <functional>
#include <list>
#include <set>

namespace
{
using namespace JUtils;

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

void FindClosestSumToTarget(const std::vector<const UserData*>& inputDataList,
    const UserData* pTarget, ResultData& outResult, bool forceGreaterEq = false)
{
    assert(pTarget);

    auto targetValue = pTarget->GetData();
    int inputSize    = static_cast<int>(inputDataList.size());

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
        currentSum += inputDataList[rightIndex]->GetData();

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
                auto toSub =
                    inputDataList[leftIndex]->GetData() + inputDataList[rightIndex]->GetData();
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
        auto& data = inputDataList[i];
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

} // namespace

namespace JUtils
{
bool JUtils::Calculator::LoadInputData(const char* fileName, std::uint64_t unitScale)
{
    return UserDataList::ReadFromFile(fileName, UnitScale::k10K, m_inputDataVec);
}

bool Calculator::LoadTargetData(const char* fileName, std::uint64_t unitScale)
{
    return UserDataList::ReadFromFile(fileName, UnitScale::k10K, m_targetDataList);
}

bool Calculator::Run(ResultDataList& resultList, std::string& errorStr)
{
    // Init result list
    auto& resultVec = resultList.m_list;
    resultVec.clear();
    resultList.m_unitScale = m_inputDataVec.GetUnitScale();

    if (m_inputDataVec.GetUnitScale() != m_targetDataList.GetUnitScale())
    {
        errorStr += " Unit scale of both target and input lists are not the same\n";
        return false;
    }

    // Init input vector of pointers, avoid extra copies
    std::vector<const UserData*> inputVec;
    inputVec.reserve(m_inputDataVec.GetList().size());
    for (auto& data : m_inputDataVec.GetList())
        inputVec.emplace_back(&data);

    // Sort inputVec inorder to use binary search
    {
        // Use pointer to compare the addresses, as input data might be the same
        std::sort(std::execution::par, inputVec.begin(), inputVec.end(),
            [](const UserData* a, const UserData* b) -> bool { return a < b; });
    }

    // Init target lists of pointers
    std::vector<const UserData*> targetVec;
    targetVec.reserve(m_targetDataList.GetList().size());
    for (auto& data : m_targetDataList.GetList())
        targetVec.emplace_back(&data);

    auto targetSize = targetVec.size();

#if 0
        {
            static constexpr auto testLen = 1;
            std::vector<UserData> testVec;
            for (int i = 1; i <= testLen; ++i)
            {
                testVec.emplace_back(std::to_wstring(i).c_str(), i * 0);
            }

            std::vector<const UserData*> testPVec;
            for (auto& data : testVec)
                testPVec.emplace_back(&data);

            UserData testTarget(L"Test Target", 0);

            SumResults testResult;
            FindClosestSumToTarget(testPVec, &testTarget, testResult);
        }
#endif // 0

    // Get best sum combinations of each target.
    {
        // Make a copy
        auto currentInputVec = inputVec;

        for (int i = 0; i < targetSize; ++i)
        {
            if (currentInputVec.empty())
                break;

            auto& target = targetVec[i];
            auto& result = resultVec.emplace_back();

            FindClosestSumToTarget(currentInputVec, target, result, true);

            // Remove elements from inputVec that has been picked by current result.
            currentInputVec.erase(std::remove_if(currentInputVec.begin(), currentInputVec.end(),
                                      [&](auto& inputData) {
                                          return std::binary_search(result.m_combination.begin(),
                                              result.m_combination.end(), inputData);
                                      }),
                currentInputVec.end());
        }

        // Make sure we have used all input data.
        assert(currentInputVec.empty());

        // Check if every target that calculated is finished.
        for (int i = 0; i < resultVec.size(); ++i)
        {
            auto& result = resultVec[i];

            // The last one can be unfinished.
            if (i < resultVec.size() - 1)
                assert(result.m_isExceeded);
        }
    }

    // Sum all results
    resultList.m_combiSum = 0;
    resultList.m_remainSum = 0;
    resultList.m_exeedSum = 0;
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
} // namespace JUtils
