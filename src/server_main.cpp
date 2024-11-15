#include <iostream>
#include <asio.hpp>
#include "network/chat_server.hpp"
#include <locale>
#include <codecvt>
#include <windows.h>

int main(int argc, char* argv[])
{
    // 设置控制台输出编码为 UTF-8
    SetConsoleOutputCP(CP_UTF8);
    
    try {
        if (argc != 2) {
            std::cout << "用法: ChatServer <端口号>\n";
            std::cout << "示例: ChatServer 8080\n";
            return 1;
        }

        // 检查端口参数是否为数字
        std::string port_str = argv[1];
        if (!std::all_of(port_str.begin(), port_str.end(), ::isdigit)) {
            std::cout << "错误: 端口必须是数字\n";
            return 1;
        }

        uint16_t port = static_cast<uint16_t>(std::atoi(argv[1]));
        if (port <= 0 || port > 65535) {
            std::cout << "错误: 端口必须在 1-65535 之间\n";
            return 1;
        }

        asio::io_context io_context;
        ChatServer server(io_context, port);
        server.start();
        io_context.run();
    }
    catch (std::exception& e) {
        std::cerr << "异常: " << e.what() << "\n";
    }

    return 0;
} 