#include "RMSession.hpp"
#include "Server.hpp"

void dropbox::RMSession::receiveClientAddress(std::string username, std::string address){
    _server.receiveClientAddress(username, address);
}
