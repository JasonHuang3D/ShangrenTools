//
// Author: Jason Huang(jasonhuang1988@gmail.com) 2021
//

#include "JUtils/pch.h"

#include "GearUserData.h"

#include "JUtils/Utils.h"
#include "nlohmann/json.hpp"

#include <cmath>
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

        configMap(u8"产晶数值", &ChanYeBuff::chanJing_add, 
            &GearData::chanyeBuff,
            nullptr,
            nullptr, &XianlvData::tianfu_chanye_buff,
            nullptr);
        configMap(u8"产能数值", &ChanYeBuff::chanNeng_add,
            &GearData::chanyeBuff,
            nullptr,
            nullptr, &XianlvData::tianfu_chanye_buff,
            nullptr);
        configMap(u8"产晶百分比", &ChanYeBuff::chanjing_percent,
            &GearData::chanyeBuff,
            nullptr,
            nullptr, &XianlvData::tianfu_chanye_buff,
            nullptr);
        configMap(u8"产能百分比", &ChanYeBuff::chanNeng_percent, 
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
    using CallBackType = std::function<bool(const std::string&, const Json&, std::string&, void*)>;

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

    bool operator()(
        const std::string& jsonKey, const Json& jsonValue, std::string& errorStr, void* pData)
    {
        if (jsonKey != desiredKey)
        {
            errorStr += FormatString(jsonKey, " Expect to be: ", desiredKey, "\n");
            assert(false);
            return false;
        }

        if ((jsonValue.*TypeCheckFuncPtr)())
        {
            auto* pTypedData           = reinterpret_cast<TypeData*>(pData);
            pTypedData->*dataMemberPtr = jsonValue.get<TypeValue>();
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
                if (processSelector.callBack(propGroupKey, propGroupValue, errorStr, pData))
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
                    processSelector.callBack(prop.key(), prop.value(), errorStr, pData);
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
                            propGroup.key(), u8" 包含未知组名\n");
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
constexpr auto k_gearKey_numEquip = u8"仙器佩戴数量";
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

                JsonUtils::TargetProcessType::NumTargets, JsonUtils::JsonProcessOpType::NumOps,
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

                JsonUtils::TargetProcessType::NumTargets, JsonUtils::JsonProcessOpType::NumOps,
                JsonUtils::ProcessSelectorCallBackWrapper<&Json::is_string, XianRenData,
                    std::string>(&XianRenData::xianZhiName, k_key),
                false // False means this key is optional in Json file.
            );
        }
        return s_outMap;
    }
};

constexpr auto k_key_baseUnitScale         = u8"仙人基础属性单位";
constexpr auto k_key_calculateXianRenArray = u8"参与运算仙人";
constexpr auto k_key_calculateAllXianRen   = "ALL";

} // namespace XianjieJson
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

    return true;
}

void XianQiFileData::Reset()
{
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

    // Pase Xianren that need to calculate
    {
        auto it = jsonRoot.find(XianjieJson::k_key_calculateXianRenArray);
        if (it == jsonRoot.end() || !it->is_array())
        {
            errorStr += FormatString(u8"文件: ", fileName, u8" 缺少有效的: ",
                XianjieJson::k_key_calculateXianRenArray, "\n");
            return false;
        }

        auto calculateStrArray = it->get<std::vector<std::string>>();
        if (calculateStrArray.empty())
        {
            errorStr += FormatString(u8"文件: ", fileName, u8" 中的: ",
                XianjieJson::k_key_calculateXianRenArray, u8" 不能为空,如希望计算全部仙人请输入",
                std::quoted(XianjieJson::k_key_calculateAllXianRen), "\n");
            return false;
        }

        if (std::find(calculateStrArray.begin(), calculateStrArray.end(),
                XianjieJson::k_key_calculateAllXianRen) != calculateStrArray.end())
        {
            // Using all Xian Ren
            const auto& source = out.m_xianRenDataVec;
            auto& target       = out.m_calcXianRenDataVec;

            target.reserve(source.size());
            for (const auto& data : source)
                target.emplace_back(&data);
        }
        else
        {
            const auto& refTable = out.m_loopUpXainRen;
            std::unordered_map<std::string, const XianRenData*> uniqueTable;
            for (const auto& strKey : calculateStrArray)
            {
                if (strKey.empty())
                    continue;

                // Check if str key is valid.
                auto itRef = refTable.find(strKey);
                if (itRef == refTable.end())
                {
                    errorStr += FormatString(u8"文件: ", fileName, ", ",
                        XianjieJson::k_key_calculateXianRenArray, u8" 中的属性: ", strKey,
                        u8" 无效!\n");
                    assert(false);
                    return false;
                }

                // Check if the str key has already been picked.
                const auto* pData = itRef->second;
                auto insertedPair = uniqueTable.try_emplace(strKey, pData);
                if (!insertedPair.second)
                {
                    errorStr += FormatString(u8"文件: ", fileName, ", ",
                        XianjieJson::k_key_calculateXianRenArray, u8" 中的节点: ", pData->name,
                        u8", 中的属性: ", strKey, u8" 已经被 ", uniqueTable.at(strKey)->name,
                        u8" 选择!\n");
                    assert(false);
                    return false;
                }

                out.m_calcXianRenDataVec.emplace_back(pData);
            }
        }
    }

    return true;
}

void XianJieFileData::Reset()
{
    m_unitScale = UnitScale::NotValid;

    m_loopUpXainLv.clear();
    m_loopUpXainZhi.clear();
    m_loopUpXainRen.clear();

    m_calcXianRenDataVec.clear();
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

std::string XianRenProp::ToString() const
{
    return FormatString(
        u8"力: ", li, u8"\n念: ", nian, u8"\n福: ", fu, u8"\n总属性: ", GetSum<PropertyMask::All>(), "\n");
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
