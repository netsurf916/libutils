/**
    NetInfo.hpp : NetInfo definition
    Copyright 2015-2021 Daniel Wilson
*/

#pragma once

#ifndef _NETINFO_HPP_
#define _NETINFO_HPP_

#include <utils/BitMask.hpp>
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

    class NetInfo
    {
        public:
            NetInfo();
            NetInfo( struct ifaddrs *a_info );
            ~NetInfo();

            ::std::string &Name();
            ::std::string &Address();
            ::std::string &NetMask();
            ::std::string &Broadcast();

            bool IsIPv4();
            bool IsIPv6();
            bool IsLoopback();
            bool IsUp();

            ::std::shared_ptr< NetInfo > &Prev();
            ::std::shared_ptr< NetInfo > &Next();

            operator bool();

            static ::std::shared_ptr< NetInfo > GetInterfaces();

        private:
            BitMask m_flags;
            ::std::string  m_name;
            ::std::string  m_address;
            ::std::string  m_netmask;
            ::std::string  m_broadcast;
            uint16_t       m_family;
            ::std::shared_ptr< NetInfo > m_prev;
            ::std::shared_ptr< NetInfo > m_next;

            const char *GetIpStr( const struct sockaddr *a_sa, char *a_str, size_t a_len );
    };
}

#endif // _NETINFO_HPP_
