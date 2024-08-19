#pragma once
#include <cstdint>
#include <memory>
#include <string_view>


namespace ZQF::ZxFS
{
    class Walker
    {
    private:
        std::uintptr_t m_hFind{};
#ifdef _WIN32
        std::unique_ptr<char[]> m_upCache{};
#elif __linux__
        char* m_aName{};
#endif
        std::size_t m_nNameBytes{};
        std::size_t m_nWalkDirBytes{};

    public:
        Walker(const std::string_view msWalkDir);
        ~Walker();

    public:
        auto GetPath() const->std::string_view;
        auto GetName() const->std::string_view;
        auto GetWalkDir() const->std::string_view;

    public:
        auto NextDir() -> bool;
        auto NextFile() -> bool;
        auto IsSuffix(const std::string_view msSuffix) const -> bool;
    };
} // namespace ZQF::ZxFS
