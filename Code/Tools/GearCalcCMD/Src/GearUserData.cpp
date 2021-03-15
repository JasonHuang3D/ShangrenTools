//
// Author: Jason Huang(jasonhuang1988@gmail.com) 2021
//

#include "JUtils/pch.h"

#include "GearUserData.h"

#include "JUtils/Utils.h"
#include "nlohmann/json.hpp"

#include <execution>
#include <fstream>
#include <functional>
#include <iomanip>

using Json = nlohmann::json;
using namespace JUtils;
using namespace GearCalc;

namespace
{
namespace JsonUtils
{

constexpr auto k_key_repeatable     = u8"重复个数";
constexpr std::uint32_t k_maxRepeat = 64;

bool LoadJsonFile(const char* fileName, std::string& errorStr, Json& outJsonRoot)
{
    if (isCharPtrEmpty(fileName))
    {
        errorStr += "fileName should not be empty!\n";
        assert(false);
        return false;
    }

    std::ifstream fileStream(fileName);
    if (fileStream.is_open())
    {
        // Parse the JSON file
        outJsonRoot = Json::parse(fileStream, nullptr, false, true);
        fileStream.close();
        // If there is any error during parsing, return false.
        if (outJsonRoot.is_discarded())
        {
            errorStr += FormatString(
                u8"JSON文件: ", fileName, u8" 有误! 推荐使用 Visual Studio Code 进行编辑.\n");
            return false;
        }

        return true;
    }
    else
    {
        errorStr += FormatString(u8"JSON文件: ", fileName, u8" 不能被读取,请检查!\n");
        return false;
    }
    return false;
}

// Base for STL containers
struct JsonProcessBase
{
    virtual ~JsonProcessBase() {}
    virtual void Process(const Json& itemValue, void* pTarget) = 0;
};
using JsonProcessPtrType = std::unique_ptr<JsonProcessBase>;

template <typename TypeTarget, typename TypeValue>
struct JsonProcess : public JsonProcessBase
{
    JsonProcess(TypeValue TypeTarget::*pMemberPtr,
        std::function<void(TypeValue&, TypeValue)> operatorFunc) :
        pMemberPtr(pMemberPtr), operatorFunc(operatorFunc)
    {
    }
    void Process(const Json& jsonValue, void* pTarget) override
    {
        auto* pTypedTarget = reinterpret_cast<TypeTarget*>(pTarget);

        operatorFunc(pTypedTarget->*pMemberPtr, jsonValue.get<TypeValue>());
    }
    TypeValue TypeTarget::*pMemberPtr;
    std::function<void(TypeValue&, TypeValue)> operatorFunc;
};
template <typename TypeTarget, typename TypeValue, typename TypeOp>
std::unique_ptr<JsonProcess<TypeTarget, TypeValue>> CreateJsonProcessHelper(
    TypeValue TypeTarget::*pMemberPtr, TypeOp operatorFunc)
{
    std::function<void(TypeValue&, TypeValue)> _f = operatorFunc;
    return std::make_unique<JsonProcess<TypeTarget, TypeValue>>(pMemberPtr, _f);
}

struct TargetProcessBase
{
    virtual ~TargetProcessBase() {}
    virtual void Process(
        void* pTarget, const JsonProcessPtrType& pJsonProcess, const Json& itemValue) = 0;
};
using TargetProcessPtrType = std::unique_ptr<TargetProcessBase>;

template <typename TypeTarget, typename TypeBuff>
struct TargetProcess : public TargetProcessBase
{
    using BufferMemberPtr = TypeBuff TypeTarget::*;

    TargetProcess(BufferMemberPtr pBufferMemberPtr) : pBufferMemberPtr(pBufferMemberPtr) {}

    void Process(
        void* pTarget, const JsonProcessPtrType& pJsonProcess, const Json& itemValue) override
    {
        auto* pTypedTarget = reinterpret_cast<TypeTarget*>(pTarget);

        pJsonProcess->Process(itemValue, &(pTypedTarget->*pBufferMemberPtr));
    }

    BufferMemberPtr pBufferMemberPtr;
};
template <typename TypeTarget, typename TypeBuff>
std::unique_ptr<TargetProcess<TypeTarget, TypeBuff>> CreateTargetProcessHelper(
    TypeBuff TypeTarget::*pBufferMemberPtr)
{
    return std::make_unique<TargetProcess<TypeTarget, TypeBuff>>(pBufferMemberPtr);
}

std::nullptr_t CreateTargetProcessHelper(std::nullptr_t pNullPtr)
{
    return nullptr;
}

struct JsonProcessOpType
{
    enum Enum : std::uint8_t
    {
        Assign = 0,
        Increase,

        NumOps,
    };
};
struct TargetProcessType
{
    enum Enum : std::uint8_t
    {
        Gear = 0,
        Touxiang,
        Xianlv_fushi,
        Xianlv_tianfu,
        Xianzhi_prop,
        Xianren_prop,

        NumTargets,
    };
};

struct ProcessContainer
{
    using JsonProcessOpArrayType = std::array<JsonProcessPtrType, JsonProcessOpType::NumOps>;
    using TargetProcessArrayType = std::array<TargetProcessPtrType, TargetProcessType::NumTargets>;

    ProcessContainer(
        JsonProcessOpArrayType&& jsonProcessList, TargetProcessArrayType&& targetProcessArray) :
        jsonProcessOpArray(std::move(jsonProcessList)),
        targetProcessArray(std::move(targetProcessArray))
    {
    }

    JsonProcessOpArrayType jsonProcessOpArray;
    TargetProcessArrayType targetProcessArray;
};

using JsonProcessMap = std::unordered_map<std::string, ProcessContainer>;

const JsonProcessMap& GetJsonProcessMap()
{
    static JsonProcessMap s_outMap;

    // clang-format off
    if (s_outMap.empty())
    {
        auto configMap = [&](const char* mapKey, auto pJsonMemberPtr,
                             auto... pTargetMembers
                            ) 
        {
            constexpr auto sizeOfMemberPtr = sizeof ...(pTargetMembers);
            static_assert(sizeOfMemberPtr <= TargetProcessType::NumTargets, "exeeded the range of TargetProcessType::NumTargets");

            s_outMap.try_emplace(mapKey,
                ProcessContainer::JsonProcessOpArrayType 
                {
                    // JsonProcessOpType::Assign
                    CreateJsonProcessHelper(pJsonMemberPtr, [](auto& a, auto b) -> void { a = b; }),
                    // JsonProcessOpType::Increase
                    CreateJsonProcessHelper(pJsonMemberPtr, [](auto& a, auto b) -> void { a += b; })
                },
                ProcessContainer::TargetProcessArrayType 
                {
                    CreateTargetProcessHelper(pTargetMembers)...
                }
            );
        };

        configMap(u8"各力数值", &XianRenPropBuff::li_add, 
            &GearData::individualBuff,
            &TouXiangData::individualBuff,
            &XianlvData::fushi_buff, &XianlvData::tianfu_individual_buff,
            &XianZhiData::individual_buff);
        configMap(u8"各念数值", &XianRenPropBuff::nian_add,
            &GearData::individualBuff,
            &TouXiangData::individualBuff,
            &XianlvData::fushi_buff, &XianlvData::tianfu_individual_buff,
            & XianZhiData::individual_buff);
        configMap(u8"各福数值", &XianRenPropBuff::fu_add, 
            &GearData::individualBuff,
            &TouXiangData::individualBuff,
            &XianlvData::fushi_buff, &XianlvData::tianfu_individual_buff,
            &XianZhiData::individual_buff);
        configMap(u8"各力百分比", &XianRenPropBuff::li_percent, 
            &GearData::individualBuff,
            &TouXiangData::individualBuff,
            &XianlvData::fushi_buff, &XianlvData::tianfu_individual_buff,
            &XianZhiData::individual_buff);
        configMap(u8"各念百分比", &XianRenPropBuff::nian_percent, 
            &GearData::individualBuff,
            &TouXiangData::individualBuff,
            &XianlvData::fushi_buff, &XianlvData::tianfu_individual_buff,
            &XianZhiData::individual_buff);
        configMap(u8"各福百分比", &XianRenPropBuff::fu_percent, 
            &GearData::individualBuff, 
            &TouXiangData::individualBuff,
            &XianlvData::fushi_buff, &XianlvData::tianfu_individual_buff,
            &XianZhiData::individual_buff);

        configMap(u8"总力数值", &XianRenPropBuff::li_add,
            &GearData::globalBuff,
            nullptr,
            nullptr, &XianlvData::tianfu_global_buff,
            nullptr);
        configMap(u8"总念数值", &XianRenPropBuff::nian_add,
            &GearData::globalBuff,
            nullptr,
            nullptr, &XianlvData::tianfu_global_buff,
            nullptr);
        configMap(u8"总福数值", &XianRenPropBuff::fu_add,
            &GearData::globalBuff,
            nullptr,
            nullptr, &XianlvData::tianfu_global_buff,
            nullptr);
        configMap(u8"总力百分比", &XianRenPropBuff::li_percent,
            &GearData::globalBuff,
            nullptr,
            nullptr, &XianlvData::tianfu_global_buff,
            nullptr);
        configMap(u8"总念百分比", &XianRenPropBuff::nian_percent,
            &GearData::globalBuff,
            nullptr,
            nullptr, &XianlvData::tianfu_global_buff,
            nullptr);
        configMap(u8"总福百分比", &XianRenPropBuff::fu_percent,
            &GearData::globalBuff,
            nullptr,
            nullptr, &XianlvData::tianfu_global_buff,
            nullptr);

        configMap(u8"产晶数值", &ChanyePropBuff::chanJing_add, 
            &GearData::chanyeBuff,
            nullptr,
            nullptr, &XianlvData::tianfu_chanye_buff,
            nullptr);
        configMap(u8"产能数值", &ChanyePropBuff::chanNeng_add,
            &GearData::chanyeBuff,
            nullptr,
            nullptr, &XianlvData::tianfu_chanye_buff,
            nullptr);
        configMap(u8"产晶百分比", &ChanyePropBuff::chanJing_percent,
            &GearData::chanyeBuff,
            nullptr,
            nullptr, &XianlvData::tianfu_chanye_buff,
            nullptr);
        configMap(u8"产能百分比", &ChanyePropBuff::chanNeng_percent, 
            &GearData::chanyeBuff,
            nullptr,
            nullptr, &XianlvData::tianfu_chanye_buff,
            nullptr);


        configMap(u8"仙人基础力", &XianRenProp::li, 
            nullptr, nullptr, nullptr, nullptr, nullptr,
            &XianRenData::baseProp
            );
        configMap(u8"仙人基础念", &XianRenProp::nian,
            nullptr, nullptr, nullptr, nullptr, nullptr,
            &XianRenData::baseProp
        );
        configMap(u8"仙人基础福", &XianRenProp::fu,
            nullptr, nullptr, nullptr, nullptr, nullptr,
            &XianRenData::baseProp
        );
    }
    // clang-format on

    return s_outMap;
}

struct ProcessSelector
{
    using CallBackType = std::function<bool(
        const std::string&, const Json&, JsonProcessOpType::Enum, std::string&, void*)>;

    ProcessSelector(TargetProcessType::Enum targetProcessType,
        JsonProcessOpType::Enum processOpType, bool isRequired = true) :
        targetProcessType(targetProcessType),
        processOpType(processOpType),
        callBack(nullptr),
        isRequired(isRequired)
    {
    }

    template <typename FunctionType>
    ProcessSelector(TargetProcessType::Enum targetProcessType,
        JsonProcessOpType::Enum processOpType, FunctionType&& callBack, bool isRequired = true) :
        targetProcessType(targetProcessType),
        processOpType(processOpType),
        callBack(std::move(callBack)),
        isRequired(isRequired)
    {
    }

    // Disable move & copy
    ProcessSelector(ProcessSelector&&) noexcept = delete;
    ProcessSelector& operator=(ProcessSelector&&) = delete;
    ProcessSelector(const ProcessSelector&)       = delete;
    ProcessSelector& operator=(const ProcessSelector&) = delete;

    TargetProcessType::Enum targetProcessType;
    JsonProcessOpType::Enum processOpType;

    CallBackType callBack;
    bool isRequired;
};

template <auto TypeCheckFuncPtr, typename TypeData, typename TypeValue>
struct ProcessSelectorCallBackWrapper
{
    ProcessSelectorCallBackWrapper(TypeValue TypeData::*dataMemberPtr, const char* desiredKey) :
        dataMemberPtr(dataMemberPtr), desiredKey(desiredKey) {};

    bool operator()(const std::string& jsonKey, const Json& jsonValue,
        JsonProcessOpType::Enum operatorType, std::string& errorStr, void* pData)
    {
        if (jsonKey != desiredKey)
        {
            errorStr += FormatString(jsonKey, " Expect to be: ", desiredKey, "\n");
            assert(false);
            return false;
        }

        if ((jsonValue.*TypeCheckFuncPtr)())
        {
            auto* pTypedData = reinterpret_cast<TypeData*>(pData);

            switch (operatorType)
            {
            case JsonProcessOpType::Assign:
                pTypedData->*dataMemberPtr = jsonValue.get<TypeValue>();
                break;
            case JsonProcessOpType::Increase:
                pTypedData->*dataMemberPtr += jsonValue.get<TypeValue>();
                break;
            default:
                errorStr += FormatString(jsonKey, " invalid operator type!\n");
                assert(false);
                return false;
            }
            return true;
        }
        else
        {
            errorStr += FormatString(jsonKey, " value is not a valid type!\n");
            assert(false);
            return false;
        }
    }

    TypeValue TypeData::*dataMemberPtr = nullptr;
    const char* desiredKey             = nullptr;
};

using PropGroupsToProcessSelectorMapType = std::unordered_map<std::string, ProcessSelector>;
const std::string k_nullSubGroupKey      = "NullSubGroup";

struct JsonArrayDescBase
{
    virtual const char* GetKeyOfArray() const                                    = 0;
    virtual const PropGroupsToProcessSelectorMapType& GetGroupProcessMap() const = 0;
};

template <bool UsePropGroups, bool IgnoreUnKnowPropGroup = false, typename TypeData>
bool ParseJsonArray(const JsonArrayDescBase& desc, const char* fileName, const Json& jsonRoot,
    std::string& errorStr, std::vector<TypeData>& outDataVec)
{

    const auto keyOfArray       = desc.GetKeyOfArray();
    const auto& groupProcessMap = desc.GetGroupProcessMap();

    if constexpr (!UsePropGroups)
    {
        if (groupProcessMap.find(k_nullSubGroupKey) == groupProcessMap.end())
        {
            assert(false);
        }
    }

    auto itElementContainer = jsonRoot.find(keyOfArray);
    if (itElementContainer == jsonRoot.end() || !itElementContainer->is_object())
    {
        errorStr += FormatString(u8"文件: ", fileName, u8" 缺少有效的: ", keyOfArray);
        return false;
    }

    const auto& elementContainer = *itElementContainer;
    outDataVec.clear();
    outDataVec.reserve(elementContainer.size());
    const auto& kBuffProcessMap = JsonUtils::GetJsonProcessMap();

    auto processObject = [&](const Json& propGroupValue, const std::string& elementKey,
                             TypeData* pData, const char* propGroupKey,
                             const ProcessSelector& processSelector, bool& outHasValue) -> bool {
        if (!propGroupValue.is_object())
        {
            // propGroupValue is a value type. We only accept value if call back is assigned.

            if (processSelector.callBack)
            {
                if (processSelector.callBack(propGroupKey, propGroupValue,
                        processSelector.processOpType, errorStr, pData))
                {
                    outHasValue = true;
                    return true;
                }
                return false;
            }
            else
            {
                errorStr += FormatString(
                    keyOfArray, ": ", elementKey, u8", 节点:", propGroupKey, u8", 应该为Object!\n");
                return false;
            }
        }
        else
        {

            for (auto& prop : propGroupValue.items())
            {
                // Skip if we are parsing k_key_repeatable
                if (prop.key() == k_key_repeatable)
                    continue;

                if (processSelector.callBack)
                {
                    processSelector.callBack(
                        prop.key(), prop.value(), processSelector.processOpType, errorStr, pData);
                    continue;
                }

                auto propIt = kBuffProcessMap.find(prop.key());
                if (propIt == kBuffProcessMap.end())
                {
                    errorStr += FormatString(keyOfArray, ": ", elementKey, u8", 词条组:",
                        propGroupKey != nullptr ? propGroupKey : u8"无", u8", 词条: ", prop.key(),
                        u8" 无效请纠正!\n");
                    return false;
                }

                auto& processContainer = propIt->second;

                auto& targetProcess =
                    processContainer.targetProcessArray[processSelector.targetProcessType];
                if (!targetProcess)
                {
                    errorStr += "targetProcess can not be null!\n";
                    return false;
                }

                auto& jsonProcess =
                    processContainer.jsonProcessOpArray[processSelector.processOpType];
                if (!jsonProcess)
                {
                    errorStr += "jsonProcess can not be null!\n";
                    return false;
                }

                targetProcess->Process(pData, jsonProcess, prop.value());
                outHasValue = true;
            }
            return true;
        }
    };

    // Sum all requiremnts group process map
    std::uint32_t sumRequirements = 0;
    for (auto& it : groupProcessMap)
    {
        if (it.second.isRequired)
            ++sumRequirements;
    }

    if constexpr (!UsePropGroups)
    {
        assert(sumRequirements == 1);
    }

    for (auto& element : elementContainer.items())
    {
        // Create data
        auto& data = outDataVec.emplace_back();
        data.name  = element.key();

        bool hasAnyValue = false;
        if constexpr (UsePropGroups)
        {
            // Count the requirement.
            std::uint32_t numMetRequirements = 0;

            for (auto& propGroup : element.value().items())
            {
                // Skip if we are parsing k_key_repeatable
                if (propGroup.key() == k_key_repeatable)
                    continue;

                auto itGroupProcess = groupProcessMap.find(propGroup.key());
                if (itGroupProcess == groupProcessMap.end())
                {

                    if constexpr (IgnoreUnKnowPropGroup)
                    {
                        continue;
                    }
                    else
                    {
                        errorStr += FormatString(keyOfArray, ": ", element.key(), u8", 组: ",
                            propGroup.key(), u8" 是未知组名\n");
                        assert(false);
                        return false;
                    }
                }

                // Check if process selector is required.
                const auto& processSelector = itGroupProcess->second;
                if (processSelector.isRequired)
                    ++numMetRequirements;

                auto succeed = processObject(propGroup.value(), element.key(), &data,

                    itGroupProcess->first.c_str(), processSelector,

                    hasAnyValue);

                if (!succeed)
                    return false;
            }

            // numMetRequirements should never greater than sumRequirements
            assert(numMetRequirements <= sumRequirements);

            if (numMetRequirements != sumRequirements)
            {
                errorStr +=
                    FormatString(keyOfArray, ": ", element.key(), u8" 缺少需要的组,请检查!\n");
                assert(false);
                return false;
            }
        }
        else
        {
            auto succeed = processObject(element.value(), element.key(), &data,

                k_nullSubGroupKey.c_str(), groupProcessMap.at(k_nullSubGroupKey),

                hasAnyValue);

            if (!succeed)
                return false;
        }

        if (!hasAnyValue)
        {
            errorStr += FormatString(keyOfArray, ": ", element.key(), u8", 未找到任何有效词条\n");
            return false;
        }

        // Config the repeat
        std::uint32_t repeatNum = 0;
        if (element.value().contains(k_key_repeatable))
        {
            auto& repeatJsonValue = element.value().at(k_key_repeatable);
            if (repeatJsonValue.is_number_integer())
            {
                repeatNum = repeatJsonValue.get<std::uint32_t>();
            }
            else
            {
                errorStr += FormatString(std::quoted(k_key_repeatable), u8"的值应该为整数数字!\n");
                assert(false);
                return false;
            }

            // Check repeatNum
            if (repeatNum < 1 || repeatNum > k_maxRepeat)
            {
                errorStr += FormatString(std::quoted(k_key_repeatable),
                    u8"的值不在范围内, 最小是1, 最大为: ", k_maxRepeat, "\n");
                assert(false);
                return false;
            }
        }

        if (repeatNum > 1)
        {
            auto copy = data;
            // Start from 1, as we skiped the original one.
            for (std::uint32_t i = 1; i < repeatNum; ++i)
            {
                auto& repeatData = outDataVec.emplace_back(copy);
                repeatData.name  = FormatString(copy.name, u8"-复制", i);
            }
        }
    }
    return true;
}

// Helper functions for parsing groups
template <bool UsePropGroups = true, bool IgnoreUnKnowPropGroup = false, typename TypeTargetData>
bool ParseGroups(const Json& jsonRoot, const char* fileName, std::string& errorStr,
    std::vector<TypeTargetData>& targetDataVec, const JsonUtils::JsonArrayDescBase& desc,
    std::unordered_map<std::string, const TypeTargetData* const>* pTargetLoopUpTable = nullptr)
{
    auto succeed = JsonUtils::ParseJsonArray<UsePropGroups, IgnoreUnKnowPropGroup>(
        desc, fileName, jsonRoot, errorStr, targetDataVec);
    if (!succeed)
        return false;

    if (pTargetLoopUpTable != nullptr)
    {
        auto& loopUpTable = *pTargetLoopUpTable;
        for (auto& data : targetDataVec)
        {
            auto insertedPair = loopUpTable.try_emplace(data.name, &data);
            if (!insertedPair.second)
            {
                errorStr += FormatString(
                    u8"文件: ", fileName, u8" 节点: ", data.name, u8" 有重复,请纠正!\n");
                assert(false);
                return false;
            }
        }
    }

    return true;
};
template <bool UsePropGroups = true, bool IgnoreUnKnowPropGroup = false, typename TypeTargetData,
    typename TypeRefData>
bool ParseGroups(const Json& jsonRoot, const char* fileName, std::string& errorStr,
    std::vector<TypeTargetData>& targetDataVec, const JsonUtils::JsonArrayDescBase& desc,
    std::unordered_map<std::string, const TypeTargetData* const>* pTargetLoopUpTable,
    std::unordered_map<std::string, const TypeRefData* const>* pRefLoopUptable,
    std::string TypeTargetData::*pRefStrMemberPtr,
    const TypeRefData* TypeTargetData::*pRefPtrMemberPtr, bool useUniqueRef = true)
{
    auto succeed = ParseGroups<UsePropGroups, IgnoreUnKnowPropGroup>(
        jsonRoot, fileName, errorStr, targetDataVec, desc, pTargetLoopUpTable);
    if (!succeed)
        return false;

    if (pRefLoopUptable != nullptr)
    {
        auto& refTable = *pRefLoopUptable;
        std::unordered_map<std::string, const TypeTargetData*> uniqueTable;

        for (auto& data : targetDataVec)
        {
            auto& strKey = data.*pRefStrMemberPtr;

            if (strKey.empty())
                continue;

            auto itRefTable = refTable.find(strKey);
            if (itRefTable != refTable.end())
            {
                if (useUniqueRef)
                {
                    auto insertedPair = uniqueTable.try_emplace(strKey, &data);
                    if (!insertedPair.second)
                    {
                        errorStr += FormatString(u8"文件: ", fileName, ", ", desc.GetKeyOfArray(),
                            u8" 中的节点: ", data.name, ", 中的属性: ", strKey, u8" 已经被 ",
                            uniqueTable.at(strKey)->name, u8" 选择!\n");
                        assert(false);
                        return false;
                    }
                }

                data.*pRefPtrMemberPtr = itRefTable->second;
            }
            else
            {
                errorStr += FormatString(u8"文件: ", fileName, ", ", desc.GetKeyOfArray(),
                    u8" 中的节点: ", data.name, u8", 中的属性: ", strKey, u8" 不存在,请检查!\n");
                assert(false);
                return false;
            }
        }
    }

    return true;
};

template <typename TypeSource>
bool ParseRequiredCalcElements(const Json& jsonRoot, const char* arrayKey,
    const std::unordered_map<std::string, const TypeSource* const>& sourceLoopUp,
    std::vector<const TypeSource*>& targetVec, const char* fileName, std::string& errorStr)
{
    static constexpr auto kAllKey = "ALL";

    auto itJson = jsonRoot.find(arrayKey);
    if (itJson == jsonRoot.end() || !itJson->is_array())
    {
        errorStr += FormatString(
            u8"文件: ", std::quoted(fileName), u8" 缺少有效的: ", std::quoted(arrayKey), "\n");
        return false;
    }

    auto calculateStrArray = itJson->get<std::vector<std::string>>();
    if (calculateStrArray.empty())
    {
        errorStr += FormatString(u8"文件: ", std::quoted(fileName), u8" 中的: ",
            std::quoted(arrayKey), u8" 不能为空,如希望计算全部请输入", std::quoted(kAllKey), "\n");
        return false;
    }

    if (std::find(calculateStrArray.begin(), calculateStrArray.end(), kAllKey) !=
        calculateStrArray.end())
    {
        // Using all Xian Ren
        targetVec.reserve(sourceLoopUp.size());
        for (auto& it : sourceLoopUp)
        {
            targetVec.emplace_back(it.second);
        }
    }
    else
    {
        std::unordered_map<std::string, const TypeSource*> uniqueTable;
        for (const auto& strKey : calculateStrArray)
        {
            if (strKey.empty())
                continue;

            // Check if str key is valid.
            auto itRef = sourceLoopUp.find(strKey);
            if (itRef == sourceLoopUp.end())
            {
                errorStr += FormatString(u8"文件: ", std::quoted(fileName), ", ",
                    std::quoted(arrayKey), u8" 中的属性: ", strKey, u8" 无效!\n");
                assert(false);
                return false;
            }

            // Check if the str key has already been picked.
            const auto* pData = itRef->second;
            auto insertedPair = uniqueTable.try_emplace(strKey, pData);
            if (!insertedPair.second)
            {
                errorStr += FormatString(u8"文件: ", std::quoted(fileName), ", ",
                    std::quoted(arrayKey), u8" 中的节点: ", pData->name, u8", 中的属性: ", strKey,
                    u8" 已经被 ", uniqueTable.at(strKey)->name, u8" 选择!\n");
                assert(false);
                return false;
            }

            targetVec.emplace_back(pData);
        }
    }

    return true;
}
} // namespace JsonUtils

namespace GearJson
{
struct XianqiProcessArrayDesc : public JsonUtils::JsonArrayDescBase
{
    const char* GetKeyOfArray() const override { return u8"仙器"; }
    const JsonUtils::PropGroupsToProcessSelectorMapType& GetGroupProcessMap() const override
    {
        static JsonUtils::PropGroupsToProcessSelectorMapType s_outMap;
        if (s_outMap.empty())
        {
            s_outMap.try_emplace(u8"白色", JsonUtils::TargetProcessType::Gear,
                JsonUtils::JsonProcessOpType::Increase);
            s_outMap.try_emplace(u8"蓝色", JsonUtils::TargetProcessType::Gear,
                JsonUtils::JsonProcessOpType::Increase);
        }

        return s_outMap;
    }
};
constexpr auto k_gearKey_numEquip          = u8"仙器佩戴数量";
constexpr auto k_key_calculateXianQi_array = u8"参与运算仙器";
} // namespace GearJson

namespace XianjieJson
{
struct TouXiangProcessArrayDesc : public JsonUtils::JsonArrayDescBase
{
    const char* GetKeyOfArray() const override { return u8"头像"; }
    const JsonUtils::PropGroupsToProcessSelectorMapType& GetGroupProcessMap() const override
    {
        static JsonUtils::PropGroupsToProcessSelectorMapType s_outMap;
        if (s_outMap.empty())
        {
            s_outMap.try_emplace(JsonUtils::k_nullSubGroupKey,
                JsonUtils::TargetProcessType::Touxiang, JsonUtils::JsonProcessOpType::Assign);
        }

        return s_outMap;
    }
};
struct XianLvProcessArrayDesc : public JsonUtils::JsonArrayDescBase
{
    const char* GetKeyOfArray() const override { return u8"仙侣"; }
    const JsonUtils::PropGroupsToProcessSelectorMapType& GetGroupProcessMap() const override
    {
        static JsonUtils::PropGroupsToProcessSelectorMapType s_outMap;
        if (s_outMap.empty())
        {
            s_outMap.try_emplace(u8"辅事", JsonUtils::TargetProcessType::Xianlv_fushi,
                JsonUtils::JsonProcessOpType::Assign);
            s_outMap.try_emplace(u8"天赋", JsonUtils::TargetProcessType::Xianlv_tianfu,
                JsonUtils::JsonProcessOpType::Assign);
        }

        return s_outMap;
    }
};
struct XianZhiProcessArrayDesc : public JsonUtils::JsonArrayDescBase
{
    const char* GetKeyOfArray() const override { return u8"仙职"; }
    const JsonUtils::PropGroupsToProcessSelectorMapType& GetGroupProcessMap() const override
    {
        static JsonUtils::PropGroupsToProcessSelectorMapType s_outMap;
        if (s_outMap.empty())
        {
            s_outMap.try_emplace(u8"属性", JsonUtils::TargetProcessType::Xianzhi_prop,
                JsonUtils::JsonProcessOpType::Assign);

            static constexpr auto k_key = u8"辅事";
            s_outMap.try_emplace(k_key,

                JsonUtils::TargetProcessType::NumTargets, JsonUtils::JsonProcessOpType::Assign,
                JsonUtils::ProcessSelectorCallBackWrapper<&Json::is_string, XianZhiData,
                    std::string>(&XianZhiData::xianlvName, k_key),
                false // False means this key is optional in Json file.
            );
        }
        return s_outMap;
    }
};

struct XianRenProcessArrayDesc : public JsonUtils::JsonArrayDescBase
{
    const char* GetKeyOfArray() const override { return u8"仙人"; }
    const JsonUtils::PropGroupsToProcessSelectorMapType& GetGroupProcessMap() const override
    {
        static JsonUtils::PropGroupsToProcessSelectorMapType s_outMap;
        if (s_outMap.empty())
        {
            s_outMap.try_emplace(u8"基础属性", JsonUtils::TargetProcessType::Xianren_prop,
                JsonUtils::JsonProcessOpType::Assign);

            static constexpr auto k_key = u8"仙职";
            s_outMap.try_emplace(k_key,

                JsonUtils::TargetProcessType::NumTargets, JsonUtils::JsonProcessOpType::Assign,
                JsonUtils::ProcessSelectorCallBackWrapper<&Json::is_string, XianRenData,
                    std::string>(&XianRenData::xianZhiName, k_key),
                false // False means this key is optional in Json file.
            );
        }
        return s_outMap;
    }
};

struct ChanyeFieldArrayDescBase : public JsonUtils::JsonArrayDescBase
{
    const JsonUtils::PropGroupsToProcessSelectorMapType& GetGroupProcessMap() const override
    {
        static JsonUtils::PropGroupsToProcessSelectorMapType s_outMap;
        static constexpr auto k_key_chanye_level_buff  = u8"产业等级百分比";
        static constexpr auto k_key_chanye_zaohua_buff = u8"造化百分比";
        if (s_outMap.empty())
        {
            // ChanyeFieldData::selfBuff = k_key_chanye_level_buff + k_key_chanye_zaohua_buff

            s_outMap.try_emplace(k_key_chanye_level_buff,

                JsonUtils::TargetProcessType::NumTargets, JsonUtils::JsonProcessOpType::Assign,
                JsonUtils::ProcessSelectorCallBackWrapper<&Json::is_number, ChanyeFieldData,
                    double>(&ChanyeFieldData::selfBuff, k_key_chanye_level_buff),
                true // true means this key is required in Json file.
            );

            s_outMap.try_emplace(k_key_chanye_zaohua_buff,

                JsonUtils::TargetProcessType::NumTargets, JsonUtils::JsonProcessOpType::Increase,
                JsonUtils::ProcessSelectorCallBackWrapper<&Json::is_number, ChanyeFieldData,
                    double>(&ChanyeFieldData::selfBuff, k_key_chanye_zaohua_buff),
                true // true means this key is required in Json file.
            );
        }
        return s_outMap;
    }
};

struct ChanyeJingFieldDesc : public ChanyeFieldArrayDescBase
{
    const char* GetKeyOfArray() const override { return u8"产晶"; }
};
struct ChanyeNengFieldDesc : public ChanyeFieldArrayDescBase
{
    const char* GetKeyOfArray() const override { return u8"产能"; }
};

constexpr auto k_key_baseUnitScale = u8"仙人基础属性单位";

constexpr auto k_key_calculateXianRen_array = u8"参与运算仙人";

constexpr auto k_key_chanye_obj     = u8"产业";
constexpr auto k_key_chanyeInterval = u8"收益间隔";

} // namespace XianjieJson

using ChanyeFiledOpMapType = std::unordered_map<std::string, ChanyeFieldInfo>;
using ChanyeFieldTableType = std::array<ChanyeFiledOpMapType, ChanyeFieldCategory::NumCategory>;
const ChanyeFieldTableType& GetChanyeTable()
{
    // clang-format off
    static ChanyeFieldTableType s_table{ {
            // ChanJing
            {
               {
                   u8"仙矿开采",
                   {
                       2,
                       [](const XianRenProp& xianRenProp) -> double
                       {
                          return std::trunc(xianRenProp.li * 0.3);
                       }
                   }
               },

               {
                   u8"仙材种植",
                   {
                       3,
                       [](const XianRenProp& xianRenProp) -> double
                       {
                           return std::trunc(xianRenProp.li * 0.03)
                               + std::trunc(xianRenProp.nian * 0.18);
                       }
                   }
               },

               {
                   u8"仙器炼制",
                   {
                       4,
                       [](const XianRenProp& xianRenProp) -> double
                       {
                           return std::trunc(xianRenProp.li * 0.16)
                               + std::trunc(xianRenProp.nian * 0.05);
                       }
                   }
               },

               {
                   u8"仙界商行",
                   {
                       5,
                       [](const XianRenProp& xianRenProp) -> double
                       {
                           return std::trunc(xianRenProp.li * 0.07)
                               + std::trunc(xianRenProp.nian * 0.07)
                               + std::trunc(xianRenProp.fu * 0.07);
                       }
                   }
               },

               {
                   u8"绝地寻宝",
                   {
                       7,
                       [](const XianRenProp& xianRenProp) -> double
                       {
                           return std::trunc(xianRenProp.li * 0.05)
                               + std::trunc(xianRenProp.nian * 0.05)
                               + std::trunc(xianRenProp.fu * 0.25);
                       }
                   }
               }

           },

        // ChanNeng
        {
            {
                u8"聚元仙阵",
                {
                    1,
                    [](const XianRenProp& xianRenProp) -> double
                    {
                        return std::trunc(xianRenProp.nian * 0.3);
                    }
                }
            },

            {
                u8"传道仙馆",
                {
                    6,
                    [](const XianRenProp& xianRenProp) -> double
                    {
                        return std::trunc(xianRenProp.li * 0.14)
                            + std::trunc(xianRenProp.nian * 0.02)
                            + std::trunc(xianRenProp.fu * 0.14);
                    }
                }
            }

        }


    } };
    // clang-format on
    return s_table;
}

} // namespace

namespace GearCalc
{
bool XianQiFileData::ReadFromJsonFile(
    const char* fileName, std::string& errorStr, XianQiFileData& out)
{
    out.Reset();
    Json jsonRoot;
    if (!JsonUtils::LoadJsonFile(fileName, errorStr, jsonRoot))
        return false;

    // Start config

    // Parse max num equip
    {
        auto it = jsonRoot.find(GearJson::k_gearKey_numEquip);
        if (it == jsonRoot.end() || !it->is_number_unsigned())
        {
            errorStr += FormatString(
                u8"文件: ", fileName, u8" 缺少有效的: ", GearJson::k_gearKey_numEquip, "\n");
            return false;
        }

        out.m_maxNumEquip = it->get<std::uint32_t>();
    }

    // Parse each gear
    {
        auto succeed =
            JsonUtils::ParseGroups<true>(jsonRoot, fileName, errorStr, out.m_gearsDataVec,
                GetSingletonInstance<GearJson::XianqiProcessArrayDesc>(), &out.m_loopUpGears);
        if (!succeed)
            return false;
    }

    // Pase gears that need to calculate
    {
        auto succeed =
            JsonUtils::ParseRequiredCalcElements(jsonRoot, GearJson::k_key_calculateXianQi_array,
                out.m_loopUpGears, out.m_calcGearsVec, fileName, errorStr);
        if (!succeed)
            return false;
    }

    return true;
}

void XianQiFileData::Reset()
{
    m_calcGearsVec.clear();
    m_maxNumEquip = 0;
    m_loopUpGears.clear();
    m_gearsDataVec.clear();
}

bool XianJieFileData::ReadFromJsonFile(
    const char* fileName, std::string& errorStr, XianJieFileData& out)
{
    out.Reset();
    Json jsonRoot;
    if (!JsonUtils::LoadJsonFile(fileName, errorStr, jsonRoot))
        return false;

    // Start config

    // Parse each Touxiang
    {
        auto succeed = JsonUtils::ParseGroups<false>(jsonRoot, fileName, errorStr,
            out.m_touXiangDataVec, GetSingletonInstance<XianjieJson::TouXiangProcessArrayDesc>());
        if (!succeed)
            return false;
    }

    // Parse each Xianlv
    {
        auto succeed =
            JsonUtils::ParseGroups<true>(jsonRoot, fileName, errorStr, out.m_xianLvDataVec,
                GetSingletonInstance<XianjieJson::XianLvProcessArrayDesc>(), &out.m_loopUpXainLv);
        if (!succeed)
            return false;
    }

    // Parse each Xianzhi
    {
        auto succeed =
            JsonUtils::ParseGroups<true>(jsonRoot, fileName, errorStr, out.m_xianZhiDataVec,
                GetSingletonInstance<XianjieJson::XianZhiProcessArrayDesc>(), &out.m_loopUpXainZhi,
                &out.m_loopUpXainLv, &XianZhiData::xianlvName, &XianZhiData::pXianlv);
        if (!succeed)
            return false;
    }

    // Parse Xianre unit scale
    {
        auto it = jsonRoot.find(XianjieJson::k_key_baseUnitScale);
        if (it == jsonRoot.end() || !it->is_string())
        {
            errorStr += FormatString(
                u8"文件: ", fileName, u8" 缺少有效的: ", XianjieJson::k_key_baseUnitScale, "\n");
            return false;
        }
        auto unitScaleStr = it->get<std::string>();
        out.m_unitScale   = UnitScale::GetEnumFromStr(unitScaleStr);

        // Check if it is valid
        if (!UnitScale::IsValid(out.m_unitScale))
        {
            errorStr +=
                FormatString(u8"文件: ", fileName, u8" 中的: ", XianjieJson::k_key_baseUnitScale,
                    u8", 其值为: ", unitScaleStr, u8", 不是有效的单位\n");
            return false;
        }
    }

    // Parse each Xianren
    {
        auto succeed =
            JsonUtils::ParseGroups<true>(jsonRoot, fileName, errorStr, out.m_xianRenDataVec,
                GetSingletonInstance<XianjieJson::XianRenProcessArrayDesc>(), &out.m_loopUpXainRen,
                &out.m_loopUpXainZhi, &XianRenData::xianZhiName, &XianRenData::pXianzhi);
        if (!succeed)
            return false;

        // Assign unit scale to each xian ren
        for (auto& data : out.m_xianRenDataVec)
            data.baseProp *= out.m_unitScale;
    }

    // Parse chan ye
    {
        // Get chan ye Json obj
        auto chanyeObjIt = jsonRoot.find(XianjieJson::k_key_chanye_obj);
        if (chanyeObjIt == jsonRoot.end() || !chanyeObjIt->is_object())
        {
            errorStr += FormatString(
                u8"文件: ", fileName, u8" 缺少有效的: ", XianjieJson::k_key_chanye_obj, "\n");
            return false;
        }

        const auto& chanyeObj = chanyeObjIt.value();

        // Parse chanye interval
        {
            // Currently not needed.
#if 0
            auto it = chanyeObj.find(XianjieJson::k_key_chanyeInterval);
            if (it == chanyeObj.end() || !it->is_number())
            {
                errorStr += FormatString(u8"文件: ", fileName, u8" 缺少有效的: ",
                    XianjieJson::k_key_chanyeInterval, "\n");
                return false;
            }
            out.m_chanyeInterval = it->get<double>();

            // Chan ye interval can not be negative
            if (out.m_chanyeInterval < 0.0)
            {
                errorStr += FormatString(XianjieJson::k_key_chanyeInterval, " 不能为负数!\n");
                return false;
            }
#endif
        }

        // Parse each chanye field
        {
            auto postProcessChanye = [&](ChanyeFieldCategory::Enum chanyeType,
                                         std::vector<ChanyeFieldData>& chanyeVec) -> bool {
                const auto& chanyeTable = GetChanyeTable();
                const auto& chanyeMap   = chanyeTable[chanyeType];

                // Check every chanye is valid
                std::unordered_set<std::string> uniqueTable;
                for (auto& chanyeFieldData : chanyeVec)
                {
                    auto it = chanyeMap.find(chanyeFieldData.name);
                    if (it == chanyeMap.end())
                    {
                        errorStr += FormatString(u8"产业: ", std::quoted(chanyeFieldData.name),
                            u8" 不是有效的产业名称\n");
                        assert(false);
                        return false;
                    }

                    auto insertedPair = uniqueTable.emplace(chanyeFieldData.name);
                    if (!insertedPair.second)
                    {
                        errorStr += FormatString(
                            u8"产业: ", std::quoted(chanyeFieldData.name), u8" 不能重复\n");
                        assert(false);
                        return false;
                    }

                    // Assign chanyeInfo
                    chanyeFieldData.chanyeInfo = it->second;
                }

                std::sort(std::execution::par_unseq, chanyeVec.begin(), chanyeVec.end(),
                    [](const ChanyeFieldData& a, const ChanyeFieldData& b) {
                        return a.chanyeInfo.index < b.chanyeInfo.index;
                    });

                return true;
            };

            // Parse ChanJing
            {
                auto succeed = JsonUtils::ParseJsonArray<true, false>(
                    GetSingletonInstance<XianjieJson::ChanyeJingFieldDesc>(), fileName, chanyeObj,
                    errorStr, out.m_chanJingFiledVec);
                if (!succeed)
                    return false;

                // Check if all data are valid.
                postProcessChanye(ChanyeFieldCategory::ChanJing, out.m_chanJingFiledVec);
            }
            // Parse ChanNeng
            {
                auto succeed = JsonUtils::ParseJsonArray<true, false>(
                    GetSingletonInstance<XianjieJson::ChanyeNengFieldDesc>(), fileName, chanyeObj,
                    errorStr, out.m_chanNengFiledVec);
                if (!succeed)
                    return false;

                // Check if all data are valid.
                postProcessChanye(ChanyeFieldCategory::ChanNeng, out.m_chanNengFiledVec);
            }
        }
    }

    // Pase Xianren that need to calculate
    {
        auto succeed = JsonUtils::ParseRequiredCalcElements(jsonRoot,
            XianjieJson::k_key_calculateXianRen_array, out.m_loopUpXainRen,
            out.m_calcXianRenDataVec, fileName, errorStr);
        if (!succeed)
            return false;
    }

    return true;
}

void XianJieFileData::Reset()
{
    m_unitScale = UnitScale::NotValid;
    m_calcXianRenDataVec.clear();
    m_loopUpXainLv.clear();
    m_loopUpXainZhi.clear();
    m_loopUpXainRen.clear();

    m_chanNengFiledVec.clear();
    m_chanJingFiledVec.clear();

    m_xianRenDataVec.clear();
    m_xianZhiDataVec.clear();
    m_xianLvDataVec.clear();
    m_touXiangDataVec.clear();
}

void XianRenPropBuff::Reset()
{
    li_add       = 0;
    nian_add     = 0;
    fu_add       = 0;
    li_percent   = 0.0;
    nian_percent = 0.0;
    fu_percent   = 0.0;
}

std::string XianRenPropBuff::ToString(const char* prefix) const
{
    // clang-format off
    return FormatString(
        prefix, u8"力数值: ", li_add,           "\n",
        prefix, u8"力百分比: ", li_percent,     "\n",
        prefix, u8"念数值: ", nian_add,         "\n",
        prefix, u8"念百分比: ", nian_percent,   "\n",
        prefix, u8"福数值: ", fu_add,           "\n",
        prefix, u8"福百分比: ", fu_percent,     "\n"
    );
    // clang-format on
}

void ChanyePropBuff::Reset()
{
    chanJing_add     = 0;
    chanNeng_add     = 0;
    chanJing_percent = 0.0;
    chanNeng_percent = 0.0;
}

std::string ChanyePropBuff::ToString() const
{
    // clang-format off
    return FormatString(
         u8"产晶数值: ", chanJing_add, "\n",
         u8"产晶百分比: ", chanJing_percent, "\n",
         u8"产能数值: ", chanNeng_add, "\n",
         u8"产能百分比: ", chanNeng_percent, "\n"
    );
    // clang-format on
}

std::string XianRenProp::ToString() const
{
    return FormatString(u8"力: ", li, u8"\n念: ", nian, u8"\n福: ", fu, u8"\n总属性: ",
        GetSum<XianRenPropertyMask::All>(), "\n");
}
void XianRenProp::Reset()
{
    li   = 0.0;
    nian = 0.0;
    fu   = 0.0;
}
XianRenProp& XianRenProp::operator*=(UnitScale::Enum unitScale)
{
    // Removing tailing fractions.
    li   = std::round(li * unitScale);
    nian = std::round(nian * unitScale);
    fu   = std::round(fu * unitScale);
    return *this;
}

} // namespace GearCalc
