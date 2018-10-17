#pragma once

#include <memory>
#include <math.h>
#include <stdio.h>
#include <cstring>
#include "Session.hpp"
#include "SessionSupervisor.hpp"
#include "FileManager.hpp"
//#define BUFFER_MAX_SIZE 256

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
        //std::string message(packet->buffer, packet->bufferLen);
        //std::cout << "Another client sent me: " << message << std::endl;
        if (packet->type == PacketType::DATA){
            if (packet->fragmentNum == packet->totalFragments-1)
                sendFile(packet->filename);
        }
        else if (packet->type == PacketType::DELETE){
          //send packet to client
          bool ack = false;
          while(!ack){
              int preturn = sendMessageServer(packet);
              if(preturn < 0) std::runtime_error("Error upon sending message to client: " + std::to_string(preturn));
              ack = waitAck(packet->packetNum);
          }
        }
    }

    void onSessionReadMessage(std::shared_ptr<Packet> packet){
        Session<true>::onSessionReadMessage(packet);    // Handles ACK

        if(packet->type == PacketType::DATA){
            std::string file(packet->filename, packet->pathLen);
            uint part = packet->fragmentNum;
            uint total = packet->totalFragments;
            double percent = 100*(((double)part+1)/(double)total);
            std::cout << "Received: " << file << " (" << part+1 << " / " << total << ") - " << percent << "%" << std::endl;

            receiveFile(packet);
            _supervisor->sendPacket(_sessionAddress, packet);
            //_packetNum++;     // Should be increased when sending a message to the client

        } else if(packet->type == PacketType::LOGIN){
            _loggedIn = true;

            std::string directory = std::string("./") + std::string(packet->buffer);
            fileMgr.check_dir(directory);
            if (fileMgr.is_valid()){
                //send the files in the user folder
                map<string, STAT_t> files;
                files = fileMgr.read_dir();
                for (auto file = files.begin(); file != files.end(); file++)
                {
                  char fname[FILENAME_MAX_SIZE];
                  strcpy(fname,file->first.c_str());
                  sendFile(fname);
                }

                // sends a positive login message
                // just return the packet
                bool ack = false;
                while(!ack){
                  int preturn = sendMessageServer(packet);
                  if(preturn < 0) std::runtime_error("Error upon sending message to client: " + std::to_string(preturn));
                  ack = waitAck(packet->packetNum);
                }
                _packetNum++;
            }
            else {
                std::cout << "Error opening/creating the user directory " << directory << std::endl;
            }

        } else if(packet->type == PacketType::ACK){
            std::cout << "Received an ACK! " << packet->packetNum << std::endl;
        } else if(packet->type == PacketType::DOWNLOAD){
          std::cout << "download" << std::endl;
          sendFile(packet->filename);
        } else if (packet->type == PacketType::DELETE){
          std::cout << "delete " << packet->filename << std::endl;
          fileMgr.delete_file(string(packet->filename));
          _supervisor->sendPacket(_sessionAddress, packet);
        }else if(packet->type == PacketType::EXIT){
          std::cout << "exit signal received" << std::endl;
          _supervisor->removeSession(_sessionAddress);
        }
    }

    std::string parsePath(char filename[FILENAME_MAX_SIZE]){
      //check if filename init by GLOBAL_TOKEN
      std::string path(filename);
      if (path.find(GLOBAL_TOKEN) != std::string::npos){
        //remove GLOBAL_TOKEN from string
        path.erase(0,std::string(GLOBAL_TOKEN).size());
      }
      return path;
    }

    void receiveFile(std::shared_ptr<Packet> packet){
      static std::string last_file;
      static int last_piece = 0;
      std::string fname = parsePath(packet->filename);

      if (fname == last_file && last_piece == packet->fragmentNum){
        return;
      }
      last_file = fname;
      last_piece = packet->fragmentNum;

      if (fileMgr.is_valid()){
        if (packet->fragmentNum == 0){
          std::cout << "creating file " << fname << std::endl;
          fileMgr.create_file(fname, packet->buffer, packet->bufferLen);
        } else
          fileMgr.append_file(fname, packet->buffer, packet->bufferLen);
      } else{
        std::cout << "Sync directory is invalid" << std::endl;
      }
    }

    bool sendFile(char filename[FILENAME_MAX_SIZE]){
      std::string fname = parsePath(filename);

      bool readFile = false;
      char buffer[BUFFER_MAX_SIZE];
      int currentFragment = 0;

      if (!fileMgr.is_valid()){
        std::cout << "sync directory is invalid" << std::endl;
        return false;
      }

      map<std::string, STAT_t> files = fileMgr.read_dir();
      if (files.find(fname)==files.end()){
        std::cout << "No such file" << std::endl;
        return false;
      }
      size_t size = files[fname].st_size;
      double nFragments = double(size)/double(BUFFER_MAX_SIZE);
      uint ceiledFragments = ceil(nFragments);

      while(!readFile){
        int amountRead = fileMgr.read_file(fname,buffer,BUFFER_MAX_SIZE);
        if (amountRead > 0){
          bool ack = false;
          std::shared_ptr<Packet> packet(new Packet);
          packet->type = PacketType::DATA;
          packet->packetNum = _packetNum;
          packet->fragmentNum = currentFragment;
          packet->totalFragments = ceiledFragments;
          packet->bufferLen = amountRead;
          packet->pathLen = strlen(filename);
          memcpy(packet->buffer, buffer, amountRead);
          strcpy(packet->filename, filename);
          while(!ack){
            int preturn = sendMessageServer(packet);
            if(preturn < 0) std::runtime_error("Error upon sending message to server: " + std::to_string(preturn));
            ack = waitAck(packet->packetNum);
          }
          _packetNum++;
          currentFragment++;
        }
        if (amountRead < BUFFER_MAX_SIZE-1) {
          readFile = true;
        }
      }
      return true;
    }


};

}
