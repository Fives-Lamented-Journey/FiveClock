//
// Created by wufan on 2026/9/21.
//
#include <gtest/gtest.h>
#include "fivecore/http.h"
#include <filesystem>
#include <nlohmann/json.hpp>

#include "fivecore/fileutil.h"
#include "fivecore/hash.h"

namespace fivecore::http::test {
    using json = nlohmann::json;

// 测试1：put_file_stream，传入不存在文件，应当返回错误
TEST(HttpPutStreamTest, FileNotExist)
{
    CurlGlobalGuard guard;
    auto resp = put_file_stream("http://127.0.0.1:9000/dummy", "no_such_file_12345.bin");
    ASSERT_FALSE(resp.ok);
    ASSERT_TRUE(resp.error_msg.find("cannot open file") != std::string::npos);
}

// 测试2：put_file_stream，空文件，打开文件成功（不执行真实网络请求）
// 注：仅测试文件打开逻辑，网络部分不会真正成功
TEST(HttpPutStreamTest, EmptyLocalFile)
{
    CurlGlobalGuard guard;
    std::string tmp_path = "tmp_http_empty.bin";
    {
        std::ofstream f(tmp_path, std::ios::binary);
    }
    auto resp = put_file_stream("http://127.0.0.1:9000/dummy", tmp_path);
    // 文件打开成功，但访问假URL，网络必然失败，ok=false
    ASSERT_FALSE(resp.ok);
    // 没有"cannot open file"错误，代表文件打开阶段通过
    ASSERT_TRUE(resp.error_msg.find("cannot open file") == std::string::npos);
    std::filesystem::remove(tmp_path);
}

// 测试3：post_json 空字符串body，本地构造请求（访问无效地址，测试参数组装）
TEST(HttpPostJsonTest, PostEmptyJson)
{
    CurlGlobalGuard guard;
    auto resp = post_json("http://127.0.0.1:9999/not_exist_server", "{}");
    ASSERT_FALSE(resp.ok);
    // 会返回连接类错误，status_code=0
    ASSERT_EQ(resp.status_code, 0L);
}

// ============ 【集成测试，需要本地启动FastAPI，默认关闭】 ============
#if HTTP_INTEGRATION_TEST
// 测试4：POST请求真实后端获取预签名URL（需要启动Day5的FastAPI服务）
TEST(HttpIntegrationTest, GetPresignedUrl)
{
    CurlGlobalGuard guard;
    json req;
    std::string path = "intergration_test.txt";
    req["object_key"] = path;
    req["file_size"] = fileutil::get_size(path);
    req["sha256_hash"] = hash::sha256_file(path);
    std::cout << req["object_key"] << "\n";
    std::cout << req["file_size"] << "\n";
    std::cout << req["sha256_hash"] << "\n";
    // std::string json_req = R"({"file_path":"test.bin","size":100,"sha256":"xxxx"})";

    std::string url = "http://127.0.0.1:8000/api/presign/upload";
    auto resp = post_json(url, req.dump());
    json response = json::parse(resp.body);
    std::cout << response.dump(4) <<"\n";

    ASSERT_TRUE(resp.ok);
    ASSERT_EQ(resp.status_code,200);
    ASSERT_FALSE(resp.body.empty());
}

// // 测试5：PUT流式上传真实文件到MinIO预签名地址
// TEST(HttpIntegrationTest, PutFileRealPresigned)
// {
//     CurlGlobalGuard guard;
//     // std::string tmp_file = "tmp_upload_test.bin";
//     std::string tmp_file = "intergration_test.txt";
//     // {
//     //     std::ofstream f(tmp_file, std::ios::binary);
//     //     f.write("hello fivecore http", 18);
//     // }
//     // 这里替换成后端返回的真实预签名URL
//     // std::string presigned_url = "REPLACE_WITH_REAL_PRESIGNED_URL";
//     std::string presigned_url = "http://127.0.0.1:9000/bucket/intergration_test.txt?X-Amz-Algorithm=AWS4-HMAC-SHA256&X-Amz-Credential=minioadmin%2F20260923%2Fus-east-1%2Fs3%2Faws4_request&X-Amz-Date=20260923T034521Z&X-Amz-Expires=3600&X-Amz-SignedHeaders=host&X-Amz-Signature=eaca3408f827416c08b7040b906e7d181db8585289a21d2fc008582e1ee1c287";
//     auto resp = put_file_stream(presigned_url, tmp_file);
//
//     ASSERT_TRUE(resp.ok);
//     ASSERT_EQ(resp.status_code,200);
//     std::filesystem::remove(tmp_file);
// }

TEST(HttpIntegrationTest,upload)
{
    CurlGlobalGuard guard;
    json req;
    std::string path = "intergration.txt";
    req["object_key"] = path;
    req["file_size"] = fileutil::get_size(path);
    req["sha256_hash"] = hash::sha256_file(path);
    std::cout << req["object_key"] << "\n";
    std::cout << req["file_size"] << "\n";
    std::cout << req["sha256_hash"] << "\n";

    std::string url = "http://127.0.0.1:8000/api/presign/upload";
    auto resp = post_json(url, req.dump());
    json response = json::parse(resp.body);
    std::cout << response.dump(4) <<"\n";

    ASSERT_TRUE(resp.ok);
    ASSERT_EQ(resp.status_code,200);
    ASSERT_FALSE(resp.body.empty());


    std::string tmp_file = "intergration.txt";
    std::ofstream new_file(tmp_file);
    std::string presigned_url = response["upload_url"];
    auto resp02 = put_file_stream(presigned_url, tmp_file);

    ASSERT_TRUE(resp02.ok);
    ASSERT_EQ(resp02.status_code,200);
    // std::filesystem::remove(tmp_file);


}


TEST(HttpIntegrationTest, download)
{
    CurlGlobalGuard guard;
    std::string path = "intergration.txt";
    std::string url = "http://127.0.0.1:8000/api/presign/download";
    std::vector<std::pair<std::string,std::string>> query = {{"object_key",path}};
    auto resp = get(url,query);
    ASSERT_TRUE(resp.ok);
    ASSERT_EQ(resp.status_code,200);

    json response = json::parse(resp.body);
    ASSERT_EQ("intergration.txt",response["object_key"]);

    std::string download_url = response["download_url"].get<std::string>();;
    auto resp_download = get_file_stream(download_url,"download_intergration.txt");
    ASSERT_TRUE(resp_download.ok);
    ASSERT_EQ(resp_download.status_code,200);
}
#endif

}
