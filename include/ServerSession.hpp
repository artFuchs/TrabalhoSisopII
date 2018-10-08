#pragma once

#include <memory>

#include "Session.hpp"
#include "FileManager.hpp"

namespace dropbox{

///
/// \brief ServerSession is the abstraction of a connection of the server with a client via UDP
///
class ServerSession : public Session<true>{
private:
    bool _loggedIn;
    uint32_t _packetNum;
    FileManager fileMgr;

public:

    ServerSession(UDPSocket& socket) : Session<true>(socket){
        _loggedIn = false;
        _packetNum = 0;
    }

    void stop(void){
    }

    void onSessionReadMessage(std::shared_ptr<Packet> packet){
        Session<true>::onSessionReadMessage(packet);    // Handles ACK

        if(packet->type == PacketType::DATA){
            std::string message(packet->buffer, packet->bufferLen);
            std::cout << "Received: " << message << "Sending back the message..." << std::endl;

            // As this is just a ping, we use the same packet that was sent to the server
            packet->packetNum = _packetNum;
            if(sendMessageServer(packet) < 0){
                // We could drop the connection here
                std::cout << "Error sending a message to the client" << std::endl;
            } else{
                waitAck(packet->packetNum);
                std::cout << "Message successfully sent!" << std::endl;
                _packetNum++;
            }
        } else if(packet->type == PacketType::LOGIN){
            _loggedIn = true;

            std::string directory = std::string("./") + std::string(packet->buffer);
            fileMgr.check_dir(directory);
            if (!fileMgr.is_valid()){
                // TODO: create a login return message
                //sendMessageServer();
                //std::cout << "Login return sent!" << std::endl;

                map<string, STAT_t> files;
                files = fileMgr.read_dir();
                if (!files.empty())
                {
                    // TODO: send the files to the client
                }
            }
            else {
                std::cout << "Error opening/creating the user directory " << directory << std::endl;
            }

        } else if(packet->type == PacketType::ACK){
            std::cout << "Received an ACK!" << std::endl;
        }
    }
};

}
