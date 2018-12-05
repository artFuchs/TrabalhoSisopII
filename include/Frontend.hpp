#include "Socket.hpp"

namespace dropbox{

    class Frontend{
    private:
        UDPSocket _socket;
        int _port;
        struct sockaddr_in _clientAddr;
        struct sockaddr_in _serverAddr;
        std::string _clientAddrStr;
        std::string _serverAddrStr;
        bool _valid;
        std::thread _listenThread;


    public:
        Frontend(int port);
        int getPort();
        void setServerAddress(const char* hostname, int port);
        void listen();
        void exit();
    };
}
