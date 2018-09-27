#pragma once

#include "Session.hpp"

namespace dropbox{

///
/// \brief ClientSession is the abstraction of a connection of a client with a UDP server
///
class ClientSession : public Session<false>{

private:

public:
    ServerSession(UDPSocket& socket) : Session<true>(socket){
    }

    void stop(void){

    }

    void onSessionReadMessage(const char* buffer, int size) override{

    }


};

}
