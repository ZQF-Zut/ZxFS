#include <ZxFS/ZxFS.h>
#include <format>
#include <stdexcept>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>


namespace ZQF::ZxFS::Private
{
    static auto PathUTF8ToWide(const std::string_view msPath) -> std::pair<std::wstring_view, std::unique_ptr<wchar_t[]>>
    {
        const std::size_t buffer_max_chars = ((msPath.size() + 1) * sizeof(char)) * 2;
        auto buffer = std::make_unique_for_overwrite<wchar_t[]>(buffer_max_chars);
        const auto char_count_real = static_cast<std::size_t>(::MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, msPath.data(), static_cast<int>(msPath.size()), buffer.get(), static_cast<int>(buffer_max_chars)));
        buffer[char_count_real] = {};
        return { std::wstring_view{ buffer.get(), char_count_real }, std::move(buffer) };
    }

    static auto PathWideToUTF8(const std::wstring_view wsPath) -> std::pair<std::string_view, std::unique_ptr<char[]>>
    {
        const std::size_t buffer_max_bytes = ((wsPath.size() + 1) * sizeof(wchar_t)) * 2;
        auto buffer = std::make_unique_for_overwrite<char[]>(buffer_max_bytes);
        BOOL not_all_cvt = TRUE;
        const auto bytes_real = static_cast<std::size_t>(::WideCharToMultiByte(CP_UTF8, 0, wsPath.data(), static_cast<int>(wsPath.size()), buffer.get(), static_cast<int>(buffer_max_bytes), nullptr, &not_all_cvt));
        buffer[bytes_real] = {};
        return { std::string_view{ buffer.get(), bytes_real }, std::move(buffer) };
    }

    static auto PathWideToUTF8(const std::wstring_view wsPath, std::string& msBuffer) -> void
    {
        const std::size_t buffer_max_bytes = ((wsPath.size() + 1) * sizeof(wchar_t)) * 2;
        msBuffer.resize_and_overwrite(buffer_max_bytes, [](char*, std::size_t nBytes) {return nBytes;});
        BOOL not_all_cvt = TRUE;
        const auto bytes_real = static_cast<std::size_t>(::WideCharToMultiByte(CP_UTF8, 0, wsPath.data(), static_cast<int>(wsPath.size()), msBuffer.data(), static_cast<int>(buffer_max_bytes), nullptr, &not_all_cvt));
        msBuffer.resize_and_overwrite(bytes_real, [](char*, std::size_t nBytes) {return nBytes;});
    }
}

namespace ZQF::ZxFS
{
    Walk::Walk(const std::string_view msWalkDir)
    {
        if (msWalkDir[msWalkDir.size() - 1] != '/') { throw std::runtime_error(std::format("ZxPath::Walk : dir format error! -> \'{}\'", msWalkDir)); }

        m_msSearchDir = msWalkDir;
        m_msSearchDir.append(1, '*');

        WIN32_FIND_DATAW find_data;
        auto u8_path = Private::PathUTF8ToWide(m_msSearchDir);
        const auto hfind = ::FindFirstFileW(u8_path.first.data(), &find_data);
        if (hfind == INVALID_HANDLE_VALUE) { throw std::runtime_error(std::format("ZxPath::Walk : dir open error! -> \'{}\'", msWalkDir)); }
        m_hFind = reinterpret_cast<std::uintptr_t>(hfind);
    }

    Walk::~Walk()
    {
        ::FindClose(reinterpret_cast<HANDLE>(m_hFind));
    }

    auto Walk::GetPath() const -> std::string
    {
        return m_msSearchDir.substr(0, m_msSearchDir.size() - 1).append(m_msPath);
    }

    auto Walk::GetName() const -> std::string_view
    {
        return m_msPath;
    }

    auto Walk::NextDir() -> bool
    {
        WIN32_FIND_DATAW find_data;
        while (::FindNextFileW(reinterpret_cast<HANDLE>(m_hFind), &find_data))
        {
            if (find_data.cFileName[0] == L'.')
            {
                if (find_data.cFileName[1] == L'\0')
                {
                    continue;
                }
                else if (find_data.cFileName[1] == L'.')
                {
                    if (find_data.cFileName[2] == L'\0')
                    {
                        continue;
                    }
                }
            }

            if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                std::size_t char_cnt = ::wcslen(find_data.cFileName);
                if ((char_cnt + 1) >= MAX_PATH) { return false; }
                find_data.cFileName[char_cnt + 0] = L'/';
                find_data.cFileName[char_cnt + 1] = L'\0';
                Private::PathWideToUTF8({ find_data.cFileName, char_cnt + 1 }, m_msPath);
                return true;
            }
        }

        return false;
    }

    auto Walk::NextFile() -> bool
    {
        WIN32_FIND_DATAW find_data;
        while (::FindNextFileW(reinterpret_cast<HANDLE>(m_hFind), &find_data))
        {
            if (find_data.cFileName[0] == L'.')
            {
                if (find_data.cFileName[1] == L'\0')
                {
                    continue;
                }
                else if (find_data.cFileName[1] == L'.')
                {
                    if (find_data.cFileName[2] == L'\0')
                    {
                        continue;
                    }
                }
            }

            if (!(find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                Private::PathWideToUTF8(find_data.cFileName, m_msPath);
                return true;
            }
        }

        return false;
    }

    auto Walk::IsSuffix(const std::string_view msSuffix) const -> bool
    {
        return ZxFS::FileSuffix(m_msPath) == msSuffix ? true : false;
    }
}

namespace ZQF::ZxFS
{
    auto SelfDir() -> std::pair<std::string_view, std::unique_ptr<char[]>>
    {
        std::size_t buffer_max_chars = MAX_PATH;
        std::unique_ptr<wchar_t[]> buffer = std::make_unique_for_overwrite<wchar_t[]>(buffer_max_chars);
        std::size_t wrirte_chars = static_cast<std::size_t>(::GetCurrentDirectoryW(static_cast<DWORD>(buffer_max_chars), buffer.get())); // include null char

        if ((wrirte_chars + 1) > buffer_max_chars)
        {
            buffer_max_chars = wrirte_chars + 1;
            buffer = std::make_unique_for_overwrite<wchar_t[]>(buffer_max_chars);
            wrirte_chars = static_cast<std::size_t>(::GetCurrentDirectoryW(static_cast<DWORD>(buffer_max_chars), buffer.get()));
        }

        // append slash
        buffer[wrirte_chars + 0] = L'\\';
        buffer[wrirte_chars + 1] = {};

        // slash format
        auto buffer_ptr = buffer.get();
        while (*buffer_ptr++) { if (*buffer_ptr == L'\\') { *buffer_ptr = L'/'; } }

        return Private::PathWideToUTF8({ buffer.get(), wrirte_chars + 1 });
    }

    auto SelfPath() -> std::pair<std::string_view, std::unique_ptr<char[]>>
    {
        std::uint32_t written_chars{};
        std::uint32_t buffer_max_chars = MAX_PATH;
        std::unique_ptr<wchar_t[]> buffer;
        do
        {
            buffer_max_chars *= 2;
            buffer = std::make_unique_for_overwrite<wchar_t[]>(buffer_max_chars);
            written_chars = ::GetModuleFileNameW(nullptr, buffer.get(), buffer_max_chars);
        } while (written_chars >= buffer_max_chars);

        // slash format
        auto buffer_ptr = buffer.get();
        while (*buffer_ptr++) { if (*buffer_ptr == L'\\') { *buffer_ptr = L'/'; } }

        return Private::PathWideToUTF8({ buffer.get(), written_chars });
    }

    auto FileSuffix(const std::string_view msPath) -> std::string_view
    {
        for (auto ite = std::rbegin(msPath); ite != std::rend(msPath); ite++)
        {
            if (*ite == '/')
            {
                break;
            }
            else if (*ite == '.')
            {
                return msPath.substr(std::distance(ite, std::rend(msPath)));
            }
        }

        return { "", 0 };
    }

    auto FileSuffixDel(const std::string_view msPath) -> std::string_view
    {
        for (auto ite = std::rbegin(msPath); ite != std::rend(msPath); ite++)
        {
            if (*ite == '/')
            {
                break;
            }
            else if (*ite == '.')
            {
                return msPath.substr(0, std::distance(ite, std::rend(msPath)) - 1);
            }
        }

        return msPath;
    }

    auto FileName(const std::string_view msPath) -> std::string_view
    {
        const auto pos = msPath.rfind('/');
        return pos != std::string_view::npos ? msPath.substr(pos + 1) : msPath;
    }

    auto FileNameStem(const std::string_view msPath) -> std::string_view
    {
        return ZxFS::FileSuffixDel(ZxFS::FileName(msPath));
    }

    auto FileDelete(const std::string_view msPath) -> bool
    {
        const auto wide_path = Private::PathUTF8ToWide(msPath);
        return ::DeleteFileW(wide_path.first.data()) != FALSE;
    }

    auto FileCopy(const std::string_view msExistPath, const std::string_view msNewPath, bool isFailIfExists) -> bool
    {
        const auto wide_exist_path = Private::PathUTF8ToWide(msExistPath);
        const auto wide_new_path = Private::PathUTF8ToWide(msNewPath);
        return ::CopyFileW(wide_exist_path.first.data(), wide_new_path.first.data(), isFailIfExists ? TRUE : FALSE) != FALSE;
    }

    auto FileSize(const std::string_view msPath) -> std::optional<std::uint64_t>
    {
        WIN32_FIND_DATAW find_data;
        const auto hfind = ::FindFirstFileW(Private::PathUTF8ToWide(msPath).first.data(), &find_data);
        if (hfind != INVALID_HANDLE_VALUE)
        {
            ::FindClose(hfind);
        }
        else
        {
            return std::nullopt;
        }
        const auto size_l = static_cast<std::uint64_t>(find_data.nFileSizeLow);
        const auto size_h = static_cast<std::uint64_t>(find_data.nFileSizeHigh);
        return static_cast<std::uint64_t>(size_l | (size_h << 32));
    }

    auto DirDelete(const std::string_view msPath, bool isRecursive) -> bool
    {
        if (!msPath.ends_with('/')) { return false; }

        auto wide_path = Private::PathUTF8ToWide(msPath);

        if (!isRecursive)
        {
            return ::RemoveDirectoryW(wide_path.first.data()) != FALSE;
        }
        else
        {
            // std::filesystem::remove_all(msPath);
            return true;
        }
    }

    auto DirMake(const std::string_view msPath, bool isRecursive) -> bool
    {
        if (!msPath.ends_with('/')) { return false; }

        auto wide_path = Private::PathUTF8ToWide(msPath);

        if (!isRecursive)
        {
            return ::CreateDirectoryW(wide_path.first.data(), nullptr) != FALSE;
        }
        else
        {
            wchar_t* path_cstr = wide_path.second.get();
            const wchar_t* path_cstr_org = path_cstr;

            while (*path_cstr++)
            {
                if (*path_cstr != L'/') { continue; }

                wchar_t tmp = *path_cstr;
                *path_cstr = {};
                if (::GetFileAttributesW(path_cstr_org) == INVALID_FILE_ATTRIBUTES)
                {
                    ::CreateDirectoryW(path_cstr_org, nullptr);
                }
                *path_cstr = tmp;
                path_cstr++;
            }

            return true;
        }
    }

    auto Exist(const std::string_view msPath) -> bool
    {
        const auto wide_path = Private::PathUTF8ToWide(msPath);
        return ::GetFileAttributesW(wide_path.first.data()) == INVALID_FILE_ATTRIBUTES ? false : true;
    }
}
#elif __linux__
#endif


