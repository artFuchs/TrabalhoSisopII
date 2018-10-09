#pragma once

#include <thread>
#include <string>
#include <memory>
#include <map>

#include "ServerSession.hpp"

namespace dropbox{

typedef std::pair<std::shared_ptr<ServerSession>, std::shared_ptr<Packet>> ServerJob;

///
/// \brief Server is the class that waits for connections with new clients and launches ServerSessions for them
///
class Server{

private:
    std::thread _listenConnectionsThread;
    UDPSocket _listenSocket;
    std::vector<std::thread> _threadPool;
    std::vector<ServerJob> _jobPool;
    std::mutex _jobPoolMutex;

    std::map<std::string, std::shared_ptr<ServerSession>> _serverSessions;

    int _port;
    bool _running;

public:
    Server(int port);

    void run(int numberOfThreads = 4);
    void stop(void);

};

}
