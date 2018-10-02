#pragma once

#include <memory>

#include "Socket.hpp"

namespace dropbox{

// We could use this class to handle keep-alive

///
/// \brief Session is the abstraction of a UDP connection
///
template<bool isServer>
class Session{

private:
    UDPSocket& _socket;
    sockaddr_in _receiverAddress;

protected:

    int sendMessageServer(std::shared_ptr<Packet> packet){
        return _socket.send(packet, &_receiverAddress);
    }

    int sendMessageClient(std::shared_ptr<Packet> packet){
        return _socket.send(packet);
    }

public:

    Session(UDPSocket& socket) : _socket(socket){
    }

    void setReceiverAddress(sockaddr_in& receiverAddress){
        _receiverAddress = receiverAddress;
    }

    virtual void onSessionReadMessage(std::shared_ptr<Packet> packet) = 0;
    
};

}


