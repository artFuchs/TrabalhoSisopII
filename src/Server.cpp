#include <iostream>

#include "Server.hpp"

namespace dropbox{

Server::Server(int port) : _listenSocket(port){
    _port = port;
    _running = false;
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
                } else{
                    // TODO: we should reject the connection
                }
            }
            
            {
                std::lock_guard<std::mutex> lck(_jobPoolMutex);
                _jobPool.push_back(std::make_pair(it->second, packet));
            }
        }
    }
    );
}

void Server::stop(void){
    _running = false;

    // Stops the sessions
    for(auto it : _serverSessions) it.second->stop();

    // Join with the thread that waits for new connections
    if(_listenConnectionsThread.joinable()) _listenConnectionsThread.join();
    // TODO: join with _threadPool threads
}


}
