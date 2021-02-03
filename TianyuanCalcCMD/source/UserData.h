//
// Author: Jason Huang(jasonhuang1988@gmail.com) 2021
//
#pragma once

#include <string>
#include <vector>

namespace JUtils
{
struct UnitScale
{
    static constexpr std::uint64_t k10K  = 10000;
    static constexpr std::uint64_t k100M = 100000000;

    static constexpr char* GetUnitStr(std::uint64_t value)
    {
        if (value == k10K) return u8"万";
        else if (value == k100M) return u8"亿";
        return "";
    }
};

class UserData
{
public:
    UserData(const char* desc, std::uint64_t data) : m_desc(desc), m_data(data) {}

    // Enbale move
    UserData(UserData&& src) noexcept : m_desc(std::move(src.m_desc)), m_data(src.m_data) {}
    UserData& operator=(UserData&& src) noexcept
    {
        m_desc = std::move(src.m_desc);
        m_data = src.m_data;
        return *this;
    }

    // Disable copy
    UserData(const UserData&) = delete;
    UserData& operator=(const UserData&) = delete;

    // Compare operators
    bool operator<(const UserData& rhs) const { return m_data < rhs.m_data; }
    bool operator<=(const UserData& rhs) const { return m_data <= rhs.m_data; }
    bool operator>(const UserData& rhs) const { return m_data > rhs.m_data; }
    bool operator>=(const UserData& rhs) const { return m_data >= rhs.m_data; }
    bool operator==(const UserData& rhs) const { return m_data == rhs.m_data; }

    const char* GetDesc() const { return m_desc.c_str(); }
    std::uint64_t GetData() const { return m_data; }

private:
    std::string m_desc;

    // Raw data without any scale
    std::uint64_t m_data;
};

class UserDataList
{
public:
    static bool ReadFromFile(const char* fileName, std::uint64_t uintScale, UserDataList& outList);

    UserDataList() {};

    // Disable copy
    UserDataList(const UserDataList&) = delete;
    UserDataList& operator=(const UserDataList&) = delete;

    std::vector<UserData>& GetList() { return m_list; }
    std::uint64_t GetUnitScale() const { return m_unitScale; };

private:
    std::uint64_t m_unitScale = UnitScale::k10K;
    std::vector<UserData> m_list;
};

struct ResultData
{
    ResultData() {};

    // Enbale move
    ResultData(ResultData&& src) = default;
    ResultData& operator=(ResultData&& src) = default;

    // Disable copy
    ResultData(const ResultData&) = delete;
    ResultData& operator=(const ResultData&) = delete;

public:
    const UserData* m_pTarget = nullptr;
    std::vector<const UserData*> m_combination;

    std::uint64_t m_sum        = 0;
    std::uint64_t m_difference = 0;

    bool m_isExceeded = false;
};

struct ResultDataList
{
    ResultDataList() {};
    // Disable copy
    ResultDataList(const ResultDataList&) = delete;
    ResultDataList& operator=(const ResultDataList&) = delete;

public:
    std::uint64_t m_unitScale = UnitScale::k10K;
    std::vector<ResultData> m_list;

    std::uint64_t m_combiSum  = 0;
    std::uint64_t m_exeedSum  = 0;
    std::uint64_t m_remainSum = 0;
};

} // namespace JUtils