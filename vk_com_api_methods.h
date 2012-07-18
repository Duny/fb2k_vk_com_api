#pragma once

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

        // Edits artist/title fields of the track m_id
        class edit : public api_method_base
        {
            void run (abort_callback & p_abort) override;
            metadb_handle_ptr m_track;
            t_vk_audio_id m_aid;
        public:
            // Answer from vk.com server returned after file was upload
            edit (const metadb_handle_ptr & p_track, t_vk_audio_id aid) : m_track (p_track), m_aid (aid) {}
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