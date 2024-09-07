#pragma once
#include <cstdint>
#include <memory>
#include <string_view>


#ifdef _WIN32
namespace ZQF::ZxFS::Plat
{
    auto PathUTF8ToWide(const std::string_view msPath) -> std::pair<std::wstring_view, std::unique_ptr<wchar_t[]>>;
    auto PathWideToUTF8(const std::wstring_view wsPath) -> std::pair<std::string_view, std::unique_ptr<char[]>>;
    auto PathUTF8ToWide(const std::string_view msPath, wchar_t* wpBuffer, const std::size_t nBufferChars) -> std::size_t;
    auto PathWideToUTF8(const std::wstring_view wsPath, char* cpBuffer, const std::size_t nBufferBytes) -> std::size_t;
}
#elif __linux__
namespace ZQF::ZxFS::Plat
{
    auto PathMaxBytes() -> std::size_t;
}
#endif
