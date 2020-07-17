//
// Created by cx on 2020-07-13.
//
#pragma once
#include <ipc/ipc.hpp>

namespace cx::ipc {
    class SocketIPC : public IPCInterface
    {
      public:
        using IPCInterface::path;
    };
}
