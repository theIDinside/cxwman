//
// Created by cx on 2020-07-13.
//

#include "UnixSocket.h"

#include <unistd.h>
#include <utility>
namespace cx::ipc
{
    void UnixSocket::handle_incoming_connection()
    {
        sockaddr_un client_addr{};
        socklen_t client_addr_len = sizeof(client_addr);
        epoll_event event{};
        cx::println("EPOLLET {} - EPOLLIN {}", EPOLLET, EPOLLIN);
        event.events = EPOLLIN | EPOLLRDHUP;
        if(auto ipc_client_fd = accept(server_fds.listening, (sockaddr*)&client_addr, &client_addr_len); ipc_client_fd == -1) {
            cx::println("failed to accept client.");
        } else {
            if(!make_socket_non_blocking(ipc_client_fd)) {
                cx::println("Failed to make socket for incoming client non-blocking. File descriptor: {}", ipc_client_fd);
                close(ipc_client_fd);
            } else {
                event.data.fd = ipc_client_fd;
                if(epoll_ctl(server_fds.epoll, EPOLL_CTL_ADD, ipc_client_fd, &event) != 0) {
                    cx::println("Failed to add client socket to watch list for EPOLL");
                    close(ipc_client_fd);
                } else {
                    connected_clients.emplace(ipc_client_fd, std::make_unique<IPCClient>(ipc_client_fd, client_addr, client_addr_len));
                    cx::println("Successfully connected to client. If it has a path it is: {}", client_addr.sun_path);
                }
            }
        }
    }

    IPCReadResult UnixSocket::poll_queue()
    {
        epoll_event event_list[1];
        auto io_events = epoll_wait(server_fds.epoll, event_list, 1, 1);
        for(auto i = 0; i < io_events; i++) {
            if(event_list[i].data.fd == server_fds.listening) { // means we have received a connect request
                handle_incoming_connection();
                // TODO(idea): Should we try reading from the socket (non-blocking), since we (most likely) have waiting data coming in close to
                //  immediately?
                return IPCReadResult(std::nullopt);
            } else {

            }
        }

        return IPCReadResult(std::nullopt);
    }
    bool UnixSocket::has_request() const { return IPCInterface::has_request(); }
    auto UnixSocket::initialize(const fs::path& socket_path, std::size_t max_connections, int epoll_fd) -> std::unique_ptr<UnixSocket>
    {
        std::string_view path_view{socket_path.c_str()};
        auto path_len = path_view.size();
        sockaddr_un unix_socket{};
        auto listening_fd = socket(AF_LOCAL, SOCK_STREAM | SOCK_NONBLOCK, 0);
        if(listening_fd < 0)
            std::perror("failed to create accepting socket");
        unix_socket.sun_family = AF_UNIX;
        cx::println("Path view: {}", path_view);
        std::copy(std::begin(path_view), std::end(path_view), unix_socket.sun_path);
        if(!make_socket_non_blocking(listening_fd)) {
            cx::println("Failed to initialize IPC socket");
            exit(EXIT_FAILURE);
        }
        if(bind(listening_fd, (sockaddr*)&unix_socket, path_len + sizeof(unix_socket.sun_family)) < 0) {
            cx::println("failed to bind accepting socket to address {}", unix_socket.sun_path);
            std::abort();
        } else {
            cx::println("Successfully bound socket to address: {}", unix_socket.sun_path);
        }
        listen(listening_fd, max_connections);

        epoll_event event{};
        event.events = EPOLLIN;
        event.data.fd = listening_fd;
        if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listening_fd, &event) != 0) {
            cx::println("Failed to install listening socket to EPOLL service");
            std::abort();
        }
        return std::make_unique<UnixSocket>(socket_path, unix_socket, IPCFileDescriptors{epoll_fd, listening_fd}, max_connections);
        std::abort();
    }

    UnixSocket::UnixSocket(fs::path socket_path, sockaddr_un addr, IPCFileDescriptors file_descriptors, std::size_t max_connections)
        : IPCInterface(std::move(socket_path), file_descriptors), socket_address(addr), max_conns(max_connections), connected_clients{}
    {
    }
    void UnixSocket::poll_event() {}
    bool UnixSocket::is_connection_request(int fd) { return IPCInterface::is_connection_request(fd); }
    void UnixSocket::read_from_input(std::optional<int> file_descriptor) {
        auto fd = file_descriptor.value();
        if(connected_clients.contains(fd)) {
            std::byte max_buf[2048];
            auto& client = connected_clients.at(fd);
            auto bytes_read = read(fd, max_buf, 2048);
            if(bytes_read > 0) {
                std::array<unsigned char, 2048> buf{};
                std::memcpy(buf.data(), max_buf, bytes_read);
                cx::println("Read {} bytes", bytes_read);
                std::copy(max_buf, max_buf + bytes_read, std::begin(client->read_buffer) + client->current_data);
                client->current_data += bytes_read;
                client->process_read_buffer();
                if(!client->read_messages.empty()) {
                    cx::println("Processed {} messages. Outputting...:", client->read_messages.size());
                    for(const auto& msg : client->read_messages) cx::println("{}", msg);
                    client->read_messages.clear();
                }
            } else {
                auto socket_is_ok =
                    [](int fd) {
                      int ret;
                      int code;
                      socklen_t len = sizeof(int);
                      ret = getsockopt(fd, SOL_SOCKET, SO_ERROR, &code, &len);
                      return ret == 0 && code == 0;
                    };
                if(!socket_is_ok(fd)) {
                    cx::println("DROPPIGN CLIENT!!!!!!!");
                    drop_client(fd);
                }
                // TODO: We must handle when the client closes the connection by inspecting our file descriptor for errors, otherwise
                //  when the client closes it's side, we will infinitely and continously get events for reads - therefore we must destroy it's
                //  association with the epoll file-descriptor most likely by some call to epoll_ctl(epfd, CTL_DEL, fd....) or something to that degree
            }
        } else {
            cx::println("We have no registered client by that file descriptor!");
        }
    }
    auto UnixSocket::inspect_for_message(std::array<std::byte, 2048> array, size_t data) -> std::optional<std::string> {
        return extract_payload(array, data);
    }
    UnixSocket::~UnixSocket() {
        close(this->server_fds.listening);
        unlink(path.c_str());
    }
    void UnixSocket::drop_client(int fd) {
        if(connected_clients.contains(fd)) {
            connected_clients.erase(fd);
        }
    }

    IPCClient::IPCClient(int fd, sockaddr_un client_address, socklen_t addr_len)
        : socket_fd(fd), address(client_address), addr_len(addr_len), current_data(0), read_buffer(), read_messages{}
    {
    }
    void IPCClient::process_read_buffer() {
        std::vector<std::string> payloads{};
        std::string_view strViewBuf{(const char*)read_buffer.data(), this->current_data};
        std::vector<std::pair<std::size_t, std::size_t>> messages_ranges{};
        auto sub_range_start_pos = 0;
        auto sub_range_len = PACKAGE_END.size();
        auto range_start = 0;
        while((sub_range_start_pos+sub_range_len) <= strViewBuf.size()) {
            if(strViewBuf.substr(sub_range_start_pos, sub_range_len) == PACKAGE_END) {
                messages_ranges.emplace_back(range_start, sub_range_start_pos+sub_range_len);
                sub_range_start_pos += sub_range_len;
                range_start = sub_range_start_pos;
            } else {
                sub_range_start_pos++;
            }
        }
        if(messages_ranges[messages_ranges.size()-1].second == current_data) {
            current_data = 0;
        }

        std::array<std::byte, 2048> tmp_buf{};
        for(const auto& [begin, end] : messages_ranges) {
            auto length = end - begin;
            // DBGLOG("Message range [{}..{}]. Length: {}", begin, end, length);
            std::copy(read_buffer.begin() + begin, read_buffer.begin() + end, tmp_buf.begin());
            // DBGLOG("tmpOutput: {}", std::string{std::string_view{(const char*)tmp_buf.data(), length}});
            auto possible_payload = extract_payload(tmp_buf, length);
            if(possible_payload) {
                payloads.push_back(possible_payload.value());
            }
        }
        if(!payloads.empty()) {
            std::copy(payloads.begin(), payloads.end(), std::back_inserter(read_messages));
            cx::println("Processed {} messages", payloads.size());
        }
    }
    IPCClient::~IPCClient() {
        close(socket_fd);
    }
} // namespace cx::ipc
