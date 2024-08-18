#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>


namespace ZQF::ZxFS
{
    class Walker
    {
    private:
        std::uintptr_t m_hFind{};
#ifdef _WIN32
        std::unique_ptr<char[]> m_upName{};
#elif __linux__
        char* m_aName{};
#endif
        std::size_t m_nNameBytes{};
        std::unique_ptr<char[]> m_upSearchDir;
        std::size_t m_nSearchDirBytes{};

    public:
        Walker(const std::string_view msWalkDir);
        ~Walker();

    public:
        auto GetPath() const->std::string;
        auto GetName() const->std::string_view;
        auto GetSearchDir() const->std::string_view;

    public:
        auto NextDir() -> bool;
        auto NextFile() -> bool;
        auto IsSuffix(const std::string_view msSuffix) const -> bool;

    public:
        static auto GetFilePaths(const std::string_view msWalkDir, const bool isWithDir, const bool isRecursive) -> std::vector<std::string>;
        static auto GetFilePaths(std::vector<std::string>& vcPaths, const std::string_view msWalkDir, const bool isWithDir, const bool isRecursive) -> bool;
    };
} // namespace ZQF::ZxFS
