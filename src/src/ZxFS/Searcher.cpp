#include <ZxFS/Searcher.h>
#include <ZxFS/Platform.h>
#include <stack>
#include <memory>
#include <memory_resource>
#include <format>
#include <stdexcept>


#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>


namespace ZQF::ZxFS
{
    constexpr auto PATH_MAX_BYTES = 0x1000;

    static auto GetFilePathsCurDir(std::vector<std::string>& vcPaths, const std::string_view msSearchDir, const bool isWithDir) -> bool
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
        if (isWithDir)
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

    static auto GetFilePathsRecursive(std::vector<std::string>& vcPaths, const std::string_view msSearchDir, const bool isWithDir) -> bool
    {
        std::stack<std::wstring> dir_stack;
        dir_stack.push(L"*");

        const auto u8paths_cache = std::make_unique_for_overwrite<char[]>(PATH_MAX_BYTES);
        std::size_t paths_prefix_bytes{};
        if (isWithDir)
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
                    std::wstring next_dir{ cur_dir_name.data(), cur_dir_name.size() - 1 };
                    next_dir.append(find_data.cFileName).append(L"/*", 2);
                    dir_stack.push(std::move(next_dir));
                }
            } while (::FindNextFileW(hfind, &find_data));

            ::FindClose(hfind);
        }

        return true;
    }

    auto Searcher::GetFilePaths(const std::string_view msSearchDir, const bool isWithDir, const bool isRecursive) -> std::vector<std::string>
    {
        std::vector<std::string> file_path_list;
        const auto status = Searcher::GetFilePaths(file_path_list, msSearchDir, isWithDir, isRecursive);
        if (status == false) { throw std::runtime_error(std::format("ZxPath::Walk::GetFilePaths(): dir open error! -> \'{}\'", msSearchDir)); }
        return file_path_list;
    }

    auto Searcher::GetFilePaths(std::vector<std::string>& vcPaths, const std::string_view msSearchDir, const bool isWithDir, const bool isRecursive) -> bool
    {
        if (!msSearchDir.ends_with('/')) { throw std::runtime_error(std::format("ZxPath::Walk::GetFilePaths(): dir format error! -> \'{}\'", msSearchDir)); }
        return isRecursive ? GetFilePathsRecursive(vcPaths, msSearchDir, isWithDir) : GetFilePathsCurDir(vcPaths, msSearchDir, isWithDir);
    }
}
#elif __linux__

#endif
