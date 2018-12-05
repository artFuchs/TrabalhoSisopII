#include "Frontend.hpp"

namespace dropbox{

    Frontend::Frontend(int port) : _socket(port){
        _port = port;
        _valid = _socket.isValid();
        _clientAddressValid = false;
        _serverAddressValid = false;
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
        _serverAddressValid = true;
        _serverAddr = tmpSocket.getReadingAddress();
        _serverAddrStr = tmpSocket.getClientAddressString();
        
        for(auto p : _pendingPacketServer){
            _socket.send(p, &_serverAddr);
        }
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
                sendAck(packet->packetNum);

                if(packet->type == PacketType::COORDINATOR){
                    std::cout << "Received new coordinator!" << std::endl;
                    _serverAddr = _socket.getReadingAddress();
                    _serverAddrStr = _socket.getClientAddressString();
                } else if(packet->type == PacketType::LOGIN){
                    std::cout << "Received login!" << std::endl;
                    
                    if(packet->loginServer){
                        _serverAddressValid = true;
                        _serverAddr = _socket.getReadingAddress();
                        _serverAddrStr = _socket.getClientAddressString();
                        
                        for(auto p : _pendingPacketServer){
                            _socket.send(p, &_serverAddr);
                        }
                    } else{
                        _clientAddressValid = true;
                        _clientAddrStr = _socket.getClientAddressString();
                        _clientAddr = _socket.getReadingAddress();

                        for(auto p : _pendingPacketClient){
                            _socket.send(p, &_clientAddr);
                        }
                    }
                }

               std::string addStr = _socket.getClientAddressString();
                if(addStr == _clientAddrStr){
                    if(_serverAddressValid){
                        _socket.send(packet, &_serverAddr);
                    } else{
                        _pendingPacketServer.push_back(packet);
                    }
                } else if(addStr == _serverAddrStr){
                    if(_clientAddressValid){
                        _socket.send(packet, &_clientAddr);
                    } else{
                        _pendingPacketClient.push_back(packet);
                    }
                }
            }
        }
    }
    
    void Frontend::sendAck(uint32_t packetNum){
        std::shared_ptr<Packet> newPacket(new Packet);
        sockaddr_in address = _socket.getReadingAddress();

        newPacket->type = PacketType::ACK;
        newPacket->packetNum = packetNum;
        
        _socket.send(newPacket, &address);
    }

    void Frontend::exit(){
        _valid = false;
    }

}
