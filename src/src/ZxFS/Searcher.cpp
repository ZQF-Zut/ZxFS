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

    static auto GetFilePathsCurDir(std::vector<std::string>& vcPaths, const std::string_view msBaseDir, const bool isWithDir) -> bool
    {
        const auto u8path_cache = std::make_unique_for_overwrite<char[]>(PATH_MAX_BYTES);

        wchar_t* base_dir_ptr = reinterpret_cast<wchar_t*>(u8path_cache.get());
        const auto base_dir_chars = Plat::PathUTF8ToWide(msBaseDir, base_dir_ptr, PATH_MAX_BYTES / sizeof(wchar_t));
        if (base_dir_chars == 0) { return false; }
        base_dir_ptr[base_dir_chars + 0] = L'*';
        base_dir_ptr[base_dir_chars + 1] = L'\0';

        WIN32_FIND_DATAW find_data;
        const auto hfind = ::FindFirstFileExW(base_dir_ptr, FindExInfoBasic, &find_data, FindExSearchNameMatch, NULL, 0);
        if (hfind == INVALID_HANDLE_VALUE) { return false; }

        char* file_path_u8_ptr = u8path_cache.get();
        const auto file_path_prefix_u8_bytes{ isWithDir ? (msBaseDir.size() * sizeof(char)) : 0 };
        if (isWithDir) { std::memcpy(file_path_u8_ptr, msBaseDir.data(), file_path_prefix_u8_bytes); }

        const auto file_path_u8_remain_bytes = PATH_MAX_BYTES - file_path_prefix_u8_bytes;
        if (file_path_u8_remain_bytes < MAX_PATH) { return false; } // make sure there's enough space for filename

        do
        {
            if ((*reinterpret_cast<std::uint32_t*>(find_data.cFileName)) == std::uint32_t(0x0000002E)) { continue; } // skip .
            if (((*reinterpret_cast<std::uint64_t*>(find_data.cFileName)) & 0x000000FFFFFFFFFF) == std::uint64_t(0x00000000002E002E)) { continue; } // skip ..

            if (!(find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                const auto file_name_u8_bytes = Plat::PathWideToUTF8(find_data.cFileName, file_path_u8_ptr + file_path_prefix_u8_bytes, file_path_u8_remain_bytes);
                const auto file_path_u8_bytes = file_path_prefix_u8_bytes + file_name_u8_bytes;
                vcPaths.emplace_back(std::string{ file_path_u8_ptr, file_path_u8_bytes });
            }
        } while (::FindNextFileW(hfind, &find_data));

        return true;
    }

    static auto GetFilePathsRecursive(std::vector<std::string>& vcPaths, const std::string_view msBaseDir, const bool isWithDir) -> bool
    {
        std::stack<std::wstring> search_dir_stack;
        search_dir_stack.push(L"*");

        const auto file_path_u8_cache = std::make_unique_for_overwrite<char[]>(PATH_MAX_BYTES);
        char* file_path_u8_ptr = file_path_u8_cache.get();
        const auto file_path_prefix_u8_bytes{ isWithDir ? (msBaseDir.size() * sizeof(char)) : 0};
        if (isWithDir) { std::memcpy(file_path_u8_ptr, msBaseDir.data(), file_path_prefix_u8_bytes); }

        const auto cur_dir_cache = std::make_unique_for_overwrite<wchar_t[]>(PATH_MAX_BYTES / sizeof(wchar_t));
        wchar_t* cur_dir_ptr = cur_dir_cache.get();
        const auto base_dir_chars = Plat::PathUTF8ToWide(msBaseDir, cur_dir_ptr, PATH_MAX_BYTES / sizeof(wchar_t));
        if (base_dir_chars == 0) { return false; }

        do
        {
            const auto search_dir_name{ std::move(search_dir_stack.top()) }; search_dir_stack.pop();
            std::memcpy(cur_dir_ptr + base_dir_chars, search_dir_name.data(), search_dir_name.size() * sizeof(wchar_t));
            cur_dir_ptr[base_dir_chars + search_dir_name.size()] = L'\0';

            WIN32_FIND_DATAW find_data;
            const auto hfind = ::FindFirstFileExW(cur_dir_ptr, FindExInfoBasic, &find_data, FindExSearchNameMatch, NULL, 0);
            if (hfind == INVALID_HANDLE_VALUE) { return false; }

            const auto search_dir_name_u8_ptr = file_path_u8_ptr + file_path_prefix_u8_bytes;
            const auto search_dir_name_u8_bytes = Plat::PathWideToUTF8({ search_dir_name.data() ,search_dir_name.size() - 1 }, search_dir_name_u8_ptr, PATH_MAX_BYTES - file_path_prefix_u8_bytes);

            const auto file_path_with_dir_u8_bytes = file_path_prefix_u8_bytes + search_dir_name_u8_bytes;
            const auto file_name_u8_ptr = file_path_u8_ptr + file_path_with_dir_u8_bytes;
            const auto file_path_u8_remain_bytes = PATH_MAX_BYTES - file_path_with_dir_u8_bytes;
            if (file_path_u8_remain_bytes < MAX_PATH) { return false; } // make sure there's enough space for filename

            do
            {
                if ((*reinterpret_cast<std::uint32_t*>(find_data.cFileName)) == std::uint32_t(0x0000002E)) { continue; } // skip .
                if (((*reinterpret_cast<std::uint64_t*>(find_data.cFileName)) & 0x000000FFFFFFFFFF) == std::uint64_t(0x00000000002E002E)) { continue; } // skip ..

                const auto file_name_chars = ::wcslen(find_data.cFileName);

                if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                {
                    find_data.cFileName[file_name_chars + 0] = L'/';
                    find_data.cFileName[file_name_chars + 1] = L'*'; // might override WIN32_FIND_DATAW::cAlternateFileName, but we never use it, so that's safe.
                    search_dir_stack.push(std::move(std::wstring{ search_dir_name.data(), search_dir_name.size() - 1 }.append(find_data.cFileName, file_name_chars + 2)));
                }
                else
                {
                    const auto file_name_u8_bytes = Plat::PathWideToUTF8({ find_data.cFileName, file_name_chars }, file_name_u8_ptr, file_path_u8_remain_bytes);
                    const auto file_path_u8_bytes = file_path_with_dir_u8_bytes + file_name_u8_bytes;
                    vcPaths.emplace_back(std::string{ file_path_u8_ptr, file_path_u8_bytes });
                }
            } while (::FindNextFileW(hfind, &find_data));

            ::FindClose(hfind);

        } while (!search_dir_stack.empty());

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
