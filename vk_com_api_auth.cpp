#include "stdafx.h"
#include "vk_com_api.h"
#include <time.h>

// user_id, access_token, timestamp of authentication, expires_in (how long access_token live)
typedef boost::tuples::tuple<pfc::string8, pfc::string8, time_t, t_uint32> auth_data;
enum { field_user_id, field_access_token, field_timestamp, field_expires_in };

DEFINE_FB2K_STREAM_READER_WRITER(auth_data);


// Shortcuts for string building
//

// vk.com api login/logout url
#define VK_COM_BLANK_URL "http://api.vk.com/blank.html"

inline pfc::string8 get_vk_com_login_url ()
{
    return pfc::string_formatter () 
        << "http://api.vk.com/oauth/authorize?display=popup&scope=audio,wall&response_type=token&client_id="<< vk_com_api::externals::vk_api_application_id 
        << "&redirect_uri=" << VK_COM_BLANK_URL;
}

inline pfc::string8 get_vk_com_logout_url ()
{
    return pfc::string_formatter () 
        << "http://api.vk.com/oauth/logout?client_id=" << vk_com_api::externals::vk_api_application_id;
}

// login/logout dialog caption text
inline pfc::string8 get_login_dlg_title ()
{
    return pfc::string_formatter () << vk_com_api::externals::auth_browser_dlg_caption_prefix << "logging in to vk.com";
}

inline pfc::string8 get_logout_dlg_title ()
{
    return pfc::string_formatter () << vk_com_api::externals::auth_browser_dlg_caption_prefix << "logging out from vk.com";
}


namespace vk_com_api
{
    const GUID vk_auth_manager::class_guid = externals::auth_manager_class_guid;


    class auth_manager_impl : public vk_auth_manager
    {
        // vk_auth_manager overrides
        bool is_valid () const override
        { 
            if (m_auth_data.get<field_user_id>().is_empty () || m_auth_data.get<field_access_token>().is_empty ())
                return false;

            if (m_auth_data.get<field_expires_in>() > 0 &&
                (time (nullptr) > (m_auth_data.get<field_timestamp>() + m_auth_data.get<field_expires_in>()))) return false;

            return true;
        }

        const pfc::string_base & get_user_id () override
        {
            check_auth_data ();
            return m_auth_data.get<field_user_id>();
        }

        const pfc::string_base & get_access_token () override
        {
            check_auth_data ();
            return m_auth_data.get<field_access_token>();
        }

        void relogin () override
        {
            const auth_data auth_data_empty;

            browser_dialog
            (
                get_logout_dlg_title (), 
                get_vk_com_logout_url (),
                [] (browser_dialog * p_dlg) { p_dlg->close (); }
            ).show ();

            m_auth_data.val () = auth_data_empty; // Clear current data

            // Get new data
            try { get_auth_data (); }
            catch (exception_aborted) {}
            catch (const std::exception & e) {
                uMessageBox (core_api::get_main_window (), e.what (), "Error during authorization", MB_OK | MB_ICONERROR);
            }
        }

        // Helpers
        void check_auth_data ()
        {
            if (!is_valid ()) get_auth_data ();
        }

        void get_auth_data ()
        {
            using namespace boost::assign;

            pfc::string8_fast location;
            bool success;

            auto navigate_callback = [&] (browser_dialog * p_dlg)
            {
                location = p_dlg->get_browser_location ();

                // Close if user pressed "Cancel" button
                if (location.find_first ("cancel=1") != pfc_infinite || location.find_first ("user_denied") != pfc_infinite) {
                    success = false;
                    p_dlg->close ();
                }
                // if address contains "blank.html#" (part of VK_COM_BLANK_URL), when auth was done successfully 
                else if (location.find_first ("blank.html#") != pfc_infinite) { 
                    success = true;
                    p_dlg->close ();
                }
            };

            do {
                success = false;

                browser_dialog
                (
                    get_login_dlg_title (),
                    get_vk_com_login_url (),
                    navigate_callback
                ).show ();

                if (!success && uMessageBox (core_api::get_main_window (), "Try again?", "vk.com authorization", MB_YESNO | MB_ICONQUESTION) == IDNO)
                    throw exception_aborted ();
            } while (!success);
            

            url_parameters params (location);

            // Normally all of this must present in the redirected url
            if (params.includes_all_of (list_of ("access_token")("user_id")("expires_in"))) 
                m_auth_data.val () = boost::make_tuple (params.get_as_str ("user_id"), params.get_as_str ("access_token"), time (nullptr), params.get_as_uint ("expires_in"));
            else if (params.includes_all_of (list_of ("error"))) // Some error occurred
                throw pfc::exception (params.get_as_str ("error"));
            else
                throw pfc::exception (pfc::string_formatter () << "Unexpected server redirect:" << location);
        }

        // Member variables
        cfg_obj<auth_data> m_auth_data;

    public:
        auth_manager_impl () : m_auth_data (externals::guid_auth_manager_cfg) {}
    };

    namespace { service_factory_single_t<auth_manager_impl> g_auth_factory; }
}