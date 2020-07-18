#pragma once
#include <cstring>
#include <array>
#include <bit>
#include <string_view>
#include <algorithm>
#include <iostream>
#include <cassert>
#include <optional>

using namespace std::literals;

namespace cx::ipc {

    enum IPCCommands : unsigned char {
        Window = 1,
        Workspace = 2,
        BindKey = 3
    };

    constexpr auto MAX_MESSAGE_SIZE = 2048;
    constexpr auto HEADER_IDENTIFIER = "cxwm-ipc"sv;
    constexpr auto PACKAGE_END = "cxwm-end"sv;
    constexpr auto PAYLOAD_LEN_FIELD_SIZE = 2;
    constexpr auto HEADER_SIZE = 8 + 2;
    constexpr auto FIXED_FIELDS_SIZE = HEADER_SIZE + PACKAGE_END.size();
    constexpr auto MAX_PAYLOAD_LENGTH = MAX_MESSAGE_SIZE - (HEADER_SIZE + PACKAGE_END.size() + PAYLOAD_LEN_FIELD_SIZE);

    template<typename A, typename B>
    inline void assert_lt(A a, B b, const char* message) {
        std::cout << "assert(" << a << "<" << b << ") " << ((a >= b) ? message : "") << "\n";
        assert(a < b);
    }

    template<typename A, typename B>
    inline void assert_gt(A a, B b, const char* message) {
        std::cout << "assert(" << a << ">" << b << ") " << ((a <= b) ? message : "") << "\n";
        assert(a > b);
    }

    template<typename A, typename B>
    inline void assert_eq(A a, B b, const char* message) {
        std::cout << "assert(" << a << "==" << b << ") " << ((a != b) ? message : "") << "\n";
        assert(a == b);
    }

    /// Template overload resolution will make sure (hopefully) that the correct value is computed
    /// These are just function declarations - they will declare for whatever endianness host computer has. Definitions are in the .cpp file
    template <std::endian Endian = std::endian::native>
    auto header_value() -> unsigned long;
    template <std::endian Endian = std::endian::native>
    auto deserialize_payload_length(const unsigned char data[2]) -> uint16_t;
    template <std::endian Endian = std::endian::native>
    auto serialize_payload_length(std::uint16_t payload_len) -> std::array<char, 2>;

    struct Message {
        std::array<unsigned char, 2048> buffer;
        std::size_t message_size;
    };

    template <std::endian Endian = std::endian::native>
    std::size_t message_payload_length(const Message& message);

    Message from_payload(const char* payload, std::uint16_t payload_len);
    Message from_payload(std::string_view payload);

    std::optional<std::string> extract_payload(const std::array<std::byte, 2048>& buf, std::size_t data_in_buffer);
    std::optional<std::string> extract_payload(const Message& msg);
    std::optional<std::vector<std::string>> extract_payloads(const std::array<std::byte, 2048>& buf, std::size_t data_in_buffer);

    [[no_discard]] bool payload_ok(const char* data, std::size_t size);
    [[no_discard]] bool payload_ok(const Message& msg);
}