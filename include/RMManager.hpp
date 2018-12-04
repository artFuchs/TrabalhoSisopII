#pragma once

#include "RMSession.hpp"
#include <iostream>
#include <string>
#include <memory>
#include <vector>


namespace dropbox {

// typedef std::map<int, struct sockaddr_in> AddressList;

class RMManager {
private:

    bool _isPrimary;
    bool _loggedIn = false;
    char* _username;
    std::vector<std::shared_ptr<RMSession>> _rmSessions;

public:
    RMManager(bool isPrimary) {
        _isPrimary = isPrimary;
        _loggedIn = false;
        _username = nullptr;
    }

    void addRMSession(std::shared_ptr<RMSession> session) {
        _rmSessions.push_back(session);
    }

    void loggedIn(char* user){
        strcpy(_username, user);
        _loggedIn = true;
    }

    bool getLoggedIn(){
        return _loggedIn;
    }

    char* getUsername(){
        return _username;
    }

    void stop(void){}

    void onSessionReadMessage(std::shared_ptr<Packet> packet) {
        if (packet->type == PacketType::DATA || packet->type == PacketType::LOGIN || packet->type == PacketType::EXIT || packet->type == PacketType::DELETE) {
            sendToAllSessions(packet);
        }
    }

    void sendToAllSessions(std::shared_ptr<Packet> packet) {
        for (int i = 0; i < _rmSessions.size(); i++) {
            _rmSessions[i]->onSessionReadMessage(packet);
        }
    }
};
}
