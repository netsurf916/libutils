/**
    NetInfo.hpp : NetInfo definition
    Description: Network information helper utilities.
    Copyright 2015-2026 Daniel Wilson
    SPDX-License-Identifier: MIT
*/

#pragma once

#ifndef _NETINFO_HPP_
#define _NETINFO_HPP_

#include <utils/BitMask.hpp>
#include <sys/types.h>
#include <ifaddrs.h>
#include <string>
#include <memory>

namespace utils
{
    namespace NetworkFlags
    {
        /**
         * @brief Flags representing interface properties.
         */
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

    /**
     * @brief Network interface information node.
     * @details Represents a single network interface and links to others in a
     *          list. Populate via GetInterfaces().
     * @note This type does not perform synchronization; use externally if needed.
     */
    class NetInfo
    {
        public:
            /**
             * @brief Construct an empty NetInfo node.
             */
            NetInfo();
            /**
             * @brief Construct from a system ifaddrs structure.
             * @param a_info Pointer to ifaddrs entry; must be non-null.
             */
            NetInfo( struct ifaddrs *a_info );
            /**
             * @brief Destroy the NetInfo node.
             */
            ~NetInfo();

            /**
             * @brief Access the interface name.
             * @return Mutable reference to the name.
             */
            ::std::string &Name();
            /**
             * @brief Access the interface address string.
             * @return Mutable reference to the address.
             */
            ::std::string &Address();
            /**
             * @brief Access the interface netmask string.
             * @return Mutable reference to the netmask.
             */
            ::std::string &NetMask();
            /**
             * @brief Access the broadcast address string.
             * @return Mutable reference to the broadcast address.
             */
            ::std::string &Broadcast();

            /**
             * @brief Check if the interface has an IPv4 address.
             * @return True if IPv4 is present.
             */
            bool IsIPv4();
            /**
             * @brief Check if the interface has an IPv6 address.
             * @return True if IPv6 is present.
             */
            bool IsIPv6();
            /**
             * @brief Check if the interface is a loopback device.
             * @return True if loopback.
             */
            bool IsLoopback();
            /**
             * @brief Check if the interface is up.
             * @return True if marked up.
             */
            bool IsUp();

            /**
             * @brief Access the previous node in the list.
             * @return Shared pointer to the previous node.
             */
            ::std::shared_ptr< NetInfo > &Prev();
            /**
             * @brief Access the next node in the list.
             * @return Shared pointer to the next node.
             */
            ::std::shared_ptr< NetInfo > &Next();

            /**
             * @brief Boolean conversion indicating valid interface info.
             * @return True if the node is populated; false otherwise.
             */
            operator bool();

            /**
             * @brief Enumerate system interfaces into a linked list.
             * @return Head node of the interface list, or nullptr on failure.
             */
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
