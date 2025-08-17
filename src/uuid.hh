//
//  uuid.hh
//  rpg
//
//  Created by George Watson on 17/08/2025.
//

#pragma once

#include <random>
#include <string>
#include <iostream>

namespace uuid::v4 {
    // Encaasulate the genaeration of a Version 4 UUID object
    // A Version 4 UUID is a universally unique identifier that is generated using random numbers.
    class UUID {
    public:
        // Factory method for creating UUID object.
        static UUID New() {
            UUID uuid;
            std::random_device rd;
            std::mt19937 engine{rd()};
            std::uniform_int_distribution<int> dist{0, 256}; //Limits of the interval

            for (int index = 0; index < 16; ++index)
                uuid._data[index] = (uint8_t)dist(engine);

            uuid._data[6] = ((uuid._data[6] & 0x0f) | 0x40); // Version 4
            uuid._data[8] = ((uuid._data[8] & 0x3f) | 0x80); // Variant is 10

            return uuid;
        }

        // Returns UUID as formatted string
        std::string String() {
            // Formats to "0065e7d7-418c-4da4-b4d6-b54b6cf7466a"
            char buffer[256] = {0};
            std::snprintf(buffer, 255,
                          "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
                          _data[0], _data[1], _data[2], _data[3],
                          _data[4], _data[5],
                          _data[6], _data[7],
                          _data[8], _data[9],
                          _data[10], _data[11], _data[12], _data[13], _data[14], _data[15]);

            std::string uuid = buffer;

            return uuid;
        }

        const uint8_t *data() const {
            return _data;
        }

    private:
        UUID() {}

        uint8_t _data[16] = {0};
    };
};

using uuid_v4 = uuid::v4::UUID;
typedef uuid_v4 uuid_t;

template <typename Derived>
class UUID {
private:
    uuid_t instance_uuid;

public:
    UUID() : instance_uuid(uuid_v4::New()) {}
    UUID(const UUID &other) : instance_uuid(uuid_v4::New()) {}
    UUID(UUID &&other) noexcept : instance_uuid(std::move(other.instance_uuid)) {}

    // Assignment operators
    UUID &operator=(const UUID &other) {
        if (this != &other)
            instance_uuid = uuid_v4::New();
        return *this;
    }

    UUID &operator=(UUID &&other) noexcept {
        if (this != &other)
            instance_uuid = std::move(other.instance_uuid);
        return *this;
    }

    bool operator==(const UUID &other) const {
        return std::memcmp(instance_uuid.data(), other.instance_uuid.data(), 16 * sizeof(uint8_t)) == 0;
    }

    const uuid_t &uuid() const {
        return instance_uuid;
    }

    std::string uuid_as_string() {
        return instance_uuid.String();
    }
};
