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
        const auto char_cnt = static_cast<std::size_t>(::MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, msPath.data(), static_cast<int>(msPath.size()), wpBuffer, MAX_PATH));
        wpBuffer[char_cnt] = {};
        return char_cnt;
    }

    static auto PathWideToUTF8(const std::wstring_view wsPath, char* cpBuffer) -> std::size_t
    {
        BOOL not_all_cvt = TRUE;
        const auto bytes = static_cast<std::size_t>(::WideCharToMultiByte(CP_UTF8, 0, wsPath.data(), static_cast<int>(wsPath.size()), cpBuffer, MAX_PATH, nullptr, &not_all_cvt));
        cpBuffer[bytes] = {};
        return bytes;
    }
}

namespace ZQF::ZxFS
{
    Walk::Walk(const std::string_view msWalkDir)
    {
        if (!msWalkDir.ends_with('/')) { throw std::runtime_error(std::format("ZxPath::Walk : dir format error! -> \'{}\'", msWalkDir)); }

        wchar_t wide_path[MAX_PATH];
        size_t wide_path_char_cnt = Private::PathUTF8ToWide(msWalkDir, wide_path);
        wide_path[wide_path_char_cnt + 0] = L'*';
        wide_path[wide_path_char_cnt + 1] = L'\0';

        WIN32_FIND_DATAW find_data;
        const auto hfind = ::FindFirstFileW(wide_path, &find_data);
        if (hfind == INVALID_HANDLE_VALUE) { throw std::runtime_error(std::format("ZxPath::Walk : dir open error! -> \'{}\'", msWalkDir)); }
        m_hFind = reinterpret_cast<std::uintptr_t>(hfind);

        m_upSearchDir = std::make_unique_for_overwrite<char[]>((msWalkDir.size() + 1) * sizeof(char));
        std::memcpy(m_upSearchDir.get(), msWalkDir.data(), msWalkDir.size() * sizeof(char));
        m_nSearchDirBytes = msWalkDir.size() * sizeof(char);
        m_upSearchDir[m_nSearchDirBytes] = '\0';
    }

    Walk::~Walk()
    {
        ::FindClose(reinterpret_cast<HANDLE>(m_hFind));
    }

    auto Walk::GetPath() const -> std::string
    {
        return std::string{}.append(m_upSearchDir.get(), m_nSearchDirBytes).append(m_aName, m_nNameBytes);
    }

    auto Walk::GetName() const -> std::string_view
    {
        return { m_aName, m_nNameBytes };
    }

    auto Walk::GetSearchDir() const->std::string_view
    {
        return { m_upSearchDir.get(), m_nSearchDirBytes };
    }

    auto Walk::NextDir() -> bool
    {
        WIN32_FIND_DATAW find_data;
        while (::FindNextFileW(reinterpret_cast<HANDLE>(m_hFind), &find_data))
        {
            if ((*reinterpret_cast<std::uint32_t*>(find_data.cFileName)) == std::uint32_t(0x0000002E)) { continue; }
            if (((*reinterpret_cast<std::uint64_t*>(find_data.cFileName)) & 0x000000FFFFFFFFFF) == std::uint64_t(0x00000000002E002E)) { continue; }

            if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                m_nNameBytes = Private::PathWideToUTF8({ find_data.cFileName }, m_aName);
                if ((m_nNameBytes + 1) >= MAX_PATH) { return false; }
                m_aName[m_nNameBytes + 0] = '/';
                m_aName[m_nNameBytes + 1] = '\0';
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
            if ((*reinterpret_cast<std::uint32_t*>(find_data.cFileName)) == std::uint32_t(0x0000002E)) { continue; }
            if (((*reinterpret_cast<std::uint64_t*>(find_data.cFileName)) & 0x000000FFFFFFFFFF) == std::uint64_t(0x00000000002E002E)) { continue; }

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
        wchar_t wide_path[MAX_PATH];
        Private::PathUTF8ToWide(msPath, wide_path);
        return ::DeleteFileW(wide_path) != FALSE;
    }

    auto FileMove(const std::string_view msExistPath, const std::string_view msNewPath) -> bool
    {
        wchar_t wide_exist_path[MAX_PATH];
        wchar_t wide_new_path[MAX_PATH];
        Private::PathUTF8ToWide(msExistPath, wide_exist_path);
        Private::PathUTF8ToWide(msNewPath, wide_new_path);
        return ::MoveFileW(wide_exist_path, wide_new_path) != FALSE;
    }

    auto FileCopy(const std::string_view msExistPath, const std::string_view msNewPath, bool isFailIfExists) -> bool
    {
        wchar_t wide_exist_path[MAX_PATH];
        wchar_t wide_new_path[MAX_PATH];
        Private::PathUTF8ToWide(msExistPath, wide_exist_path);
        Private::PathUTF8ToWide(msNewPath, wide_new_path);
        return ::CopyFileW(wide_exist_path, wide_new_path, isFailIfExists ? TRUE : FALSE) != FALSE;
    }

    auto FileSize(const std::string_view msPath) -> std::optional<std::uint64_t>
    {
        WIN32_FIND_DATAW find_data;
        wchar_t wide_path[MAX_PATH];
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
        if ((wsPath.size() + 1) >= MAX_PATH) { return false; }

        WCHAR path_buffer[MAX_PATH];
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
            if ((item_path_size + 1) >= MAX_PATH) { return false; }

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
        if (!msPath.ends_with('/')) { return false; }
        wchar_t wide_path[MAX_PATH];
        Private::PathUTF8ToWide(msPath, wide_path);
        return DirContentDeleteImp(wide_path);
    }

    auto DirDelete(const std::string_view msPath, bool isRecursive) -> bool
    {
        if (!msPath.ends_with('/')) { return false; }
        wchar_t wide_path[MAX_PATH];
        std::size_t char_cnt = Private::PathUTF8ToWide(msPath, wide_path);
        if (isRecursive) { DirContentDeleteImp({ wide_path ,char_cnt }); }
        return ::RemoveDirectoryW(wide_path) != FALSE;
    }

    auto DirMake(const std::string_view msPath, bool isRecursive) -> bool
    {
        if (!msPath.ends_with('/')) { return false; }

        wchar_t wide_path[MAX_PATH];
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
        wchar_t wide_path[MAX_PATH];
        Private::PathUTF8ToWide(msPath, wide_path);
        return ::GetFileAttributesW(wide_path) == INVALID_FILE_ATTRIBUTES ? false : true;
    }
}
#elif __linux__
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>

namespace ZQF::ZxFS
{
    Walk::Walk(const std::string_view msWalkDir)
    {
        if (!msWalkDir.ends_with('/')) { throw std::runtime_error(std::format("ZxPath::Walk : dir format error! -> \'{}\'", msWalkDir)); }

        const auto dir_ptr = ::opendir(msWalkDir.data());
        if (dir_ptr == nullptr) { throw std::runtime_error(std::format("ZxPath::Walk : dir open error! -> \'{}\'", msWalkDir)); }
        m_hFind = reinterpret_cast<std::uintptr_t>(dir_ptr);

        const auto walk_dir_bytes = msWalkDir.size() * sizeof(char);
        m_upSearchDir = std::make_unique_for_overwrite<char[]>(walk_dir_bytes + 1);
        std::memcpy(m_upSearchDir.get(), msWalkDir.data(), walk_dir_bytes);
        m_nSearchDirBytes = walk_dir_bytes;
        m_upSearchDir[m_nSearchDirBytes] = '\0';
    }

    Walk::~Walk()
    {
        ::closedir(reinterpret_cast<DIR*>(m_hFind));
    }

    auto Walk::GetPath() const -> std::string
    {
        return std::string{}.append(m_upSearchDir.get(), m_nSearchDirBytes).append(m_aName, m_nNameBytes);
    }

    auto Walk::GetName() const -> std::string_view
    {
        return { m_aName, m_nNameBytes };
    }

    auto Walk::GetSearchDir() const->std::string_view
    {
        return { m_upSearchDir.get(), m_nSearchDirBytes };
    }

    auto Walk::NextDir() -> bool
    {
        while (const auto entry_ptr = ::readdir(reinterpret_cast<DIR*>(m_hFind)))
        {
            if ((*reinterpret_cast<std::uint16_t*>(entry_ptr->d_name)) == std::uint32_t(0x002E)) { continue; }
            if (((*reinterpret_cast<std::uint32_t*>(entry_ptr->d_name)) & 0x00FFFFFF) == std::uint32_t(0x00002E2E)) { continue; }
            if (entry_ptr->d_type != DT_DIR) { continue; }

            m_nNameBytes = std::strlen(entry_ptr->d_name);
            m_aName = entry_ptr->d_name;
            m_aName[m_nNameBytes + 0] = '/';
            m_aName[m_nNameBytes + 1] = '\0';
            return true;
        }

        return false;
    }

    auto Walk::NextFile() -> bool
    {
        while (const auto entry_ptr = ::readdir(reinterpret_cast<DIR*>(m_hFind)))
        {
            if ((*reinterpret_cast<std::uint16_t*>(entry_ptr->d_name)) == std::uint32_t(0x002E)) { continue; }
            if (((*reinterpret_cast<std::uint32_t*>(entry_ptr->d_name)) & 0x00FFFFFF) == std::uint32_t(0x00002E2E)) { continue; }
            if (entry_ptr->d_type != DT_REG) { continue; }

            m_nNameBytes = std::strlen(entry_ptr->d_name);
            m_aName = entry_ptr->d_name;
            return true;
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
    static auto GetPathMaxBytes() -> ssize_t
    {
        auto path_max_byte_ret = ::pathconf(".", _PC_PATH_MAX);
        return static_cast<ssize_t>(path_max_byte_ret == -1 ? PATH_MAX : path_max_byte_ret);
    }

    auto SelfDir() -> std::pair<std::string_view, std::unique_ptr<char[]>>
    {
        ssize_t path_max_byte = ZxFS::GetPathMaxBytes();

        do
        {
            auto buffer = std::make_unique_for_overwrite<char[]>(path_max_byte);
            const auto write_bytes = readlink("/proc/self/cwd", buffer.get(), path_max_byte);
            if (write_bytes == -1) { break; }
            if (write_bytes != path_max_byte)
            {
                buffer[write_bytes + 0] = '/';
                buffer[write_bytes + 1] = {};
                return { { buffer.get(), static_cast<std::size_t>(write_bytes) + 1 }, std::move(buffer) };
            }
            path_max_byte *= 2;
        } while (true);

        return { "", nullptr };
    }

    auto SelfPath() -> std::pair<std::string_view, std::unique_ptr<char[]>>
    {
        ssize_t path_max_byte = ZxFS::GetPathMaxBytes();

        do
        {
            auto buffer = std::make_unique_for_overwrite<char[]>(path_max_byte);
            const auto write_bytes = readlink("/proc/self/exe", buffer.get(), path_max_byte);
            if (write_bytes == -1) { break; }
            if (write_bytes != path_max_byte)
            {
                buffer[write_bytes] = {};
                return { { buffer.get(), static_cast<std::size_t>(write_bytes) }, std::move(buffer) };
            }
            path_max_byte *= 2;
        } while (true);

        return { "", nullptr };
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
        return ::remove(msPath.data()) != -1;
    }

    auto FileMove(const std::string_view msExistPath, const std::string_view msNewPath) -> bool
    {
        return ::rename(msExistPath.data(), msNewPath.data()) != -1;
    }

    auto FileCopy(const std::string_view msExistPath, const std::string_view msNewPath, bool isFailIfExists) -> bool
    {
        const auto fd_exist = ::open(msExistPath.data(), O_RDONLY);
        if (fd_exist == -1)
        {
            return false;
        }

        const auto fd_new = ::open(msNewPath.data(), isFailIfExists ? O_CREAT | O_WRONLY | O_EXCL : O_CREAT | O_WRONLY | O_TRUNC, 0777);
        if (fd_new == -1)
        {
            ::close(fd_exist);
            return false;
        }

        struct stat st;
        const auto fstat_status = fstat(fd_exist, &st);
        if (fstat_status == -1)
        {
            ::close(fd_exist);
            ::close(fd_new);
            return false;
        }

        ssize_t cp_bytes{};
        auto remain_bytes = st.st_size;
        do
        {
            cp_bytes = ::copy_file_range(fd_exist, 0, fd_new, 0, remain_bytes, 0);
            if (cp_bytes == -1) { break; }
            remain_bytes -= cp_bytes;
        } while (remain_bytes > 0 && cp_bytes > 0);

        ::close(fd_exist);
        ::close(fd_new);
        return remain_bytes == 0 ? true : false;
    }

    auto FileSize(const std::string_view msPath) -> std::optional<std::uint64_t>
    {
        struct stat st;
        const auto status = stat(msPath.data(), &st);
        return (status != -1) ? std::optional{ static_cast<std::uint16_t>(st.st_size) } : std::nullopt;
    }

    static auto DirContentDeleteImp(const std::string_view msPath, const ssize_t nPathMaxBytes) -> bool
    {
        if (msPath.size() >= static_cast<std::size_t>(nPathMaxBytes)) { return false; }

        auto path_buffer = std::make_unique_for_overwrite<char[]>(static_cast<std::size_t>(nPathMaxBytes));
        {
            std::memcpy(path_buffer.get(), msPath.data(), msPath.size() * sizeof(char));
            path_buffer[msPath.size()] = '\0';
        }

        const auto dir_ptr = ::opendir(path_buffer.get());
        if (dir_ptr == nullptr) { return false; }

        while (const auto entry_ptr = ::readdir(dir_ptr))
        {
            if ((*reinterpret_cast<std::uint16_t*>(entry_ptr->d_name)) == std::uint32_t(0x002E)) { continue; }
            if (((*reinterpret_cast<std::uint32_t*>(entry_ptr->d_name)) & 0x00FFFFFF) == std::uint32_t(0x00002E2E)) { continue; }

            const std::string_view name{ entry_ptr->d_name };
            const auto path_bytes = msPath.size() + name.size();
            if ((path_bytes + 1) >= static_cast<std::size_t>(nPathMaxBytes)) { return false; }
            ::memcpy(path_buffer.get() + msPath.size(), name.data(), name.size());
            if (entry_ptr->d_type == DT_DIR)
            {
                path_buffer[path_bytes + 0] = '/';
                path_buffer[path_bytes + 1] = '\0';
                bool status = ZxFS::DirContentDeleteImp({ path_buffer.get(), path_bytes + 1 }, nPathMaxBytes);
                if (status == false) { return false; }
                ::rmdir(path_buffer.get());
            }
            else
            {
                path_buffer[path_bytes] = '\0';
                ::remove(path_buffer.get());
            }
        }

        return ::closedir(dir_ptr) != -1 ? true : false;
    }

    auto DirContentDelete(const std::string_view msPath) -> bool
    {
        if (!msPath.ends_with('/')) { return false; }
        return DirContentDeleteImp(msPath, ZxFS::GetPathMaxBytes());
    }

    auto DirDelete(const std::string_view msPath, bool isRecursive) -> bool
    {
        if (!msPath.ends_with('/')) { return false; }
        if (isRecursive) { DirContentDeleteImp(msPath, ZxFS::GetPathMaxBytes()); }
        return ::rmdir(msPath.data()) != -1;
    }

    auto DirMake(const std::string_view msPath, bool isRecursive) -> bool
    {
        if (!msPath.ends_with('/')) { return false; }

        if (!isRecursive)
        {
            return ::mkdir(msPath.data(), 0777) != -1;
        }
        else
        {
            auto path_buffer = std::make_unique_for_overwrite<char[]>(msPath.size() + 1);
            std::memcpy(path_buffer.get(), msPath.data(), msPath.size() * sizeof(char));
            path_buffer[msPath.size()] = {};

            char* cur_path_cstr = path_buffer.get();
            const char* org_path_cstr = path_buffer.get();

            while (*cur_path_cstr++ != '\0')
            {
                if (*cur_path_cstr != '/') { continue; }

                *cur_path_cstr = {};
                {
                    if (::access(org_path_cstr, X_OK) == -1)
                    {
                        ::mkdir(org_path_cstr, 0777);
                    }
                }
                *cur_path_cstr = '/';
                cur_path_cstr++;
            }

            return true;
        }
    }

    auto Exist(const std::string_view msPath) -> bool
    {
        return ::access(msPath.data(), F_OK) != -1;
    }
}

#endif


