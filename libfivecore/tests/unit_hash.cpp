#include <gtest/gtest.h>
#include "fivecore/hash.h"
#include <fstream>
#include <filesystem>

namespace fivecore::hash::test {

// 1. 基础内存字符串流式哈希
TEST(Sha256StreamTest, SimpleText)
{
    Sha256Stream st;
    const char* data = "hello world";
    st.update(reinterpret_cast<const uint8_t*>(data), 11);
    auto res = st.finalize();
    const std::string expect = "b94d27b9934d3e08a52e52d7da7dabfac484efe37a5380ee9088f7ace2efcde9";
    ASSERT_EQ(res, expect);
}

// 2. 分多次update（分段喂数据，模拟流式）
TEST(Sha256StreamTest, MultiUpdate)
{
    Sha256Stream st;
    st.update((const uint8_t*)"hello ", 6);
    st.update((const uint8_t*)"world", 5);
    auto res = st.finalize();
    const std::string expect = "b94d27b9934d3e08a52e52d7da7dabfac484efe37a5380ee9088f7ace2efcde9";
    ASSERT_EQ(res, expect);
}

// 3. finalize 之后再次update，应当失效
TEST(Sha256StreamTest, UpdateAfterFinalizeReturnFalse)
{
    Sha256Stream st;
    st.update((const uint8_t*)"abc", 3);
    auto h = st.finalize();
    bool ok = st.update((const uint8_t*)"xyz",3);
    ASSERT_FALSE(ok);
}

// 4. 空数据哈希
TEST(Sha256StreamTest, EmptyInput)
{
    Sha256Stream st;
    auto res = st.finalize();
    const std::string expect = "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855";
    ASSERT_EQ(res, expect);
}

// 5. sha256_file：正常文件
TEST(Sha256FileTest, NormalFile)
{
    // 临时文件
    std::string tmp_path = "tmp_hash_test.bin";
    std::ofstream f(tmp_path, std::ios::binary);
    f << "hello world";
    f.close();

    auto h = sha256_file(tmp_path);
    const std::string expect = "b94d27b9934d3e08a52e52d7da7dabfac484efe37a5380ee9088f7ace2efcde9";
    ASSERT_EQ(h, expect);

    // 清理临时文件
    std::filesystem::remove(tmp_path);
}

// 6. sha256_file：文件不存在，返回空字符串
TEST(Sha256FileTest, FileNotExistReturnsEmpty)
{
    auto h = sha256_file("non_exist_xxxxxx.file");
    ASSERT_TRUE(h.empty());
}

// 7. sha256_file：空文件
TEST(Sha256FileTest, EmptyFile)
{
    std::string tmp_path = "tmp_empty.bin";
    std::ofstream f(tmp_path, std::ios::binary);
    f.close();

    auto h = sha256_file(tmp_path);
    const std::string expect = "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855";
    ASSERT_EQ(h, expect);

    std::filesystem::remove(tmp_path);
}

} // namespace
