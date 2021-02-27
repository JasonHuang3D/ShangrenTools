//
// Author: Jason Huang(jasonhuang1988@gmail.com) 2021
//

#include "JUtils/pch.h"

#include "UserData.h"

#include "JUtils/Utils.h"
#include "nlohmann/json.hpp"

#include <fstream>
#include <functional>
#include <unordered_map>
#include <unordered_set>

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

struct JsonProcessOp
{
    enum Enum : std::uint8_t
    {
        Assign = 0,
        Increase,

        NumOps
    };
};
struct TargetProcessType
{
    enum Enum : std::uint8_t
    {
        Gear = 0,
        Xianlv,

        NumTargets
    };
};

struct ProcessContainer
{
    using JsonProcessOpArrayType = std::array<JsonProcessPtrType, JsonProcessOp::NumOps>;
    using TargetProcessArrayType   = std::array<TargetProcessPtrType, TargetProcessType::NumTargets>;

    ProcessContainer(
        JsonProcessOpArrayType&& jsonProcessList, TargetProcessArrayType&& buffProcessList) :
        jsonProcessOpArray(std::move(jsonProcessList)),
        targetProcessArray(std::move(buffProcessList))
    {
    }

    JsonProcessOpArrayType jsonProcessOpArray;
    TargetProcessArrayType targetProcessArray;
};

using BuffProcessMap = std::unordered_map<std::string, ProcessContainer>;
// std::array<std::unique_ptr<JsonProcessBase>, JsonProcessOp::NumOps>>;
const BuffProcessMap& GetBuffProcessMap()
{
    static BuffProcessMap s_outMap;
    if (s_outMap.empty())
    {
        auto configMap = [](const char* mapKey, auto pJsonMemberPtr, auto pGearMemberPtr) {
            s_outMap.try_emplace(mapKey,
                ProcessContainer::JsonProcessOpArrayType {

                    // JsonProcessOp::Assign
                    CreateJsonProcessHelper(pJsonMemberPtr, [](auto& a, auto b) -> void { a = b; }),
                    // JsonProcessOp::Increase
                    CreateJsonProcessHelper(pJsonMemberPtr, [](auto& a, auto b) -> void { a += b; })

                },
                ProcessContainer::TargetProcessArrayType {

                    CreateTargetProcessHelper(pGearMemberPtr)

                }

            );
        };

        configMap(u8"各力数值", &IndividualBuff::geLi_add, &GearData::individualBuff);
        configMap(u8"各念数值", &IndividualBuff::geNian_add, &GearData::individualBuff);
        configMap(u8"各福数值", &IndividualBuff::geFu_add, &GearData::individualBuff);
        configMap(u8"各力百分比", &IndividualBuff::geLi_percent, &GearData::individualBuff);
        configMap(u8"各念百分比", &IndividualBuff::geNian_percent, &GearData::individualBuff);
        configMap(u8"各福百分比", &IndividualBuff::geFu_percent, &GearData::individualBuff);

        configMap(u8"总力数值", &GlobalBuff::zongLi_add, &GearData::globalBuff);
        configMap(u8"总念数值", &GlobalBuff::zongNian_add, &GearData::globalBuff);
        configMap(u8"总福数值", &GlobalBuff::zongFu_add, &GearData::globalBuff);
        configMap(u8"总力百分比", &GlobalBuff::zongLi_percent, &GearData::globalBuff);
        configMap(u8"总念百分比", &GlobalBuff::zongNian_percent, &GearData::globalBuff);
        configMap(u8"总福百分比", &GlobalBuff::zongFu_percent, &GearData::globalBuff);

        configMap(u8"产晶数值", &ChanyeBuff::chanJing_add, &GearData::chanyeBuff);
        configMap(u8"产能数值", &ChanyeBuff::chanNeng_add, &GearData::chanyeBuff);
        configMap(u8"产晶百分比", &ChanyeBuff::chanjing_percent, &GearData::chanyeBuff);
        configMap(u8"产能百分比", &ChanyeBuff::chanNeng_percent, &GearData::chanyeBuff);
    }
    return s_outMap;
}
} // namespace JsonUtils

namespace GearJson
{
const std::unordered_set<std::string> k_gearKey_propGroups { u8"白色", u8"蓝色" };

constexpr auto k_gearKey_gearArray = u8"仙器";
constexpr auto k_gearKey_numEquip  = u8"仙器佩戴数量";
} // namespace GearJson
} // namespace

namespace GearCalc
{
bool GearFileData::ReadFromJsonFile(
    const char* fileName, std::string& errorStr, GearFileData& outGearFileData)
{
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
                u8"文件: ", fileName, u8" 缺少有效的: ", GearJson::k_gearKey_numEquip);
            return false;
        }

        outGearFileData.m_maxNumEquip = it->get<std::uint32_t>();
    }

    // Parse each gear
    {

        auto itGearObj = jsonRoot.find(GearJson::k_gearKey_gearArray);
        if (itGearObj == jsonRoot.end() || !itGearObj->is_object())
        {
            errorStr += StringFormat::FormatString(
                u8"文件: ", fileName, u8" 缺少有效的: ", GearJson::k_gearKey_gearArray);
            return false;
        }

        const auto& gearsObj = *itGearObj;
        auto& outGearDataVec = outGearFileData.m_gearsDataVec;
        outGearDataVec.clear();
        outGearDataVec.reserve(gearsObj.size());
        const auto& kBuffProcessMap = JsonUtils::GetBuffProcessMap();
        for (auto& gearElement : gearsObj.items())
        {
            // Create GearData
            auto& gearData = outGearDataVec.emplace_back();
            gearData.name  = gearElement.key().c_str();

            bool hasAnyValue = false;
            for (auto& gearPropGroup : gearElement.value().items())
            {
                if (GearJson::k_gearKey_propGroups.find(gearPropGroup.key()) ==
                    GearJson::k_gearKey_propGroups.end())
                {
                    errorStr += StringFormat::FormatString(u8"仙器: ", gearElement.key(),
                        u8", 未找到任何词条的组, 请填写 \"白色\" 或 \"蓝色\" \n");
                    return false;
                }

                for (auto& gearProp : gearPropGroup.value().items())
                {
                    auto propIt = kBuffProcessMap.find(gearProp.key());
                    if (propIt == kBuffProcessMap.end())
                    {
                        errorStr += StringFormat::FormatString(u8"仙器: ", gearElement.key(),
                            u8", 词条组:", gearPropGroup.key(), u8", 词条: ", gearProp.key(),
                            u8" 无效请纠正!\n");
                        return false;
                    }

                    auto& processEntity = propIt->second;

                    auto& targetProcess =
                        processEntity.targetProcessArray[JsonUtils::TargetProcessType::Gear];
                    auto& jsonProcess =
                        processEntity.jsonProcessOpArray[JsonUtils::JsonProcessOp::Increase];

                    targetProcess->Process(&gearData, jsonProcess, gearProp.value());
                    hasAnyValue = true;
                }
            }

            if (!hasAnyValue)
            {
                errorStr += StringFormat::FormatString(
                    u8"仙器: ", gearElement.key(), u8", 未找到任何有效词条\n");
                return false;
            }
        }
    }

    return true;
}

} // namespace GearCalc
