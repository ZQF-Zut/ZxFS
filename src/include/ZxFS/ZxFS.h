#pragma once
#include <string>
#include <memory>
#include <optional>
#include <string_view>


namespace ZQF::ZxFS
{
    constexpr std::size_t ZXFS_MAX_PATH = 512;

	class Walk
	{
	private:
        std::uintptr_t m_hFind{};
        char m_aName[ZXFS_MAX_PATH]{};
        std::size_t m_nNameBytes{};
        char m_aSearchDir[ZXFS_MAX_PATH];
        std::size_t m_nSearchDirBytes{};

	public:
		Walk(const std::string_view msWalkDir);
		~Walk();

	public:
		auto GetPath() const -> std::string;
		auto GetName() const -> std::string_view;
        auto GetSearchDir() const -> std::string_view;

	public:
		auto NextDir() -> bool;
		auto NextFile() -> bool;
		auto IsSuffix(const std::string_view msSuffix) const -> bool;
	};

    auto SelfDir() -> std::pair<std::string_view, std::unique_ptr<char[]>>;
    auto SelfPath() -> std::pair<std::string_view, std::unique_ptr<char[]>>;

	auto FileName(const std::string_view msPath) -> std::string_view;
    auto FileNameStem(const std::string_view msPath) -> std::string_view;
    auto FileSuffix(const std::string_view msPath) -> std::string_view;
    auto FileSuffixDel(const std::string_view msPath) -> std::string_view;

    auto FileDelete(const std::string_view msPath) -> bool;
    auto FileMove(const std::string_view msExistPath, const std::string_view msNewPath) -> bool;
    auto FileCopy(const std::string_view msExistPath, const std::string_view msNewPath, bool isFailIfExists) -> bool;
    auto FileSize(const std::string_view msPath) -> std::optional<std::uint64_t>;

    auto DirContentDelete(const std::string_view msPath) -> bool;
    auto DirDelete(const std::string_view msPath, bool isRecursive) -> bool;
    auto DirMake(const std::string_view msPath, bool isRecursive) -> bool;

    auto Exist(const std::string_view msPath) -> bool;
}
