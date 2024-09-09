#pragma once
#include <vector>
#include <string>
#include <string_view>


namespace ZQF::Zut::ZxFS
{
    class Searcher
    {
    public:
        static auto GetFilePaths(const std::string_view msSearchDir, const bool isWithDir, const bool isRecursive) -> std::vector<std::string>;
        static auto GetFilePaths(std::vector<std::string>& vcPaths, const std::string_view msSearchDir, const bool isWithDir, const bool isRecursive) -> bool;

    };
} // namespace ZQF::Zut::ZxFS
