/**
    Marker.cpp : Marker class implementation
    Copyright 2015-2019 Daniel Wilson
*/

#include <utils/Marker.hpp>

namespace utils
{
    Marker::Marker()
    {
    }

    Marker::~Marker()
    {
    }

    uint8_t Marker::Type() noexcept
    {
        return static_cast< uint8_t >( SerializableType::Marker );
    }

    bool Marker::Serialize( Writable &a_out ) noexcept
    {
        return SerializeType( a_out );
    }

    bool Marker::Deserialize( Readable &a_in  ) noexcept
    {
        return DeserializeType( a_in );
    }
}
