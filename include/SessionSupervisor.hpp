#pragma once

#include <vector>
#include <thread>
#include <string>
#include <mutex>
#include <map>

#include "Socket.hpp"

namespace dropbox{

// TODO: save this user's data in a file

///
/// \brief The SessionSupervisor class manages the sessions. Each SessionSupervisor is responsible
/// for a single username.
///
template<class ServerSession>
class SessionSupervisor{

private:
    std::map<std::string, std::shared_ptr<ServerSession>> _sessions;
    std::string _username;
    std::mutex _mutex;

public:
    SessionSupervisor(std::string& username){
        _username = username;
    }

    void addSession(std::pair<std::string, std::shared_ptr<ServerSession>> newSession){
        std::lock_guard<std::mutex> lck(_mutex);
        _sessions.insert(newSession);
    }

    void removeSession(std::string address){
        std::lock_guard<std::mutex> lck(_mutex);
        auto it = _sessions.find(address);
        if(it != _sessions.end()){
            _sessions.erase(it);
        }
    }

    // Needs to be in a separate a separate file to have accesss to ServerSession methods
    void sendPacket(std::string senderAddress, std::shared_ptr<Packet> packet){
        for(auto it : _sessions){
            if(it.first != senderAddress){
                it.second->onAnotherSessionMessage(packet);
            }
        }
    }

};

}
