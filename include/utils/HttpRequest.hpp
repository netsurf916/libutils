/**
    HttpRequest.hpp : HttpRequest definition
    Copyright 2021 Daniel Wilson
*/

#include <utils/Socket.hpp>
#include <utils/LogFile.hpp>
#include <utils/KeyValuePair.hpp>
#include <string.h>

namespace utils
{
    class HttpRequest : public Lockable
    {
        private:
            ::std::string m_method;
            ::std::string m_uri;
            ::std::string m_version;
            uint64_t      m_length;
            int64_t       m_start;
            int64_t       m_end;
            ::std::string m_body;
            ::std::string m_host;
            uint32_t      m_port;
            bool          m_timeout;
            ::std::shared_ptr< KeyValuePair< ::std::string, ::std::string > > m_meta;
            ::std::string m_response;

        public:
            HttpRequest();
            ~HttpRequest();

            void Reset();

            ::std::string &Uri();
            ::std::string &Method();
            ::std::string &Version();
            ::std::shared_ptr< KeyValuePair< ::std::string, ::std::string > > &Meta();
            ::std::string  Host();
            ::std::string &Response();

            bool    Read( Socket &a_socket );
            int32_t Respond( Socket &a_socket, ::std::string &a_fileName, ::std::string &a_type );

            void Log( LogFile &a_logger );
    };

    class HttpHelpers
    {
        public:
            static uint8_t  CharToHex( char a_value );
            static uint32_t UriDecode( ::std::string &a_uri, ::std::string &a_ext );
            static bool     UriDecode( ::std::string &a_base, ::std::string &a_defaultDoc, ::std::string &a_uri, ::std::string &a_ext, ::std::string &a_defmime );
    };
}