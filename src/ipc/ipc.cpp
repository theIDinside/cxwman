//
// Created by cx on 2020-07-10.
//

#include "ipc.hpp"

#include <coreutils/core.hpp>
#include <utility>
namespace cx::ipc
{
    std::unique_ptr<IPC> cx::ipc::IPC::setup_ipc(const std::filesystem::path& p)
    {
        auto handle_key = ftok(p.c_str(), 64);
        auto identifier = msgget(handle_key, 0666 | IPC_CREAT);

        return std::make_unique<IPC>(p, handle_key, identifier);
    }
    IPC::IPC(fs::path fpath, key_t handle, int queue_ident) : path{std::move(fpath)}, handle{handle}, queue_identifier(queue_ident) {}
    auto IPC::poll_queue() const -> IPCReadResult
    {
        IPCBuffer msg{};
        auto bytes_read = msgrcv(queue_identifier, &msg, sizeof(msg.buffer), READ_FIFO,
                                 IPC_NOWAIT); // We DON'T want to block here as the Window Manager. That would be disastrous.
        if(bytes_read == -1) {
            DBGLOG("Failed to interpret message/command in queue with type {}", msg.type);
            return IPCReadResult{IPCError{errno, std::strerror(errno)}};
        } else if(msg.type > static_cast<long>(CommandTypes::N)) {
            return IPCReadResult{IPCError{0, "Unknown message type was read"}};
        } else if(bytes_read == 0) {
            return IPCReadResult{};
        } else {
            std::string_view data{msg.buffer.data()};
            auto type = static_cast<CommandTypes>(msg.type);
            IPCMessage ipcMessage{type, data};
            return IPCReadResult(ipcMessage);
        }
    }

    IPCMessage::IPCMessage(CommandTypes type, std::string_view view) noexcept : type(type), payload(view) {}
} // namespace cx::ipc
