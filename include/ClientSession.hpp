#pragma once

#include <iostream>
#include <chrono>
#include <thread>
#include <stdio.h>
#include <cstring>
#include <math.h>
#include <fstream>
#include "Session.hpp"
#include "FileManager.hpp"
#define FILENAME_MAX_SIZE   256
#define BUFFER_MAX_SIZE 256

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
    FileManager fileMgr;

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


        fileMgr.check_dir(string("./sync_") + username);

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
          std::string message(packet->buffer, packet->bufferLen);
          std::cout << "Received: " << message << "Sending back the message..." << std::endl;
          downloadFile(packet);
          _packetNum++;
        } else{
            std::cout << "ACK received " <<"\n";
        }
    }

    bool uploadFile(char filename[FILENAME_MAX_SIZE]){
      bool readFile = false;
      char buffer[BUFFER_MAX_SIZE];
      int currentFragment = 0;

      if (!fileMgr.is_valid())
      {
        std::cout << "Directory is invalid" << std::endl;
        return false;
      }

      map<std::string, STAT_t> files = fileMgr.read_dir();
      if (files.find(filename)==files.end())
      {
        std::cout << "No such file" << std::endl;
        return false;
      }
      //get size
      int size = files[filename].st_size;

      double nFragments = double(size)/double(BUFFER_MAX_SIZE);
      int ceiledFragments = ceil(nFragments);

      while(!readFile){
        int amountRead = fileMgr.read_file(filename,buffer,BUFFER_MAX_SIZE);
        if (amountRead > 0){
          bool ack = false;
          std::shared_ptr<Packet> packet(new Packet);
          packet->type = PacketType::DATA;
          packet->packetNum = _packetNum;
          packet->fragmentNum = currentFragment;
          packet->totalFragments = ceiledFragments;
          packet->bufferLen = amountRead;
          packet->pathLen = strlen(filename);
          strcpy(packet->buffer, buffer);
          strcpy(packet->filename, filename);
          while(!ack){
            int preturn = sendMessageClient(packet);
            if(preturn < 0) std::runtime_error("Error upon sending message to server: " + std::to_string(preturn));
            ack = waitAck(packet->packetNum);
          }
          _packetNum++;
          currentFragment++;
        }
        if (amountRead < BUFFER_MAX_SIZE) {
          readFile = true;
        }
      }
      return true;
    }

    void downloadFile(std::shared_ptr<Packet> packet){
      std::string message(packet->buffer, packet->bufferLen);

      fileMgr.create_file_part(packet->filename, packet->buffer, packet->fragmentNum, packet->totalFragments);

      if (packet->fragmentNum == packet->totalFragments-1)
      {
        fileMgr.join_files(packet->filename);
        fileMgr.clean_parts(packet->filename);
      }
    }

    void requestDownload(char filename[FILENAME_MAX_SIZE]){
      std::shared_ptr<Packet> packet(new Packet);
      packet->type = PacketType::DOWNLOAD;
      strcpy(packet->filename, filename);
      sendMessageClient(packet);
    }
};

}
