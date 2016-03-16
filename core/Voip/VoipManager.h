#ifndef __VOIP_MANAGER_H__
#define __VOIP_MANAGER_H__

#ifdef _WIN32
    #include "libvoip/include/voip/voip2.h"
#else
    #include "libvoip/include/voip/voip2.h"
#endif

#include "VoipManagerDefines.h"

#include <vector>
#include <memory>

namespace core {
    class core_dispatcher;
}

namespace voip_manager {
    class VoipManagerImpl;
    
    class VoipManager {
        std::shared_ptr<VoipManagerImpl> _impl;

    public:
        VoipManager(core::core_dispatcher& dispatcher);

    public:
        std::shared_ptr<ICallManager>       get_call_manager      ();
        std::shared_ptr<IWindowManager>     get_window_manager    ();
        std::shared_ptr<IMediaManager>      get_media_manager     ();
        std::shared_ptr<IDeviceManager>     get_device_manager    ();
        std::shared_ptr<IConnectionManager> get_connection_manager();
        std::shared_ptr<IVoipManager>       get_voip_manager      ();
    };
}

#endif//__VOIP_MANAGER_H__