#include <iostream>
#include <exception>

#include "Client.hpp"

namespace dropbox{

Client::Client(const char* hostname, int port) : _listenSocket(hostname, port), _clientSession(_listenSocket){
    _running = true;
    _connected = false;
    
    _clientSession.setReceiverAddress(_listenSocket.getReadingAddress());
    _clientSession.connect();
    _clientSession.start();
    _listeningThread = std::thread([&] {
        while(_running){
            std::shared_ptr<Packet> packet(new Packet);
            sockaddr_in ser;
            *packet = _listenSocket.read(&ser);

            // Checks if there was an error upon reading the next packet
            if(packet->bufferLen < 0){
                std::cout << "Error while trying to read from socket" << std::endl;
                break;
            } else{
                if(!_connected){
                    _connected = true;
                }

                _clientSession.onSessionReadMessage(packet);
            }
        }
    });
}

void Client::start(void){
}

void Client::stop(void){
    _running = false;
    _connected = false;

    if(_listeningThread.joinable()){
        _listeningThread.join();
    }
}


}
