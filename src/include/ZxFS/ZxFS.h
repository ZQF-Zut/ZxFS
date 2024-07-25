#pragma once
#include <string>
#include <memory>
#include <optional>
#include <string_view>


namespace ZQF::ZxFS
{
	class Walk
	{
	private:
        std::uintptr_t m_hFind;
        std::string m_msPath;
        std::string m_msSearchDir;

	public:
		Walk(const std::string_view msWalkDir);
		~Walk();

	public:
		auto GetPath() const -> std::string;
		auto GetName() const -> std::string_view;

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
    auto FileCopy(const std::string_view msExistPath, const std::string_view msNewPath, bool isFailIfExists) -> bool;
    auto FileSize(const std::string_view msPath) -> std::optional<std::uint64_t>;

    auto DirDelete(const std::string_view msPath, bool isRecursive) -> bool;
    auto DirMake(const std::string_view msPath, bool isRecursive) -> bool;

    auto Exist(const std::string_view msPath) -> bool;
    
}
