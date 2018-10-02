#pragma once

#include <iostream>
#include <chrono>
#include <thread>

#include <cstring>

#include "Session.hpp"

namespace dropbox{

///
/// \brief ClientSession is the abstraction of a connection of a client with a UDP server
///
class ClientSession : public Session<false>{

private:

    std::thread _sendingThread;

    bool _running;

public:

    ClientSession(UDPSocket& socket) : Session<false>(socket){
    }

    ~ClientSession(void){
    }

    void connect(void){
        std::string message = "connecting";
        std::shared_ptr<Packet> packet(new Packet);
        bzero(packet->buffer, 256);
        packet->bufferLen = message.size();
        memcpy(static_cast<void*>(packet->buffer), static_cast<const void*>(message.c_str()), message.size());
        int preturn = sendMessageClient(packet);
        if(preturn < 0){
            std::cout << "Error upon sending message to server " << preturn << std::endl;
        } else{
            std::cout << "Message successfully sent" << std::endl;
        }
    }

    void start(void){
        _running = true;

        _sendingThread = std::thread([&] { 
            while(_running){
                std::string message = "ping";
                std::shared_ptr<Packet> packet(new Packet);
                bzero(packet->buffer, 256);
                packet->bufferLen = message.size();
                memcpy(static_cast<void*>(packet->buffer), static_cast<const void*>(message.c_str()), message.size());
                std::cout << "Sending message..." << std::endl;
                int preturn = sendMessageClient(packet);
                if(preturn < 0){
                    std::cout << "Error upon sending message to server " << preturn << std::endl;
                } else{
                    std::cout << "Message successfully sent!" << std::endl;
                }
                std::cout << "Sleeping for 2 seconds..." << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(2000));
                std::cout << "Woke up!" << std::endl;
            }
        });
    }

    void stop(void){
        _running = false;

        if(_sendingThread.joinable()){
            _sendingThread.join();
        }
    }

    void onSessionReadMessage(std::shared_ptr<Packet> packet) override{
        std::string message(packet->buffer, packet->bufferLen);
        std::cout << "Received: " << std::endl << message << std::endl;
    }


};

}
