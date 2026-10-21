#include "fivecore/hash.h"
#include <sstream>
#include <iomanip>
#include <fstream>
#include <cassert>

#include "fivecore/fileutil.h"

namespace fivecore::hash {

/// @copydoc fivecore::hash::Sha256Stream::Sha256Stream
Sha256Stream::Sha256Stream()
{
    ctx_ = EVP_MD_CTX_new();
    if (ctx_)
    {
        const EVP_MD* md = EVP_sha256();
        // 初始化摘要上下文
        EVP_DigestInit_ex(ctx_, md, nullptr);
    }
}

/// @copydoc fivecore::hash::Sha256Stream::~Sha256Stream
Sha256Stream::~Sha256Stream()
{
    if (ctx_)
    {
        EVP_MD_CTX_free(ctx_);
        ctx_ = nullptr;
    }
}

/// @copydoc fivecore::hash::Sha256Stream::Sha256Stream(Sha256Stream&&)
Sha256Stream::Sha256Stream(Sha256Stream&& other) noexcept
{
    ctx_ = other.ctx_;
    finished_ = other.finished_;
    other.ctx_ = nullptr;
}

/// @copydoc fivecore::hash::Sha256Stream::operator=(Sha256Stream&&)
Sha256Stream& Sha256Stream::operator=(Sha256Stream&& other) noexcept
{
    if (this == &other) return *this;
    // 释放当前资源
    if (ctx_)
        EVP_MD_CTX_free(ctx_);

    ctx_ = other.ctx_;
    finished_ = other.finished_;
    other.ctx_ = nullptr;
    return *this;
}

/// @copydoc fivecore::hash::Sha256Stream::update
bool Sha256Stream::update(const uint8_t* data, size_t len)
{
    if (!ctx_ || finished_)
        return false;
    // EVP_DigestUpdate：1成功，0失败
    return EVP_DigestUpdate(ctx_, data, len) == 1;
}

/// @copydoc fivecore::hash::Sha256Stream::finalize
std::string Sha256Stream::finalize()
{
    if (!ctx_ || finished_)
        return "";
    finished_ = true;

    unsigned char digest[EVP_MAX_MD_SIZE]{};
    unsigned int digest_len = 0;

    int ret = EVP_DigestFinal_ex(ctx_, digest, &digest_len);
    if (ret != 1)
        return "";

    // 转小写16进制字符串
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (unsigned int i = 0; i < digest_len; ++i)
    {
        oss << std::setw(2) << static_cast<int>(digest[i]);
    }
    return oss.str();
}

/// @copydoc fivecore::hash::Sha256Stream::res_update
Result<bool> Sha256Stream::res_update(const uint8_t* data, size_t len)
{
    if (!ctx_) return Result<bool>::Err(ErrorInfo::make(ErrorCode::HashComputeFailed,"哈希函数上下文未初始化")) ;
    if (finished_) return Result<bool>::Err(ErrorInfo::make(ErrorCode::HashComputeFailed,"哈希上下文已销毁"));

    // EVP_DigestUpdate：1成功，0失败
    if ( EVP_DigestUpdate(ctx_, data, len) == 1 ) return Result<bool>::Ok(true);
    else return Result<bool>::Err(ErrorInfo::make(ErrorCode::HashComputeFailed,"哈希中间迭代错误"));
    // else return Result<bool>::Ok(false);
}

/// @copydoc fivecore::hash::Sha256Stream::res_finalize
Result<std::string> Sha256Stream::res_finalize()
{
    if (!ctx_) return Result<std::string>::Err(ErrorInfo::make(ErrorCode::HashComputeFailed,"哈希上下文未初始化")) ;
    if (finished_) return Result<std::string>::Err(ErrorInfo::make(ErrorCode::HashComputeFailed,"哈希上下文已销毁"));

    finished_ = true;

    unsigned char digest[EVP_MAX_MD_SIZE]{};
    unsigned int digest_len = 0;

    int ret = EVP_DigestFinal_ex(ctx_, digest, &digest_len);
    if (ret != 1) return Result<std::string>::Err(ErrorInfo::make(ErrorCode::HashComputeFailed,"哈希最后迭代错误"));

    // 转小写16进制字符串
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (unsigned int i = 0; i < digest_len; ++i)
    {
        oss << std::setw(2) << static_cast<int>(digest[i]);
    }
    return Result<std::string>::Ok(oss.str());
}

/// @copydoc fivecore::hash::Sha256Stream::res_void_update
VoidResult Sha256Stream::res_void_update(const uint8_t* data, size_t len)
{
    if (!ctx_) return VoidResult::Err(ErrorInfo::make(ErrorCode::HashComputeFailed,"哈希函数上下文未初始化")) ;
    if (finished_) return VoidResult::Err(ErrorInfo::make(ErrorCode::HashComputeFailed,"哈希上下文已销毁"));

    // EVP_DigestUpdate：1成功，0失败
    if ( EVP_DigestUpdate(ctx_, data, len) == 1 ) return OkVoid();
    else return VoidResult::Err(ErrorInfo::make(ErrorCode::HashComputeFailed,"哈希中间迭代错误"));
}

/// @copydoc fivecore::hash::sha256_file
std::string sha256_file(const std::string& path)
{
    std::ifstream f(path, std::ios::binary);
    if (!f.is_open())
        return "";

    Sha256Stream sh;
    constexpr size_t BUF_SIZE = 65536;
    uint8_t buf[BUF_SIZE];

    while (f.read(reinterpret_cast<char*>(buf), BUF_SIZE))
    {
        if (!sh.update(buf, BUF_SIZE))
            return "";
    }
    auto read_len = f.gcount();
    if (read_len > 0)
    {
        if (!sh.update(buf, static_cast<size_t>(read_len)))
            return "";
    }
    return sh.finalize();
}

/// @copydoc fivecore::hash::res_sha256_file
Result<std::string> res_sha256_file(const std::string& path)
{
    auto existRes = fileutil::res_exists(path);

    if(auto e = take_err(existRes)){
        return Result<std::string>::Err(*e);
    }

    if(!existRes.value()){
        return Result<std::string>::Err(ErrorInfo::make(ErrorCode::FileNotFound, "文件不存在"));
    }

    std::ifstream f(path, std::ios::binary);
    if (!f.is_open()) return Result<std::string>::Err(ErrorInfo::make(ErrorCode::FileOpenFailed,"文件无法打开"));


    Sha256Stream sh;
    constexpr size_t BUF_SIZE = 65536;
    uint8_t buf[BUF_SIZE];

    while (f.read(reinterpret_cast<char*>(buf), BUF_SIZE))
    {
        // auto res = sh.res_update(buf,BUF_SIZE);
        auto res = sh.res_void_update(buf,BUF_SIZE);
        if (auto e = take_err(res))
        {
          return Result<std::string>::Err(*e);
        }

        // bool ok = res.value();
        // if (!ok)
        // {
        //     return Result<std::string>::Err(ErrorInfo::make(ErrorCode::HashComputeFailed,"哈希中间迭代计算错误"));
        // }
    }

    auto read_len = f.gcount();
    if (read_len > 0)
    {
        // auto res = sh.res_update(buf,static_cast<size_t>(read_len));
        auto res = sh.res_void_update(buf,static_cast<size_t>(read_len));
        if (auto e = take_err(res))
        {
            return Result<std::string>::Err(*e);
        }

        // bool ok = res.value();
        // if (!ok)
        // {
        //     return Result<std::string>::Err(ErrorInfo::make(ErrorCode::HashComputeFailed,"哈希中间迭代计算错误"));
        // }

    }
    auto res = sh.res_finalize();
    if (auto e = take_err(res))
    {
        return Result<std::string>::Err(*e);
    }


    return res;
    // return Result<std::string>::Ok(res.value());
    // return sh.finalize();
}
}
