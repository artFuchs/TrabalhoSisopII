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
    std::mutex _modifyingDirectory;
    uint32_t _packetNum;
    bool _running;

public:

    ClientSession(UDPSocket& socket) : Session<false>(socket){
        _packetNum = 0;
    }

    ~ClientSession(void){
    }

    void connect(const char* username){
        std::string message = username;
        std::shared_ptr<Packet> packet(new Packet);

        bzero(packet->buffer, BUFFER_MAX_SIZE);
        packet->type = PacketType::LOGIN;
        packet->packetNum = _packetNum;
        _packetNum++;
        packet->bufferLen = message.size();
        memcpy(static_cast<void*>(packet->buffer), static_cast<const void*>(message.c_str()), message.size());

        try{
        bool messageSent = false;
        while(not messageSent){
            // Sends a message
            int preturn = sendMessageClient(packet);
            if(preturn < 0) std::runtime_error("Error upon sending message to server: " + std::to_string(preturn));

            // Waits for an ack
            messageSent = waitAck(packet->packetNum);
            }
        } catch(std::exception& e){
            std::cout << e.what() << std::endl;
        }           

        std::cout << "Session connected!" << std::endl;
    }

    void start(void){
        _running = true;

        _sendingThread = std::thread([&] {
            while(_running){
                //std::string message = "ping";
                //std::shared_ptr<Packet> packet(new Packet);
                //bzero(packet->buffer, BUFFER_MAX_SIZE);
                //packet->bufferLen = message.size();
                //memcpy(static_cast<void*>(packet->buffer), static_cast<const void*>(message.c_str()), message.size());
                //std::cout << "Sending message..." << std::endl;
                //int preturn = sendMessageClient(packet);
                //if(preturn < 0){
                //    std::cout << "Error upon sending message to server " << preturn << std::endl;
                //} else{
                //    std::cout << "Message successfully sent!" << std::endl;
                //}
                //std::cout << "Sleeping for 2 seconds..." << std::endl;
                //std::this_thread::sleep_for(std::chrono::milliseconds(2000));
                //std::cout << "Woke up!" << std::endl;
            }
        });
    }

    void stop(void){
        _running = false;

        if(_sendingThread.joinable()){
            _sendingThread.join();
        }
    }

    void onSessionReadMessage(std::shared_ptr<Packet> packet){
        Session<false>::onSessionReadMessage(packet);    // Handles ACK

        if(packet->type == PacketType::DATA){

        } else{
            std::cout << "ACK received" << std::endl;
        }
    }


};

}
