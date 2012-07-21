#pragma once
#include "vk_com_api_auth_browser_dlg.h"

// Authorization process for desktop apps: http://vkontakte.ru/developers.php?oid=-1&p=Авторизация_клиентских_приложений

namespace vk_com_api
{
    class NOVTABLE vk_auth_manager : public service_base
    {
        FB2K_MAKE_SERVICE_INTERFACE_ENTRYPOINT(vk_auth_manager)
    public:
        // True if use had logged in and session does not expired
        virtual bool user_logged () const = 0;
        // All this functions may throw exception_aborted then user cancels authorization (by pressing "Cancel" button in dialog)
        virtual const pfc::string_base & get_user_id () = 0;
        virtual const pfc::string_base & get_access_token () = 0;

        virtual void relogin () = 0;
    };

    typedef static_api_ptr_t<vk_com_api::vk_auth_manager> get_auth_manager;

    // Helpers
    inline t_uint32 get_user_id_as_uint32 ()
    {
        return static_cast<t_uint32>(pfc::atoui_ex (get_auth_manager ()->get_user_id (), pfc_infinite));
    }
}