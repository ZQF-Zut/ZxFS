#include <print>
#include <cassert>
#include <iostream>
#include <ZxFS/ZxFS.h>

#include 	<direct.h>


auto main() -> int
{
    try
    {
        std::string_view path_0 = "dirx/asfas/r/weg/wfqwr/123.jpg";
        auto file_name_0 = ZQF::ZxFS::FileName(path_0);
        assert(file_name_0 == "123.jpg");
        auto fle_suffix_0 = ZQF::ZxFS::FileSuffix(path_0);
        assert(fle_suffix_0 == ".jpg");
        auto file_suffix_remove_0 = ZQF::ZxFS::FileSuffixDel(path_0);
        assert(file_suffix_remove_0 == "dirx/asfas/r/weg/wfqwr/123");
        auto file_stem_0 = ZQF::ZxFS::FileNameStem(path_0);
        assert(file_stem_0 == "123");

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
        assert(fle_suffix_3 == ".");
        auto file_suffix_remove_3 = ZQF::ZxFS::FileSuffixDel(path_3);
        assert(file_suffix_remove_3 == "dirx/asfas/r/weg/wfqwr/");

        std::string_view path_4 = "dirx/asfas/r/weg/wfqwr/..";
        auto file_name_4 = ZQF::ZxFS::FileName(path_4);
        assert(file_name_4 == "..");
        auto fle_suffix_4 = ZQF::ZxFS::FileSuffix(path_4);
        assert(fle_suffix_4 == ".");
        auto file_suffix_remove_4 = ZQF::ZxFS::FileSuffixDel(path_4);
        assert(file_suffix_remove_4 == "dirx/asfas/r/weg/wfqwr/.");

        std::string_view path_5 = "my.jpg";
        auto file_name_5 = ZQF::ZxFS::FileName(path_5);
        assert(file_name_5 == "my.jpg");
        auto fle_suffix_5 = ZQF::ZxFS::FileSuffix(path_5);
        assert(fle_suffix_5 == ".jpg");
        auto file_suffix_remove_5 = ZQF::ZxFS::FileSuffixDel(path_5);
        assert(file_suffix_remove_5 == "my");
        auto file_stem_5 = ZQF::ZxFS::FileNameStem(path_5);
        assert(file_stem_5 == "my");


        std::string_view path_6 = "y.j";
        auto file_suffix_remove_6 = ZQF::ZxFS::FileSuffixDel(path_6);
        assert(file_suffix_remove_6 == "y");

        std::string_view path_7 = ".j";
        auto fle_suffix_7 = ZQF::ZxFS::FileSuffix(path_7);
        assert(fle_suffix_7 == ".j");
        auto file_suffix_remove_7 = ZQF::ZxFS::FileSuffixDel(path_7);
        assert(file_suffix_remove_7 == "");

        std::string_view path_8 = ".";
        auto fle_suffix_8 = ZQF::ZxFS::FileSuffix(path_8);
        assert(fle_suffix_8 == ".");


        auto [self_dir_sv, self_dir_buf] = ZQF::ZxFS::SelfDir();
        auto [self_path_sv, self_path_buf] = ZQF::ZxFS::SelfPath();

        auto self_dir_exist = ZQF::ZxFS::Exist(self_dir_sv);
        assert(self_dir_exist == true);

        auto self_path_exist = ZQF::ZxFS::Exist(self_path_sv);
        assert(self_path_exist == true);

        ZQF::ZxFS::DirDelete("123x/",false);
        auto mkdir_status0 = ZQF::ZxFS::DirMake("123x/", false);
        assert(mkdir_status0 == true);
        auto check_mkdir_status_0 = ZQF::ZxFS::Exist("123x/");
        assert(check_mkdir_status_0 == true);
        ZQF::ZxFS::DirDelete("123x/", false);


        bool copy_status_0 = ZQF::ZxFS::FileCopy(self_path_sv, "test.bin", false);
        assert(copy_status_0 == true);

        bool copy_status_1 = ZQF::ZxFS::FileCopy(self_path_sv, "test.bin", true);
        assert(copy_status_1 == false);

        ZQF::ZxFS::DirMake("weufbuiwef/214124/41241/", true);
        bool move_file_status_0 = ZQF::ZxFS::FileMove("test.bin", "weufbuiwef/214124/41241/test.bin");
        assert(move_file_status_0 == true);

        bool move_file_status_1 = ZQF::ZxFS::FileMove("weufbuiwef/214124/41241/test.bin", "test.bin");
        assert(move_file_status_1 == true);

        bool del_status = ZQF::ZxFS::FileDelete("test.bin");
        assert(del_status == true);

        for (ZQF::ZxFS::Walk walk{ self_dir_sv }; walk.NextFile(); )
        {
            std::println("{}{}", walk.GetSearchDir(), walk.GetName());
        }

        auto dir_make_recursive_status = ZQF::ZxFS::DirMake("123/41245/215/125/1251/", true);
        assert(dir_make_recursive_status == true);
        assert(ZQF::ZxFS::Exist("123/41245/215/125/1251/"));
        ZQF::ZxFS::DirDelete("123/", true);

        [[maybe_unused]] int x = 0;

    }
    catch (const std::exception& err)
    {
        std::println(std::cerr, "std::exception: {}", err.what());
    }

}
