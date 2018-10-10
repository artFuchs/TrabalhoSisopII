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
    // ServerSessions organized by ServerSession
    // Only one ServerSession is stored here
    std::map<std::string, std::shared_ptr<SessionSupervisor<ServerSession>>> _serverSessionsByUsername;
    //std::mutex _ssByUsernameMutex; // Useful if we want to add another thread to receive new messages

    int _port;
    bool _running;

public:
    Server(int port);

    void run(int numberOfThreads = 4);
    void stop(void);

};

}
