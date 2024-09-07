#include <ZxFS/Walker.h>
#include <ZxFS/Core.h>
#include <ZxFS/Platform.h>
#include <stdexcept>


#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>


namespace ZQF::ZxFS
{
    constexpr auto PATH_MAX_BYTES = 0x1000;

    Walker::Walker(const std::string_view msWalkDir)
    {
        if (!msWalkDir.ends_with('/')) { throw std::runtime_error(std::string{ "ZxPath::Walk::Walk(): walk dir format error! -> " }.append(msWalkDir)); }

        m_upCache = std::make_unique_for_overwrite<char[]>(PATH_MAX_BYTES);

        wchar_t* wide_path = reinterpret_cast<wchar_t*>(m_upCache.get());
        const auto wide_path_char_cnt = Plat::PathUTF8ToWide(msWalkDir, wide_path, PATH_MAX_BYTES / sizeof(wchar_t));
        wide_path[wide_path_char_cnt + 0] = L'*';
        wide_path[wide_path_char_cnt + 1] = L'\0';

        WIN32_FIND_DATAW find_data;
        const auto hfind = ::FindFirstFileExW(wide_path, FindExInfoBasic, &find_data, FindExSearchNameMatch, nullptr, 0);
        if (hfind == INVALID_HANDLE_VALUE) { throw std::runtime_error(std::string{ "ZxPath::Walk::Walk(): walk dir open error! -> " }.append(msWalkDir)); }
        m_hFind = reinterpret_cast<std::uintptr_t>(hfind);

        std::memcpy(m_upCache.get(), msWalkDir.data(), msWalkDir.size() * sizeof(char));
        m_upCache[msWalkDir.size()] = {};
        m_nWalkDirBytes = msWalkDir.size() * sizeof(char);
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
                auto name_bytes = Plat::PathWideToUTF8({ find_data.cFileName }, m_upCache.get() + m_nWalkDirBytes, PATH_MAX_BYTES - m_nWalkDirBytes);
                if ((name_bytes + 1) >= PATH_MAX_BYTES) { return false; }
                m_upCache[m_nWalkDirBytes + name_bytes + 0] = '/';
                m_upCache[m_nWalkDirBytes + name_bytes + 1] = '\0';
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
                m_nNameBytes = Plat::PathWideToUTF8(find_data.cFileName, m_upCache.get() + m_nWalkDirBytes, PATH_MAX_BYTES - m_nWalkDirBytes);
                return true;
            }
        }

        return false;
    }
}

#elif __linux__

#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>
#include <cstring>


namespace ZQF::ZxFS
{
    Walker::Walker(const std::string_view msWalkDir)
    {
        if (!msWalkDir.ends_with('/')) { throw std::runtime_error(std::string{ "ZxPath::Walker::Walker: walk dir format error! -> " }.append(msWalkDir)); }

        const auto dir_ptr = ::opendir(msWalkDir.data());
        if (dir_ptr == nullptr) { throw std::runtime_error(std::string{ "ZxPath::Walker::Walker: walk dir open error! -> " }.append(msWalkDir)); }
        m_hFind = reinterpret_cast<std::uintptr_t>(dir_ptr);

        const auto path_max_byte = ::pathconf(".", _PC_PATH_MAX);
        m_upCache = std::make_unique_for_overwrite<char[]>(path_max_byte == -1 ? 1024 : path_max_byte);
        m_nWalkDirBytes = msWalkDir.size() * sizeof(char);
        std::memcpy(m_upCache.get(), msWalkDir.data(), m_nWalkDirBytes);
        m_upCache[m_nWalkDirBytes] = '\0';
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

            const auto name_bytes = std::strlen(entry_ptr->d_name);
            std::memcpy(m_upCache.get() + m_nWalkDirBytes, entry_ptr->d_name, name_bytes);
            m_upCache[m_nWalkDirBytes + m_nNameBytes + 0] = '/';
            m_upCache[m_nWalkDirBytes + m_nNameBytes + 1] = '\0';
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
            std::memcpy(m_upCache.get() + m_nWalkDirBytes, entry_ptr->d_name, m_nNameBytes);
            m_upCache[m_nWalkDirBytes + m_nNameBytes] = {};
            return true;
        }

        return false;
    }
}

#endif

namespace ZQF::ZxFS
{
    auto Walker::GetName() const -> std::string_view
    {
        return { m_upCache.get() + m_nWalkDirBytes, m_nNameBytes };
    }

    auto Walker::GetPath() const -> std::string_view
    {
        return { m_upCache.get(), m_nWalkDirBytes + m_nNameBytes };
    }

    auto Walker::GetWalkDir() const->std::string_view
    {
        return { m_upCache.get(), m_nWalkDirBytes };
    }

    auto Walker::IsSuffix(const std::string_view msSuffix) const -> bool
    {
        return ZxFS::FileSuffix(this->GetName()) == msSuffix ? true : false;
    }
}
