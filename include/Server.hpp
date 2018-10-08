#pragma once

#include <thread>
#include <string>
#include <memory>
#include <map>

#include "ServerSession.hpp"

namespace dropbox{

///
/// \brief Server is the class that waits for connections with new clients and launches ServerSessions for them
///
class Server{

private:
    std::thread _listenConnectionsThread;
    UDPSocket _listenSocket;

    std::map<std::string, std::shared_ptr<ServerSession>> _serverSessions;

    int _port;
    bool _running;

public:
    Server(int port);

    void run(void);
    void stop(void);

};

}
