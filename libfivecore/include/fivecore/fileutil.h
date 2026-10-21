#pragma once
#include <string>
#include <cstdint>
#include "fivecore/result.h"

/**
 * @file fileutil.h
 * @brief 跨平台文件系统工具函数（基于 std::filesystem）
 */
namespace fivecore::fileutil {

    /**
     * @brief 判断文件是否存在
     * @param path 文件路径
     * @return true 存在；false 不存在或发生异常
     */
    bool exists(const std::string& path);

    /**
     * @brief 获取文件字节大小
     * @param path 文件路径
     * @return 文件大小（字节）；文件不存在或读取失败返回 0
     * @todo 缺陷:无法区分空文件和读取失败错误，可以用std::optional<uint64_t>改进
     */
    uint64_t get_size(const std::string& path);

    /**
     * @brief Result 版本：判断文件是否存在
     * @param path 文件路径
     * @return Ok(bool) 成功时 bool 表示是否存在；Err(FileReadError) 底层文件系统异常
     * @todo 可优化:用error_code版本的std::filesystem::exists替代异常，更加简洁高效
     */
    Result<bool> res_exists(const std::string& path);

    /**
     * @brief Result 版本：获取文件字节大小
     * @param path 文件路径
     * @return Ok(uint64_t) 文件大小；Err(FileNotFound) 文件不存在；Err(FileReadError) 读取异常
     */
    Result<uint64_t> res_get_size(const std::string& path);
}
