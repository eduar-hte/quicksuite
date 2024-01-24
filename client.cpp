#include <array>
#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <thread>
#include <iomanip>
#include <vector>
#include <cstring>
#include <cassert>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "messages.h"
#include "network.h"
#include "cipher.h"
#include "misc.h"
#include "external/cxxopts.hpp"

void login(const int client_socket, const LoginRequestBody::UsernameType &username, const LoginRequestBody::PasswordType &password, const std::uint8_t seq)
{
    auto msg = make_msg<LoginRequestMsg>(seq);
    msg.body.username = username;
    msg.body.password = password;
    send_msg(client_socket, msg);

    LoginResponseMsg rsp;
    recv_msg(client_socket, rsp);
    assert(rsp.header.type == MessageHeader::MessageType::LoginResponse);
    assert(rsp.header.seq == msg.header.seq);
    assert(rsp.body.status_code == LoginResponseBody::StatusCodeType::Ok);
}

std::string echo(const int client_socket, const LoginRequestBody::UsernameType &username, const LoginRequestBody::PasswordType &password, const std::string &plain_text, const std::uint8_t seq)
{
    const auto plain_text_len = plain_text.length();
    auto msg = make_echo_req_msg(seq, static_cast<EchoMessageBody::MsgSizeType>(plain_text_len));

    assert(plain_text.length() <= msg.body.message.size());
    std::memcpy(msg.body.message.data(), plain_text.c_str(), plain_text_len);

    cipher_helper({msg.body.message.data(), plain_text_len}, seq, username, password);

    send_msg(client_socket, msg);

    EchoResponseMsg rsp;
    recv_msg(client_socket, rsp, msg.header.size); // NOTE: response should have same size as request
    assert(rsp.header.type == MessageHeader::MessageType::EchoResponse);
    assert(rsp.header.size == msg.header.size);
    assert(rsp.header.seq == msg.header.seq);
    assert(std::memcmp(rsp.body.message.data(), plain_text.c_str(), plain_text_len) == 0);

    return std::string(reinterpret_cast<char *>(rsp.body.message.data()), rsp.body.msg_size);
}

int connect_to_server()
{
    // create client socket
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1)
    {
        perror("Error creating socket");
        return -1;
    }

    // set up server address
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // loopback
    server_addr.sin_port = htons(SERVER_PORT);

    // connect to the server
    if (connect(client_socket, reinterpret_cast<sockaddr *>(&server_addr), sizeof(server_addr)) == -1)
    {
        perror("Error connecting to server");
        close(client_socket);
        return -1;
    }

    return client_socket;
}

int interactive_client()
{
    auto client_socket = connect_to_server();
    if (client_socket == -1)
        return -1;

    std::cout << "Connected to the server on port " << SERVER_PORT << std::endl;

    std::uint8_t seq = 0;

    auto get_input = [](const char *msg, std::string &value)
    {
        std::cout << msg;
        std::getline(std::cin, value);
    };

    // login
    std::string usr, pwd;
    while (usr.empty() == true)
        get_input("Enter username: ", usr);
    get_input("Enter password: ", pwd);

    const auto username = init_credential<LoginRequestBody::UsernameType>(usr.c_str());
    const auto password = init_credential<LoginRequestBody::PasswordType>(pwd.c_str());
    login(client_socket, username, password, seq++);

    while (true)
    {
        // get message
        std::string message;
        get_input("Enter a message (or 'exit' to quit): ", message);

        if (message == "exit")
        {
            break;
        }

        // get echo from server
        const auto ret = echo(client_socket, username, password, message, seq++);
        std::cout << "Server echoed: " << ret << std::endl;
    }

    // close connection
    close(client_socket);

    return 0;
}

std::vector<std::string> read_sample_text()
{
    std::ifstream file("lorem_ipsum.txt", std::ios::in);
    assert(file.is_open());

    std::vector<std::string> lines;

    std::string line;
    while (std::getline(file, line))
        lines.push_back(line);

    return lines;
}

void benchmark_server()
{
    const auto sample_lines = read_sample_text();

    const auto t1 = std::chrono::high_resolution_clock::now();

    {
        constexpr auto NUM_THREADS = 10;

        std::array<std::jthread, NUM_THREADS> threads;

        for (auto ndx = 0; ndx != threads.size(); ++ndx)
            // NOTE: sample_lines is not modified in each thread, so it's safe
            // to capture it by reference and access it concurrently
            threads[ndx] = std::jthread([&sample_lines, ndx]()
                                        {
                                            // read first two words in the line at position id to use as username & password
                                            assert(ndx < sample_lines.size());
                                            std::istringstream iss{sample_lines[ndx]};
                                            std::string usr, pwd;
                                            iss >> usr >> pwd;

                                            const auto client_socket = connect_to_server();
                                            assert(client_socket != -1);

                                            std::uint8_t seq = 0;

                                            // login
                                            const auto username = init_credential<LoginRequestBody::UsernameType>(usr.c_str());
                                            const auto password = init_credential<LoginRequestBody::PasswordType>(pwd.c_str());
                                            login(client_socket, username, password, seq++);

                                            // get echo from server
                                            for (unsigned int i = 0; i != 1000; i++)
                                                for (const auto &line : sample_lines)
                                                    echo(client_socket, username, password, line, seq++); });
    }

    const auto t2 = std::chrono::high_resolution_clock::now();
    const std::chrono::duration<double, std::milli> ms = t2 - t1;

    std::cout << std::fixed << std::setprecision(1);
    std::cout << "benchmark time: " << ms.count() << " ms\n";
}

int main(int argc, char **argv)
{
    cxxopts::Options options(argv[0], "TCP Echo Client");

    options.add_options()("b,benchmark", "Benchmark server", cxxopts::value<bool>()->default_value("false"))("h,help", "Print usage");

    const auto args = options.parse(argc, argv);

    if (args.count("help"))
    {
        std::cout << options.help() << std::endl;
        return 0;
    }

    if (args["benchmark"].as<bool>() == true)
        benchmark_server();
    else
        return interactive_client();

    return 0;
}