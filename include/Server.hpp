#pragma once

#include <thread>
#include <string>
#include <memory>
#include <map>

#include "ServerSession.hpp"
#include "RMSession.hpp"

#include "ElectionManager.hpp"

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

    int _port;
    bool _running;

    // information about RMs
    // maybe it would be a good idea to create a RM Manager class
    std::thread _listemRMThread;
    UDPSocket _listenSocketRM;
    std::vector<std::thread> _threadPoolRM;
    std::mutex _jobPoolMutexRM;
    std::vector<RMJob> _jobPoolRM;
    std::map<std::string, std::shared_ptr<RMSession>> _RMSessions;
    AddressList _RMAdresses;

    ElectionManager _electionManager;

    int _RMport;
    bool _primary;
    int _id;  //server id
    int _last_id;  //last given id

    // connection with primaryServer
    int _priPort;
    std::string _priIp;

    void updateLastID(int);
public:
    //primary server constructor
    Server(int port, int RMport);
    //secondary server constructor
    Server(int port, int RMport, std::string priIp, int priPort);

    void run(int numberOfThreads = 4);
    void stop(void);
};

}
