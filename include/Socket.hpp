#pragma once

#include <exception>
#include <iostream>
#include <memory>
#include <thread>
#include <mutex>

#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <vector>

#define BUFFER_MAX_SIZE 256
#define FILENAME_MAX_SIZE   256

namespace dropbox{

namespace PacketType{
    enum E{
        ACK,
        DATA,
        LOGIN
    };

    static constexpr const char* str[] = {
        "ACK",
        "DATA",
        "LOGIN"
    };

}

///
/// \brief The Packet struct contains the data that is sent between
/// clients through the server
///
struct Packet{
    // Defines what the Packet contains
    PacketType::E type;
    // Incremented after each message; the side that receives a packet with a given
    // packetNum p must send an Packet ACK whose packetNum is p
    uint32_t packetNum;          
    // The first fragment is always zero; independent of packetNum
    uint32_t fragmentNum;
    // Number of fragments the file was divided in
    uint32_t totalFragments;
    // Size of this particular Packet's data
    uint16_t bufferLen;
    // Size of this particular Packet's filename
    uint16_t pathLen;

    // Data itself
    char buffer[BUFFER_MAX_SIZE];
    char filename[FILENAME_MAX_SIZE];
};

///
/// \brief UDPSocket contains every information related to a UDP socket.
///
class UDPSocket{

private:
    int _sockfd;
    socklen_t _serverLen, _readingLen;
    struct sockaddr_in _serverAddr, _readingAddr;
    hostent *_serverHostent; // For Client
    Packet _buffer;
    std::mutex _socketMutex;

    bool _valid;
public:
    UDPSocket(void) = delete;
    UDPSocket(UDPSocket& socket) = delete;

    // Creates a new server Socket that listens to port
    // Used by the Server
    UDPSocket(int port){
        _serverAddr.sin_family = AF_INET;
        _serverAddr.sin_port = htons(port);
        _serverAddr.sin_addr.s_addr = INADDR_ANY;
        bzero(&(_serverAddr.sin_zero), 8);
        _readingLen = sizeof(struct sockaddr_in);
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

    // Establishes a connection with a server
    // Used by the Client
    UDPSocket(const char* hostname, int port){
        _serverHostent = gethostbyname(hostname);
	    if (_serverHostent == NULL) {
            throw std::runtime_error("Error: no such host");
        }

        _sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if(_sockfd < 0){
            throw std::runtime_error("Error upon trying to initialize the socket");
        }

        _readingAddr.sin_family = AF_INET;
        _readingAddr.sin_port = htons(port);
        _readingAddr.sin_addr = *((struct in_addr *)_serverHostent->h_addr);
    	  bzero(&(_readingAddr.sin_zero), 8);
        _valid = true;
    }

    ~UDPSocket(void){
    }

    // Server
    Packet read(void){
        // Not sure if necessary; also it's causing problems with concurrent reading and sending
        // If it is necessary, we could:
        // - Use a socket for sending and one for receiving
        // - Find a way to perform recvfrom non-blocking
        //std::lock_guard<std::mutex> lck(_socketMutex);

        if(_valid){
            std::vector<char> buff(sizeof(Packet));
            recvfrom(_sockfd, (void*) &buff[0], sizeof(Packet), 0, (struct sockaddr *) &_readingAddr, (socklen_t*) &_readingLen);
            memcpy(&_buffer, &buff[0], sizeof(Packet));
            return _buffer;
        } else{
           throw std::runtime_error("Socket is not valid");
        }
    }

    // Client
    Packet read(sockaddr_in* readingAddr){
        // Not sure if necessary; also it's causing problems with concurrent reading and sending
        // If it is necessary, we could:
        // - Use a socket for sending and one for receiving
        // - Find a way to perform recvfrom non-blocking
        //std::lock_guard<std::mutex> lck(_socketMutex);

        if(_valid){
            std::vector<char> buff(sizeof(Packet));
            recvfrom(_sockfd, (void*) &buff[0], sizeof(Packet), 0, (struct sockaddr *) readingAddr, (socklen_t*) &_readingLen);
            memcpy(&_buffer, &buff[0], sizeof(Packet));
            return _buffer;
        } else{
           throw std::runtime_error("Socket is not valid");
        }
    }

    // Server
    int send(std::shared_ptr<Packet> packet, sockaddr_in* sendingAddress){
        // Not sure if necessary; also it's causing problems with concurrent reading and sending
        // If it is necessary, we could:
        // - Use a socket for sending and one for receiving
        // - Find a way to perform recvfrom non-blocking
        //std::lock_guard<std::mutex> lck(_socketMutex);

        if(_valid){
            char * b = reinterpret_cast<char*> (packet.get());
            return sendto(_sockfd, b, sizeof(Packet), 0, (sockaddr *) sendingAddress, sizeof(sockaddr));
        } else{
           throw std::runtime_error("Socket is not valid");
        }
    }

    // Client
    int send(std::shared_ptr<Packet> packet){
        // Not sure if necessary; also it's causing problems with concurrent reading and sending
        // If it is necessary, we could:
        // - Use a socket for sending and one for receiving
        // - Find a way to perform recvfrom non-blocking
        //std::lock_guard<std::mutex> lck(_socketMutex);

        if(_valid){
            char * b = reinterpret_cast<char*>(packet.get());
            return sendto(_sockfd, b, sizeof(Packet), 0, (sockaddr *) &_readingAddr, sizeof(sockaddr));
        } else{
            std::cout << "ERROR" << std::endl;
            throw std::runtime_error("Socket is not valid");
        }
    }

    std::string getClientAddressString(void){
        return std::string(std::to_string(_readingAddr.sin_addr.s_addr) + ":" + std::to_string(_readingAddr.sin_port));
    }

    sockaddr_in& getReadingAddress(void){
        return _readingAddr;
    }
};

}
