#pragma once

#include <memory>

#include "Session.hpp"

namespace dropbox{

///
/// \brief ServerSession is the abstraction of a connection of the server with a client via UDP
///
class ServerSession : public Session<true>{

public:

    ServerSession(UDPSocket& socket) : Session<true>(socket){
    }

    void stop(void){
    }
    
    void onSessionReadMessage(std::shared_ptr<Packet> packet) override{
        std::string message(packet->buffer, packet->bufferLen);
        std::cout << "Received: " << message << "Sending back the message..." << std::endl;
        
        // As this is just a ping, we use the same packet that was sent to the server
        if(sendMessageServer(packet) < 0){
            // We could drop the connection here
            std::cout << "Error sending a message to the client" << std::endl;
        } else{
            std::cout << "Message successfully sent!" << std::endl;
        }
    }

};

}
