#include <iostream>
#include <map>
#include <thread>
#include <cassert>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "messages.h"
#include "network.h"
#include "cipher.h"
#include "external/cxxopts.hpp"

constexpr int MAX_CLIENTS = 10;

struct LoginInfo
{
    bool logged = false;
    LoginRequestBody::UsernameType username;
    LoginRequestBody::PasswordType password;
};

bool handle_request(const int client_socket, LoginInfo &login_info)
{
    MessageHeader msg_header;
    const auto ret = recv_msg<MessageHeader, false>(client_socket, msg_header);

    if (ret == false)
    {
        // client closed the connection
        std::cout << "Client disconnected" << std::endl;
        close(client_socket);
        return false;
    }

    if (msg_header.type == MessageHeader::MessageType::LoginRequest)
    {
        assert(login_info.logged == false);

        LoginRequestBody login_request;
        assert(msg_header.size - sizeof(msg_header) == sizeof(login_request));
        recv_msg(client_socket, login_request);

        // update login info
        login_info.logged = true;
        login_info.username = login_request.username;
        login_info.password = login_request.password;

        auto rsp = make_msg<LoginResponseMsg>(msg_header.seq);
        rsp.body.status_code = LoginResponseBody::StatusCodeType::Ok;
        send_msg(client_socket, rsp);
    }
    else if (msg_header.type == MessageHeader::MessageType::EchoRequest)
    {
        assert(login_info.logged == true);
        assert(msg_header.size - sizeof(msg_header) >= sizeof(EchoMessageBody::MsgSizeType));

        // read the body of the echo request msg
        // NOTE: we read into the response msg's body in order
        // to decrypt inplace and then send back
        auto rsp = make_echo_rsp_msg(msg_header);
        recv_msg(client_socket, rsp.body, rsp.header.size - sizeof(MessageHeader));
        assert(rsp.body.msg_size == rsp.header.size - sizeof(MessageHeader) - sizeof(EchoMessageBody::MsgSizeType));

        cipher_helper({rsp.body.message.data(), rsp.body.msg_size}, msg_header.seq, login_info.username, login_info.password);

        send_msg(client_socket, rsp);
    }
    else
        assert(false); // unknown request type

    return true;
}

int accept_connection(const int server_socket)
{
    sockaddr_in client_addr{};
    socklen_t client_addr_len = sizeof(client_addr);
    const auto client_socket = accept(server_socket, reinterpret_cast<sockaddr *>(&client_addr), &client_addr_len);

    if (client_socket == -1)
        perror("Error accepting connection");
    else
        std::cout << "New connection from " << inet_ntoa(client_addr.sin_addr) << ":" << ntohs(client_addr.sin_port) << std::endl;

    return client_socket;
}

void threaded_server(const int server_socket)
{
    while (true)
    {
        const auto client_socket = accept_connection(server_socket);
        if (client_socket == -1)
            continue;

        // launch & detach from thread to handle client connection
        std::thread([client_socket]()
                    {
                        LoginInfo login_info;
                        while(handle_request(client_socket, login_info) == true); })
            .detach();
    }
}

void io_socket_multiplexing_server(const int server_socket)
{
    std::map<int, LoginInfo> clients;

    while (true)
    {
        // setup read set with server & client connection sockets
        fd_set read_set;
        FD_ZERO(&read_set);

        FD_SET(server_socket, &read_set);
        auto maxFd = server_socket;

        for (auto [client_socket, _] : clients)
        {
            FD_SET(client_socket, &read_set);
            maxFd = std::max(maxFd, client_socket);
        }

        // wait for an event
        if (select(maxFd + 1, &read_set, nullptr, nullptr, nullptr) == -1)
        {
            perror("Error in select");
            break;
        }

        // check if there's a new connection
        if (FD_ISSET(server_socket, &read_set))
        {
            const auto client_socket = accept_connection(server_socket);
            if (client_socket != -1)
                clients.insert({client_socket, {}});
        }

        // check if there's a client request
        for (auto it = clients.begin(); it != clients.end();)
        {
            auto client_socket = it->first;

            if (FD_ISSET(client_socket, &read_set))
            {
                auto ret = handle_request(client_socket, it->second);
                if (ret == false)
                    it = clients.erase(it);
                else
                    ++it;
            }
            else
                ++it;
        }
    }

    for (auto [client_socket, _] : clients)
        close(client_socket);
}

int main(int argc, char **argv)
{
    cxxopts::Options options(argv[0], "TCP Echo Server");

    options.add_options()("t,threaded", "Threaded server version", cxxopts::value<bool>()->default_value("false"))("h,help", "Print usage");

    const auto args = options.parse(argc, argv);

    if (args.count("help"))
    {
        std::cout << options.help() << std::endl;
        return 0;
    }

    const int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1)
    {
        perror("Error creating socket");
        return -1;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(SERVER_PORT);

    if (bind(server_socket, reinterpret_cast<sockaddr *>(&server_addr), sizeof(server_addr)) == -1)
    {
        perror("Error binding");
        close(server_socket);
        return -1;
    }

    if (listen(server_socket, MAX_CLIENTS) == -1)
    {
        perror("Error listening");
        close(server_socket);
        return -1;
    }

    std::cout << "Server listening on port " << SERVER_PORT << std::endl;

    if (args["threaded"].as<bool>() == true)
    {
        std::cout << "Threaded server version" << std::endl;
        threaded_server(server_socket);
    }
    else
    {
        std::cout << "IO socket multiplexing server version" << std::endl;
        io_socket_multiplexing_server(server_socket);
    }

    close(server_socket);

    return 0;
}
