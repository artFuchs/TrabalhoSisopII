#include "Socket.hpp"

#include <vector>

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
        
        bool _clientAddressValid;
        bool _serverAddressValid;
        std::vector<std::shared_ptr<Packet>> _pendingPacketClient;
        std::vector<std::shared_ptr<Packet>> _pendingPacketServer;

        void sendAck(uint32_t packetNum);
        
    public:
        Frontend(int port);
        int getPort();
        void setServerAddress(const char* hostname, int port);
        void listen();
        void exit();
    };
}
