#pragma once
#include <string>
#include <cstdint>
#include <openssl/evp.h>
#include "fivecore/result.h"

/**
 * @file hash.h
 * @brief SHA256 哈希计算（基于 OpenSSL EVP 接口）
 */
namespace fivecore::hash {

    /**
     * @brief 流式 SHA256 计算器
     *
     * 适用于大文件 / 流式数据：通过多次 update() 追加数据块，最后调用 finalize()
     * 得到最终哈希。内部持有 OpenSSL 的 EVP_MD_CTX 原生句柄，禁止拷贝，支持移动。
     */
    class Sha256Stream {
    public:
        /// 构造：创建并初始化 EVP 摘要上下文
        Sha256Stream();
        /// 析构：释放 EVP 摘要上下文
        ~Sha256Stream();

        /// 禁止拷贝（持有原生句柄）
        Sha256Stream(const Sha256Stream&) = delete;
        /// 禁止拷贝赋值
        Sha256Stream& operator=(const Sha256Stream&) = delete;

        /// 移动构造：接管对方的上下文句柄
        Sha256Stream(Sha256Stream&& other) noexcept;
        /// 移动赋值：释放自身句柄后接管对方的上下文句柄
        Sha256Stream& operator=(Sha256Stream&& other) noexcept;

        /**
         * @brief 追加一个数据块到哈希计算中，可多次调用
         * @param data 数据缓冲区指针
         * @param len 数据长度（字节）
         * @return true 成功；false 上下文未初始化或已 finalize
         */
        bool update(const uint8_t* data, size_t len);

        /**
         * @brief 结束计算，返回小写 16 进制 SHA256 字符串
         * @return 64 字符小写十六进制串；失败返回空串
         * @note 调用后不可再 update()，否则返回 false
         */
        std::string finalize();

        /** @brief Result 版本的 update，失败时携带 HashComputeFailed */
        Result<bool> res_update(const uint8_t* data, size_t len);
        /** @brief Result 版本的 finalize，失败时携带 HashComputeFailed */
        Result<std::string> res_finalize();
        /** @brief VoidResult 版本的 update，仅反馈成功/失败，不返回值 */
        VoidResult res_void_update(const uint8_t* data, size_t len);


    private:
        EVP_MD_CTX* ctx_{nullptr}; ///< OpenSSL 摘要上下文句柄
        bool finished_{false};     ///< 是否已调用 finalize（finalize 后不可再 update）
    };

    /**
     * @brief 一次性计算整个文件的 SHA256（内部使用 Sha256Stream 流式读取）
     * @param path 文件路径
     * @return 64 字符小写十六进制串；文件无法打开或计算失败返回空串
     */
    std::string sha256_file(const std::string& path);

    /**
     * @brief Result 版本：计算文件 SHA256
     * @param path 文件路径
     * @return Ok(hex_string) 成功；Err(FileNotFound/FileOpenFailed/HashComputeFailed) 失败
     */
    Result<std::string>  res_sha256_file(const std::string& path);


}
