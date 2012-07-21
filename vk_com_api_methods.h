#pragma once

// List of all api methods :
// http://vkontakte.ru/developers.php?o=-1&p=%D0%9E%D0%BF%D0%B8%D1%81%D0%B0%D0%BD%D0%B8%D0%B5_%D0%BC%D0%B5%D1%82%D0%BE%D0%B4%D0%BE%D0%B2_API&s=0


namespace vk_com_api
{
    class NOVTABLE api_method_base
    {
    public:
        api_method_base () : m_aborted (false) {}

        // Does actual call of vk api method
        bool call (abort_callback & p_abort = abort_callback_impl ())
        {
            m_aborted = false;
            m_error.reset ();

            try { run (p_abort); }
            catch (const exception_aborted &) { m_aborted = true; }
            catch (const pfc::exception & e) { m_error = e.what (); }

            return m_error.is_empty () && !m_aborted;
        }

        bool aborted () const { return m_aborted; }
        const char * get_error () const { return m_error; }

    private:
        virtual void run (abort_callback & p_abort) = 0;
        bool m_aborted; // Call was aborted by user
    protected:
        pfc::string8 m_error;
    };

    // helper implementation of some apis
    namespace audio
    {
        namespace albums
        {
            // Reads a list of user albums (from vk.com profile)
            // Represents as list of (album_name, album_id) pairs
            class get : public api_method_base,
                        public pfc::list_t<audio_album_info>
            {
                void run (abort_callback & p_abort) override;
            };


            // Creates new album in user profile
            class add : public api_method_base
            {
                void run (abort_callback & p_abort) override;
                audio_album_info m_new_album;
            public:
                explicit add (const pfc::string_base & title) : m_new_album (boost::make_tuple (title.get_ptr (), 0)) {}

                operator audio_album_info const & () const  { return m_new_album; }
            };


            // Deletes album and all of it contents (!) from user profile
            class del : public api_method_base
            {
                void run (abort_callback & p_abort) override;
                t_vk_album_id m_id_to_delete;
            public:
                del (t_vk_album_id id) : m_id_to_delete (id) {}
            };


            // Renames album in user profile
            class ren : public api_method_base
            {
                void run (abort_callback & p_abort) override;
                t_vk_album_id m_album_id;
                pfc::string8  m_new_title;
            public:
                ren (t_vk_album_id album_id, const pfc::string_base & p_new_title)
                    : m_album_id (album_id), m_new_title (p_new_title) {}
            };
        } // namespace albums


        // Moves vk audio track to specified album
        class move_to_album : public api_method_base
        {
            void run (abort_callback & p_abort) override;
            t_vk_audio_id m_audio_id;
            t_vk_album_id m_album_id;
        public:
            move_to_album (t_vk_audio_id audio_id, t_vk_album_id album_id)
                : m_audio_id (audio_id), m_album_id (album_id) {}
        };


        // Queries url for posting upload to
        class get_upload_server : public api_method_base
        {
            void run (abort_callback & p_abort) override;
            pfc::string8 m_url;
        public:
            operator const char* () const { return m_url.get_ptr (); }
        };


        // Saves newly uploaded audio file in users profile
        class save : public api_method_base
        {
            void run (abort_callback & p_abort) override;
            response_json_ptr m_result;
            t_vk_audio_id m_id; // Id of newly uploaded mp3 file
            pfc::string8 m_artist;
            pfc::string8 m_title;
        public:
            // p_answer from vk.com server returned after file was upload
            save (const pfc::string_base & p_answer, const metadb_handle_ptr & p_item_to_save);

            t_vk_audio_id get_id () const { return m_id; }
        };

        // Returns user track count
        class get_count : public api_method_base
        {
            t_uint32 count;
            t_int64 oid;
            void run (abort_callback & p_abort) override;
        public:
            // owner_id > 0 for filter by user_id,
            // otherwise filter by group_id (-gid)
            get_count (t_int64 owner_id) : count (0), oid (owner_id) {}

            operator t_uint32 () const { return count; }
        };

        // Returns user track list
        class get : public api_method_base,
                    public pfc::list_t<audio_track_info>
        {
            void run (abort_callback & p_abort) override;

            const t_vk_user_id  uid;
            const t_vk_group_id gid;
            const t_vk_album_id album_id;
            const pfc::string   aids; // comma separated list of t_vk_user_id's for filter with uid or gid
            const t_size        need_user; // if 1 then server will return user info
            const t_size        count;
            const t_size        offset;
        public:
            get (t_vk_user_id        p_uid      = pfc_infinite,
                 t_vk_group_id       p_gid      = pfc_infinite,
                 t_vk_album_id       p_album_id = pfc_infinite,
                 const pfc::string & p_aids     = "",
                 t_size              p_count    = pfc_infinite,
                 t_size              p_offset   = pfc_infinite);
        };

    } // namespace audio


    namespace wall
    {
        // Makes post on wall this optimal message and audios attached
        class post : public api_method_base
        {
            void run (abort_callback & p_abort) override;
            pfc::string8 m_message;
            const pfc::list_t<t_vk_audio_id> & m_audio_ids;
        public:
            post (const pfc::string_base & p_msg, const pfc::list_t<t_vk_audio_id> & p_audio_ids)
                : m_message (p_msg), m_audio_ids (p_audio_ids) {}
        };
    }
}