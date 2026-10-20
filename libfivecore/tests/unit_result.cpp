//
// Created by wufan on 2026/9/28.
//
#include <gtest/gtest.h>
#include <fstream>
#include <filesystem>
#include "fivecore/hash.h"
#include "fivecore/fileutil.h"
#include "fivecore/result.h"

namespace fs = std::filesystem;

// 测试夹具：自动生成临时文件，测试结束自动删除
/**
 * @todo Sha256Stream: 构造函数EVP_MD_CTX_new分配失败时无返回，后续改成静态工厂函数 Result<Sha256Stream> create()
 * @todo TearDown: fs::remove_all增加try-catch，防止文件占用抛出异常
 * @todo OpenSSL错误栈：失败时读取ERR_get_error原生错误信息填充ErrorInfo.message
 */
class unit_result_hash : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_tempDir = fs::temp_directory_path() / "fivecore_test";
        fs::create_directories(m_tempDir);
        m_fileEmpty = m_tempDir / "empty.txt";
        m_fileAbc = m_tempDir / "abc.txt";

        // 空文件
        std::ofstream f_empty(m_fileEmpty);
        f_empty.close();

        // 文件写入abc
        std::ofstream f_abc(m_fileAbc);
        f_abc << "abc";
        f_abc.close();
    }

    void TearDown() override
    {
        fs::remove_all(m_tempDir);
    }

    fs::path m_tempDir;
    fs::path m_fileEmpty;
    fs::path m_fileAbc;
};

// ========= fileutil res_exists 测试 =========
TEST_F(unit_result_hash, ResExists_FileExist_ReturnOkTrue)
{
    auto res = fivecore::fileutil::res_exists(m_fileAbc.string());
    ASSERT_TRUE(res.is_ok());
    EXPECT_EQ(res.value(), true);
}

TEST_F(unit_result_hash, ResExists_FileNotExist_ReturnOkFalse)
{
    auto res = fivecore::fileutil::res_exists((m_tempDir / "no_exist.txt").string());
    ASSERT_TRUE(res.is_ok());
    EXPECT_EQ(res.value(), false);
}

// ========= fileutil res_get_size 测试 =========
TEST_F(unit_result_hash, ResGetSize_EmptyFile_SizeZero)
{
    auto res = fivecore::fileutil::res_get_size(m_fileEmpty.string());
    ASSERT_TRUE(res.is_ok());
    EXPECT_EQ(res.value(), 0ULL);
}

TEST_F(unit_result_hash, ResGetSize_AbcFile_Size3)
{
    auto res = fivecore::fileutil::res_get_size(m_fileAbc.string());
    ASSERT_TRUE(res.is_ok());
    EXPECT_EQ(res.value(), 3ULL);
}

TEST_F(unit_result_hash, ResGetSize_FileMissing_ReturnFileNotFound)
{
    auto res = fivecore::fileutil::res_get_size((m_tempDir / "no_exist.txt").string());
    ASSERT_TRUE(res.is_err());
    EXPECT_EQ(res.error().code, fivecore::ErrorCode::FileNotFound);
}

// ========= res_sha256_file 正向测试 =========
TEST_F(unit_result_hash, ResSha256File_EmptyFile_CorrectHash)
{
    auto res = fivecore::hash::res_sha256_file(m_fileEmpty.string());
    ASSERT_TRUE(res.is_ok());
    const std::string expect = "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855";
    EXPECT_EQ(res.value(), expect);
}

TEST_F(unit_result_hash, ResSha256File_AbcContent_CorrectHash)
{
    auto res = fivecore::hash::res_sha256_file(m_fileAbc.string());
    ASSERT_TRUE(res.is_ok());
    const std::string expect = "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad";
    EXPECT_EQ(res.value(), expect);
}

// ========= res_sha256_file 负向错误路径 =========
TEST_F(unit_result_hash, ResSha256File_FileMissing_ReturnFileNotFound)
{
    std::string path = (m_tempDir / "not_here.bin").string();
    auto res = fivecore::hash::res_sha256_file(path);
    ASSERT_TRUE(res.is_err());
    EXPECT_EQ(res.error().code, fivecore::ErrorCode::FileNotFound);
}

// ========= Sha256Stream 单独单元测试（内存数据，不读文件） =========
TEST(unit_result_Sha256Stream, StreamUpdateFinalize_AbcData)
{
    fivecore::hash::Sha256Stream sh;
    const char data[] = "abc";
    auto resUpdate = sh.res_void_update(reinterpret_cast<const uint8_t*>(data), 3);
    ASSERT_TRUE(resUpdate.is_ok());

    auto resFinal = sh.res_finalize();
    ASSERT_TRUE(resFinal.is_ok());
    const std::string expect = "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad";
    EXPECT_EQ(resFinal.value(), expect);
}

TEST(unit_result_Sha256Stream, Stream_FinalizeTwice_ReturnErr)
{
    fivecore::hash::Sha256Stream sh;
    auto r1 = sh.res_void_update((const uint8_t*)"123",3);
    ASSERT_TRUE(r1.is_ok());
    auto fin1 = sh.res_finalize();
    ASSERT_TRUE(fin1.is_ok());

    // 第二次finalize，已经finished，返回Err
    auto fin2 = sh.res_finalize();
    ASSERT_TRUE(fin2.is_err());
    EXPECT_EQ(fin2.error().code, fivecore::ErrorCode::HashComputeFailed);
}

// int main(int argc, char **argv)
// {
//     testing::InitGoogleTest(&argc, argv);
//     return RUN_ALL_TESTS();
// }
