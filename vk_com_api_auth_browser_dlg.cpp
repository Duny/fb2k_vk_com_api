#include "StdAfx.h"
#include "vk_com_api.h"


namespace
{
    CComModule g_module;

    class myinitquit : public initquit
    {
        void on_init () override { g_module.Init (NULL, NULL, &LIBID_ATLLib); }
        void on_quit () override { g_module.Term (); }
    };
    initquit_factory_t<myinitquit> g_initquit;

    cfgDialogPosition g_dlg_pos (vk_com_api::externals::guid_browser_dlg_cfg);
}

namespace vk_com_api
{
    browser_dialog::browser_dialog (const char * p_title,
                                    const char * p_url,
                                    const on_navigate_complete_callback & p_callback)
        : m_title (p_title),
          m_inital_location (p_url),
          m_callback (p_callback)
    {}


    void browser_dialog::show ()
    {
        DoModal (core_api::get_main_window ());
    } 

    void browser_dialog::navigate (const char * url)
    {
        m_current_location.reset ();
        if (m_wb2)
            m_wb2->Navigate (CComBSTR (url), nullptr, nullptr, nullptr, nullptr);
    }

    void browser_dialog::on_init_dialog ()
    {
        DlgResize_Init (true, false);
        g_dlg_pos.AddWindow (*this);

        // Query IWebBrowser2 interface
        CAxWindow ie = GetDlgItem (IDC_VK_COM_API_LOGIN_DLG_IE_CTRL);
        if (ie.IsWindow () != TRUE || ie.QueryControl (&m_wb2) != S_OK)
            close ();
        else {
            SetWindowText (pfc::stringcvt::string_os_from_utf8 (m_title));
            navigate (m_inital_location);
            ShowWindow (SW_SHOWNORMAL);
        }
    }   

    void browser_dialog::on_destroy ()
    {
        g_dlg_pos.RemoveWindow (*this);
        if (m_wb2) m_wb2.Release ();
    }

    void __stdcall browser_dialog::on_navigate_complete2 (IDispatch*, VARIANT *p_url)
    {
        m_current_location = pfc::stringcvt::string_utf8_from_os (p_url->bstrVal);
        m_callback (this);
    }
}