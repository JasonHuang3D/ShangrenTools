//
// Author: Jason Huang(jasonhuang1988@gmail.com) 2021
//

#include "JUtils/pch.h"

#include "UserData.h"

#include "JUtils/Utils.h"
#include "nlohmann/json.hpp"

#include <fstream>
#include <functional>

using Json = nlohmann::json;
using namespace JUtils;
using namespace GearCalc;

namespace
{
namespace JsonUtils
{
bool ParseJsonFile(const char* fileName, std::string& errorStr, Json& outJsonRoot)
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
            errorStr += StringFormat::FormatString(
                u8"JSON文件: ", fileName, u8" 有误! 推荐使用 Visual Studio Code 进行编辑. \n");
            return false;
        }

        return true;
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

        configMap(u8"各力数值", &IndividualBuff::geLi_add, 
            &GearData::individualBuff,
            &TouXiangData::individualBuff,
            &XianlvData::fushi_buff, &XianlvData::tianfu_individual_buff,
            &XianZhiData::individual_buff);
        configMap(u8"各念数值", &IndividualBuff::geNian_add,
            &GearData::individualBuff,
            &TouXiangData::individualBuff,
            &XianlvData::fushi_buff, &XianlvData::tianfu_individual_buff,
            & XianZhiData::individual_buff);
        configMap(u8"各福数值", &IndividualBuff::geFu_add, 
            &GearData::individualBuff,
            &TouXiangData::individualBuff,
            &XianlvData::fushi_buff, &XianlvData::tianfu_individual_buff,
            &XianZhiData::individual_buff);
        configMap(u8"各力百分比", &IndividualBuff::geLi_percent, 
            &GearData::individualBuff,
            &TouXiangData::individualBuff,
            &XianlvData::fushi_buff, &XianlvData::tianfu_individual_buff,
            &XianZhiData::individual_buff);
        configMap(u8"各念百分比", &IndividualBuff::geNian_percent, 
            &GearData::individualBuff,
            &TouXiangData::individualBuff,
            &XianlvData::fushi_buff, &XianlvData::tianfu_individual_buff,
            &XianZhiData::individual_buff);
        configMap(u8"各福百分比", &IndividualBuff::geFu_percent, 
            &GearData::individualBuff, 
            &TouXiangData::individualBuff,
            &XianlvData::fushi_buff, &XianlvData::tianfu_individual_buff,
            &XianZhiData::individual_buff);

        configMap(u8"总力数值", &GlobalBuff::zongLi_add, 
            &GearData::globalBuff,
            nullptr,
            nullptr, &XianlvData::tianfu_global_buff,
            nullptr);
        configMap(u8"总念数值", &GlobalBuff::zongNian_add, 
            &GearData::globalBuff,
            nullptr,
            nullptr, &XianlvData::tianfu_global_buff,
            nullptr);
        configMap(u8"总福数值", &GlobalBuff::zongFu_add, 
            &GearData::globalBuff,
            nullptr,
            nullptr, &XianlvData::tianfu_global_buff,
            nullptr);
        configMap(u8"总力百分比", &GlobalBuff::zongLi_percent, 
            &GearData::globalBuff,
            nullptr,
            nullptr, &XianlvData::tianfu_global_buff,
            nullptr);
        configMap(u8"总念百分比", &GlobalBuff::zongNian_percent, 
            &GearData::globalBuff,
            nullptr,
            nullptr, &XianlvData::tianfu_global_buff,
            nullptr);
        configMap(u8"总福百分比", &GlobalBuff::zongFu_percent, 
            &GearData::globalBuff,
            nullptr,
            nullptr, &XianlvData::tianfu_global_buff,
            nullptr);

        configMap(u8"产晶数值", &ChanyeBuff::chanJing_add, 
            &GearData::chanyeBuff,
            nullptr,
            nullptr, &XianlvData::tianfu_chanye_buff,
            nullptr);
        configMap(u8"产能数值", &ChanyeBuff::chanNeng_add,
            &GearData::chanyeBuff,
            nullptr,
            nullptr, &XianlvData::tianfu_chanye_buff,
            nullptr);
        configMap(u8"产晶百分比", &ChanyeBuff::chanjing_percent,
            &GearData::chanyeBuff,
            nullptr,
            nullptr, &XianlvData::tianfu_chanye_buff,
            nullptr);
        configMap(u8"产能百分比", &ChanyeBuff::chanNeng_percent, 
            &GearData::chanyeBuff,
            nullptr,
            nullptr, &XianlvData::tianfu_chanye_buff,
            nullptr);


        configMap(u8"力", &XianrenProp::li, 
            nullptr, nullptr, nullptr, nullptr, nullptr,
            &XianRenData::baseProp
            );
        configMap(u8"念", &XianrenProp::nian,
            nullptr, nullptr, nullptr, nullptr, nullptr,
            &XianRenData::baseProp
        );
        configMap(u8"福", &XianrenProp::fu,
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
        JsonProcessOpType::Enum processOpType, FunctionType callBack, bool isRequired = true) :
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
struct ProcessSelectorWrapper
{
    ProcessSelectorWrapper(TypeValue TypeData::*dataMemberPtr, const char* desiredKey) :
        dataMemberPtr(dataMemberPtr), desiredKey(desiredKey) {};

    bool operator()(
        const std::string& jsonKey, const Json& jsonValue, std::string& errorStr, void* pData)
    {
        if (jsonKey != desiredKey)
        {
            errorStr += StringFormat::FormatString(jsonKey, " Expect to be: ", desiredKey, "\n");
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
            errorStr += StringFormat::FormatString(jsonKey, " value is not a valid type!\n");
            assert(false);
            return false;
        }
    }

    TypeValue TypeData::*dataMemberPtr = nullptr;
    const char* desiredKey             = nullptr;
};

using PropGroupsToProcessSelectorMapType = std::unordered_map<std::string, ProcessSelector>;
const std::string k_nullSubGroupKey      = "NullSubGroup";

struct BufferArrayDescBase
{
    virtual const char* GetKeyOfArray() const                                    = 0;
    virtual const PropGroupsToProcessSelectorMapType& GetGroupProcessMap() const = 0;
};

template <bool UsePropGroups, bool IgnoreUnKnowPropGroup = false, typename TypeData>
bool ParseBuffArray(const BufferArrayDescBase& desc, const char* fileName, const Json& jsonRoot,
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
        errorStr += StringFormat::FormatString(u8"文件: ", fileName, u8" 缺少有效的: ", keyOfArray);
        return false;
    }

    const auto& elementContainer = *itElementContainer;
    outDataVec.clear();
    outDataVec.reserve(elementContainer.size());
    const auto& kBuffProcessMap = JsonUtils::GetJsonProcessMap();

    auto processProp = [&](const Json& propGroupValue, const std::string& elementKey,
                           TypeData* pData, const char* propGroupKey,
                           const ProcessSelector& processSelector, bool& outHasValue) -> bool {
        if (!propGroupValue.is_object())
        {
            // propGroupValue is a value type.

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
                errorStr += StringFormat::FormatString(
                    keyOfArray, ": ", elementKey, u8", 节点:", propGroupKey, u8", 应该为Object!\n");
                return false;
            }
        }
        else
        {

            for (auto& prop : propGroupValue.items())
            {
                if (processSelector.callBack)
                {
                    processSelector.callBack(prop.key(), prop.value(), errorStr, pData);
                    continue;
                }

                auto propIt = kBuffProcessMap.find(prop.key());
                if (propIt == kBuffProcessMap.end())
                {
                    errorStr += StringFormat::FormatString(keyOfArray, ": ", elementKey,
                        u8", 词条组:", propGroupKey != nullptr ? propGroupKey : u8"无",
                        u8", 词条: ", prop.key(), u8" 无效请纠正!\n");
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
        data.name  = element.key().c_str();

        bool hasAnyValue = false;

        if constexpr (UsePropGroups)
        {
            // Count the requirement.
            std::uint32_t numMetRequirements = 0;

            for (auto& propGroup : element.value().items())
            {
                auto itGroupProcess = groupProcessMap.find(propGroup.key());
                if (itGroupProcess == groupProcessMap.end())
                {
                    if constexpr (IgnoreUnKnowPropGroup)
                    {
                        continue;
                    }
                    else
                    {
                        errorStr += StringFormat::FormatString(keyOfArray, ": ", element.key(),
                            u8", 组: ", propGroup.key(), u8" 包含未知组名\n");
                        assert(false);
                        return false;
                    }
                }

                // Check if process selector is required.
                const auto& processSelector = itGroupProcess->second;
                if (processSelector.isRequired)
                    ++numMetRequirements;

                auto succeed = processProp(propGroup.value(), element.key(), &data,

                    itGroupProcess->first.c_str(), processSelector,

                    hasAnyValue);

                if (!succeed)
                    return false;
            }

            // numMetRequirements should never greater than sumRequirements
            assert(numMetRequirements <= sumRequirements);

            if (numMetRequirements != sumRequirements)
            {
                errorStr += StringFormat::FormatString(
                    keyOfArray, ": ", element.key(), u8" 缺少需要的组,请检查!\n");
                assert(false);
                return false;
            }
        }
        else
        {
            auto succeed = processProp(element.value(), element.key(), &data,

                k_nullSubGroupKey.c_str(), groupProcessMap.at(k_nullSubGroupKey),

                hasAnyValue);

            if (!succeed)
                return false;
        }

        if (!hasAnyValue)
        {
            errorStr += StringFormat::FormatString(
                keyOfArray, ": ", element.key(), u8", 未找到任何有效词条\n");
            return false;
        }
    }
    return true;
}

} // namespace JsonUtils

namespace GearJson
{
struct XianqiProcessArrayDesc : public JsonUtils::BufferArrayDescBase
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
struct TouXiangProcessArrayDesc : public JsonUtils::BufferArrayDescBase
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
struct XianLvProcessArrayDesc : public JsonUtils::BufferArrayDescBase
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
struct XianZhiProcessArrayDesc : public JsonUtils::BufferArrayDescBase
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
                JsonUtils::ProcessSelectorWrapper<&Json::is_string, XianZhiData, std::string>(
                    &XianZhiData::xianlvName, k_key),
                false // False means this key is optional in Json file.
            );
        }
        return s_outMap;
    }
};
struct XianRenProcessArrayDesc : public JsonUtils::BufferArrayDescBase
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
                JsonUtils::ProcessSelectorWrapper<&Json::is_string, XianRenData, std::string>(
                    &XianRenData::xianZhiName, k_key),
                false // False means this key is optional in Json file.
            );
        }
        return s_outMap;
    }
};

// Helper functions for parsing groups
template <bool UsePropGroups = true, bool IgnoreUnKnowPropGroup = false, typename TypeTargetData>
bool ParseGroups(const Json& jsonRoot, const char* fileName, std::string& errorStr,
    std::vector<TypeTargetData>& targetDataVec, const JsonUtils::BufferArrayDescBase& desc,
    std::unordered_map<std::string, const TypeTargetData* const>* pTargetLoopUpTable = nullptr)
{
    auto succeed = JsonUtils::ParseBuffArray<UsePropGroups, IgnoreUnKnowPropGroup>(
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
                errorStr += StringFormat::FormatString(
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
    std::vector<TypeTargetData>& targetDataVec, const JsonUtils::BufferArrayDescBase& desc,
    std::unordered_map<std::string, const TypeTargetData* const>* pTargetLoopUpTable,
    std::unordered_map<std::string, const TypeRefData* const>* pRefLoopUptable,
    std::string TypeTargetData::*pRefStrMemberPtr,
    const TypeRefData* TypeTargetData::*pRefPtrMemberPtr)
{
    auto succeed = ParseGroups<UsePropGroups, IgnoreUnKnowPropGroup>(
        jsonRoot, fileName, errorStr, targetDataVec, desc, pTargetLoopUpTable);
    if (!succeed)
        return false;

    if (pRefLoopUptable != nullptr)
    {
        auto& refTable = *pRefLoopUptable;
        for (auto& data : targetDataVec)
        {
            if (!(data.*pRefStrMemberPtr).empty())
            {
                auto it = refTable.find(data.*pRefStrMemberPtr);
                if (it != refTable.end())
                {
                    data.*pRefPtrMemberPtr = it->second;
                }
                else
                {
                    errorStr += StringFormat::FormatString(u8"文件: ", fileName, ", ",
                        desc.GetKeyOfArray(), u8" 中的节点: ", data.name,
                        ", 中的属性: ", data.*pRefStrMemberPtr, u8" 不存在,请检查!\n");
                    assert(false);
                    return false;
                }
            }
        }
    }

    return true;
};

constexpr auto k_xianrenKey_baseUnitScale = u8"仙人基础属性单位";
} // namespace XianjieJson
} // namespace

namespace GearCalc
{
bool XianqiFileData::ReadFromJsonFile(
    const char* fileName, std::string& errorStr, XianqiFileData& outGearFileData)
{
    outGearFileData.Reset();
    Json jsonRoot;
    if (!JsonUtils::ParseJsonFile(fileName, errorStr, jsonRoot))
        return false;

    // Start config

    // Parse max num equip
    {
        auto it = jsonRoot.find(GearJson::k_gearKey_numEquip);
        if (it == jsonRoot.end() || !it->is_number_unsigned())
        {
            errorStr += StringFormat::FormatString(
                u8"文件: ", fileName, u8" 缺少有效的: ", GearJson::k_gearKey_numEquip, "\n");
            return false;
        }

        outGearFileData.m_maxNumEquip = it->get<std::uint32_t>();
    }

    // Parse each gear
    {
        auto succeed = JsonUtils::ParseBuffArray<true>(
            GetSingletonInstance<GearJson::XianqiProcessArrayDesc>(), fileName, jsonRoot, errorStr,
            outGearFileData.m_gearsDataVec);

        if (!succeed)
            return false;
    }

    return true;
}

void XianqiFileData::Reset()
{
    m_maxNumEquip = 0;
    m_gearsDataVec.clear();
}

bool XianjieFileData::ReadFromJsonFile(
    const char* fileName, std::string& errorStr, XianjieFileData& outXianjieFileData)
{
    outXianjieFileData.Reset();
    Json jsonRoot;
    if (!JsonUtils::ParseJsonFile(fileName, errorStr, jsonRoot))
        return false;

    // Start config

    // Parse each Touxiang
    {
        auto succeed = XianjieJson::ParseGroups<false>(jsonRoot, fileName, errorStr,
            outXianjieFileData.m_touxiangDataVec,
            GetSingletonInstance<XianjieJson::TouXiangProcessArrayDesc>());
        if (!succeed)
            return false;
    }

    // Parse each Xianlv
    {
        auto succeed = XianjieJson::ParseGroups<true>(jsonRoot, fileName, errorStr,
            outXianjieFileData.m_xianLvDataVec,
            GetSingletonInstance<XianjieJson::XianLvProcessArrayDesc>(),
            &outXianjieFileData.m_loopUpXainlv);
        if (!succeed)
            return false;
    }

    // Parse each Xianzhi
    {
        auto succeed = XianjieJson::ParseGroups<true>(jsonRoot, fileName, errorStr,
            outXianjieFileData.m_xianZhiDataVec,
            GetSingletonInstance<XianjieJson::XianZhiProcessArrayDesc>(),
            &outXianjieFileData.m_loopUpXainZhi, &outXianjieFileData.m_loopUpXainlv,
            &XianZhiData::xianlvName, &XianZhiData::pXianlv);
        if (!succeed)
            return false;
    }

    // Parse Xianre unit scale
    {
        auto it = jsonRoot.find(XianjieJson::k_xianrenKey_baseUnitScale);
        if (it == jsonRoot.end() || !it->is_string())
        {
            errorStr += StringFormat::FormatString(u8"文件: ", fileName, u8" 缺少有效的: ",
                XianjieJson::k_xianrenKey_baseUnitScale, "\n");
            return false;
        }
        auto unitScaleStr                         = it->get<std::string>();
        outXianjieFileData.m_xianrenBaseUnitScale = UnitScale::GetEnumFromStr(unitScaleStr);

        // Check if it is valid
        if (!UnitScale::IsValid(outXianjieFileData.m_xianrenBaseUnitScale))
        {
            errorStr += StringFormat::FormatString(u8"文件: ", fileName, u8" 中的: ",
                XianjieJson::k_xianrenKey_baseUnitScale, u8", 其值为: ", unitScaleStr,
                u8", 不是有效的单位\n");
            return false;
        }
    }

    // Parse each Xianren
    {
        auto succeed = XianjieJson::ParseGroups<true>(jsonRoot, fileName, errorStr,
            outXianjieFileData.m_xianrenDataVec,
            GetSingletonInstance<XianjieJson::XianRenProcessArrayDesc>(),
            &outXianjieFileData.m_loopUpXainRen, &outXianjieFileData.m_loopUpXainZhi,
            &XianRenData::xianZhiName, &XianRenData::pXianzhi);
        if (!succeed)
            return false;
    }

    return true;
}

void XianjieFileData::Reset()
{
    m_xianrenBaseUnitScale = UnitScale::NotValid;

    m_loopUpXainRen.clear();
    m_loopUpXainZhi.clear();
    m_loopUpXainlv.clear();

    m_xianrenDataVec.clear();
    m_xianZhiDataVec.clear();
    m_xianLvDataVec.clear();
    m_touxiangDataVec.clear();
}

} // namespace GearCalc
