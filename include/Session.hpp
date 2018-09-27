#pragma once

#include <memory>

#include "Socket.hpp"

namespace dropbox{

///
/// \brief Session is the abstraction of a UDP connection
///
template<bool isServer>
class Session{

public:
    Session(void){
    }

    virtual void onSessionReadMessage(std::shared_ptr<Packet> packet) = 0;
    virtual void sendMessage(std::shared_ptr<Packet> packet) = 0;

};

}

//#include "../src/Session.ipp"

