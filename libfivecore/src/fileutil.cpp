#include "fivecore/fileutil.h"
#include <filesystem>

namespace fivecore::fileutil {

    /// @copydoc fivecore::fileutil::exists
    bool exists(const std::string& path) {
        return std::filesystem::exists(path);
    }

    /// @copydoc fivecore::fileutil::get_size
    uint64_t get_size(const std::string& path) {
        if (!exists(path)) return 0;
        return static_cast<uint64_t>(std::filesystem::file_size(path));
    }

    /// @copydoc fivecore::fileutil::res_exists
    Result<bool> res_exists(const std::string& path)
    {
        try
        {
            bool ok = std::filesystem::exists(path);
            return Result<bool>::Ok(ok);
        }
        catch (...)
        {
            return Result<bool>::Err(ErrorInfo::make( ErrorCode::FileReadError,"检测文件状态失败") );
        }

    }

    /// @copydoc fivecore::fileutil::res_get_size
    Result<uint64_t> res_get_size(const std::string& path)
    {
        auto existRes =  res_exists(path);
        if (existRes.is_err())
        {
            return Result<uint64_t>::Err(existRes.error());
        }

        bool file_exist = existRes.value();
        if (!file_exist)
        {
            return Result<uint64_t>::Err(ErrorInfo::make(ErrorCode::FileNotFound,"文件不存在"));
        }


        uint64_t size;
        try
        {
            size = static_cast<uint64_t>(std::filesystem::file_size(path));
        }
        catch (...)
        {
            return Result<uint64_t>::Err(ErrorInfo::make(ErrorCode::FileReadError,"获取文件大小失败 "));
        }

        return Result<uint64_t>::Ok(size);
    }


}
