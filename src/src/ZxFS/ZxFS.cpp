#include <ZxFS/ZxFS.h>
#include <span>
#include <format>
#include <cstring>
#include <stdexcept>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>


namespace ZQF::ZxFS::Private
{
    //static auto PathUTF8ToWide(const std::string_view msPath) -> std::pair<std::wstring_view, std::unique_ptr<wchar_t[]>>
    //{
    //    const std::size_t buffer_max_chars = ((msPath.size() + 1) * sizeof(char)) * 2;
    //    auto buffer = std::make_unique_for_overwrite<wchar_t[]>(buffer_max_chars);
    //    const auto char_count_real = static_cast<std::size_t>(::MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, msPath.data(), static_cast<int>(msPath.size()), buffer.get(), static_cast<int>(buffer_max_chars)));
    //    buffer[char_count_real] = {};
    //    return { std::wstring_view{ buffer.get(), char_count_real }, std::move(buffer) };
    //}

    static auto PathWideToUTF8(const std::wstring_view wsPath) -> std::pair<std::string_view, std::unique_ptr<char[]>>
    {
        const std::size_t buffer_max_bytes = ((wsPath.size() + 1) * sizeof(wchar_t)) * 2;
        auto buffer = std::make_unique_for_overwrite<char[]>(buffer_max_bytes);
        BOOL not_all_cvt = TRUE;
        const auto bytes_real = static_cast<std::size_t>(::WideCharToMultiByte(CP_UTF8, 0, wsPath.data(), static_cast<int>(wsPath.size()), buffer.get(), static_cast<int>(buffer_max_bytes), nullptr, &not_all_cvt));
        buffer[bytes_real] = {};
        return { std::string_view{ buffer.get(), bytes_real }, std::move(buffer) };
    }

    static auto PathUTF8ToWide(const std::string_view msPath, wchar_t* wpBuffer) -> std::size_t
    {
        const auto char_cnt = static_cast<std::size_t>(::MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, msPath.data(), static_cast<int>(msPath.size()), wpBuffer, static_cast<int>(ZXFS_MAX_PATH)));
        wpBuffer[char_cnt] = {};
        return char_cnt;
    }

    static auto PathWideToUTF8(const std::wstring_view wsPath, char* cpBuffer) -> std::size_t
    {
        BOOL not_all_cvt = TRUE;
        const auto bytes = static_cast<std::size_t>(::WideCharToMultiByte(CP_UTF8, 0, wsPath.data(), static_cast<int>(wsPath.size()), cpBuffer, static_cast<int>(ZXFS_MAX_PATH), nullptr, &not_all_cvt));
        cpBuffer[bytes] = {};
        return bytes;
    }
}

namespace ZQF::ZxFS
{
    Walk::Walk(const std::string_view msWalkDir)
    {
        if (!msWalkDir.ends_with('/')) { throw std::runtime_error(std::format("ZxPath::Walk : dir format error! -> \'{}\'", msWalkDir)); }

        std::memcpy(m_aSearchDir, msWalkDir.data(), msWalkDir.size() * sizeof(char));
        m_aSearchDir[msWalkDir.size() + 0] = '*';
        m_aSearchDir[msWalkDir.size() + 1] = '\0';
        m_nSearchDirBytes = msWalkDir.size() + 1;

        wchar_t wide_path[ZXFS_MAX_PATH];
        wide_path[0] = {};
        WIN32_FIND_DATAW find_data;
        Private::PathUTF8ToWide({ m_aSearchDir ,msWalkDir.size() + 1 }, wide_path);
        const auto hfind = ::FindFirstFileW(wide_path, &find_data);
        if (hfind == INVALID_HANDLE_VALUE) { throw std::runtime_error(std::format("ZxPath::Walk : dir open error! -> \'{}\'", msWalkDir)); }
        m_hFind = reinterpret_cast<std::uintptr_t>(hfind);
    }

    Walk::~Walk()
    {
        ::FindClose(reinterpret_cast<HANDLE>(m_hFind));
    }

    auto Walk::GetPath() const -> std::string
    {
        return std::string{}.append(m_aSearchDir, m_nSearchDirBytes - 1).append(m_aName, m_nNameBytes);
    }

    auto Walk::GetName() const -> std::string_view
    {
        return { m_aName, m_nNameBytes };
    }

    auto Walk::GetSearchDir() const->std::string_view
    {
        return { m_aSearchDir, m_nSearchDirBytes - 1 };
    }

    auto Walk::NextDir() -> bool
    {
        WIN32_FIND_DATAW find_data;
        while (::FindNextFileW(reinterpret_cast<HANDLE>(m_hFind), &find_data))
        {
            if ((*reinterpret_cast<std::uint32_t*>(find_data.cFileName)) == uint32_t(0x0000002E)) { continue; }
            if (((*reinterpret_cast<std::uint64_t*>(find_data.cFileName)) & 0x000000FFFFFFFFFF) == uint64_t(0x00000000002E002E)) { continue; }

            if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                std::size_t char_cnt = ::wcslen(find_data.cFileName);
                if ((char_cnt + 1) >= MAX_PATH) { return false; }
                find_data.cFileName[char_cnt + 0] = L'/';
                find_data.cFileName[char_cnt + 1] = L'\0';
                m_nNameBytes = Private::PathWideToUTF8({ find_data.cFileName, char_cnt + 1 }, m_aName);
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
            if ((*reinterpret_cast<std::uint32_t*>(find_data.cFileName)) == uint32_t(0x0000002E)) { continue; }
            if (((*reinterpret_cast<std::uint64_t*>(find_data.cFileName)) & 0x000000FFFFFFFFFF) == uint64_t(0x00000000002E002E)) { continue; }

            if (!(find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                m_nNameBytes = Private::PathWideToUTF8(find_data.cFileName, m_aName);
                return true;
            }
        }

        return false;
    }

    auto Walk::IsSuffix(const std::string_view msSuffix) const -> bool
    {
        return ZxFS::FileSuffix(m_aName) == msSuffix ? true : false;
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


    auto FileName(const std::string_view msPath) -> std::string_view
    {
        const auto pos = msPath.rfind('/');
        return pos != std::string_view::npos ? msPath.substr(pos + 1) : msPath;
    }

    auto FileNameStem(const std::string_view msPath) -> std::string_view
    {
        return ZxFS::FileSuffixDel(ZxFS::FileName(msPath));
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
                return msPath.substr(std::distance(ite, std::rend(msPath)) - 1);
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


    auto FileDelete(const std::string_view msPath) -> bool
    {
        wchar_t wide_path[ZXFS_MAX_PATH];
        Private::PathUTF8ToWide(msPath, wide_path);
        return ::DeleteFileW(wide_path) != FALSE;
    }

    auto FileMove(const std::string_view msExistPath, const std::string_view msNewPath) -> bool
    {
        wchar_t wide_exist_path[ZXFS_MAX_PATH];
        wchar_t wide_new_path[ZXFS_MAX_PATH];
        Private::PathUTF8ToWide(msExistPath, wide_exist_path);
        Private::PathUTF8ToWide(msNewPath, wide_new_path);
        return ::MoveFileW(wide_exist_path, wide_new_path) != FALSE;
    }

    auto FileCopy(const std::string_view msExistPath, const std::string_view msNewPath, bool isFailIfExists) -> bool
    {
        wchar_t wide_exist_path[ZXFS_MAX_PATH];
        wchar_t wide_new_path[ZXFS_MAX_PATH];
        Private::PathUTF8ToWide(msExistPath, wide_exist_path);
        Private::PathUTF8ToWide(msNewPath, wide_new_path);
        return ::CopyFileW(wide_exist_path, wide_new_path, isFailIfExists ? TRUE : FALSE) != FALSE;
    }

    auto FileSize(const std::string_view msPath) -> std::optional<std::uint64_t>
    {
        WIN32_FIND_DATAW find_data;
        wchar_t wide_path[ZXFS_MAX_PATH];
        Private::PathUTF8ToWide(msPath, wide_path);
        const auto hfind = ::FindFirstFileW(wide_path, &find_data);
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

    static auto DirContentDeleteImp(const std::wstring_view wsPath) -> bool
    {
        if ((wsPath.size() + 1) >= ZXFS_MAX_PATH) { return false; }

        WCHAR path_buffer[ZXFS_MAX_PATH];
        {
            std::memcpy(path_buffer, wsPath.data(), wsPath.size() * sizeof(wchar_t));
            path_buffer[wsPath.size() + 0] = L'*';
            path_buffer[wsPath.size() + 1] = L'\0';
        }
        
        WIN32_FIND_DATAW find_data;
        const auto hfind = ::FindFirstFileW(path_buffer, &find_data);
        if (hfind == INVALID_HANDLE_VALUE) { return false; }

        do
        {
            if ((*reinterpret_cast<std::uint32_t*>(find_data.cFileName)) == std::uint32_t(0x0000002E)) { continue; }
            if (((*reinterpret_cast<std::uint64_t*>(find_data.cFileName)) & 0x000000FFFFFFFFFF) == std::uint64_t(0x00000000002E002E)) { continue; }

            std::wstring_view item_name{ find_data.cFileName };
            std::size_t item_path_size = wsPath.size() + item_name.size();
            if ((item_path_size + 1) >= ZXFS_MAX_PATH) { return false; }

            std::memcpy(path_buffer + wsPath.size(), item_name.data(), (item_name.size() + 1) * sizeof(wchar_t));

            if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) // dir
            {
                path_buffer[item_path_size + 0] = L'/';
                path_buffer[item_path_size + 1] = L'\0';
                bool del_status = DirContentDeleteImp(std::wstring_view{ path_buffer, item_path_size + 1 });
                if (del_status == false) { return false; }
                ::RemoveDirectoryW(path_buffer);
            }
            else // file
            {
                ::DeleteFileW(path_buffer);
            }
        } while (::FindNextFileW(hfind, &find_data));

        ::FindClose(hfind);

        return true;
    }

    auto DirContentDelete(const std::string_view msPath) -> bool
    {
        wchar_t wide_path[ZXFS_MAX_PATH];
        Private::PathUTF8ToWide(msPath, wide_path);
        return DirContentDeleteImp(wide_path);
    }

    auto DirDelete(const std::string_view msPath, bool isRecursive) -> bool
    {
        if (!msPath.ends_with('/')) { return false; }
        wchar_t wide_path[ZXFS_MAX_PATH];
        std::size_t char_cnt = Private::PathUTF8ToWide(msPath, wide_path);
        if (isRecursive) { DirContentDeleteImp({ wide_path ,char_cnt }); }
        return ::RemoveDirectoryW(wide_path) != FALSE;
    }

    auto DirMake(const std::string_view msPath, bool isRecursive) -> bool
    {
        if (!msPath.ends_with('/')) { return false; }

        wchar_t wide_path[ZXFS_MAX_PATH];
        Private::PathUTF8ToWide(msPath, wide_path);

        if (!isRecursive)
        {
            return ::CreateDirectoryW(wide_path, nullptr) != FALSE;
        }
        else
        {
            wchar_t* path_cstr = wide_path;
            const wchar_t* path_cstr_org = path_cstr;

            while (*path_cstr++)
            {
                if (*path_cstr != L'/') { continue; }

                *path_cstr = {};
                if (::GetFileAttributesW(path_cstr_org) == INVALID_FILE_ATTRIBUTES)
                {
                    ::CreateDirectoryW(path_cstr_org, nullptr);
                }
                *path_cstr = L'/';
                path_cstr++;
            }

            return true;
        }
    }

    auto Exist(const std::string_view msPath) -> bool
    {
        wchar_t wide_path[ZXFS_MAX_PATH];
        Private::PathUTF8ToWide(msPath, wide_path);
        return ::GetFileAttributesW(wide_path) == INVALID_FILE_ATTRIBUTES ? false : true;
    }
}
#elif __linux__
#endif


