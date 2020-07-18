/* a client in the unix domain */
#include <algorithm>
#include <bit>
#include <bitset>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <library.h>
#include <string_view>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

using namespace std::literals;

constexpr auto MAGIC_NUMBER = 0b0000111111110000;

void error(const char*);

auto make_path(int argc, char** argv) -> std::string_view
{
    if(argc > 1)
        return argv[1];
    else
        return "foobar";
}

auto BITSET64 = [](auto header) {
  auto HEADER_BITSET = std::bitset<64>(header);
  return HEADER_BITSET;
};

constexpr auto CheckEndian = []() {
  if constexpr(std::endian::native == std::endian::big) {
      std::cout << "Endianness on this system is big-endian\n";
  } else if constexpr(std::endian::native == std::endian::little) {
      std::cout << "Endianness on this system is little-endian\n";
  } else {
      std::cout << "Can't tell endianness on this system!\n";
  }
};

int main(int argc, char* argv[])
{
    CheckEndian();
    const char HEADER[]{"cxwm-ipc"};
    std::string_view v{HEADER};
    std::string reversed_str{};
    std::copy(v.rbegin(), v.rend(), std::back_inserter(reversed_str));
    unsigned long reversed = 0;
    std::memcpy(&reversed, reversed_str.c_str(), 8);
    auto rev_bits = BITSET64(reversed);

    unsigned long hValue = 0;
    auto shift_by = 64;
    std::cout << "0b";
    for(auto i = 0; i < 8; i++) {
        std::cout << std::bitset<8>(HEADER[i]) << "_";
        shift_by -= 8;
        uint8_t v = (uint8_t)HEADER[i];
        uint64_t r = (uint64_t)v << shift_by;
        hValue |= r;
    }
    std::cout << std::endl;

    unsigned long header_value = 0;
    try {
        std::memcpy(&header_value, &HEADER, sizeof(unsigned long));
        std::cout << "Scanned bitset:  0b" << BITSET64(hValue) << "\t Value: " << hValue << std::endl;
        std::cout << "Reversed bitset: 0b" << rev_bits << "\t Value: " << reversed << std::endl;
        std::cout << "memcpy bitset:   0b" << BITSET64(header_value) << "\t Value: " << header_value << std::endl;
    } catch(std::exception& e) {
        std::cout << "failed to mem-copy value: " << e.what() << std::endl;
    }

    constexpr auto b = [](auto& container) { return std::begin(container); };
    constexpr auto e = [](auto& container) { return std::end(container); };
    std::cout << "Size of int: " << sizeof(int) << std::endl;
    std::cout << "Size of size_t: " << sizeof(std::size_t) << std::endl;
    std::cout << "Size of unsigned int:" << sizeof(unsigned int) << std::endl;
    std::cout << "Size of unsigned long: " << sizeof(unsigned long) << std::endl;
    std::cout << "Size of unsigned long long: " << sizeof(unsigned long long) << std::endl;
    const auto socket_path = make_path(argc, argv);
    auto path_length = socket_path.length();
    int sockfd, servlen, n;
    sockaddr_un serv_addr;
    char buffer[82];

    serv_addr.sun_family = AF_LOCAL;
    std::copy(b(socket_path), e(socket_path), serv_addr.sun_path);
    servlen = path_length + sizeof(serv_addr.sun_family);
    if((sockfd = socket(AF_LOCAL, SOCK_STREAM, 0)) < 0)
        error("Creating socket");
    if(connect(sockfd, (struct sockaddr*)&serv_addr, servlen) < 0) {
        std::cout << "Found no address with path: " << serv_addr.sun_path << std::endl;
        error("Connecting");
    }

    std::array<std::string_view, 5> payloads{
        "move window 123 to 1"sv, "move window 123 to 22"sv, "move window 123 to 333"sv,
        "move window 123 to 4444"sv, "move window 123 to 55555"sv,
    };

    constexpr auto message_header = "cxwm-ipc"sv;
    auto running = true;
    auto msg_index = 0;
    while(running) {
        std::cout << "Hit enter to send one of 5 messages in a loop. Type 'q' without the ticks, to quit" << std::endl;
        auto message = cx::ipc::from_payload(payloads[msg_index]);
        char c;
        std::cin >> c;
        if(c == 'q') {
            std::cout << "QUITTING" << std::endl;
            running = false;
        } else {
            std::cout << "Trying to send message: [" << std::string_view{(const char*)message.buffer.data(), message.message_size} << "]\t\t...";
            n = write(sockfd, message.buffer.data(), message.message_size);
            std::cout << "Sent " << n << "bytes. Successful = " << std::boolalpha << (n == message.message_size) << std::endl;
            msg_index = (msg_index + 1) % 5;
        }
    }


    close(sockfd);
    return 0;
}

void error(const char* msg)
{
    perror(msg);
    exit(0);
}
