#include "Core.h"
#include "Plat.h"
#include <span>


namespace ZQF::Zut::ZxFS
{
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
}

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <stack>


namespace ZQF::Zut::ZxFS
{
    constexpr auto PATH_MAX_BYTES = 0x1000;


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

        return Plat::PathWideToUTF8({ buffer.get(), wrirte_chars + 1 });
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

        return Plat::PathWideToUTF8({ buffer.get(), written_chars });
    }

    auto FileDelete(const std::string_view msPath) -> bool
    {
        return ::DeleteFileW(Plat::PathUTF8ToWide(msPath).second.get()) != FALSE;
    }

    auto FileMove(const std::string_view msExistPath, const std::string_view msNewPath) -> bool
    {
        return ::MoveFileW(Plat::PathUTF8ToWide(msExistPath).second.get(), Plat::PathUTF8ToWide(msNewPath).second.get()) != FALSE;
    }

    auto FileCopy(const std::string_view msExistPath, const std::string_view msNewPath, bool isFailIfExists) -> bool
    {
        return ::CopyFileW(Plat::PathUTF8ToWide(msExistPath).second.get(), Plat::PathUTF8ToWide(msNewPath).second.get(), isFailIfExists ? TRUE : FALSE) != FALSE;
    }

    auto FileSize(const std::string_view msPath) -> std::optional<std::uint64_t>
    {
        WIN32_FILE_ATTRIBUTE_DATA find_data;
        const auto status = ::GetFileAttributesExW(Plat::PathUTF8ToWide(msPath).second.get(), GetFileExInfoStandard, &find_data);
        if (status == FALSE) { return std::nullopt; }
        const auto size_l = static_cast<std::uint64_t>(find_data.nFileSizeLow);
        const auto size_h = static_cast<std::uint64_t>(find_data.nFileSizeHigh);
        return static_cast<std::uint64_t>(size_l | (size_h << 32));
    }

    static auto DirContentDeleteImp(const std::string_view msBasePath, const bool isRemoveBaseDir) -> bool
    {
        std::stack<std::wstring> search_dir_stack;
        search_dir_stack.push(L"*");

        const auto path_cache{ std::make_unique_for_overwrite<wchar_t[]>(PATH_MAX_BYTES / sizeof(wchar_t)) };

        const auto base_dir_chars{ Plat::PathUTF8ToWide(msBasePath, path_cache.get(), PATH_MAX_BYTES / sizeof(wchar_t)) };
        if (base_dir_chars == 0) { return false; }

        wchar_t* cur_path_ptr{ path_cache.get() };

        do
        {
            const auto search_dir_name{ std::move(search_dir_stack.top()) };
            search_dir_stack.pop();

            // do not remove base directory
            if (search_dir_name.empty()) { break; }

            // make current dir path.
            std::memcpy(cur_path_ptr + base_dir_chars, search_dir_name.data(), search_dir_name.size() * sizeof(wchar_t));
            const auto cur_path_chars = base_dir_chars + search_dir_name.size();
            cur_path_ptr[cur_path_chars] = {};

            // remove the empty directory.
            if (search_dir_name.ends_with(L'/'))
            {
                ::RemoveDirectoryW(cur_path_ptr);
                continue;
            }

            WIN32_FIND_DATAW find_data;
            const auto hfind{ ::FindFirstFileExW(cur_path_ptr, FindExInfoBasic, &find_data, FindExSearchNameMatch, NULL, 0) };
            if (hfind == INVALID_HANDLE_VALUE){ return false; }

            bool is_save_search_dir{ true };

            do
            {
                if ((*reinterpret_cast<std::uint32_t*>(find_data.cFileName)) == std::uint32_t(0x0000002E)) { continue; } // skip .
                if (((*reinterpret_cast<std::uint64_t*>(find_data.cFileName)) & 0x000000FFFFFFFFFF) == std::uint64_t(0x00000000002E002E)) { continue; } // skip ..

                const auto file_name_chars = ::wcslen(find_data.cFileName);

                if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                {
                    const std::wstring_view search_dir_name_sv{ search_dir_name.data(), search_dir_name.size() - 1 };

                    if (is_save_search_dir)
                    {
                        is_save_search_dir = false;

                        if (search_dir_name_sv.empty() == false)
                        {
                            search_dir_stack.push(std::move(std::wstring{ search_dir_name_sv }));
                        }
                    }

                    find_data.cFileName[file_name_chars + 0] = L'/';
                    find_data.cFileName[file_name_chars + 1] = L'*';
                    search_dir_stack.push(std::move(std::wstring{ search_dir_name_sv }.append(find_data.cFileName, file_name_chars + 2)));
                }
                else
                {
                    // make file path
                    std::memcpy(cur_path_ptr + cur_path_chars - 1, find_data.cFileName, file_name_chars * sizeof(wchar_t));
                    cur_path_ptr[cur_path_chars - 1 + file_name_chars] = {};

                    // remove read-only attribute
                    if (find_data.dwFileAttributes & FILE_ATTRIBUTE_READONLY) { ::SetFileAttributesW(cur_path_ptr, find_data.dwFileAttributes ^ FILE_ATTRIBUTE_READONLY); }

                    ::DeleteFileW(cur_path_ptr);
                }
            } while (::FindNextFileW(hfind, &find_data));

            ::FindClose(hfind);

            // current directory is empyt now, so remove it.
            if (is_save_search_dir == true)
            {
                cur_path_ptr[cur_path_chars - 1] = L'\0';
                ::RemoveDirectoryW(cur_path_ptr);
            }

        } while (!search_dir_stack.empty());


        if (isRemoveBaseDir)
        {
            cur_path_ptr[base_dir_chars] = L'\0';
            return ::RemoveDirectoryW(cur_path_ptr) != FALSE;
        }

        return true;
    }

    auto DirContentDelete(const std::string_view msPath) -> bool
    {
        if (!msPath.ends_with('/')) { return false; }
        return ZxFS::DirContentDeleteImp(msPath, false);
    }

    auto DirDelete(const std::string_view msPath, bool isRecursive) -> bool
    {
        if (isRecursive) { return ZxFS::DirContentDeleteImp(msPath, true); }

        if (!msPath.ends_with('/')) { return false; }
        return ::RemoveDirectoryW(Plat::PathUTF8ToWide(msPath).second.get()) != FALSE;
    }

    auto DirMake(const std::string_view msPath, bool isRecursive) -> bool
    {
        if (!msPath.ends_with('/')) { return false; }

        const auto path_w = Plat::PathUTF8ToWide(msPath);

        if (!isRecursive)
        {
            return ::CreateDirectoryW(path_w.second.get(), nullptr) != FALSE;
        }
        else
        {
            wchar_t* path_cstr = path_w.second.get();
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
        return ::GetFileAttributesW(Plat::PathUTF8ToWide(msPath).second.get()) == INVALID_FILE_ATTRIBUTES ? false : true;
    }
} // namespace ZQF::Zut::ZxFS
#elif __linux__
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>
#include <cstring>


namespace ZQF::Zut::ZxFS
{
    auto SelfDir() -> std::pair<std::string_view, std::unique_ptr<char[]>>
    {
        auto path_max_byte{ Plat::PathMaxBytes() };

        do
        {
            auto buffer = std::make_unique_for_overwrite<char[]>(path_max_byte);
            const auto write_bytes = ::readlink("/proc/self/cwd", buffer.get(), path_max_byte);
            if (write_bytes == -1) { break; }
            if (static_cast<std::size_t>(write_bytes) != path_max_byte)
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
        auto path_max_byte{ Plat::PathMaxBytes() };

        do
        {
            auto buffer = std::make_unique_for_overwrite<char[]>(path_max_byte);
            const auto write_bytes = ::readlink("/proc/self/exe", buffer.get(), path_max_byte);
            if (write_bytes == -1) { break; }
            if (static_cast<std::size_t>(write_bytes) != path_max_byte)
            {
                buffer[write_bytes] = {};
                return { { buffer.get(), static_cast<std::size_t>(write_bytes) }, std::move(buffer) };
            }
            path_max_byte *= 2;
        } while (true);

        return { "", nullptr };
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
        const auto fstat_status = ::fstat(fd_exist, &st);
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
        const auto status = ::stat(msPath.data(), &st);
        return (status != -1) ? std::optional{ static_cast<std::uint16_t>(st.st_size) } : std::nullopt;
    }

    static auto DirContentDeleteImp(const std::string_view msPath, const std::size_t nPathMaxBytes) -> bool
    {
        if (msPath.size() >= nPathMaxBytes) { return false; }

        auto path_buffer = std::make_unique_for_overwrite<char[]>(nPathMaxBytes);
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
            if ((path_bytes + 1) >= nPathMaxBytes) { return false; }
            std::memcpy(path_buffer.get() + msPath.size(), name.data(), name.size());
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
        return ZxFS::DirContentDeleteImp(msPath, Plat::PathMaxBytes());
    }

    auto DirDelete(const std::string_view msPath, bool isRecursive) -> bool
    {
        if (!msPath.ends_with('/')) { return false; }
        if (isRecursive) { ZxFS::DirContentDeleteImp(msPath, Plat::PathMaxBytes()); }
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
            const auto path_buffer{ std::make_unique_for_overwrite<char[]>(msPath.size() + 1) };
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
} // namespace ZQF::Zut::ZxFS
#endif


