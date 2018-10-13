#pragma once

#include <memory>
#include <math.h>
#include <stdio.h>
#include <cstring>
#include "Session.hpp"
#include "FileManager.hpp"
#define BUFFER_MAX_SIZE 256

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
            receiveFile(packet);
            _packetNum++;

        } else if(packet->type == PacketType::LOGIN){
            _loggedIn = true;

            std::string directory = std::string("./") + std::string(packet->buffer);
            fileMgr.check_dir(directory);
            if (fileMgr.is_valid()){
                /* TODO: create a login return message
                  this login return message must tell the client if it
                  successfully logged in */
                // sendMessageServer();
                // std::cout << "Login return sent!" << std::endl;

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
        } else if(packet->type == PacketType::DOWNLOAD){
          std::cout << "download" << std::endl;
          sendFile(packet->filename);
        }
    }

    string buildFullPath(char filename[FILENAME_MAX_SIZE]){
      string fullPath (fileMgr.getPath());
      fullPath += "/";
      fullPath += filename;
      return fullPath;
    }

    void receiveFile(std::shared_ptr<Packet> packet){
      //string fullPath = buildFullPath(packet->filename);
      string fname = string(packet->filename);
      std::string message(packet->buffer, packet->bufferLen);

      // ofstream f;
      // if (packet->fragmentNum == 0)
      //   f.open(fullPath);
      // else
      //   f.open(fullPath, fstream::app);
      //
      // f << message;
      // f.close();

      if (fileMgr.is_valid()){
        if (packet->fragmentNum == 0)
          fileMgr.create_file(fname, message);
        else
          fileMgr.append_file(fname, message);
      } else{
        std::cout << "Sync directory is invalid" << std::endl;
      }

    }

    bool sendFile(char filename[FILENAME_MAX_SIZE]){
      bool readFile = false;
      char buffer[BUFFER_MAX_SIZE];
      //string fullPath = buildFullPath(filename);
      //FILE * file = fopen(fullPath.c_str(), "r");
      int currentFragment = 0;

      if (!fileMgr.is_valid()){
        std::cout << "sync directory is invalid" << std::endl;
        return false;
      }

      //check if file exists
      // if (!file){
      //   std::cout << "No such file\n";
      //   return false;
      // }
      map<std::string, STAT_t> files = fileMgr.read_dir();
      if (files.find(filename)==files.end())
      {
        std::cout << "No such file" << std::endl;
        return false;
      }

      //get size
      // fseek(file, 0, SEEK_END);
      // int size = ftell(file);
      // fseek(file, 0, SEEK_SET);
      int size = files[filename].st_size;

      double nFragments = double(size)/double(BUFFER_MAX_SIZE);
      int ceiledFragments = ceil(nFragments);

      while(!readFile){
        //int amountRead = fread(buffer, 1, BUFFER_MAX_SIZE, file);
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
            int preturn = sendMessageServer(packet);
            if(preturn < 0) std::runtime_error("Error upon sending message to server: " + std::to_string(preturn));
            ack = waitAck(packet->packetNum);
          }
          _packetNum++;
          currentFragment++;
        // } else {
        //   readFile = true;
        }
        if (amountRead < BUFFER_MAX_SIZE-1) {
          readFile = true;
        }
      }
      return true;
    }


};

}
