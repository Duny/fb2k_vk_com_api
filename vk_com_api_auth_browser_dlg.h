#pragma once
#include "vk_com_api_resource.h"

namespace vk_com_api
{
    class browser_dialog :
        public CAxDialogImpl<browser_dialog>,
        public CDialogResize<browser_dialog>,
        public IDispEventImpl<IDC_VK_COM_API_LOGIN_DLG_IE_CTRL, browser_dialog>
    {
    public:
        enum { IDD = IDD_VK_COM_API_LOGIN_DLG };

        typedef std::function<void (browser_dialog *p_dlg)> on_navigate_complete_callback;


        browser_dialog (const char * p_title, const char * p_url, const on_navigate_complete_callback & p_callback);


        void show ();

        void navigate (const char * url);
        const pfc::string_base & get_browser_location () const { return m_current_location; }

    private:
        BEGIN_MSG_MAP(browser_dialog)
            CHAIN_MSG_MAP(CAxDialogImpl<browser_dialog>)
            CHAIN_MSG_MAP(CDialogResize<browser_dialog>)
            MESSAGE_HANDLER_SIMPLE(WM_INITDIALOG, on_init_dialog)
            MSG_WM_CLOSE(close)
            MSG_WM_DESTROY(on_destroy)
        END_MSG_MAP()

        BEGIN_SINK_MAP(browser_dialog)
            SINK_ENTRY(IDC_VK_COM_API_LOGIN_DLG_IE_CTRL, DISPID_NAVIGATECOMPLETE2, on_navigate_complete2)
        END_SINK_MAP()

        BEGIN_DLGRESIZE_MAP(browser_dialog)
            DLGRESIZE_CONTROL(IDC_VK_COM_API_LOGIN_DLG_IE_CTRL, DLSZ_SIZE_X | DLSZ_SIZE_Y)
        END_DLGRESIZE_MAP()


        // Message handlers

        // Dialog
        void on_init_dialog ();
        void close () { EndDialog (IDOK); }
        void on_destroy ();

        // Web browser control
        void __stdcall on_navigate_complete2 (IDispatch*, VARIANT *p_url);

        // Member variables
        pfc::string8 m_title, m_inital_location, m_current_location;
        CComPtr<IWebBrowser2>         m_wb2;
        on_navigate_complete_callback m_callback;
    };
}