#include <ZxFS/Walker.h>
#include <ZxFS/Core.h>
#include <ZxFS/Platform.h>
#include <stack>
#include <format>
#include <stdexcept>


namespace ZQF::ZxFS
{
    auto Walker::GetPath() const -> std::string
    {
        return std::string{}.append(m_upSearchDir.get(), m_nSearchDirBytes).append(this->GetName());
    }

    auto Walker::GetSearchDir() const->std::string_view
    {
        return { m_upSearchDir.get(), m_nSearchDirBytes };
    }

    auto Walker::IsSuffix(const std::string_view msSuffix) const -> bool
    {
        return ZxFS::FileSuffix(this->GetName()) == msSuffix ? true : false;
    }
}


#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>


namespace ZQF::ZxFS
{
    constexpr auto PATH_MAX_BYTES = 0x1000;

    Walker::Walker(const std::string_view msWalkDir)
    {
        if (!msWalkDir.ends_with('/')) { throw std::runtime_error(std::format("ZxPath::Walk.Walk(): dir format error! -> \'{}\'", msWalkDir)); }

        m_upName = std::make_unique_for_overwrite<char[]>(PATH_MAX_BYTES);

        wchar_t* wide_path = reinterpret_cast<wchar_t*>(m_upName.get());
        const auto wide_path_char_cnt = Plat::PathUTF8ToWide(msWalkDir, wide_path, PATH_MAX_BYTES);
        wide_path[wide_path_char_cnt + 0] = L'*';
        wide_path[wide_path_char_cnt + 1] = L'\0';

        WIN32_FIND_DATAW find_data;
        const auto hfind = ::FindFirstFileExW(wide_path, FindExInfoBasic, &find_data, FindExSearchNameMatch, nullptr, 0);
        if (hfind == INVALID_HANDLE_VALUE) { throw std::runtime_error(std::format("ZxPath::Walk.Walk(): dir open error! -> \'{}\'", msWalkDir)); }
        m_hFind = reinterpret_cast<std::uintptr_t>(hfind);

        m_upName[0] = {};

        m_upSearchDir = std::make_unique_for_overwrite<char[]>((msWalkDir.size() + 1) * sizeof(char));
        std::memcpy(m_upSearchDir.get(), msWalkDir.data(), msWalkDir.size() * sizeof(char));
        m_nSearchDirBytes = msWalkDir.size() * sizeof(char);
        m_upSearchDir[m_nSearchDirBytes] = '\0';
    }

    Walker::~Walker()
    {
        ::FindClose(reinterpret_cast<HANDLE>(m_hFind));
    }

    auto Walker::NextDir() -> bool
    {
        WIN32_FIND_DATAW find_data;
        while (::FindNextFileW(reinterpret_cast<HANDLE>(m_hFind), &find_data))
        {
            if ((*reinterpret_cast<std::uint32_t*>(find_data.cFileName)) == std::uint32_t(0x0000002E)) { continue; } // skip .
            if (((*reinterpret_cast<std::uint64_t*>(find_data.cFileName)) & 0x000000FFFFFFFFFF) == std::uint64_t(0x00000000002E002E)) { continue; } // skip ..

            if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                auto name_bytes = Plat::PathWideToUTF8({ find_data.cFileName }, m_upName.get(), PATH_MAX_BYTES);
                if ((name_bytes + 1) >= PATH_MAX_BYTES) { return false; }
                m_upName[name_bytes + 0] = '/';
                m_upName[name_bytes + 1] = '\0';
                m_nNameBytes = name_bytes + 1;
                return true;
            }
        }

        return false;
    }

    auto Walker::NextFile() -> bool
    {
        WIN32_FIND_DATAW find_data;
        while (::FindNextFileW(reinterpret_cast<HANDLE>(m_hFind), &find_data))
        {
            if ((*reinterpret_cast<std::uint32_t*>(find_data.cFileName)) == std::uint32_t(0x0000002E)) { continue; } // skip .
            if (((*reinterpret_cast<std::uint64_t*>(find_data.cFileName)) & 0x000000FFFFFFFFFF) == std::uint64_t(0x00000000002E002E)) { continue; } // skip ..

            if (!(find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                m_nNameBytes = Plat::PathWideToUTF8(find_data.cFileName, m_upName.get(), PATH_MAX_BYTES);
                return true;
            }
        }

        return false;
    }

    auto Walker::GetName() const -> std::string_view
    {
        return { m_upName.get(), m_nNameBytes };
    }

    static auto GetFilePathsCurDir(std::vector<std::string>& vcPaths, const std::string_view msSearchDir, const bool isAppendDir) -> bool
    {
        const auto u8path_cache = std::make_unique_for_overwrite<char[]>(PATH_MAX_BYTES);

        wchar_t* walk_dir_buffer = reinterpret_cast<wchar_t*>(u8path_cache.get());
        const auto walk_dir_chars = Plat::PathUTF8ToWide(msSearchDir, walk_dir_buffer, PATH_MAX_BYTES / sizeof(wchar_t));
        walk_dir_buffer[walk_dir_chars + 0] = L'*';
        walk_dir_buffer[walk_dir_chars + 1] = L'\0';

        WIN32_FIND_DATAW find_data;
        const auto hfind = ::FindFirstFileExW(walk_dir_buffer, FindExInfoBasic, &find_data, FindExSearchNameMatch, NULL, 0);
        if (hfind == INVALID_HANDLE_VALUE) { return false; }


        std::size_t paths_prefix_bytes{};
        if (isAppendDir)
        {
            paths_prefix_bytes = msSearchDir.size() * sizeof(char);
            std::memcpy(u8path_cache.get(), msSearchDir.data(), paths_prefix_bytes);
        }
        else
        {
            paths_prefix_bytes = 0;
            u8path_cache[0] = {};
        }

        do
        {
            if ((*reinterpret_cast<std::uint32_t*>(find_data.cFileName)) == std::uint32_t(0x0000002E)) { continue; } // skip .
            if (((*reinterpret_cast<std::uint64_t*>(find_data.cFileName)) & 0x000000FFFFFFFFFF) == std::uint64_t(0x00000000002E002E)) { continue; } // skip ..

            if (!(find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                const auto file_name_u8_bytes = Plat::PathWideToUTF8(find_data.cFileName, u8path_cache.get() + paths_prefix_bytes, PATH_MAX_BYTES - paths_prefix_bytes);
                const auto file_path_u8_bytes = paths_prefix_bytes + file_name_u8_bytes;
                vcPaths.emplace_back(std::string_view{ u8path_cache.get(), file_path_u8_bytes });
            }
        } while (::FindNextFileW(hfind, &find_data));

        return true;
    }

    static auto GetFilePathsRecursive(std::vector<std::string>& vcPaths, const std::string_view msSearchDir, const bool isAppendDir) -> bool
    {
        std::stack<std::wstring> dir_stack;
        dir_stack.push(L"*");

        const auto u8paths_cache = std::make_unique_for_overwrite<char[]>(PATH_MAX_BYTES);
        std::size_t paths_prefix_bytes{};
        if (isAppendDir)
        {
            paths_prefix_bytes = msSearchDir.size() * sizeof(char);
            std::memcpy(u8paths_cache.get(), msSearchDir.data(), paths_prefix_bytes);
        }
        else
        {
            paths_prefix_bytes = 0;
            u8paths_cache[0] = {};
        }

        const auto cur_dir_u16_cache = std::make_unique_for_overwrite<wchar_t[]>(PATH_MAX_BYTES / sizeof(wchar_t));
        wchar_t* cur_dir_path_u16_buffer = cur_dir_u16_cache.get();
        const auto cur_dir_path_u16_chars = Plat::PathUTF8ToWide(msSearchDir, cur_dir_path_u16_buffer, PATH_MAX_BYTES / sizeof(wchar_t));
        while (!dir_stack.empty())
        {
            const auto cur_dir_name{ std::move(dir_stack.top()) }; dir_stack.pop();
            std::memcpy(cur_dir_path_u16_buffer + cur_dir_path_u16_chars, cur_dir_name.data(), (cur_dir_name.size() + 1) * sizeof(wchar_t));

            WIN32_FIND_DATAW find_data;
            const auto hfind = ::FindFirstFileExW(cur_dir_path_u16_buffer, FindExInfoBasic, &find_data, FindExSearchNameMatch, NULL, 0);
            if (hfind == INVALID_HANDLE_VALUE) { return false; }

            const auto cur_dir_name_u8_bytes = Plat::PathWideToUTF8(cur_dir_name.data(), u8paths_cache.get() + paths_prefix_bytes, PATH_MAX_BYTES - paths_prefix_bytes) - 1;

            do
            {
                if ((*reinterpret_cast<std::uint32_t*>(find_data.cFileName)) == std::uint32_t(0x0000002E)) { continue; } // skip .
                if (((*reinterpret_cast<std::uint64_t*>(find_data.cFileName)) & 0x000000FFFFFFFFFF) == std::uint64_t(0x00000000002E002E)) { continue; } // skip ..

                if (!(find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
                {                    
                    const auto file_name_u8_bytes = Plat::PathWideToUTF8(find_data.cFileName, u8paths_cache.get() + paths_prefix_bytes + cur_dir_name_u8_bytes, PATH_MAX_BYTES - paths_prefix_bytes - cur_dir_name_u8_bytes);
                    const auto file_path_u8_bytes = paths_prefix_bytes + cur_dir_name_u8_bytes + file_name_u8_bytes;
                    vcPaths.emplace_back(std::string_view{ u8paths_cache.get(), file_path_u8_bytes });
                }
                else
                {
                    std::wstring next_dir{ cur_dir_name.substr(0, cur_dir_name.size() - 1) };
                    next_dir.append(find_data.cFileName).append(L"/*", 2);
                    dir_stack.push(std::move(next_dir));
                }
            } while (::FindNextFileW(hfind, &find_data));

            ::FindClose(hfind);
        }

        return true;
    }

    auto Walker::GetFilePaths(const std::string_view msWalkDir, const bool isWithDir, const bool isRecursive) -> std::vector<std::string>
    {
        std::vector<std::string> file_path_list;
        const auto status = Walker::GetFilePaths(file_path_list, msWalkDir, isWithDir, isRecursive);
        if (status == false) { throw std::runtime_error(std::format("ZxPath::Walk::GetFilePaths(): dir open error! -> \'{}\'", msWalkDir)); }
        return file_path_list;
    }

    auto Walker::GetFilePaths(std::vector<std::string>& vcPaths, const std::string_view msWalkDir, const bool isWithDir, const bool isRecursive) -> bool
    {
        if (!msWalkDir.ends_with('/')) { throw std::runtime_error(std::format("ZxPath::Walk::GetFilePaths(): dir format error! -> \'{}\'", msWalkDir)); }
        return isRecursive ? GetFilePathsRecursive(vcPaths, msWalkDir, isWithDir) : GetFilePathsCurDir(vcPaths, msWalkDir, isWithDir);
    }
}

#elif __linux__

#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>


namespace ZQF::ZxFS
{
    Walker::Walker(const std::string_view msWalkDir)
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

    Walker::~Walker()
    {
        ::closedir(reinterpret_cast<DIR*>(m_hFind));
    }

    auto Walker::NextDir() -> bool
    {
        while (const auto entry_ptr = ::readdir(reinterpret_cast<DIR*>(m_hFind)))
        {
            if ((*reinterpret_cast<std::uint16_t*>(entry_ptr->d_name)) == std::uint32_t(0x002E)) { continue; }
            if (((*reinterpret_cast<std::uint32_t*>(entry_ptr->d_name)) & 0x00FFFFFF) == std::uint32_t(0x00002E2E)) { continue; }
            if (entry_ptr->d_type != DT_DIR) { continue; }

            auto name_bytes = std::strlen(entry_ptr->d_name);
            m_aName = entry_ptr->d_name;
            m_aName[m_nNameBytes + 0] = '/';
            m_aName[m_nNameBytes + 1] = '\0';
            m_nNameBytes = name_bytes + 1;
            return true;
        }

        return false;
    }

    auto Walker::NextFile() -> bool
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

    auto Walker::GetName() const -> std::string_view
    {
        return { m_aName, m_nNameBytes };
    }
}

#endif
