#pragma once

#include <cstdint>
#include <array>
#include <limits>
#include <optional>

#pragma pack(push, 1)

struct MessageHeader
{
    enum class MessageType : std::uint8_t
    {
        LoginRequest = 0,
        LoginResponse = 1,
        EchoRequest = 2,
        EchoResponse = 3
    };
    using SizeType = std::uint16_t;
    using SequenceType = std::uint8_t;

    SizeType size;
    MessageType type;
    SequenceType seq;
};

template <typename BodyType, MessageHeader::MessageType MsgType>
struct Message
{
    static constexpr auto Type = MsgType;

    MessageHeader header;
    BodyType body;
};

struct LoginRequestBody
{
    using UsernameType = std::array<char, 32>;
    using PasswordType = std::array<char, 32>;

    UsernameType username = {}; // zero-initialize, as requested in spec
    PasswordType password = {}; // zero-initialize, as requested in spec
};

using LoginRequestMsg = Message<LoginRequestBody, MessageHeader::MessageType::LoginRequest>;

struct LoginResponseBody
{
    enum class StatusCodeType : std::uint16_t
    {
        Failed = 0,
        Ok = 1,
    };

    StatusCodeType status_code;
};

using LoginResponseMsg = Message<LoginResponseBody, MessageHeader::MessageType::LoginResponse>;

struct EchoMessageBody
{
    using MsgSizeType = std::uint16_t;
    static constexpr auto MaxMsgSize = std::numeric_limits<MsgSizeType>::max();
    using MsgType = std::array<std::uint8_t, MaxMsgSize>;

    MsgSizeType msg_size;
    MsgType message; // do not zero-initialize for performance reasons (we only use the required buffer size)
};

using EchoRequestMsg = Message<EchoMessageBody, MessageHeader::MessageType::EchoRequest>;
using EchoResponseMsg = Message<EchoMessageBody, MessageHeader::MessageType::EchoResponse>;

#pragma pack(pop)

template <typename MsgType>
MsgType make_msg(MessageHeader::SequenceType seq, std::optional<MessageHeader::SizeType> body_size = std::nullopt)
{
    MsgType msg;
    msg.header.size = static_cast<MessageHeader::SizeType>(body_size.has_value() == true ? sizeof(MessageHeader) + body_size.value() : sizeof(MsgType));
    msg.header.type = MsgType::Type;
    msg.header.seq = seq;
    return msg;
}

constexpr auto EchoBodyHeaderSize = sizeof(EchoMessageBody) - EchoMessageBody::MaxMsgSize;

template <typename EchoMsgType>
EchoMsgType make_echo_msg(MessageHeader::SequenceType seq, EchoMessageBody::MsgSizeType msg_size)
{
    auto msg = make_msg<EchoMsgType>(seq, EchoBodyHeaderSize + msg_size);
    msg.body.msg_size = msg_size;
    return msg;
}

inline EchoRequestMsg make_echo_req_msg(MessageHeader::SequenceType seq, EchoMessageBody::MsgSizeType msg_size)
{
    return make_echo_msg<EchoRequestMsg>(seq, msg_size);
}

inline EchoResponseMsg make_echo_rsp_msg(const MessageHeader &echo_req_msg_header)
{
    const auto msg_size = echo_req_msg_header.size - sizeof(MessageHeader) - EchoBodyHeaderSize;
    return make_echo_msg<EchoResponseMsg>(echo_req_msg_header.seq, msg_size);
}
