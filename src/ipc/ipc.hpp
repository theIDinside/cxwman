//
// Created by cx on 2020-07-10.
//

#pragma once
#include <coreutils/core.hpp>
#include <filesystem>
#include <optional>
#include <queue>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <utility>
#include <variant>

namespace cx::ipc
{
    namespace fs = std::filesystem;
    /// Tells the linux sys call msgrcv to just pick out the oldest message, and not pick out by type
    constexpr auto READ_FIFO = 0;

    template<typename... Args>
    struct IPCResultVisitor : public Args... {
        using Args::operator()...;
    };

    template<typename... Args>
    IPCResultVisitor(Args&&...) -> IPCResultVisitor<Args...>;
    enum class CommandTypes : long { Window = 1, Workspace = 2, BindKey = 3, N = BindKey };
    /// Internal representation. This is is the struct we let Linux write into from the message queue

    struct IPCFileDescriptors {
        IPCFileDescriptors(int epollFD, int listeningFD) : epoll(epollFD), listening(listeningFD) {}
        int epoll;
        int listening;
    };

    struct IPCBuffer {
        /// 8 bytes
        long type;
        /// 2040 + 8 bytes lines up on a 8 byte boundary.
        char buffer[2040];
    };

    struct IPCMessage {
        /// Commands that we understand
        CommandTypes type;
        /// Client name - used as path for message queues to where we write responses to, or simply as an client identifier when it comes to sockets
        /// (if we want to, it's not necessary there as we have the file descriptor)
        std::string client_name;
        /// payload containing command arguments
        std::string payload;
        IPCMessage(CommandTypes type, std::string_view client_identifier, std::string_view payload) noexcept;
    };

    struct IPCError {
        int err_code;
        std::string_view err_message;
    };

    struct IPCReadResult {
        std::variant<IPCMessage, IPCError, std::nullopt_t> result;
        explicit IPCReadResult(std::nullopt_t noop) noexcept : result(noop) {}
        explicit IPCReadResult(IPCMessage msg) noexcept : result{msg} {}
        explicit IPCReadResult(IPCError err) noexcept : result(err) {}
        template<typename Fn>
        [[maybe_unused]] void then(Fn fn)
        {
            std::visit(IPCResultVisitor{[fn](IPCMessage m) { fn(m); },
                                        [](IPCError err) { cx::println("Failed to read message queue. System message: {}", err.err_message); },
                                        [](std::nullopt_t) {}},
                       result);
        }
    };

    enum IPCType {
        SYS_V_MQ = 1,
        POSIX_MQ = 2,
        POSIX_SOCKET = 3,
    };

    class IPCInterface;
    namespace factory
    {
        [[nodiscard]] auto ipc_setup_unix_socket(const fs::path& p, int epoll_fd) -> std::unique_ptr<IPCInterface>;

    } // namespace factory

    class IPCInterface
    {
      public:
        fs::path path;
        explicit IPCInterface(fs::path qpath, IPCFileDescriptors fds) : path(std::move(qpath)), server_fds(fds) {}
        virtual ~IPCInterface() = default;
        [[nodiscard]] virtual auto poll_queue() -> IPCReadResult = 0;
        virtual auto poll_event() -> void = 0;
        /// If we are using (possibly) IPC that is non-compliant with poll/epoll or select, we don't override this function
        [[nodiscard]] virtual auto has_request() const -> bool { return false; }
        [[nodiscard]] virtual auto is_connection_request(int fd) -> bool { return fd == server_fds.listening; }
        virtual void handle_incoming_connection() = 0;
        virtual void read_from_input(std::optional<int> file_descriptor) = 0;
        virtual void drop_client(int fd) {}
      protected:
        IPCFileDescriptors server_fds;
    };
} // namespace cx::ipc
