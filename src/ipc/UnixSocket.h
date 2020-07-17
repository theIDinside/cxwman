//
// Created by cx on 2020-07-13.
//
#pragma once
#include <fcntl.h>
#include <ipc/ipc.hpp>
#include <map>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <cxprotocol/src/library.h>

namespace cx::ipc
{
    namespace fs = std::filesystem;

    constexpr auto make_socket_non_blocking = [](auto fileDescriptor) {
        auto sock_flags = fcntl(fileDescriptor, F_GETFL, 0);
        if(sock_flags == -1) {
            cx::println("Failed to get flags for socket {}", fileDescriptor);
            return false;
        }
        assert_eq(SOCK_NONBLOCK, O_NONBLOCK, "Flags are not equal");
        if(fcntl(fileDescriptor, F_SETFL, sock_flags | SOCK_NONBLOCK) == -1) {
            cx::println("Failed to set socket to non-blocking");
            return false;
        }
        return true;
    };

    /// Holds file descriptors for sockets, internal buffer for reply / received messages
    struct IPCClient {
        IPCClient(int fd, sockaddr_un client_address, socklen_t addr_len);
        ~IPCClient();
        void process_read_buffer();
        int socket_fd;
        sockaddr_un address;
        socklen_t addr_len;
        std::size_t current_data;
        std::array<std::byte, 2048> read_buffer;
        std::vector<std::string> read_messages;
    };

    class UnixSocket : public IPCInterface
    {
      public:
        UnixSocket(fs::path socket_path, sockaddr_un addr, IPCFileDescriptors file_descriptors, std::size_t max_connections);
        /// !Close sockets & other potential resources!
        ~UnixSocket() override;
        /// This function returns a IPCReadResult which can (mutually exclusive) include one of the following:<br>
        /// 1. An IPC message was received<br>
        /// 2. An IPC error when trying to read from connected clients<br>
        /// 3. A nullopts, which is returned when the activity on UnixSocket is for example an accepted client (we have theoretically "read" from the
        /// socket, but not data, therefore nullopt)<br>
        /// Since this function polls, it also just calls epoll_wait with a maximum events of 1
        [[nodiscard]] IPCReadResult poll_queue() override;
        void handle_incoming_connection() override;
        [[nodiscard]] bool has_request() const override;
        void poll_event() override;
        static auto initialize(const fs::path& socket_path, std::size_t max_connections, int epoll_fd) -> std::unique_ptr<UnixSocket>;
        bool is_connection_request(int fd) override;
        void read_from_input(std::optional<int> file_descriptor) override;
        void drop_client(int fd) override ;
      private:
        /// C-interface data. the filesystem::path in base class IPCInterface is for our convenience
        sockaddr_un socket_address;
        /// Contains Epoll file descriptor & listening fd, which listens for incoming *connections* to accept for. Data is _not_ transferred via these
        std::size_t max_conns;
        std::map<int, std::unique_ptr<IPCClient>> connected_clients;
        auto inspect_for_message(std::array<std::byte, 2048> array, size_t data) -> std::optional<std::string>;
    };
} // namespace cx::ipc
