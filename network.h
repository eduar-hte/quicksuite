#pragma once

#include <cstdint>
#include <span>
#include <cassert>
#include <optional>
#include <sys/socket.h>

constexpr int SERVER_PORT = 8080;

template <bool check_bytes_read, typename MsgType, typename TransferOp>
bool transfer_helper(const int socket, MsgType &msg, const std::size_t msg_size, TransferOp op)
{
    const auto bytes_transferred = op(socket, &msg, msg_size, 0);
    const auto ret = bytes_transferred == msg_size;
    if constexpr (check_bytes_read == true)
        assert(ret == true);
    return ret;
}

template <typename MsgType, bool check_bytes_read = true>
bool send_msg(const int socket, const MsgType &msg)
{
    return transfer_helper<check_bytes_read>(socket, msg, msg.header.size, send);
}

template <typename MsgType, bool check_bytes_read = true>
bool recv_msg(const int socket, MsgType &msg, const std::optional<std::size_t> size = std::nullopt)
{
    return transfer_helper<check_bytes_read>(socket, msg, size.value_or(sizeof(MsgType)), recv);
}
