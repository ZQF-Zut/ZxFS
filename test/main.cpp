#include <print>
#include <cassert>
#include <iostream>
#include <ZxFS/ZxFS.h>


auto main() -> int
{
    try
    {
        std::string_view path_0 = "dirx/asfas/r/weg/wfqwr/123.jpg";
        auto file_name_0 = ZQF::ZxFS::FileName(path_0);
        assert(file_name_0 == "123.jpg");
        auto fle_suffix_0 = ZQF::ZxFS::FileSuffix(path_0);
        assert(fle_suffix_0 == "jpg");

        std::string_view path_1 = "dirx/asfas/r/weg/wfqwr/";
        auto file_name_1 = ZQF::ZxFS::FileName(path_1);
        assert(file_name_1 == "");
        auto fle_suffix_1 = ZQF::ZxFS::FileSuffix(path_1);
        assert(fle_suffix_1 == "");

        std::string_view path_2 = "";
        auto file_name_2 = ZQF::ZxFS::FileName(path_2);
        assert(file_name_2 == "");
        auto fle_suffix_2 = ZQF::ZxFS::FileSuffix(path_2);
        assert(fle_suffix_2 == "");

        std::string_view path_3 = "dirx/asfas/r/weg/wfqwr/.";
        auto file_name_3 = ZQF::ZxFS::FileName(path_3);
        assert(file_name_3 == ".");
        auto fle_suffix_3 = ZQF::ZxFS::FileSuffix(path_3);
        assert(fle_suffix_3 == "");

        std::string_view path_4 = "dirx/asfas/r/weg/wfqwr/..";
        auto file_name_4 = ZQF::ZxFS::FileName(path_4);
        assert(file_name_4 == "..");
        auto fle_suffix_4 = ZQF::ZxFS::FileSuffix(path_4);
        assert(fle_suffix_4 == "");

        std::string_view path_5 = "my.jpg";
        auto file_name_5 = ZQF::ZxFS::FileName(path_5);
        assert(file_name_5 == "my.jpg");
        auto fle_suffix_5 = ZQF::ZxFS::FileSuffix(path_5);
        assert(fle_suffix_5 == "jpg");

        auto [self_dir_sv, self_dir_buf] = ZQF::ZxFS::SelfDir();
        auto [self_path_sv, self_path_buf] = ZQF::ZxFS::SelfPath();

        auto self_dir_exist = ZQF::ZxFS::Exist(self_dir_sv);
        assert(self_dir_exist == true);

        auto self_path_exist = ZQF::ZxFS::Exist(self_path_sv);
        assert(self_path_exist == true);

        ZQF::ZxFS::DirDelete("123/",false);
        auto mkdir_status0 = ZQF::ZxFS::DirMake("123/", false);
        assert(mkdir_status0 == true);
        auto check_mkdir_status_0 = ZQF::ZxFS::Exist("123/");
        assert(check_mkdir_status_0 == true);
        ZQF::ZxFS::DirDelete("123/", false);


        bool copy_status_0 = ZQF::ZxFS::FileCopy(self_path_sv, "test.bin", false);
        assert(copy_status_0 == true);

        bool copy_status_1 = ZQF::ZxFS::FileCopy(self_path_sv, "test.bin", true);
        assert(copy_status_1 == false);

        bool del_status = ZQF::ZxFS::FileDelete("test.bin");
        assert(del_status == true);

        for (ZQF::ZxFS::Walk walk{ self_dir_sv }; walk.NextFile(); )
        {
            std::println("{}", walk.GetName());
        }


        [[maybe_unused]] int x = 0;

    }
    catch (const std::exception& err)
    {
        std::println(std::cerr, "std::exception: {}", err.what());
    }

}
