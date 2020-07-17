//
// Created by cx on 2020-07-10.
//

#include "ipc.hpp"
/// Derived IPC Mechanisms from base class IPCInterface
#include "UnixSocket.h"
// ----

#include <coreutils/core.hpp>
#include <utility>
namespace cx::ipc
{

    auto factory::ipc_setup_unix_socket(const fs::path& p, int epoll_fd) -> std::unique_ptr<IPCInterface>
    {
        return UnixSocket::initialize(p, 10, epoll_fd);
    }

    IPCMessage::IPCMessage(CommandTypes type, std::string_view client_identifier, std::string_view payload) noexcept
        : type(type), client_name(client_identifier), payload(payload)
    {
    }
} // namespace cx::ipc
