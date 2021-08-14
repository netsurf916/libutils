/**
    Marker.hpp : Marker definition
    Copyright 2015-2019 Daniel Wilson
*/

#pragma once

#ifndef _MARKER_HPP_
#define _MARKER_HPP_

#include <utils/Serializable.hpp>

namespace utils
{
    class Marker : public Serializable
    {
        public:
            Marker();
            ~Marker();

            // Serializable functions
            uint8_t Type() noexcept final;
            bool    Serialize  ( Writable &a_out ) noexcept final;
            bool    Deserialize( Readable &a_in  ) noexcept final;
    };
}

#endif // _MARKER_HPP_
