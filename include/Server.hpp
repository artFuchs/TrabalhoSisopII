#pragma once

#include <thread>
#include <string>
#include <memory>
#include <map>

#include "ServerSession.hpp"
#include "RMSession.hpp"

namespace dropbox{

typedef std::pair<std::shared_ptr<ServerSession>, std::shared_ptr<Packet>> ServerJob;
typedef std::pair<std::shared_ptr<RMSession>, std::shared_ptr<Packet>> RMJob;

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

    std::thread _listemRMThread;
    UDPSocket _listenRMSocket;
    bool _primary;

    int _port;
    int _RMport;
    bool _running;

    // connection with primaryServer
    int _priPort;
    std::string _priIp;
    RMSession _RMSession;


public:
    //primary server constructor
    Server(int port, int RMport);
    //secondary server constructor
    Server(int port, int RMport, std::string priIp, int priPort);

    void run(int numberOfThreads = 4);
    void stop(void);

};

}
