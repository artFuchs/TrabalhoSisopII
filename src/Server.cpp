#include <iostream>

#include "Server.hpp"

namespace dropbox{

Server::Server(int port, int RMport) :
    _listenSocket(port), _listenSocketRM(RMport){
    _port = port;
    _RMport = RMport;
    _primary = true;
    _running = false;
    _last_id = 0;
    _id = 0;
    _RMAdresses.clear();
    cout << "PORT: " << port << ", RMPORT: " << RMport << endl;
}

Server::Server(int port, int RMport, std::string priIp, int priPort) :
    _listenSocket(port), _listenSocketRM(RMport), _priIp(priIp){
    _port = port;
    _RMport = RMport;
    _priPort = priPort;
    _primary = false;
    _running = false;
    _last_id = 0;
    _id = -1;
    _RMAdresses.clear();
    cout << "PORT: " << port << ", RMPORT: " << RMport << ", primary RM port: " << priPort << endl;
}

void Server::run(int numberOfThreads){
    _running = true;

    for(int i = 0; i < numberOfThreads; i++){
        _threadPool.push_back(std::thread([&] {
            while(_running){
                ServerJob job;
                bool hasJob = false;
                {
                    // The lock is destroyed before calling onSessionReadMessage
                    std::lock_guard<std::mutex> lck(_jobPoolMutex);
                    if(!_jobPool.empty()){
                        hasJob = true;
                        auto it = _jobPool.begin();
                        job = *it;
                        _jobPool.erase(it);
                    }
                }

                if(hasJob){
                    job.first->onSessionReadMessage(job.second);
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
        }));
    }

    // Creates the thread that waits for new connections
    _listenConnectionsThread = std::thread([&] {
        while(_running){
            // Receiving a new message
            std::shared_ptr<Packet> packet(new Packet);
            *packet = _listenSocket.read();

            // If there was a problem while reading, it shuts down the connection
            if(packet->bufferLen < 0){
                std::cout << "Error while trying to read from socket" << std::endl;
                break;
            }

            std::string clientAddress = _listenSocket.getClientAddressString();
            auto it = _serverSessions.find(clientAddress);

            // Checks if there is a ServerSession for the client that has sent the message
            if(it == _serverSessions.end()){
                std::cout << "NEW SESSION" << std::endl;
                if(packet->type == PacketType::LOGIN){
                    std::string username(packet->buffer, packet->bufferLen);
                    std::shared_ptr<SessionSupervisor<ServerSession>> supervisor(new SessionSupervisor<ServerSession>(username));   // Is lost if the supervisor already exists
                    auto sup = _serverSessionsByUsername.find(username);
                    if(sup != _serverSessionsByUsername.end()){
                        supervisor = sup->second;
                    } else{
                        _serverSessionsByUsername.insert(std::make_pair(username, supervisor));
                    }
                    std::shared_ptr<ServerSession> newSession(new ServerSession(clientAddress, _listenSocket, supervisor));
                    newSession->setReceiverAddress(_listenSocket.getReadingAddress());
                    _serverSessions.insert(std::make_pair(clientAddress, newSession));
                    supervisor->addSession(std::make_pair(clientAddress, newSession));
                    it = _serverSessions.find(clientAddress);
                }
            }

            {
                std::lock_guard<std::mutex> lck(_jobPoolMutex);
                _jobPool.push_back(std::make_pair(it->second, packet));
            }
        }
    }
    );

    // from this point, start the RMs
    // start threads to
    for(int i = 0; i < numberOfThreads; i++){
        _threadPoolRM.push_back(std::thread([&] {
            while(_running){
                RMJob job;
                bool hasJob = false;
                {
                    // The lock is destroyed before calling onSessionReadMessage
                    std::lock_guard<std::mutex> lck(_jobPoolMutexRM);
                    if(!_jobPoolRM.empty()){
                        hasJob = true;
                        auto it = _jobPoolRM.begin();
                        job = *it;
                        _jobPoolRM.erase(it);
                    }
                }

                if(hasJob){
                    job.first->onSessionReadMessage(job.second);
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
        }));
    }

    // thread that awaits for connection with RMs
    _listemRMThread = std::thread([&]{
        while (_running)
        {
            std::shared_ptr<Packet> packet(new Packet);
            *packet = _listenSocketRM.read();
            bool isLogin = packet->type == PacketType::LOGIN;

            std::cout << "SERVER received: " << PacketType::str[packet->type] << std::endl;

            // Secondary, not yet connected
            if (!_primary && (_id < 0) ){
                // waits for LOGIN message
                if (isLogin and packet->bufferLen>0){
                    // get ID
                    memcpy(&_id, packet->buffer, sizeof(_id));
                    updateLastID(_id);
                }
            }

            std::string clientAddress = _listenSocketRM.getClientAddressString();
            auto it = _RMSessions.find(clientAddress);

            bool found = it != _RMSessions.end();
            bool hasID = _id > -1;
            if (!found && hasID && isLogin) {
                std::cout << "new RMSession" << std::endl;
                // generate a new ID
                if (_primary){
                    packet->id = _last_id+1;
                }
                // create RMSession
                std::shared_ptr<RMSession> newSession(new RMSession(_listenSocketRM, _primary, _id));
                newSession->setAddresses(_RMAdresses);
                newSession->setReceiverAddress(_listenSocketRM.getReadingAddress());
                _RMAdresses[packet->id] = _listenSocketRM.getReadingAddress();
                _RMSessions.insert(std::make_pair(clientAddress, newSession));
                updateLastID(packet->id);
                it = _RMSessions.find(clientAddress);
            }

            // add session to job_pool
            if (it != _RMSessions.end()){
                std::lock_guard<std::mutex> lck(_jobPoolMutexRM);
                _jobPoolRM.push_back(std::make_pair(it->second, packet));
            }
        }
    });

    if (!_primary){
        // creates a temporary socket just to send the LOGIN message
        UDPSocket tmpSocket(_priIp.data(),_priPort);
        RMSession tmpSession(_listenSocketRM, false);
        tmpSession.setReceiverAddress(tmpSocket.getReadingAddress());
        // try sending the message until have an ID
        while (_id < 0){
            std::cout << "trying to connect..." << std::endl;
            tmpSession.connect();
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }
}

void Server::stop(void){
    _running = false;

    // Stops the sessions
    for(auto it : _serverSessions) it.second->stop();

    // Join with the thread that waits for new connections
    if(_listenConnectionsThread.joinable()) _listenConnectionsThread.join();
}

void Server::updateLastID(int id){
    if (id > _last_id)
      _last_id = id;
}

}
