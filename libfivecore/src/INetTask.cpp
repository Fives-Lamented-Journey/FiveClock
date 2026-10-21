
#include "fivecore/INetTask.h"

namespace fivecore::http_async
{
    /// @copydoc fivecore::http_async::INetTask::write_callback
    size_t INetTask::write_callback(char* ptr, size_t size, size_t nmemb, void* userdata)
    {
        auto* task = static_cast<INetTask*>(userdata);
        if (!task)
        {
            return 0;
        }
        return  task->on_write(ptr,size,nmemb);
    }

    /// @copydoc fivecore::http_async::INetTask::read_callback
    size_t INetTask::read_callback(char* ptr, size_t size, size_t nmemb, void* userdata)
    {
        auto* task = static_cast<INetTask*>(userdata);
        if (!task)
        {
            return 0;
        }
        return  task->on_read(ptr,size,nmemb);
    }

    /// @copydoc fivecore::http_async::INetTask::cancel
    VoidResult INetTask::cancel()
    {
        is_cancelled_ = true;
        return OkVoid();
    }
}
