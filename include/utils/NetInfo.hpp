/**
    NetInfo.hpp : NetInfo definition
    Copyright 2015-2019 Daniel Wilson
*/

#pragma once

#ifndef _NETINFO_HPP_
#define _NETINFO_HPP_

#include <utils/Serializable.hpp>
#include <utils/BitMask.hpp>
#include <utils/String.hpp>
#include <sys/types.h>
#include <ifaddrs.h>
#include <memory>

namespace utils
{
    namespace NetworkFlags
    {
        enum Types : uint8_t
        {
            Up,
            Loopback,
            IPv4,
            Ipv6,
            Remote,
        };
    }
    typedef NetworkFlags::Types NetworkFlag;

    class NetInfo : public Serializable
    {
        public:
            NetInfo();
            NetInfo( struct ifaddrs *a_info );
            ~NetInfo();

            String &Name();
            String &Address();
            String &NetMask();
            String &Broadcast();

            bool IsIPv4();
            bool IsIPv6();
            bool IsLoopback();
            bool IsUp();

            ::std::shared_ptr< NetInfo > &Prev();
            ::std::shared_ptr< NetInfo > &Next();

            operator bool();

            static ::std::shared_ptr< NetInfo > GetInterfaces();

            // Serializable functions
            uint8_t Type() noexcept final;
            bool    Serialize  ( Writable &a_out ) noexcept final;
            bool    Deserialize( Readable &a_in  ) noexcept final;

        private:
            BitMask m_flags;
            String  m_name;
            String  m_address;
            String  m_netmask;
            String  m_broadcast;
            Primitive< uint16_t > m_family;
            ::std::shared_ptr< NetInfo > m_prev;
            ::std::shared_ptr< NetInfo > m_next;

            const char *GetIpStr( const struct sockaddr *a_sa, char *a_str, size_t a_len );
    };
}

#endif // _NETINFO_HPP_
