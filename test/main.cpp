#include <print>
#include <cassert>
#include <iostream>
#include <chrono>
#include <filesystem>
#include <Zut/ZxFS.h>


// # Utils #
namespace ZQF
{
    class ZxRecord
    {
    private:
        std::chrono::steady_clock::time_point m_tpBeg;
        std::chrono::steady_clock::time_point m_tpEnd;
        std::vector<std::chrono::duration<double, std::milli>> m_vcRecord;

    public:
        auto Beg() -> void;
        auto End() -> void;
        auto Log() -> void;
    };

    auto ZxRecord::Beg() -> void
    {
        m_tpBeg = std::chrono::steady_clock::now();
    }

    auto ZxRecord::End() -> void
    {
        m_tpEnd = std::chrono::steady_clock::now();
        m_vcRecord.emplace_back((m_tpEnd - m_tpBeg));
    }

    auto ZxRecord::Log() -> void
    {
        std::chrono::duration<double, std::milli> cout{};

        for (auto& dur : m_vcRecord)
        {
            cout += dur;
            std::println("{}", dur);
        }

        std::println("Avg:{}", cout / m_vcRecord.size());
    }
} // namespace ZQF


static auto MyAssert(bool isStatus) -> void
{
    if (isStatus == false)
    {
        throw std::runtime_error("assert failed");
    }
}


auto main() -> int
{
    try
    {
        // const auto file_list = ZxFS::Searcher::GetFilePaths("/home/linuxdev/.vs/", false, true);

        // for (auto& path : file_list) { ZxFS::FileDelete(path); }
        // const auto file_list_2 = ZxFS::Searcher::GetFilePaths("C:/Users/Ptr/Desktop/qtdeclarative/", true, true);
        // MyAssert(file_list_2.empty());

        ZxFS::DirContentDelete("C:/Users/Ptr/Desktop/qtdeclarative/");

        std::string_view path_0 = "dirx/asfas/r/weg/wfqwr/123.jpg";
        auto file_name_0 = ZxFS::FileName(path_0);
        MyAssert(file_name_0 == "123.jpg");
        auto fle_suffix_0 = ZxFS::FileSuffix(path_0);
        MyAssert(fle_suffix_0 == ".jpg");
        auto file_suffix_remove_0 = ZxFS::FileSuffixDel(path_0);
        MyAssert(file_suffix_remove_0 == "dirx/asfas/r/weg/wfqwr/123");
        auto file_stem_0 = ZxFS::FileNameStem(path_0);
        MyAssert(file_stem_0 == "123");

        std::string_view path_1 = "dirx/asfas/r/weg/wfqwr/";
        auto file_name_1 = ZxFS::FileName(path_1);
        MyAssert(file_name_1 == "");
        auto fle_suffix_1 = ZxFS::FileSuffix(path_1);
        MyAssert(fle_suffix_1 == "");

        std::string_view path_2 = "";
        auto file_name_2 = ZxFS::FileName(path_2);
        MyAssert(file_name_2 == "");
        auto fle_suffix_2 = ZxFS::FileSuffix(path_2);
        MyAssert(fle_suffix_2 == "");

        std::string_view path_3 = "dirx/asfas/r/weg/wfqwr/.";
        auto file_name_3 = ZxFS::FileName(path_3);
        MyAssert(file_name_3 == ".");
        auto fle_suffix_3 = ZxFS::FileSuffix(path_3);
        MyAssert(fle_suffix_3 == ".");
        auto file_suffix_remove_3 = ZxFS::FileSuffixDel(path_3);
        MyAssert(file_suffix_remove_3 == "dirx/asfas/r/weg/wfqwr/");

        std::string_view path_4 = "dirx/asfas/r/weg/wfqwr/..";
        auto file_name_4 = ZxFS::FileName(path_4);
        MyAssert(file_name_4 == "..");
        auto fle_suffix_4 = ZxFS::FileSuffix(path_4);
        MyAssert(fle_suffix_4 == ".");
        auto file_suffix_remove_4 = ZxFS::FileSuffixDel(path_4);
        MyAssert(file_suffix_remove_4 == "dirx/asfas/r/weg/wfqwr/.");

        std::string_view path_5 = "my.jpg";
        auto file_name_5 = ZxFS::FileName(path_5);
        MyAssert(file_name_5 == "my.jpg");
        auto fle_suffix_5 = ZxFS::FileSuffix(path_5);
        MyAssert(fle_suffix_5 == ".jpg");
        auto file_suffix_remove_5 = ZxFS::FileSuffixDel(path_5);
        MyAssert(file_suffix_remove_5 == "my");
        auto file_stem_5 = ZxFS::FileNameStem(path_5);
        MyAssert(file_stem_5 == "my");


        std::string_view path_6 = "y.j";
        auto file_suffix_remove_6 = ZxFS::FileSuffixDel(path_6);
        MyAssert(file_suffix_remove_6 == "y");

        std::string_view path_7 = ".j";
        auto fle_suffix_7 = ZxFS::FileSuffix(path_7);
        MyAssert(fle_suffix_7 == ".j");
        auto file_suffix_remove_7 = ZxFS::FileSuffixDel(path_7);
        MyAssert(file_suffix_remove_7 == "");

        std::string_view path_8 = ".";
        auto fle_suffix_8 = ZxFS::FileSuffix(path_8);
        MyAssert(fle_suffix_8 == ".");


        auto [self_dir_sv, self_dir_buf] = ZxFS::SelfDir();
        auto [self_path_sv, self_path_buf] = ZxFS::SelfPath();

        auto self_dir_exist = ZxFS::Exist(self_dir_sv);
        MyAssert(self_dir_exist == true);

        auto self_path_exist = ZxFS::Exist(self_path_sv);
        MyAssert(self_path_exist == true);

        ZxFS::DirDelete("123x/", false);
        auto mkdir_status0 = ZxFS::DirMake("123x/", false);
        MyAssert(mkdir_status0 == true);
        auto check_mkdir_status_0 = ZxFS::Exist("123x/");
        MyAssert(check_mkdir_status_0 == true);
        ZxFS::DirDelete("123x/", false);


        bool copy_status_0 = ZxFS::FileCopy(self_path_sv, "test.bin", false);
        MyAssert(copy_status_0 == true);

        bool copy_status_1 = ZxFS::FileCopy(self_path_sv, "test.bin", true);
        MyAssert(copy_status_1 == false);

        ZxFS::DirMake("weufbuiwef/214124/41241/", true);
        bool move_file_status_0 = ZxFS::FileMove("test.bin", "weufbuiwef/214124/41241/test.bin");
        MyAssert(move_file_status_0 == true);

        bool move_file_status_1 = ZxFS::FileMove("weufbuiwef/214124/41241/test.bin", "test.bin");
        MyAssert(move_file_status_1 == true);

        bool del_status = ZxFS::FileDelete("test.bin");
        MyAssert(del_status == true);

        ZxFS::DirDelete("weufbuiwef/", true);

        for (ZxFS::Walker walk{ self_dir_sv }; walk.NextFile(); )
        {
            std::println("{}\n{}", walk.GetWalkDir(), walk.GetName());
        }

        auto dir_make_recursive_status = ZxFS::DirMake("123/41245/215/125/1251/", true);
        MyAssert(dir_make_recursive_status == true);
        MyAssert(ZxFS::Exist("123/41245/215/125/1251/"));
        ZxFS::DirDelete("123/", true);

        [[maybe_unused]] int x = 0;

        std::println("all passed!");
    }
    catch (const std::exception& err)
    {
        std::println(std::cerr, "std::exception: {}", err.what());
    }

}
