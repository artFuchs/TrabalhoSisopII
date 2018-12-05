#include "RMSession.hpp"
#include "Server.hpp"

void dropbox::RMSession::receiveClientAddress(std::string username, sockaddr_in address){
    _server.receiveClientAddress(username, address);
}
