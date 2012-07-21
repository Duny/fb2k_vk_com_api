#include "StdAfx.h"
#include "vk_com_api.h"

namespace vk_com_api
{
    url_parameters::url_parameters (const pfc::string_base & p_url)
    {
        t_size pos;
        // Find beginning of parameters in URL
        if ((pos = p_url.find_first ('#')) != pfc_infinite || (pos = p_url.find_first ('?')) != pfc_infinite) {
            t_size len = p_url.length (), pos2, pos3;
            for (pos = pos + 1; pos < len; pos = pos3 + 1) {
                if ((pos2 = p_url.find_first ('=', pos)) == pfc_infinite)
                    break;

                if ((pos3 = p_url.find_first ('&', ++pos2)) == pfc_infinite)
                    pos3 = len;

                this->push_back (std::make_pair (
                    /*name*/ pfc::string_part (p_url.get_ptr () + pos,  pos2 - pos - 1), 
                    /*value*/pfc::string_part (p_url.get_ptr () + pos2, pos3 - pos2)));
            }
        }
    }

    url_parameters::url_parameters (const pfc::string8 & p_name, const pfc::string8 & p_value)
    {
        push_back (std::make_pair (p_name, p_value));
    }

    // Helper : 
    // returns predicate (function) to be used with std::find_if algorithm
    template<class T>
    std::function<bool (const name_value_pair & p)> get_stricmp_pred (const T & val)
    {
        return [&val] (const name_value_pair & p) -> bool { return pfc::stringCompareCaseInsensitive (p.first, val) == 0; };
    }

    bool url_parameters::includes_all_of (const std::vector<const char*> & p_names)
    { 
        auto b = this->cbegin (), e = this->cend ();
        for (size_t i = 0, n = p_names.size (); i < n; ++i) {
            auto it = std::find_if (b, e, get_stricmp_pred (p_names[i]));
            if (it == e) return false;
        }
        return true;
    }

    const pfc::string8 & url_parameters::get_as_str (const char * p_name)
    {
        auto b = this->cbegin (), e = this->cend ();
        auto it = std::find_if (b, e, get_stricmp_pred (p_name));
        if (it == e)
        {
            static pfc::string8 empty;
            error_log () << "url_parameters::get_as_str () failed. This indicates some bug in code!";
            return empty;
        }
        return it->second;
    }

    bool includes_all_names (const Json::Value & json_val, const std::vector<std::string> & names)
    {
        if (!json_val.isObject ())
            return false;

        const auto member_names = json_val.getMemberNames ();

        auto i1 = member_names.cbegin (), e1 = member_names.cend ();
        auto i2 = names.cbegin (), e2 = names.cend ();

        for (; i2 != e2; ++i2)
        {
            auto pos = std::find_if (i1, e1, [&] (const std::string & s) {
                return pfc::stringCompareCaseInsensitive (s.c_str (), (*i2).c_str ()) == 0;
            });
            if (pos == e1)
                return false;
        }
        return true;
    }
}