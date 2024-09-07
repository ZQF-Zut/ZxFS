#include <ZxFS/Platform.h>


#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>


namespace ZQF::ZxFS::Plat
{
    auto PathUTF8ToWide(const std::string_view msPath) -> std::pair<std::wstring_view, std::unique_ptr<wchar_t[]>>
    {
        const auto buffer_max_chars = ((msPath.size() + 1) * sizeof(char)) * 2;
        auto buffer = std::make_unique_for_overwrite<wchar_t[]>(buffer_max_chars);
        const auto chars_real = PathUTF8ToWide(msPath, buffer.get(), buffer_max_chars);
        return { std::wstring_view{ buffer.get(), chars_real }, std::move(buffer) };
    }

    auto PathWideToUTF8(const std::wstring_view wsPath) -> std::pair<std::string_view, std::unique_ptr<char[]>>
    {
        const auto buffer_max_bytes = ((wsPath.size() + 1) * sizeof(wchar_t)) * 2;
        auto buffer = std::make_unique_for_overwrite<char[]>(buffer_max_bytes);
        const auto bytes_real = PathWideToUTF8(wsPath, buffer.get(), buffer_max_bytes);
        return { std::string_view{ buffer.get(), bytes_real }, std::move(buffer) };
    }

    auto PathUTF8ToWide(const std::string_view msPath, wchar_t* wpBuffer, const std::size_t nBufferChars) -> std::size_t
    {
        const auto chars = static_cast<std::size_t>(::MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, msPath.data(), static_cast<int>(msPath.size()), wpBuffer, static_cast<int>(nBufferChars)));
        wpBuffer[chars] = {};
        return chars;
    }

    auto PathWideToUTF8(const std::wstring_view wsPath, char* cpBuffer, const std::size_t nBufferBytes) -> std::size_t
    {
        BOOL not_all_cvt = TRUE;
        const auto bytes = static_cast<std::size_t>(::WideCharToMultiByte(CP_UTF8, 0, wsPath.data(), static_cast<int>(wsPath.size()), cpBuffer, static_cast<int>(nBufferBytes), nullptr, &not_all_cvt));
        cpBuffer[bytes] = {};
        return bytes;
    }
}
#elif __linux__
#include <unistd.h>


namespace ZQF::ZxFS::Plat
{
    auto PathMaxBytes() -> std::size_t
    {
        const auto path_max_byte_ret{ ::pathconf("/", _PC_PATH_MAX) };
        return static_cast<std::size_t>(path_max_byte_ret == -1 ? PATH_MAX : path_max_byte_ret);
    }
}
#endif
