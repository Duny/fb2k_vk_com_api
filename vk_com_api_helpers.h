#pragma once
#include "json/json.h"

// helpers for tuple stream i/o
template<class T1>
inline stream_reader_formatter<> & read_tuple (stream_reader_formatter<> & stream, boost::tuples::cons<T1, boost::tuples::null_type> & value) { return stream >> value.head; }

inline stream_reader_formatter<> & read_tuple (stream_reader_formatter<> & stream, boost::tuples::null_type & value) { return stream; }

template<class T1, class T2>
inline stream_reader_formatter<> & read_tuple (stream_reader_formatter<> & stream, boost::tuples::cons<T1, T2> & value) { stream >> value.head; return read_tuple (stream, value.tail); }

template<class T1>
inline stream_writer_formatter<> & write_tuple (stream_writer_formatter<> & stream, const boost::tuples::cons<T1, boost::tuples::null_type> & value) { return stream << value.head; }

inline stream_writer_formatter<> & write_tuple (stream_writer_formatter<> & stream, const boost::tuples::null_type & value) { return stream; }

template<class T1, class T2>
inline stream_writer_formatter<> & write_tuple (stream_writer_formatter<> & stream, const boost::tuples::cons<T1, T2> & value) { stream << value.head; return write_tuple (stream, value.tail); }

#define DEFINE_FB2K_STREAM_READER_WRITER(tuple_type)\
    FB2K_STREAM_READER_OVERLOAD(tuple_type) { return read_tuple  (stream, value); }\
    FB2K_STREAM_WRITER_OVERLOAD(tuple_type) { return write_tuple (stream, value); }

namespace vk_com_api
{
    typedef t_uint32 t_vk_album_id;
    typedef t_uint32 t_vk_audio_id;

    // used for storing information about user albums
    // first field is the title of album, second is id
    typedef boost::tuples::tuple<pfc::string8, t_vk_album_id> audio_album_info;


    class response_json_ptr
    {
        bool is_valid () const {
            if (!m_val.is_valid ()) return false;
            const Json::Value & p_val = *m_val;
            if (p_val.isNull ()) return false;
            if (p_val.isObject ()) return !p_val.isMember ("error");
            return true;
        };

        void assert_valid ()
        {
            if (!is_valid ()) {
                if (!m_val.is_valid ())
                    m_response_text.insert_chars (0, "Invalid response: ");
                else {
                    const Json::Value & val = *m_val;
                    if (val.isObject () && val.isMember ("error")) {
                        const Json::Value &error = val.get ("error", Json::nullValue);
                        if (error.isObject () && error.isMember ("error_msg"))
                            m_response_text.set_string_ (error["error_msg"].asCString ());
                    }
                    else
                        m_response_text.set_string_ (val.toStyledString ().c_str ());
                }
                throw pfc::exception (m_response_text.get_ptr ());
            }
        }

        pfc::rcptr_t<Json::Value> m_val;
        mutable pfc::string8 m_response_text;

    public:
        response_json_ptr (const pfc::string_base & p_data) : m_response_text (p_data) {
            const char * begin = p_data.get_ptr ();
            const char *   end = begin + p_data.get_length ();
            Json::Value val;

            if (Json::Reader ().parse (begin, end, val, false)) {
                bool has_response_field = val.isObject () && val.isMember ("response");
                m_val.new_t (has_response_field ? val["response"] : val);
            }

            assert_valid (); // Throw exception if we have bad response
        }

        Json::Value & operator[] (Json::UInt index) { return (*m_val)[index]; }
        const Json::Value & operator[] (Json::UInt index) const { return (*m_val)[index]; }

        Json::Value & operator[] (const char * key) { assert_valid (); return (*m_val)[key]; }
        const Json::Value & operator[] (const char * key) const { return (*m_val)[key]; }

        Json::Value* operator-> () { assert_valid (); return &(*m_val); }
        const Json::Value* operator-> () const { return &(*m_val); }

        bool has_members (const std::vector<const char*> & names)
        {
            for (auto iter = names.cbegin (), end = names.cend (); iter != end; ++iter)
                if (!m_val->isMember (*iter))
                    return false;
            return true;
        }
    };


    // Represents a list of name=>value string pairs of URL parameters
    // (http://wwww.vk.com/?name1=value1&name2=value2&..)
    typedef std::pair<pfc::string8, pfc::string8> name_value_pair;
    typedef std::vector<name_value_pair> t_url_parameters;

    class url_parameters : public t_url_parameters
    {
    public:
        url_parameters () {}

        explicit url_parameters (const pfc::string_base & p_url);

        bool includes_all_of (const std::vector<const char*> & p_names);

        // Use carefully, it does not check whatever was p_name found or not. So p_name should exist
        const pfc::string8 & get_as_str (const char * p_name);

        unsigned get_as_uint (const char * p_name)
        {
            return pfc::atoui_ex (get_as_str (p_name), pfc_infinite);
        }
    };

    // Console logging helpers
    struct error_log : public pfc::string_formatter
    {
        ~error_log () { if (!is_empty()) console::formatter () << "Error("COMPONENT_NAME"):" << get_ptr (); }
    };

    struct debug_log : public pfc::string_formatter
    {
        ~debug_log () { if (!is_empty ()) console::formatter () << "Debug("COMPONENT_NAME"):" << get_ptr (); }
    };
}