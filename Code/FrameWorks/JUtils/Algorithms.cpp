//
// Author: Jason Huang(jasonhuang1988@gmail.com) 2021
//

#include "pch.h"

#include "Algorithms.h"

#include <unordered_set>
#include <execution>

#include "vorbrodt/pool.hpp"

namespace JUtils
{
std::size_t SelectCombination::RunSingleThread(std::size_t numElelment, std::size_t numSelect,
    std::function<void(const std::vector<std::size_t>&, std::size_t)> callBack)
{
    std::vector<std::size_t> stack(numSelect);
    std::size_t numCombs   = 0;
    const std::size_t kEnd = numElelment - numSelect;
    auto combRecursion =
        LambdaCombinator([&](auto& selfLambda, std::size_t offset, std::size_t stackIndex) -> void {
            if (stackIndex == numSelect)
            {
                if (callBack)
                {
                    callBack(stack, numCombs);
                }
                ++numCombs;
                return;
            }

            auto endIndex = kEnd + stackIndex;
            for (std::size_t i = offset; i <= endIndex; ++i)
            {
                stack[stackIndex] = i;
                selfLambda(i + 1, stackIndex + 1);
            }
        });

    combRecursion(0, 0);

#ifdef M_DEBUG
    {
        const auto correctNumCombs = GetNumOfSelectionComb(numElelment, numSelect);
        if (correctNumCombs != numCombs)
        {
            assert(false);
            throw std::runtime_error(" Result of SelectCombination is not correct! \n");
            return 0;
        }
    }
#endif // M_DEBUG

    return numCombs;
}

std::size_t SelectCombination::RunMultiThread(std::size_t numElelment, std::size_t numSelect,
    std::function<void(const std::vector<std::size_t>&, std::size_t)> callBack)
{
    std::vector<std::size_t> inputIndices(numElelment);
    for (std::size_t i = 0; i < numElelment; ++i)
        inputIndices[i] = i;

    // Compute num per thread envoke
    const auto totalNumCombs = GetNumOfSelectionComb(numElelment, numSelect);
    auto hardWareThreads     = std::thread::hardware_concurrency();
    auto numPerThread        = totalNumCombs / hardWareThreads;
    if (numPerThread == 0)
        numPerThread = totalNumCombs;

    std::size_t numCombs = 0;
    {

        std::vector<std::size_t> stack(numSelect);
        const std::size_t kEnd = numElelment - numSelect;

        struct Args
        {
            Args(const std::vector<std::size_t>& stack, std::size_t stackIndex) :
                stack(stack), stackIndex(stackIndex)
            {
            }
            std::vector<std::size_t> stack;
            std::size_t stackIndex = 0;
        };
        std::vector<Args> taskVecStack;
        std::vector<std::thread> threadVec;
        auto combRecursion = LambdaCombinator(
            [&](auto& selfLambda, std::size_t offset, std::size_t stackIndex) -> void {
                if (stackIndex == numSelect)
                {
                    if (callBack)
                    {
                        if (taskVecStack.size() == numPerThread)
                        {
                            // When stack size reached expected numPerThread, we move the stack to
                            // the task lambda.
                            threadVec.emplace_back([&, vec = std::move(taskVecStack)]() {
                                for (auto& arg : vec)
                                {
                                    callBack(arg.stack, arg.stackIndex);
                                }
                            });
                        }

                        taskVecStack.emplace_back(stack, numCombs);
                    }
                    ++numCombs;
                    return;
                }

                auto endIndex = kEnd + stackIndex;
                for (std::size_t i = offset; i <= endIndex; ++i)
                {
                    stack[stackIndex] = inputIndices[i];
                    selfLambda(i + 1, stackIndex + 1);
                }
            });

        combRecursion(0, 0);

        // if the stack is not empty, that means we have talling combs.
        if (!taskVecStack.empty())
        {
            threadVec.emplace_back([&, vec = std::move(taskVecStack)]() {
                for (auto& arg : vec)
                {
                    callBack(arg.stack, arg.stackIndex);
                }
            });
        }

        for (auto& thread : threadVec)
            thread.join();
    }
#ifdef M_DEBUG
    {
        if (totalNumCombs != numCombs)
        {
            assert(false);
            throw std::runtime_error(" Result of SelectCombination is not correct! \n");
            return 0;
        }
    }
#endif // M_DEBUG
    return numCombs;
}

void SelectCombination::SelectGroupComb::Run(bool useMultiThread)
{
    auto mainRecursion =
        LambdaCombinator([&](auto& mainRecLambda, std::vector<std::uint64_t>& resultStack,
                             std::uint32_t resultStackIndex, std::uint64_t selectedIndices,
                             std::uint32_t numInputs, vorbrodt::thread_pool* pThreadPool) -> void {
            if (resultStackIndex == m_numGroups || numInputs < m_numPerGroup)
            {
                m_callBack(resultStack);
                return;
            }

            std::uint64_t remainIndices = selectedIndices ^ m_maxSelectInputMask;
            if (remainIndices == 0)
            {
                m_callBack(resultStack);
                return;
            }

            std::vector<std::uint32_t> inputIndices;
            inputIndices.reserve(numInputs);
            std::uint32_t index = 0;
            while (remainIndices != 0)
            {
                if (remainIndices & 1)
                {
                    inputIndices.emplace_back(index);
                }
                remainIndices >>= 1;
                ++index;
            }

            std::vector<std::uint32_t> selectStack(m_numPerGroup);
            const std::uint32_t kEnd = numInputs - m_numPerGroup;
            auto selectRecursion     = LambdaCombinator(
                [&](auto& selectLambda, std::uint32_t offset, std::uint32_t stackIndex) -> void {
                    if (stackIndex == m_numPerGroup)
                    {
                        std::uint64_t newComb = 0;
                        for (std::uint32_t i = 0; i < m_numPerGroup; ++i)
                        {
                            auto selectIndex = selectStack[i];

                            auto indexOfInput = inputIndices[selectIndex];
                            newComb |= PickIndex::k_inputIndexBitMask[indexOfInput];
                        }

                        resultStack[resultStackIndex] = newComb;
                        if (pThreadPool != nullptr)
                        {
                            // We found the first group, and let the rest group to be pushed to
                            // thread pool.

                            // Capture by value
                            pThreadPool->enqueue_work([=]() mutable {
                                mainRecLambda(resultStack, resultStackIndex + 1,
                                    selectedIndices | newComb, kEnd, nullptr);
                            });

                            // Note: enqueue_work will copy resultStack
                            // pThreadPool->enqueue_work(mainRecLambda, resultStack,
                            //    resultStackIndex + 1, selectedIndices | newComb, kEnd, false);
                        }
                        else
                        {
                            mainRecLambda(resultStack, resultStackIndex + 1,
                                selectedIndices | newComb, kEnd, nullptr);
                        }
                        return;
                    }

                    auto endIndex = kEnd + stackIndex;
                    for (std::uint32_t i = offset; i <= endIndex; ++i)
                    {
                        selectStack[stackIndex] = i;
                        selectLambda(i + 1, stackIndex + 1);
                    }
                });

            selectRecursion(0, 0);
        }

        );

    // Invoke
    std::vector<std::uint64_t> resultStack(m_numGroups);
    if (useMultiThread)
    {
        vorbrodt::thread_pool threadPool;
        mainRecursion(resultStack, 0, 0, m_numInputs, &threadPool);
    }
    else
    {
        mainRecursion(resultStack, 0, 0, m_numInputs, nullptr);
    }
}

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

template <bool UseHashTable, std::uint32_t MaxCombSizeBits, typename TypeData>
bool Combination::FindSumToTargetBackTracking(
    const InputSumToTargetDesc<TypeData>& inputDesc, std::string& errorStr)
{
    if (inputDesc.targetValue == 0)
    {
        errorStr += "Target can not be 0\n";
        assert(false);
        return false;
    }

    if (inputDesc.unitScale == 0)
    {
        errorStr += "UnitScale can not be 0\n";
        assert(false);
        return false;
    }

    if (inputDesc.inputVec.empty())
    {
        errorStr += "InputVec can not be empty\n";
        assert(false);
        return false;
    }

    auto targetValue = inputDesc.targetValue;
    auto inputSize   = static_cast<std::uint32_t>(inputDesc.inputVec.size());

    // Init closest sum index of combsVec
    bool needsToOutPutClosestComb      = inputDesc.pOutClosestCombIndices != nullptr;
    std::uint64_t outClosetCombIndices = 0;

    // Start the calculation scope
    constexpr auto kInvalidCombSize = GetInvalidValue(MaxCombSizeBits);
    {
        // Init optimized inputData
        std::vector<float> optimizedInputDataVec;
        optimizedInputDataVec.reserve(inputSize);
        for (auto& input : inputDesc.inputVec)
        {
            auto dInput = static_cast<double>(input);
            optimizedInputDataVec.emplace_back(static_cast<float>(dInput / inputDesc.unitScale));
        }
        // Init optimized target data
        auto optimizedTargetData =
            static_cast<float>(static_cast<double>(targetValue) / inputDesc.unitScale);

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
            auto& inputBitMask = PickIndex::k_inputIndexBitMask[i];

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
            msg << "Current targetValue is: " << targetValue
                << " Size of combs: " << combsVec.size() << std::endl;
            std::cout << msg.str();
        }
#endif // M_DEBUG

        auto maxPickedIndices = PickIndex::GetMaxPickedIndices(inputSize);
        // Out put closest comb
        if (needsToOutPutClosestComb)
        {
            auto& closestComb = combsVec[closetCombIndexOfVec];
            // Reverse "remove" to "pick" indices.
            outClosetCombIndices = ~closestComb.bitFlag;
            // Remove the unused bits.
            outClosetCombIndices &= maxPickedIndices;

            *inputDesc.pOutClosestCombIndices = outClosetCombIndices;
        }

        // Out put all combs vec
        if (inputDesc.pOutAllCombIndicesVec)
        {
            auto& outCombVec = *inputDesc.pOutAllCombIndicesVec;
            outCombVec.clear();
            auto size = combsVec.size();
            outCombVec.reserve(size);

            bool hasRefMinSum = inputDesc.refMinExeedSum >= 0.0f;

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

                if (hasRefMinSum)
                {
                    // Skip the diff that is already greater than ref min exeed.
                    if (diff > inputDesc.refMinExeedSum)
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
                msg << "Current targetValue is: " << targetValue
                    << " Size of OutAllCombIndicesVec: " << outCombVec.size() << std::endl;
                std::cout << msg.str();
            }
#endif // M_DEBUG
        }

    } // End calculation scope

    return true;
}

// Explicit template instanciation
template bool Combination::FindSumToTargetBackTracking<true, 32, std::uint64_t>(
    const InputSumToTargetDesc<std::uint64_t>&, std::string&);

template bool Combination::FindSumToTargetBackTracking<false, 32, std::uint64_t>(
    const InputSumToTargetDesc<std::uint64_t>&, std::string&);

} // namespace JUtils
