#include <iostream>

#include "Server.hpp"

namespace dropbox{

Server::Server(int port) : _listenSocket(port){
    _port = port;
    _running = false;
}

void Server::run(void){
    _running = true;

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
                std::shared_ptr<ServerSession> newSession(new ServerSession(_listenSocket, _listenSocket.getClientAddress()));
                _serverSessions.insert(std::make_pair(clientAddress, newSession));
                it = _serverSessions.find(clientAddress);
            }

            it->second->onSessionReadMessage(packet);
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
}

}


