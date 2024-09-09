#pragma once
#include <cstdint>
#include <string>
#include <memory>
#include <optional>
#include <string_view>


namespace ZQF::Zut::ZxFS
{
    auto SelfDir() -> std::pair<std::string_view, std::unique_ptr<char[]>>;
    auto SelfPath() -> std::pair<std::string_view, std::unique_ptr<char[]>>;

    auto FileName(const std::string_view msPath) -> std::string_view;
    auto FileNameStem(const std::string_view msPath) -> std::string_view;
    auto FileSuffix(const std::string_view msPath) -> std::string_view;
    auto FileSuffixDel(const std::string_view msPath) -> std::string_view;

    auto FileDelete(const std::string_view msPath) -> bool;
    auto FileMove(const std::string_view msExistPath, const std::string_view msNewPath) -> bool;
    auto FileCopy(const std::string_view msExistPath, const std::string_view msNewPath, bool isFailIfExists) -> bool;
    auto FileSize(const std::string_view msPath) -> std::optional<std::uint64_t>;

    auto DirContentDelete(const std::string_view msPath) -> bool;
    auto DirDelete(const std::string_view msPath, bool isRecursive) -> bool;
    auto DirMake(const std::string_view msPath, bool isRecursive) -> bool;

    auto Exist(const std::string_view msPath) -> bool;
} // namespace ZQF::Zut::ZxFS
