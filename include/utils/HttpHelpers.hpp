/**
    HttpHelpers.hpp : HttpHelpers definition
    Copyright 2026 Daniel Wilson
*/

#include <utils/Socket.hpp>
#include <utils/LogFile.hpp>
#include <utils/KeyValuePair.hpp>
#include <string.h>

namespace utils
{
    class HttpHelpers
    {
        public:
            static uint8_t       HexToInt( char a_value );
            static char          IntToHex( uint8_t a_value );
            static ::std::string HtmlEscape( const ::std::string &a_string );
            static ::std::string UriEncode( const ::std::string &a_string );
            static ::std::string UriDecode( const ::std::string &a_string );
            static uint32_t      UriDecode( ::std::string &a_uri, ::std::string &a_ext );
            static bool          UriDecode( ::std::string &a_base, ::std::string &a_defaultDoc, ::std::string &a_uri, ::std::string &a_ext, ::std::string &a_defmime );
            static bool          IsDirectory( ::std::string &a_path );
            static bool          IsFile( ::std::string &a_path );
    };
}
