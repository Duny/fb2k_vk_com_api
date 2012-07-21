#include "stdafx.h"
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
        const response_json_ptr result = vk_com_api::get_provider ()->invoke ("audio.getUploadServer", p_abort);
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
        const std::vector<std::string> vec_field_names (required_fields.begin (), required_fields.end ());
        
        if (includes_all_names (*m_result, vec_field_names))
        {
            url_parameters params;
            std::for_each (required_fields.begin (), required_fields.end (), [&] (const char * field) {
                    params.push_back (std::make_pair (field, m_result[field].asCString ()));
                });

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
        else
        {
            pfc::string_formatter err_mgs = "Not enough parameters in response from file upload. Expected fields: \n";
            std::for_each (required_fields.begin (), required_fields.end (), [&] (const char * field) {
                    err_mgs << "(" << field << ")";
                });
            err_mgs << "\nGot:\n" << m_result->toStyledString ().c_str ();
            throw pfc::exception (err_mgs);
        }
    }

    void audio::get_count::run (abort_callback & p_abort)
    {
        const response_json_ptr response = vk_com_api::get_provider ()->invoke ("audio.getCount", url_parameters ("oid", pfc::string_formatter () << oid), p_abort);

        if (!response->isNull ())
            count = pfc::atoui_ex (response->asCString (), pfc_infinite);
    }

    audio::get::get (
        t_vk_user_id        p_uid,
        t_vk_group_id       p_gid,
        t_vk_album_id       p_album_id,
        const pfc::string & p_aids,
        t_size              p_count,
        t_size              p_offset) :

        uid                 (p_uid),
        gid                 (p_gid),
        album_id            (p_album_id),
        aids                (p_aids),
        need_user           (0),
        count               (p_count),
        offset              (p_offset)
    {}

    void audio::get::run (abort_callback & p_abort)
    {
        using namespace boost::assign;

        url_parameters params;

        if (uid != 0 && uid != pfc_infinite)
            params.push_back (std::make_pair ("uid", pfc::format_uint (uid)));
        if (gid != 0 && gid != pfc_infinite)
            params.push_back (std::make_pair ("gid", pfc::format_uint (gid)));
        if (album_id != 0 && album_id != pfc_infinite)
            params.push_back (std::make_pair ("album_id", pfc::format_uint (album_id)));
        if (aids.get_length () != 0)
            params.push_back (std::make_pair ("aids", aids.get_ptr ()));
        if (need_user)
            params.push_back (std::make_pair ("need_user", "1"));
        if (count != 0 && count != pfc_infinite)
            params.push_back (std::make_pair ("count", pfc::format_uint (count)));
        if (offset != 0 && offset != pfc_infinite)
            params.push_back (std::make_pair ("offset", pfc::format_uint (offset)));

        const response_json_ptr response = vk_com_api::get_provider ()->invoke ("audio.get", params, p_abort);

        if (!response->isArray ())
            throw pfc::exception ("Unexpected response from audio.get (response is not an array)");


        const auto required_fields = list_of ("aid")("owner_id")("artist")("title")("duration")("url");
        const std::vector<std::string> vec_field_names (required_fields.begin (), required_fields.end ());

        for (t_size n = response->size (), i = 0; i < n; ++i)
        {
            const Json::Value & track_info_object = response[i];

            if (!track_info_object.isObject ())
                throw pfc::exception ("Unexpected response from audio.get (track info is not an object)");

            if (includes_all_names (track_info_object, required_fields))
            {
                this->add_item (audio_track_info (
                    track_info_object["aid"].asUInt (),
                    track_info_object["owner_id"].asInt64 (),
                    track_info_object["artist"].asCString (),
                    track_info_object["title"].asCString (),
                    track_info_object["duration"].asUInt (),
                    track_info_object["url"].asCString ()));
            }
            else
            {                
                pfc::string_formatter err_mgs = "Not enough parameters in response from audio.get. Expected fields: \n";
                std::for_each (vec_field_names.cbegin (), vec_field_names.cend (), [&] (const std::string & field) {
                    err_mgs << "(" << field.c_str () << ")";
                });
                err_mgs << "\nGot:\n" << track_info_object.toStyledString ().c_str ();
                throw pfc::exception (err_mgs);
            }
        }
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