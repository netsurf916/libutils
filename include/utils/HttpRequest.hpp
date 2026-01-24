/**
    HttpRequest.hpp : HttpRequest definition
    Description: HTTP request parsing and representation.
    Copyright 2021-2026 Daniel Wilson
    SPDX-License-Identifier: MIT
*/

#include <utils/HttpHelpers.hpp>
#include <utils/Socket.hpp>
#include <utils/LogFile.hpp>
#include <utils/KeyValuePair.hpp>
#include <string>

namespace utils
{
    /**
     * @brief HTTP request parser and response helper.
     * @details Parses HTTP requests from a socket, stores headers/body, and
     *          supports generating a basic file response. Not intended for full
     *          HTTP compliance; limits and behavior are determined by the
     *          underlying implementation and configuration.
     * @note Not safe for concurrent access without external synchronization.
     */
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
            ::std::string m_addr; // Remote host
            uint32_t      m_port; // Remote port
            bool          m_sset; // If true, then m_start was assigned a value
            bool          m_eset; // If true, then m_end was assigned a value
            bool          m_timeout;
            ::std::shared_ptr< KeyValuePair< ::std::string, ::std::string > > m_meta;
            ::std::string m_response;
            ::std::string m_lasterror;

        public:
            /**
             * @brief Construct a new empty request.
             */
            HttpRequest();
            /**
             * @brief Destroy the request and clear stored state.
             */
            ~HttpRequest();

            /**
             * @brief Reset all stored request state.
             */
            void Reset();

            /**
             * @brief Access the request URI.
             * @return Mutable reference to the URI string.
             */
            ::std::string &Uri();
            /**
             * @brief Access the request method.
             * @return Mutable reference to the method string.
             */
            ::std::string &Method();
            /**
             * @brief Access the HTTP version string.
             * @return Mutable reference to the version string.
             */
            ::std::string &Version();
            /**
             * @brief Access the metadata header list.
             * @return Shared pointer to key/value metadata storage.
             */
            ::std::shared_ptr< KeyValuePair< ::std::string, ::std::string > > &Meta();
            /**
             * @brief Retrieve the Host header value.
             * @return Host header value, or empty if not present.
             */
            ::std::string  Host();
            /**
             * @brief Access the response string buffer.
             * @return Mutable reference to the response.
             */
            ::std::string &Response();
            /**
             * @brief Access the remote address string.
             * @return Mutable reference to the remote address.
             */
            ::std::string &RemoteAddress();
            /**
             * @brief Access the remote port.
             * @return Mutable reference to the remote port value.
             */
            uint32_t      &RemotePort();
            /**
             * @brief Retrieve the last error message.
             * @return Last error string (may be empty).
             */
            ::std::string  LastError();

            /**
             * @brief Read and parse a request from a socket.
             * @param a_socket Socket to read from; must be non-null and connected.
             * @return True if a request was successfully parsed; false otherwise.
             * @note The request may be partial if a timeout occurs.
             */
            bool    Read( ::std::shared_ptr< Socket > &a_socket );
            /**
             * @brief Send a file response for the current request.
             * @param a_socket Socket to write to; must be non-null and connected.
             * @param a_fileName File path to serve.
             * @param a_type MIME type to return.
             * @param a_listDirs Whether to list directories when applicable.
             * @return HTTP status code or negative value on error.
             */
            int32_t Respond( ::std::shared_ptr< Socket > &a_socket, ::std::string &a_fileName, ::std::string &a_type, bool a_listDirs = false );

            /**
             * @brief Log request details to a logger.
             * @param a_logger Logger to write to; must be initialized.
             */
            void Log( LogFile &a_logger );
    };
}
