#include "StdAfx.h"
#include "vk_com_api.h"

namespace vk_com_api
{
    void audio::albums::get::run (abort_callback & p_abort)
    {
        using namespace boost::assign;

        response_json_ptr result = vk_com_api::get_provider ()->invoke ("audio.getAlbums", list_of<name_value_pair> ("count", "100"), p_abort);
        for (t_size n = result->size (), i = 1; i < n; ++i) {
            Json::Value & al = result[i];
            if (al.isMember ("title") && al.isMember ("album_id"))
                add_item (boost::make_tuple (al["title"].asCString (), al["album_id"].asUInt ()));
            else
                throw pfc::exception ("not enough fields in response from audio.getAlbums");
        }
    }

    void audio::albums::add::run (abort_callback & p_abort)
    {
        using namespace boost::assign;

        response_json_ptr result = vk_com_api::get_provider ()->invoke ("audio.addAlbum", list_of<name_value_pair> ("title", m_new_album.get<0>()), p_abort);
        if (result->isMember ("album_id"))
            m_new_album.get<1>() = result["album_id"].asUInt ();
        else
            throw pfc::exception ("no 'album_id' field in response from audio.addAlbum");
    }

    void audio::albums::del::run (abort_callback & p_abort)
    {
        using namespace boost::assign;

        vk_com_api::get_provider ()->invoke ("audio.deleteAlbum", list_of<name_value_pair> ("album_id", pfc::string_formatter () << m_id_to_delete), p_abort);
        ///!!!! TEST ME: test error codes from vk server ("Application is disabled. Enable your application or use test mode.", etc)
    }

    void audio::albums::ren::run (abort_callback & p_abort)
    {
        using namespace boost::assign;

        vk_com_api::get_provider ()->invoke ("audio.editAlbum", 
            list_of<name_value_pair>
                ("title", m_new_title)
                ("album_id", pfc::string_formatter () << m_album_id),
            p_abort);
    }

    void audio::move_to_album::run (abort_callback & p_abort)
    {
        using namespace boost::assign;

        vk_com_api::get_provider ()->invoke ("audio.moveToAlbum", 
            list_of<name_value_pair>
                ("aids",     pfc::string_formatter () << m_audio_id)
                ("album_id", pfc::string_formatter () << m_album_id),
            p_abort);
    }

    void audio::get_upload_server::run (abort_callback & p_abort)
    {
        response_json_ptr result = vk_com_api::get_provider ()->invoke ("audio.getUploadServer", p_abort);
        if (result->isMember ("upload_url"))
            m_url = result["upload_url"].asCString ();
        else
            throw pfc::exception ("no 'upload_url' field in response from audio.getUploadServer");
    }

    audio::save::save (const pfc::string_base & p_answer, const metadb_handle_ptr & p_item_to_save) : m_result (p_answer)
    {
        if (p_item_to_save.is_valid ())
        {
            service_ptr_t<titleformat_object> p_object;
            static_api_ptr_t<titleformat_compiler> p_compiler;
            p_compiler->compile (p_object, "%artist%");
            p_item_to_save->format_title (nullptr, m_artist, p_object, &titleformat_text_filter_nontext_chars ());
            p_compiler->compile (p_object, "%title%");
            p_item_to_save->format_title (nullptr, m_title, p_object, &titleformat_text_filter_nontext_chars ());
        }
    }

    void audio::save::run (abort_callback & p_abort)
    {
        using namespace boost::assign;

        const auto required_fields = list_of ("server")("audio")("hash");
        
        if (m_result.has_members (required_fields)) {

            url_parameters params;
            std::for_each (required_fields.begin (), required_fields.end (),
                [&] (const char * field)
                {
                    params.push_back (std::make_pair (field, m_result[field].asCString ()));
                }
            );

            if (!m_artist.is_empty () && !m_title.is_empty ())
            {
                params.push_back (std::make_pair ("artist", m_artist));
                params.push_back (std::make_pair ("title", m_title));
            }            

            m_result = vk_com_api::get_provider ()->invoke ("audio.save", params, p_abort);

            if (m_result->isMember ("aid"))
                m_id = m_result["aid"].asUInt ();
            else
                throw pfc::exception ("no 'aid' field in response from audio.save");
        }
        else {
            pfc::string_formatter err_mgs = "Not enough parameters in response from file upload. Expected fields: \n";
            std::for_each (required_fields.begin (), required_fields.end (),
                [&] (const char * field)
                {
                    err_mgs << "(" << field << ")";
                }
            );
            err_mgs << "\nGot:\n" << m_result->toStyledString ().c_str ();
            throw pfc::exception (err_mgs);
        }
    }

    void audio::edit::run (abort_callback & p_abort)
    {
        using namespace boost::assign;

        static_api_ptr_t<titleformat_compiler> p_compiler;
        service_ptr_t<titleformat_object> p_object;
        p_compiler->compile (p_object, "%artist%||%title%");

        pfc::string8 str;
        m_track->format_title (nullptr, str, p_object, &titleformat_text_filter_nontext_chars ());
        auto sep_pos = str.find_first ("||");

        vk_com_api::get_provider ()->invoke ("audio.edit", 
            list_of<name_value_pair> 
                ("aid", pfc::string_formatter () << m_aid)
                ("oid", get_auth_manager ()->get_user_id ())
                ("artist", pfc::string_part (str.get_ptr (), sep_pos))
                ("title", pfc::string_part (str.get_ptr () + sep_pos + 2, str.get_length () - sep_pos - 2)),
            p_abort);
    }

    void wall::post::run (abort_callback & p_abort)
    {
        url_parameters params;

        if (!m_message.is_empty ())
            params.push_back (std::make_pair ("message", m_message));

        pfc::string8 attachments;
        pfc::string8 user_id = get_auth_manager ()->get_user_id ();
        for (t_size i = 0, n = m_audio_ids.get_size (); i < n; ++i)
            attachments << "audio" << user_id << "_" << m_audio_ids[i] << ",";
        if (!attachments.is_empty ()) {
            attachments.truncate (attachments.length () - 1); // remove last ',' symbol
            params.push_back (std::make_pair ("attachment", attachments));
        }

        if (params.size ())
            vk_com_api::get_provider ()->invoke ("wall.post", params, p_abort);
    }
}