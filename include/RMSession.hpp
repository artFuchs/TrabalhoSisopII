#pragma once

#include "Session.hpp"

namespace dropbox{

class RMSession : public Session<true>{
private:
    uint32_t _packetNum;
    int id;
    std::string _sessionAddress;

public:
    RMSession(std::string& sessionAddress, UDPSocket& socket) :
        Session<true>(socket), _sessionAddress(sessionAddress){
        _packetNum = 0;
    }

    void stop(void){}

    void onSessionReadMessage(std::shared_ptr<Packet> packet){
        Session<true>::onSessionReadMessage(packet); // Handles ACK

        if (packet->type == PacketType::LOGIN)
        {

        }

    }



};


}
