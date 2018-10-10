#pragma once

#include <memory>
#include <math.h>
#include <stdio.h>
#include <cstring>
#include "Session.hpp"
#include "SessionSupervisor.hpp"
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
    std::string _sessionAddress;
    std::shared_ptr<SessionSupervisor<ServerSession>> _supervisor;

public:

    ServerSession(std::string& sessionAddress, UDPSocket& socket, std::shared_ptr<SessionSupervisor<ServerSession>> supervisor) :
        Session<true>(socket), _supervisor(supervisor), _sessionAddress(sessionAddress){
        _loggedIn = false;
        _packetNum = 0;
    }

    void stop(void){
    }

    void onAnotherSessionMessage(std::shared_ptr<Packet> packet){
        std::string message(packet->buffer, packet->bufferLen);
        std::cout << "Another client sent me: " << message << std::endl;
    }

    void onSessionReadMessage(std::shared_ptr<Packet> packet){
        Session<true>::onSessionReadMessage(packet);    // Handles ACK

        if(packet->type == PacketType::DATA){
            std::string message(packet->buffer, packet->bufferLen);
            std::cout << "Received: " << message << "Sending back the message..." << std::endl;
            receiveFile(packet);
            _supervisor->sendPacket(_sessionAddress, packet);
            //_packetNum++;     // Should be increased when sending a message to the client

        } else if(packet->type == PacketType::LOGIN){
            _loggedIn = true;

            std::string directory = std::string("./") + std::string(packet->buffer);
            fileMgr.check_dir(directory);
            if (fileMgr.is_valid()){
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
        } else if(packet->type == PacketType::DOWNLOAD){
          sendFile(packet->filename);
        } else if(packet->type == PacketType::EXIT){

        }
    }

    string buildFullPath(char filename[FILENAME_MAX_SIZE]){
      string fullPath (fileMgr.getPath());
      fullPath += "/";
      fullPath += filename;
      return fullPath;
    }

    void receiveFile(std::shared_ptr<Packet> packet){
      string fullPath = buildFullPath(packet->filename);

      std::string message(packet->buffer, packet->bufferLen);

      ofstream f;
      if (packet->fragmentNum == 0)
        f.open(fullPath);
      else
        f.open(fullPath, fstream::app);

      f << message;
      f.close();
    }

    bool sendFile(char filename[FILENAME_MAX_SIZE]){
      bool readFile = false;
      char buffer[BUFFER_MAX_SIZE];
      string fullPath = buildFullPath(filename);
      FILE * file = fopen(fullPath.c_str(), "r");
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
            int preturn = sendMessageServer(packet);
            if(preturn < 0) std::runtime_error("Error upon sending message to server: " + std::to_string(preturn));
            ack = waitAck(packet->packetNum);
          }
          _packetNum++;
          currentFragment++;
        } else {
          readFile = true;
        }

      }
      return true;
    }


};

}
