#include "Searcher.h"
#include "Plat.h"
#include <stack>
#include <memory>
#include <stdexcept>


#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>


namespace ZQF::Zut::ZxFS
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
        const auto file_path_prefix_u8_bytes{ isWithDir ? (msBaseDir.size() * sizeof(char)) : 0 };
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
        if (status == false) { throw std::runtime_error(std::string{ "ZxPath::Walk::GetFilePaths(): dir open error! -> " }.append(msSearchDir)); }
        return file_path_list;
    }

    auto Searcher::GetFilePaths(std::vector<std::string>& vcPaths, const std::string_view msSearchDir, const bool isWithDir, const bool isRecursive) -> bool
    {
        if (!msSearchDir.ends_with('/')) { throw std::runtime_error(std::string{ "ZxPath::Walk::GetFilePaths(): dir format error! -> " }.append(msSearchDir)); }
        return isRecursive ? GetFilePathsRecursive(vcPaths, msSearchDir, isWithDir) : GetFilePathsCurDir(vcPaths, msSearchDir, isWithDir);
    }
} // namespace ZQF::Zut::ZxFS
#elif __linux__
#include <dirent.h>
#include <cstring>

namespace ZQF::Zut::ZxFS
{
    static auto GetFilePathsCurDir(std::vector<std::string>& vcPaths, const std::string_view msBaseDir, const bool isWithDir) -> bool
    {
        const auto dir_ptr{ ::opendir(msBaseDir.data()) };
        if (dir_ptr == nullptr) { return false; }

        auto entry_ptr{ ::readdir(dir_ptr) };
        if (entry_ptr == nullptr) { return false; }

        if (isWithDir)
        {
            const auto path_max_bytes{ Plat::PathMaxBytes() };
            const auto path_cache{ std::make_unique_for_overwrite<char[]>(path_max_bytes) };
            auto file_path_ptr{ path_cache.get() };

            std::memcpy(file_path_ptr, msBaseDir.data(), msBaseDir.size());

            do
            {
                if ((*reinterpret_cast<std::uint16_t*>(entry_ptr->d_name)) == std::uint32_t(0x002E)) { continue; }// skip .
                if (((*reinterpret_cast<std::uint32_t*>(entry_ptr->d_name)) & 0x00FFFFFF) == std::uint32_t(0x00002E2E)) { continue; }// skip ..
                if (entry_ptr->d_type != DT_REG) { continue; }

                const auto file_name_bytes{ std::strlen(entry_ptr->d_name) };
                std::memcpy(file_path_ptr + msBaseDir.size(), entry_ptr->d_name, file_name_bytes);
                const auto file_path_bytes{ msBaseDir.size() + file_name_bytes };
                vcPaths.emplace_back(file_path_ptr, file_path_bytes);
            } while ((entry_ptr = ::readdir(dir_ptr)) != nullptr);
        }
        else
        {
            do
            {
                if ((*reinterpret_cast<std::uint16_t*>(entry_ptr->d_name)) == std::uint32_t(0x002E)) { continue; }// skip .
                if (((*reinterpret_cast<std::uint32_t*>(entry_ptr->d_name)) & 0x00FFFFFF) == std::uint32_t(0x00002E2E)) { continue; }// skip ..
                if (entry_ptr->d_type != DT_REG) { continue; }
                vcPaths.emplace_back(entry_ptr->d_name);
            } while ((entry_ptr = ::readdir(dir_ptr)) != nullptr);
        }

        const auto status{ ::closedir(dir_ptr) };
        return status != (-1) ? true : false;
    }

    static auto GetFilePathsRecursive(std::vector<std::string>& vcPaths, const std::string_view msBaseDir, const bool isWithDir) -> bool
    {
        std::stack<std::string> search_dir_stack;
        search_dir_stack.push("");

        const auto path_max_bytes{ Plat::PathMaxBytes() };
        const auto file_path_cache{ std::make_unique_for_overwrite<char[]>(path_max_bytes) };
        std::memcpy(file_path_cache.get(), msBaseDir.data(), msBaseDir.size() * sizeof(char));

        const auto search_dir_path_ptr{ file_path_cache.get() };

        const auto file_path_ptr{ isWithDir ? search_dir_path_ptr : search_dir_path_ptr + msBaseDir.size() };
        const auto file_path_prefix_bytes{ isWithDir ? msBaseDir.size() : 0 };

        do
        {
            const auto search_dir_name{ std::move(search_dir_stack.top()) }; search_dir_stack.pop();

            std::memcpy(search_dir_path_ptr + msBaseDir.size(), search_dir_name.data(), search_dir_name.size() * sizeof(char));
            search_dir_path_ptr[msBaseDir.size() + search_dir_name.size()] = {};

            const auto dir_ptr{ ::opendir(search_dir_path_ptr) };
            if (dir_ptr == nullptr) { return false; }

            auto entry_ptr{ ::readdir(dir_ptr) };
            if (entry_ptr == nullptr) { return false; }

            do
            {
                if ((*reinterpret_cast<std::uint16_t*>(entry_ptr->d_name)) == std::uint32_t(0x002E)) { continue; }// skip .
                if (((*reinterpret_cast<std::uint32_t*>(entry_ptr->d_name)) & 0x00FFFFFF) == std::uint32_t(0x00002E2E)) { continue; }// skip ..

                if (entry_ptr->d_type == DT_DIR)
                {
                    const auto dir_name_bytes{ std::strlen(entry_ptr->d_name) };
                    entry_ptr->d_name[dir_name_bytes] = '/';
                    search_dir_stack.push(std::move(std::string{ search_dir_name.data(), search_dir_name.size() }.append(entry_ptr->d_name, dir_name_bytes + 1)));
                }
                else
                {
                    vcPaths.push_back(std::move(std::string{ file_path_ptr, file_path_prefix_bytes + search_dir_name.size() }.append(entry_ptr->d_name)));
                }
            } while ((entry_ptr = ::readdir(dir_ptr)) != nullptr);

            const auto status{ ::closedir(dir_ptr) };
            if (status == -1) { return false; }

        } while (!search_dir_stack.empty());

        return true;
    }

    auto Searcher::GetFilePaths(const std::string_view msSearchDir, const bool isWithDir, const bool isRecursive) -> std::vector<std::string>
    {
        std::vector<std::string> file_path_list;
        const auto status = Searcher::GetFilePaths(file_path_list, msSearchDir, isWithDir, isRecursive);
        if (status == false) { throw std::runtime_error(std::string{ "ZxPath::Walk::GetFilePaths(): dir open error! -> " }.append(msSearchDir)); }
        return file_path_list;
    }

    auto Searcher::GetFilePaths(std::vector<std::string>& vcPaths, const std::string_view msSearchDir, const bool isWithDir, const bool isRecursive) -> bool
    {
        if (!msSearchDir.ends_with('/')) { throw std::runtime_error(std::string{ "ZxPath::Walk::GetFilePaths(): dir format error! -> " }.append(msSearchDir)); }
        return isRecursive ? GetFilePathsRecursive(vcPaths, msSearchDir, isWithDir) : GetFilePathsCurDir(vcPaths, msSearchDir, isWithDir);
    }
} // namespace ZQF::Zut::ZxFS
#endif
