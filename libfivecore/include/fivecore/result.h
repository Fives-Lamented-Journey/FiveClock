//
// Created by wufan on 2026/9/27.
//

/**
 * @file result.h
 * @brief fivecore 统一错误处理类型：Result<T> / ErrorInfo / ErrorCode
 *
 * 提供类似 Rust Result 的值类型，用于在函数返回值中携带成功值或错误信息，
 * 避免通过出参 / 异常 / 全局 errno 传递错误。所有 fivecore 模块的 res_* 系列
 * 接口均返回 Result<T>。
 */

#ifndef LIBFIVECORE_RESULT_H
#define LIBFIVECORE_RESULT_H
#include <assert.h>
#include <optional>
#include <string>
#include <variant>

namespace fivecore
{
    /**
     * @brief 统一错误码枚举，覆盖文件 / 哈希 / 网络 / curl / 业务各层
     */
    enum class ErrorCode
    {
        Ok = 0,
        //文件层
        FileNotFound,       ///< 文件不存在
        FileOpenFailed,     ///< 文件打开失败
        FileReadError,      ///< 文件读取失败
        FileWriteError,     ///< 文件写入失败
        //哈希层
        HashComputeFailed,  ///< SHA256 计算失败
        //网络层
        NetworkConnectFail, ///< 网络连接失败
        HttpBadStatus,      ///< HTTP 状态码非 2xx
        HttpTransferError,  ///< curl 传输过程出错

        CurlCreateHandleFailed, ///< curl_easy_init 创建句柄失败
        CurlMultiInitFailed,    ///< curl_multi_init 失败
        CurlMultiAddFailed,     ///< 将 easy handle 加入 multi 失败
        ContextStopped,         ///< CurlMultiContext 已停止，拒绝新任务
        CurlRuntimeError,       ///< curl 运行时其它错误
        //业务/服务端
        ServerResponseInvalid   ///< 服务端响应格式非法
    };

    /**
     * @brief 错误载体：错误码 + 可读描述短语
     */
    struct ErrorInfo
    {
        ErrorCode code;       ///< 错误码
        std::string message;  ///< 人类可读的错误描述

        /**
         * @brief 便捷构造函数
         * @param c 错误码
         * @param msg 错误描述（string_view，内部拷贝为 std::string）
         * @return 构造好的 ErrorInfo
         */
        static ErrorInfo make(ErrorCode c, std::string_view msg)
        {
            return ErrorInfo{c,std::string{msg}};
        }
    };

    /**
     * @brief 结果类型：要么持有成功值 T，要么持有错误 ErrorInfo
     *
     * 用法：
     * @code
     *   Result<int> r = some_func();
     *   if (r.is_ok()) use(r.value());
     *   else           log(r.error().message);
     * @endcode
     *
     * @tparam T 成功时持有的值类型
     */
    template<typename  T>
    class  Result
    {
    private:
        std::variant<T,ErrorInfo> m_storage; ///< 内部存储：成功值或错误

        Result(T&& val): m_storage(std::move(val)) {}
        Result(ErrorInfo&& err): m_storage(std::move(err)) {}

    public:
        /**
         * @brief 构造一个成功的 Result
         * @param value 成功值（会被移动）
         */
        static Result Ok(T value)
        {
            return Result(std::move(value));
        }

        /**
         * @brief 构造一个失败的 Result
         * @param err 错误信息（会被移动）
         */
        static Result Err(ErrorInfo err)
        {
            return Result(std::move(err));
        }

        /** @return 是否持有成功值 */
        [[nodiscard]] bool is_ok() const noexcept
        {
            return std::holds_alternative<T>(m_storage);
        }

        /** @return 是否持有错误 */
        [[nodiscard]] bool is_err() const noexcept
        {
            return std::holds_alternative<ErrorInfo>(m_storage);
        }

        /**
         * @brief 获取成功值（可写引用）
         * @pre is_ok() == true，否则 assert 失败
         */
        T& value()
        {
            assert(is_ok());
            return std::get<T>(m_storage);
        }

        /**
         * @brief 获取成功值（只读引用）
         * @pre is_ok() == true，否则 assert 失败
         */
        const T& value() const
        {
            assert(is_ok());
            return std::get<T>(m_storage);
        }

        /**
         * @brief 获取错误信息（可写引用）
         * @pre is_err() == true，否则行为未定义
         */
        ErrorInfo& error()
        {
            return std::get<ErrorInfo>(m_storage);
        }

        /**
         * @brief 获取错误信息（只读引用）
         * @pre is_err() == true，否则行为未定义
         */
        const ErrorInfo& error() const
        {
            return std::get<ErrorInfo>(m_storage);
        }

        /**
         * @brief 安全取值：成功返回内部值，失败返回传入的默认值
         * @param default_val 失败时的兜底值
         */
        T value_or(T default_val) const
        {
            if (is_ok())
                return  value();
            return std::move(default_val);
        }

    };

    /// 无返回值场景的 Result 别名（成功时持有 std::monostate）
    using VoidResult = Result<std::monostate>;

    /** @brief 构造一个成功的 VoidResult */
    inline VoidResult OkVoid()
    {
        return VoidResult::Ok(std::monostate{});
    }

    /**
     * @brief 从 Result 中提取错误（若存在）
     * @tparam T Result 持有的成功值类型
     * @param r 输入 Result
     * @return 若 r 为 Err，返回对应的 ErrorInfo；否则返回 std::nullopt
     */
    template<typename T>
    std::optional<ErrorInfo> take_err(const Result<T>& r)
    {
        if(r.is_err()) return r.error();
        return std::nullopt;
    }
}
#endif //LIBFIVECORE_RESULT_H
