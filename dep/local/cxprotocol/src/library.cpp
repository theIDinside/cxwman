#include "library.h"
// cx-ipc

namespace cx::ipc
{
    using u64 = std::uint64_t;
    using uchar = unsigned char;

    template <>
    auto deserialize_payload_length<std::endian::big>(const unsigned char data[2]) -> uint16_t {
        std::uint16_t payload_size = 0;
        std::memcpy(&payload_size, data, 2);
        return payload_size;
    }

    template <>
    auto deserialize_payload_length<std::endian::little>(const unsigned char data[2]) -> uint16_t {
        char little_end_buf[2];
        little_end_buf[0] = data[1];
        little_end_buf[1] = data[0];
        std::uint16_t payload_size = 0;
        std::memcpy(&payload_size, little_end_buf, 2);
        return payload_size;
    }

    template <>
    auto serialize_payload_length<std::endian::big>(std::uint16_t payload_len) -> std::array<char, 2> {
        std::array<char, 2> serialized{};
        std::memcpy(serialized.data(), &payload_len, 2);
        return serialized;
    }

    template <>
    auto serialize_payload_length<std::endian::little>(std::uint16_t payload_len) -> std::array<char, 2> {
        std::array<char, 2> serialized{};
        std::memcpy(serialized.data(), &payload_len, 2);
        char hi = serialized[1];
        char lo = serialized[0];
        serialized[0] = hi;
        serialized[1] = lo;
        return serialized;
    }


    template <>
    auto header_value<std::endian::big>() -> unsigned long {
        unsigned long headerValue = 0;
        std::memcpy(&headerValue, HEADER_IDENTIFIER.data(), 8);
        return headerValue;
    }

    template <>
    auto header_value<std::endian::little>() -> unsigned long {
        char little_endianed[HEADER_IDENTIFIER.size()];
        std::copy(HEADER_IDENTIFIER.rbegin(), HEADER_IDENTIFIER.rend(), little_endianed);
        unsigned long headerValue = 0;
        std::memcpy(&headerValue, HEADER_IDENTIFIER.data(), 8);
        return headerValue;
    }

        
    template <>
    std::size_t message_payload_length<std::endian::big>(const Message& message) {
        unsigned char payload_field[2]{message.buffer[HEADER_IDENTIFIER.size()], message.buffer[HEADER_IDENTIFIER.size() + 1]};
        return deserialize_payload_length(payload_field);
    }

    template <>
    std::size_t message_payload_length<std::endian::little>(const Message& message) {
        unsigned char payload_field[2]{message.buffer[HEADER_IDENTIFIER.size()], message.buffer[HEADER_IDENTIFIER.size() + 1]};
        return deserialize_payload_length(payload_field);
    }

    bool payload_ok(const char* data, std::size_t size) {
        std::string_view view{data, size};
        auto pay_load_size = 0;
        if(view.substr(0, HEADER_IDENTIFIER.size()) != HEADER_IDENTIFIER) {
            return false;
        } else {
            view.remove_prefix(HEADER_IDENTIFIER.size());
            uchar pl[2]{(uchar)view.at(0), (uchar)view.at(1)};
            auto payload_size = deserialize_payload_length(pl);
            auto supposed_full_size = HEADER_SIZE + PACKAGE_END.size() + pay_load_size;
            view.remove_prefix(2);
            view.remove_suffix(PACKAGE_END.size());
            assert(view.size() == pay_load_size && "Payload size not computed correctly");
            assert(size == supposed_full_size && "Data packet and extracted payload size does not match");
            view.remove_prefix(2);
            try {
                if(payload_size + HEADER_SIZE > size) throw std::out_of_range{"Decoded payload size larger than provided data"};
                view.remove_prefix(payload_size);
                if(view.substr(0, PACKAGE_END.size()) == PACKAGE_END) {
                    return true;
                } else {
                    return false;
                }
            } catch(std::exception& e) {
                return false;
            }
        }
    }

    bool payload_ok(const Message& msg) {
        unsigned char header_field[HEADER_IDENTIFIER.size()];
        unsigned char payload_size_field[2];

    }

    std::optional<std::string> extract_payload(const std::array<std::byte, 2048>& buf, std::size_t data_in_buffer) {
        using uchar = unsigned char;
        std::string_view buf_view{(const char*)buf.data(), data_in_buffer};
        auto header_field = buf_view.substr(0, HEADER_IDENTIFIER.size());
        assert(header_field == HEADER_IDENTIFIER);
        // assert_eq(header_field, HEADER_IDENTIFIER, "Header field must be exactly: 'cxwm-ipc'");
        buf_view.remove_prefix(HEADER_IDENTIFIER.size());
        uchar payload_field[2]{(uchar)buf_view.at(0), (uchar)buf_view.at(1)};
        buf_view.remove_prefix(2);
        auto payload_len = deserialize_payload_length(payload_field);
        assert(buf_view.size() > payload_len);
        assert(buf_view.substr(payload_len, PACKAGE_END.size()) == PACKAGE_END);
        // assert_gt(buf_view.size(), payload_len, "Remaining buffer data must be larger than payload, since it includes the 'cxwm-end' footer");
        // assert_eq(buf_view.substr(payload_len, PACKAGE_END.size()), PACKAGE_END, "Footer/Package end must be included for a well-defined message");
        std::string data{buf_view.substr(0, payload_len)};
        assert(data.size() == payload_len);
        // assert_eq(data.size(), payload_len, "De-coded payload size and data size does not match");
        return data;
    }

    std::optional<std::string> extract_payload(const Message& msg) {
        using uchar = unsigned char;
        std::string_view buf_view{(const char*)msg.buffer.data(), msg.message_size};
        auto header_field = buf_view.substr(0, HEADER_IDENTIFIER.size());
        buf_view.remove_prefix(HEADER_IDENTIFIER.size());
        uchar payload_field[2]{(uchar)buf_view.at(0), (uchar)buf_view.at(1)};
        buf_view.remove_prefix(2);
        auto payload_len = deserialize_payload_length(payload_field);
        // assert_gt(buf_view.size(), payload_len, "Remaining buffer data must be larger than payload, since it includes the 'cxwm-end' footer");
        // assert_eq(buf_view.substr(payload_len, PACKAGE_END.size()), PACKAGE_END, "Footer/Package end must be included for a well-defined message");
        std::string data{buf_view.substr(0, payload_len)};
        // // assert_eq(data.size(), payload_len, "De-coded payload size and data size does not match");
        return data;
    }

    std::optional<std::vector<std::string>> extract_payloads(const std::array<std::byte, 2048>& buf, std::size_t data_in_buffer)
    {
        std::vector<std::string> payloads{};
        std::string_view strViewBuf{(const char*)buf.data(), data_in_buffer};
        std::vector<std::pair<std::size_t, std::size_t>> messages_ranges{};
        auto sub_range_begin = 0;
        auto sub_range_end = PACKAGE_END.size();
        auto range_start = 0;
        while(sub_range_end <= strViewBuf.size()) {
            if(strViewBuf.substr(sub_range_begin, sub_range_end) == PACKAGE_END) {
                messages_ranges.emplace_back(range_start, sub_range_end);
                range_start = sub_range_end+1;
            }
            sub_range_begin++; sub_range_end++;
        }

        std::array<std::byte, 2048> tmp_buf{};
        for(const auto& [begin, end] : messages_ranges) {
            auto length = end - begin;
            std::copy(buf.begin() + begin, buf.begin() + end, tmp_buf.begin());
            auto possible_payload = extract_payload(tmp_buf, length);
            if(possible_payload) {
                payloads.push_back(possible_payload.value());
            }
        }

        if(payloads.empty()) return {};
        else return payloads;
    }

    Message from_payload(const char* payload, std::uint16_t payload_len) {
        // // assert_lt(payload_len, MAX_PAYLOAD_LENGTH, "Payload length too large");
        auto ser_size = serialize_payload_length(payload_len);
        std::array<unsigned char, 2048> buffer{};
        auto payload_len_start = std::copy(HEADER_IDENTIFIER.begin(), HEADER_IDENTIFIER.end(), std::begin(buffer));
        auto payload_begin = std::copy(ser_size.begin(), ser_size.end(), payload_len_start);
        auto footer_begin = std::copy(payload, payload + payload_len, payload_begin);
        std::copy(PACKAGE_END.begin(), PACKAGE_END.end(), footer_begin);
        return Message{buffer, FIXED_FIELDS_SIZE + payload_len};
    }

    
    Message from_payload(std::string_view payload) {
        auto payload_len = payload.size();
        // // assert_lt(payload_len, MAX_PAYLOAD_LENGTH, "Payload length too large");
        auto ser_size = serialize_payload_length<std::endian::native>(payload_len);
        std::array<unsigned char, 2048> buffer{};
        std::fill(buffer.begin(), buffer.end(), '\0');
        auto payload_len_start = std::copy(HEADER_IDENTIFIER.begin(), HEADER_IDENTIFIER.end(), buffer.begin());
        auto payload_begin = std::copy(ser_size.begin(), ser_size.end(), payload_len_start);
        auto footer_begin = std::copy(payload.begin(), payload.end(), payload_begin);
        std::copy(PACKAGE_END.begin(), PACKAGE_END.end(), footer_begin);
        return Message{buffer, FIXED_FIELDS_SIZE + payload_len};
    }


} // namespace cx::ipc


