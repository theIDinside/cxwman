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
#include <variant>

namespace cx::ipc
{
    namespace fs = std::filesystem;
    /// Tells the linux sys call msgrcv to just pick out the oldest message, and not pick out by type
    constexpr auto READ_FIFO = 0;

    template<typename... Args>
    struct IPCResultVisitor {
        using Args::operator()...;
    };

    template<typename... Args>
    IPCResultVisitor(Args&&...) -> IPCResultVisitor<Args...>;

    enum class CommandTypes : long { Window = 1, Workspace = 2, BindKey = 3, N = BindKey };
    /// Internal representation. This is is the struct we let Linux write into from the message queue
    struct IPCBuffer {
        long type;                    /// 8 bytes
        std::array<char, 120> buffer; /// 120 + 8 bytes lines up on a 8 byte boundary.
    };

    struct IPCMessage {
        CommandTypes type;
        std::string_view payload;
        IPCMessage(CommandTypes type, std::string_view view) noexcept;
    };

    struct IPCError {
        int err_code;
        std::string_view err_message;
    };

    struct IPCReadResult {
        std::variant<IPCMessage, IPCError, std::nullopt_t> result;

        explicit IPCReadResult() noexcept : result{std::nullopt} {}
        explicit IPCReadResult(IPCMessage msg) noexcept : result{msg} {}
        explicit IPCReadResult(IPCError err) noexcept : result(err) {}
        template<typename Fn>
        [[maybe_unused]] auto then(Fn fn)
        {
            std::visit(
                IPCResultVisitor{[fn](IPCMessage&& m) { fn(m); }, [](IPCError err) { cx::println("Failed to read message queue"); }},
                [](std::nullopt_t) {}, result);
        }

        template<typename OrDoFn>
        auto as_message_or(OrDoFn fn) -> std::optional<IPCMessage>
        {
            return std::visit(IPCResultVisitor{[](IPCMessage&& msg) { return std::make_optional<IPCMessage>(msg); },
                                               [fn](auto&& other) {
                                                   fn();
                                                   return std::optional<IPCMessage>{std::nullopt};
                                               }},
                              result);
        }
    };

    class IPC
    {
      private:
        fs::path path;
        key_t handle;
        int queue_identifier;

      public:
        IPC(fs::path fpath, key_t handle, int queue_ident);

        [[nodiscard]] auto poll_queue() const -> IPCReadResult;
        [[nodiscard]] static auto setup_ipc(const fs::path& p) -> std::unique_ptr<IPC>;
    };

} // namespace cx::ipc
