#include "Frontend.hpp"

namespace dropbox{

    Frontend::Frontend(int port) : _socket(port){
        _port = port;
        _valid = _socket.isValid();
        _clientAddrStr = "";
        _serverAddrStr = "";
        _listenThread = std::thread(&Frontend::listen, this);
    }

    int Frontend::getPort(){
        int p = _port;
        if (!_valid)
            p = -1;
        return p;
    }

    void Frontend::setServerAddress(const char* hostname, int port){
        UDPSocket tmpSocket(hostname, port);
        _serverAddr = tmpSocket.getReadingAddress();
        _serverAddrStr = tmpSocket.getClientAddressString();
    }

    void Frontend::listen(){
        while (_valid){
            std::shared_ptr<Packet> packet(new Packet);
            *packet = _socket.read();
            std::cout << "FRONTEND recebeu mensagem : " << std::endl;
            // Checks if there was an error upon reading the next packet
            if(packet->bufferLen < 0){
                std::cout << "FRONTEND: Error while trying to read from socket" << std::endl;
                break;
            } else{

                if (packet->type == PacketType::COORDINATOR){
                    _serverAddr = socket.readingAddr();
                    _serverAddrStr = socket.getClientAddressString();
                }


                if (packet->type == PacketType::LOGIN){
                    if (_clientAddrStr == ""){
                        _clientAddrStr = _socket.getClientAddressString();
                        _clientAddr = _socket.getReadingAddress();
                    }
                }

                std::string addStr = _socket.getClientAddressString();
                if (addStr == _clientAddrStr){
                    _socket.send(packet, &_serverAddr);
                }else if (addStr == _serverAddrStr){
                    _socket.send(packet, &_clientAddr);
                }


            }
        }
    }

    void Frontend::exit(){
        _valid = false;
    }

}
