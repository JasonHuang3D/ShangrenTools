//
// Author: Jason Huang(jasonhuang1988@gmail.com) 2021
//

#include "pch.h"

#include "Utils.h"

#include <thread>
#include "vorbrodt/pool.hpp"

namespace JUtils
{
Timer::Timer()
{
    Reset();
}
void Timer::Reset()
{
    m_t0 = m_clock.now();
}
double Timer::DurationInSec()
{
    return Elapsed() * 0.001;
}
double Timer::DurationInMsec()
{
    return Elapsed();
}
double Timer::Elapsed() const
{
    std::chrono::time_point<std::chrono::high_resolution_clock> t1 = m_clock.now();
    std::chrono::milliseconds dt = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - m_t0);
    return static_cast<double>(dt.count());
}

// Helper to check if char ptr is empty
bool isCharPtrEmpty(const char* str)
{
    if (str == nullptr)
        return true;
    if (std::strlen(str) == 0 || !str[0])
        return true;
    return false;
}


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

} // namespace JUtils

void JUtils::SelectCombination::SelectGroupComb::Run(bool useMultiThread)
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
                auto selectRecursion =
                    LambdaCombinator([&](auto& selectLambda, std::uint32_t offset,
                        std::uint32_t stackIndex) -> void {
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
                                    //pThreadPool->enqueue_work(mainRecLambda, resultStack,
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
