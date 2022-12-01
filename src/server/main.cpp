#include "chatserver.hpp"
#include "chatservice.hpp"
#include <iostream>
using namespace std;

int main(int argc, char **argv)
{
    EventLoop loop;
    InetAddress addr("127.0.0.1", 6666);

    ChatServer server(&loop, addr, "ChatServer");

    server.start();
    loop.loop();

    return 0;
}