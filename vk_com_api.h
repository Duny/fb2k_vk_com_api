#pragma once
// boost includes
#include "boost/assign.hpp"
#include "boost/scope_exit.hpp"
#include "boost/foreach.hpp"
#include "boost/tuple/tuple.hpp"
#include "boost/tuple/tuple_comparison.hpp"

// crt includes
#include <tuple>
#include <functional>
#include <algorithm>

// foobar2000 includes
#include "foobar2000/ATLHelpers/ATLHelpers.h"
#include <exdispid.h>
#include <atlframe.h>


// vk_com_api additional headers
#include "vk_com_api_external.h" // Externals symbols from this file must be defined
#include "vk_com_api_helpers.h"
#include "vk_com_api_auth.h"


// Documentation:
//
// Authorization process :
// http://vkontakte.ru/developers.php?oid=-1&p=%D0%90%D0%B2%D1%82%D0%BE%D1%80%D0%B8%D0%B7%D0%B0%D1%86%D0%B8%D1%8F_%D1%81%D0%B0%D0%B9%D1%82%D0%BE%D0%B2

// Calling api methods :
// http://vkontakte.ru/developers.php?oid=-1&p=%D0%92%D1%8B%D0%BF%D0%BE%D0%BB%D0%BD%D0%B5%D0%BD%D0%B8%D0%B5_%D0%B7%D0%B0%D0%BF%D1%80%D0%BE%D1%81%D0%BE%D0%B2_%D0%BA_API

// List of all api methods :
// http://vkontakte.ru/developers.php?o=-1&p=%D0%9E%D0%BF%D0%B8%D1%81%D0%B0%D0%BD%D0%B8%D0%B5_%D0%BC%D0%B5%D1%82%D0%BE%D0%B4%D0%BE%D0%B2_API&s=0



namespace vk_com_api
{
    __declspec(selectany) extern const char * version = "1.0";


    typedef pfc::rcptr_t<pfc::array_t<t_uint8>> membuf_ptr;

    enum
    {
        // Not sure if this is a right value for desktop apps
        // In practice call rate is about 0.75 api calls per second
        max_api_calls_per_second = 3,

        max_mp3_file_size = 21 * 1024 * 1024 // Limit for mp3 file upload
    };

    class NOVTABLE vk_api_provider : public service_base
    {
        FB2K_MAKE_SERVICE_INTERFACE_ENTRYPOINT(vk_api_provider)
    public:
        virtual response_json_ptr invoke (const char * p_api_name, const t_url_parameters & p_params, abort_callback & p_abort) = 0;
        response_json_ptr invoke (const char * p_api_name, abort_callback & p_abort) { return invoke (p_api_name, url_parameters (), p_abort); }

        // Returns raw answer from vk server after file has been uploaded
        virtual membuf_ptr upload_audio_file (const char * p_url, const char * p_file_path, abort_callback & p_abort) = 0;
    };

    typedef static_api_ptr_t<vk_api_provider> get_provider;
}

#include "vk_com_api_methods.h"