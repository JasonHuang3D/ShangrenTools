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
    enum Values : std::uint64_t
    {
        k_10K  = 10000,
        k_100M = 100000000
    };

    template <typename T>
    static constexpr char* GetUnitStr(T value)
    {
        switch (value)
        {
        case k_10K:
            return u8"万";
        case k_100M:
            return u8"亿";
        default:
            return "";
        }
    }

    template <typename T>
    static constexpr bool IsValid(T value)
    {
        switch (value)
        {
        case k_10K:
        case k_100M:
            return true;
        default:
            return false;
        }
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
    std::uint64_t GetOriginalData() const { return m_data; }
    std::uint64_t GetFixedData() const { return m_data + m_offset; }

public:
    mutable int m_offset = 0;
private:
    std::string m_desc;

    // Raw data without any scale
    std::uint64_t m_data = 0;
};

class UserDataList
{
public:
    static bool ReadFromFile(const char* fileName, std::uint64_t uintScale, UserDataList& outList);

    UserDataList() {};

    // Disable copy
    UserDataList(const UserDataList&) = delete;
    UserDataList& operator=(const UserDataList&) = delete;

    const std::vector<UserData>& GetList() const { return m_list; }
    std::uint64_t GetUnitScale() const { return m_unitScale; };

private:
    std::uint64_t m_unitScale = UnitScale::k_10K;
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

    bool isFinished() const;

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
    std::uint64_t m_unitScale = UnitScale::k_10K;
    std::vector<ResultData> m_selectedInputs;
    std::vector<const UserData*> m_remainInputs;

    std::uint64_t m_combiSum  = 0;
    std::uint64_t m_exeedSum  = 0;
    std::uint64_t m_remainSum = 0;

    std::uint32_t m_numfinished = 0;
};

} // namespace JUtils