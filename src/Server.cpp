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

            std::cout << "recebeu mensagem de " << packet->id << std::endl;
            std::cout << PacketType::str[packet->type] << endl;
            std::string clientAddress = _listenSocket.getClientAddressString();
            std::cout << "endereço do mandante: " << clientAddress << std::endl;

            auto it = _RMSessions.find(packet->id);

            if (it == _RMSessions.end()) {
                if (packet->type == PacketType::LOGIN){
                    std::cout << "new RM SESSION" << std::endl;
                    if (_primary)
                    {
                        _last_id ++;
                        std::cout << "dando o id"  << _last_id;
                        std::cout << " para a sessão" << std::endl;
                        packet->id = _last_id;
                    }
                    else
                    {
                        memcpy(&_id, packet->buffer, sizeof(_id));
                    }

                    std::shared_ptr<RMSession> newSession(new RMSession(_listenSocketRM,_primary,_id));
                    newSession->setReceiverAddress(_listenSocketRM.getReadingAddress());
                    _RMSessions.insert(std::make_pair(packet->id, newSession));
                    it = _RMSessions.find(packet->id);
                }
            }

            if (it != _RMSessions.end())
            {
                std::lock_guard<std::mutex> lck(_jobPoolMutexRM);
                _jobPoolRM.push_back(std::make_pair(it->second, packet));
            }
        }
    });

    if (!_primary){
        UDPSocket tmpSocket(_priIp.data(),_priPort);
        std::shared_ptr<RMSession> newSession(new RMSession(_listenSocketRM,_primary));
        newSession->setReceiverAddress(tmpSocket.getReadingAddress());
        _RMSessions.insert(std::make_pair(0,newSession));
        newSession->init_connection();

    }
}

void Server::stop(void){
    _running = false;

    // Stops the sessions
    for(auto it : _serverSessions) it.second->stop();

    // Join with the thread that waits for new connections
    if(_listenConnectionsThread.joinable()) _listenConnectionsThread.join();
}


}
