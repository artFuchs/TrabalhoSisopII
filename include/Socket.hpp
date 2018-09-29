#pragma once

#include <exception>
#include <memory>
#include <thread>
#include <mutex>

#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <netdb.h>
#include <stdio.h>

#define BUFFER_MAX_SIZE 256

namespace dropbox{

// We probably send just the buffer, not the entire Packet; we could change packetLen to bufferLen
struct Packet{
    int bufferLen;
    char buffer[256];
};

///
/// \brief UDPSocket contains every information related to a UDP socket.
/// After creating an object of this class, one must call bind().
/// To read from it, one must first call read() and make sure that the
/// length of the message is greater than zero. After that, you can
/// obtain the pointer of the message with buffer()
///
class UDPSocket{

private:
    int _sockfd;
    socklen_t _serverLen, _clientLen;
    struct sockaddr_in _serverAddr, _clientAddr;
    Packet _buffer;
    std::mutex _socketMutex;    

    bool _valid;
public:
    UDPSocket(void) = delete;
    UDPSocket(UDPSocket& socket) = delete;

    // Creates a new server Socket that listens to port
    UDPSocket(int port){
        _serverAddr.sin_family = AF_INET;
        _serverAddr.sin_port = htons(port);
        _serverAddr.sin_addr.s_addr = INADDR_ANY;
        bzero(&(_serverAddr.sin_zero), 8);
        _clientLen = sizeof(struct sockaddr_in);
        _valid = false;
        
        if ((_sockfd = socket(AF_INET, SOCK_DGRAM, 0)) != -1){
            int bindReturn = ::bind(_sockfd, (struct sockaddr *) &_serverAddr, sizeof(struct sockaddr));
            if(bindReturn >= 0){
                _valid = true;
            } else{
                std::cout << "Error " << bindReturn << " while trying to bind the socket" << std::endl;
            }
        } else{
            std::cout << "Error " << _sockfd << " while trying to create the socket" << std::endl;        
        }
    }    
    
    ~UDPSocket(void){
    }
        
    Packet read(void){
        std::lock_guard<std::mutex> lck(_socketMutex);

        if(_valid){
            _buffer.bufferLen = recvfrom(_sockfd, (void*) _buffer.buffer, BUFFER_MAX_SIZE, 0, (struct sockaddr *) &_clientAddr, (socklen_t*) &_clientLen);
            return _buffer;
        } else{
           throw std::runtime_error("Socket is not valid");
        }
    }
 
    int send(std::shared_ptr<Packet> packet, sockaddr_in* sendingAddress){
        std::lock_guard<std::mutex> lck(_socketMutex);

        if(_valid){
            return sendto(_sockfd, packet->buffer, packet->bufferLen, 0, (sockaddr *) sendingAddress, sizeof(sockaddr)); 
        } else{
           throw std::runtime_error("Socket is not valid");
        }
    }
       
    std::string getClientAddressString(void){
        return std::string(std::to_string(_clientAddr.sin_addr.s_addr) + ":" + std::to_string(_clientAddr.sin_port));
    }

    sockaddr_in& getClientAddress(void){
        return _clientAddr;
    }
};

}
