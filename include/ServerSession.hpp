#pragma once

#include <memory>

#include "Session.hpp"

namespace dropbox{

///
/// \brief ServerSession is the abstraction of a connection of the server with a client via UDP
///
class ServerSession : public Session<true>{

private:

public:
    ServerSession(sockaddr_in& clientAddress){
    }

    void stop(void){
    }
    
    void onSessionReadMessage(std::shared_ptr<Packet> packet){
        std::string message(packet->buffer, packet->packetLen);
        std::cout << "Received: " << message << std::endl;
    }

    void sendMessage(std::shared_ptr<Packet> packet){

    }


};

}
