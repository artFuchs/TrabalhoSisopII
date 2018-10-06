#pragma once

#include "ClientSession.hpp"

namespace dropbox{

///
/// \brief Server is the class that creates a connection with a server
///
class Client{

private:

    UDPSocket _listenSocket;
    ClientSession _clientSession;
    std::thread _listeningThread;
    std::thread _monitoringThread; // thread that will monitor the folder;
    bool _running;
    bool _connected;

public:

    Client(const char* username, const char* hostname, int port);

    void stop(void);

    void start(void);

};

}
