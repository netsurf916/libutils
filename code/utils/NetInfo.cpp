/**
    NetInf.cpp : NetInfo class implementation
    Copyright 2015-2026 Daniel Wilson
    SPDX-License-Identifier: MIT
*/

#include <utils/NetInfo.hpp>
#include <arpa/inet.h>
#include <net/if.h>

namespace utils
{
    const char *NetInfo::GetIpStr( const struct sockaddr *a_sa, char *a_str, size_t a_len )
    {
        switch( a_sa->sa_family )
        {
            case AF_INET:
                inet_ntop(
                         AF_INET,
                         &( ( ( struct sockaddr_in * ) a_sa )->sin_addr ),
                         a_str,
                         a_len
                         );
                break;

            case AF_INET6:
                inet_ntop(
                         AF_INET6,
                         &( ( ( struct sockaddr_in6 * ) a_sa )->sin6_addr ),
                         a_str,
                         a_len
                         );
                break;

            default:
                while( a_len > 0 )
                {
                    a_str[ --a_len ] = 0;
                }
                break;
        }

        return a_str;
    }

    NetInfo::NetInfo()
    {
    }

    NetInfo::NetInfo( struct ifaddrs *a_info )
    {
        char buffer[ 64 ];
        if( nullptr != a_info->ifa_addr )
        {
            m_name = a_info->ifa_name;
            if( nullptr != a_info->ifa_addr )
            {
                m_address =
                    GetIpStr
                    (
                    a_info->ifa_addr,
                    buffer,
                    sizeof( buffer )
                    );
            }
            if( nullptr != a_info->ifa_netmask )
            {
                m_netmask =
                    GetIpStr
                    (
                    a_info->ifa_netmask,
                    buffer,
                    sizeof( buffer )
                    );
            }
            if( nullptr != a_info->ifa_broadaddr )
            {
                m_broadcast =
                    GetIpStr
                    (
                    a_info->ifa_broadaddr,
                    buffer,
                    sizeof( buffer )
                    );
            }
            m_family = a_info->ifa_addr->sa_family;
            for( int32_t i = 31; i >= 0; --i )
            {
                if( a_info->ifa_flags & ( 1 << i ) )
                {
                    m_flags.SetBit( i, true );
                }
            }
        }
    }

    NetInfo::~NetInfo()
    {
    }

    ::std::string &NetInfo::Name()
    {
        return m_name;
    }

    ::std::string &NetInfo::Address()
    {
        return m_address;
    }

    ::std::string &NetInfo::NetMask()
    {
        return m_netmask;
    }

    ::std::string &NetInfo::Broadcast()
    {
        return m_broadcast;
    }

    bool NetInfo::IsIPv4()
    {
        return ( AF_INET == m_family );
    }

    bool NetInfo::IsIPv6()
    {
        return ( AF_INET6 == m_family );
    }

    bool NetInfo::IsLoopback()
    {
        bool up = false;
        return ( m_flags.GetBit( IFF_LOOPBACK, up ) && up );
    }

    bool NetInfo::IsUp()
    {
        bool up = false;
        return ( m_flags.GetBit( IFF_UP, up ) && up );
    }

    ::std::shared_ptr< NetInfo > &NetInfo::Prev()
    {
        return m_prev;
    }

    ::std::shared_ptr< NetInfo > &NetInfo::Next()
    {
        return m_next;
    }

    NetInfo::operator bool()
    {
        return( ( IsIPv4() || IsIPv6() )
                && IsUp()
                && ( Name().length() > 0 ) && ( Address().length() > 0 )
                && ( NetMask().length() > 0 )
                && ( Broadcast().length() > 0 ) );
    }

    ::std::shared_ptr< NetInfo > NetInfo::GetInterfaces()
    {
        ::std::shared_ptr< NetInfo > netInfo;
        struct ifaddrs *ifInfoStart = nullptr;
        struct ifaddrs *ifInfo      = nullptr;

        getifaddrs( &ifInfoStart );
        ifInfo = ifInfoStart;

        while( ifInfo )
        {
            if( netInfo )
            {
                netInfo->Prev() = ::std::make_shared< NetInfo >( ifInfo );
                if( netInfo->Prev() )
                {
                    netInfo->Prev()->Next() = netInfo;
                    netInfo = netInfo->Prev();
                }
            }
            else
            {
                netInfo = ::std::make_shared< NetInfo >( ifInfo );
            }
            ifInfo = ifInfo->ifa_next;
        }

        if( ifInfoStart )
        {
            freeifaddrs( ifInfoStart );
            ifInfoStart = nullptr;
        }

        return netInfo;
    }
}
