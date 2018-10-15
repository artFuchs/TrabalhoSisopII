#pragma once

#include <iostream>
#include <chrono>
#include <thread>
#include <stdio.h>
#include <cstring>
#include <math.h>
#include <fstream>
#include "Session.hpp"
#include "FileMonitor.hpp"
//#define FILENAME_MAX_SIZE   256
//#define BUFFER_MAX_SIZE 256

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
    bool _loggedIn;
    FileManager fileMgr;

public:

    ClientSession(UDPSocket& socket) : Session<false>(socket){
        _packetNum = 0;
        _running = false;
        _loggedIn = false;
    }

    ~ClientSession(void){
    }

    void connect(const char* username){
        std::string message = username;

        fileMgr.check_dir(string("./sync_") + username);
        if (!fileMgr.is_valid())
            throw std::runtime_error("couldn't find nor create sync directory ");

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

        // wait until _connected
        while (!_loggedIn){}

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

        //send EXIT message to server

        if(_sendingThread.joinable()){
            _sendingThread.join();
        }
    }

    void onSessionReadMessage(std::shared_ptr<Packet> packet){
        Session<false>::onSessionReadMessage(packet);    // Handles ACK
        if(packet->type == PacketType::DATA){
          //std::string message(packet->buffer, packet->bufferLen);
          std::string file(packet->filename, packet->pathLen);
          uint part = packet->fragmentNum;
          uint total = packet->totalFragments;
          double percent = 100*(((double)part+1)/(double)total);
          std::cout << "Received: " << file << " (" << part << " / " << total-1 << ") - " << percent << "%" << std::endl;
          //std::cout << "Received: " << message << "Sending back the message..." << std::endl;
          downloadFile(packet);
          _packetNum++;
        } else if (packet->type == PacketType::LOGIN){
          _loggedIn = true;
        } else{
          std::cout << "ACK received " <<"\n";
        }
    }

    bool uploadFile(char filename[FILENAME_MAX_SIZE]){
      bool readFile = false;
      char buffer[BUFFER_MAX_SIZE];
      FILE * file = fopen(filename, "r");
      int currentFragment = 0;

      //check if file exists
      if (!file){
        std::cout << "No such file\n";
        return false;
      }
      //get size
      fseek(file, 0, SEEK_END);
      int size = ftell(file);
      fseek(file, 0, SEEK_SET);

      double nFragments = double(size)/double(BUFFER_MAX_SIZE);
      int ceiledFragments = ceil(nFragments);

      while(!readFile){
        int amountRead = fread(buffer, 1, BUFFER_MAX_SIZE, file);
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
        } else {
          readFile = true;
        }
      }
    }


    std::string parsePath(char filename[FILENAME_MAX_SIZE]){
      //check if filename init by GLOBAL_TOKEN
      std::string path(filename);
      if (path.find(GLOBAL_TOKEN) != std::string::npos){
        //remove GLOBAL_TOKEN from string
        path.erase(0,std::string(GLOBAL_TOKEN).size());
      }
      else
      {
        path = fileMgr.getPath() + path;
      }
      return path;
    }

    void downloadFile(std::shared_ptr<Packet> packet){
      std::string message(packet->buffer, packet->bufferLen);

      std::string filename = parsePath(packet->filename);

      std::ofstream f;

      if (packet->fragmentNum == 0)
        f.open(filename);
      else
        f.open(filename, std::fstream::app);

      f << message;
      f.close();
    }

    void requestDownload(char filename[FILENAME_MAX_SIZE]){
      std::shared_ptr<Packet> packet(new Packet);
      packet->type = PacketType::DOWNLOAD;
      strcpy(packet->filename, filename);
      sendMessageClient(packet);
    }
};

}
