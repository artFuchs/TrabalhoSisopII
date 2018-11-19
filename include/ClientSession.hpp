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
    std::thread _monitoringThread; // thread that will monitor the folder;

    std::mutex _modifyingDirectory;

    uint32_t _packetNum;
    bool _running;
    bool _loggedIn;
    FileMonitor fileMgr;
    char* user = (char*)malloc(sizeof(char)*FILENAME_MAX_SIZE);
    map<string, STAT_t> serverFiles;

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
        strcpy(user, username);
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

        _monitoringThread = std::thread([&]{
            if (!fileMgr.is_valid())
            {
                throw std::runtime_error("couldn't find nor create sync directory ");
            }
            else
            {
                while(_running){

                    map<std::string, FILE_MOD_t> m;

                    {
                      std::lock_guard<std::mutex> lck(_modifyingDirectory);
                      m = fileMgr.diff_dir();
                    }

                    if (!m.empty())
                    {
                        std::cout << "CHANGES!" << endl;
                        for (auto it = m.begin(); it!=m.end(); it++)
                        {
                            char filename[FILENAME_MAX_SIZE];
                            std::cout << it->first;
                            switch (it->second.mod){
                            case MOVED:
                                std::cout << " moved/created" << std::endl;
                                strcpy(filename, it->first.c_str());
                                uploadFile(filename);
                                break;
                            case MODIFIED:
                                std::cout << " modified" << std::endl;
                                strcpy(filename, it->first.c_str());
                                uploadFile(filename);
                                break;
                            case ERASED:
                                std::cout << " erased" << std::endl;
                                strcpy(filename, it->first.c_str());
                                requestDelete(filename);
                                break;
                            }
                        }
                        std::cout << std::endl;
                    }
                    sleep(3);
                }
            }
        });
    }

    void stop(void){
        _running = false;

        if(_monitoringThread.joinable()){
            _monitoringThread.join();
        }
    }

    void onSessionReadMessage(std::shared_ptr<Packet> packet){
        Session<false>::onSessionReadMessage(packet);    // Handles ACK
        if(packet->type == PacketType::DATA){
            std::string file(packet->filename, packet->pathLen);
            uint part = packet->fragmentNum + 1;
            uint total = packet->totalFragments;
            double percent = 100*(((double)part)/(double)total);
            std::cout << "Received: " << file << " (" << part << " / " << total << ") - " << percent << "%" << std::endl;
            downloadFile(packet);
            _packetNum++;
        } else if (packet->type == PacketType::LOGIN){
            _loggedIn = true;
        } else if (packet->type == PacketType::DELETE){
            deleteFile(packet->filename);
        } else if (packet->type == PacketType::EXIT){
            stop();
        } else if (packet->type == PacketType::LIST) {
            uint part = packet->fragmentNum + 1;
            uint total = packet->totalFragments;
            double percent = 100*(((double)part)/(double)total);
            std::cout << "Received part of server files listing: " << " (" << part << " / " << total << ") - " << percent << "%" << std::endl;
            receiveServerFileInfo(packet);
        } else{
            // std::cout << "ACK received " <<"\n";
        }
    }

    std::string removePath(char filename[FILENAME_MAX_SIZE]){
        std::string path(filename);
        std::size_t last_bar = path.rfind("/");
        if (last_bar!=std::string::npos){
            path.erase(0,last_bar+1);
        }
        return path;
    }

    bool uploadFile(char _filename[]){
        std::string fname = removePath(_filename);
        std::string fpath = parsePath(_filename);


        bool readFile = false;
        char buffer[BUFFER_MAX_SIZE];
        ifstream file;

        try {
            file.open(fpath, std::fstream::binary);
        }catch (std::ifstream::failure e){
            std::cout << "No such file\n";
            return false;
        }

        //get size
        std::streampos begin, end;
        begin = file.tellg();
        file.seekg (0, ios::end);
        end = file.tellg();
        int size = end - begin;
        file.seekg(0, ios::beg);
        double nFragments = double(size)/double(BUFFER_MAX_SIZE);
        uint ceiledFragments = ceil(nFragments);
        uint currentFragment = 0;

        while(!readFile){
            file.read(buffer, BUFFER_MAX_SIZE);
            if (file.gcount() > 0)
            {
                bool ack = false;
                std::shared_ptr<Packet> packet(new Packet);
                packet->type = PacketType::DATA;
                packet->packetNum = _packetNum;
                packet->fragmentNum = currentFragment;
                packet->totalFragments = ceiledFragments;
                packet->bufferLen = file.gcount();
                packet->pathLen = fname.size();
                memcpy(packet->buffer, buffer, file.gcount());
                strcpy(packet->filename, fname.c_str());

                while(!ack){
                    int preturn = sendMessageClient(packet);
                    if(preturn < 0) std::runtime_error("Error upon sending message to server: " + std::to_string(preturn));
                    ack = waitAck(packet->packetNum);
                }
                _packetNum++;
                currentFragment++;
            } else{
                readFile = true;
            }
        }

        std::cout << "file succesfully sent to server" << std::endl;

        return true;
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
        std::lock_guard<std::mutex> lck(_modifyingDirectory);
        static std::string last_file;
        static uint waited_piece = 0;

        std::string filename = parsePath(packet->filename);

        if (filename == last_file && waited_piece != packet->fragmentNum){
          return;
        }
        if (filename != last_file)
        {
          last_file = filename;
          waited_piece = 0;
        }
        waited_piece++;

        if (waited_piece == packet->totalFragments)
          waited_piece = 0;

        std::ofstream f;
        if (packet->fragmentNum == 0)
        {
          f.open(filename, std::fstream::binary);
          f.write(packet->buffer, packet->bufferLen);
          f.close();
        }
        else
        {
          f.open(filename, std::fstream::binary | std::fstream::app);
          f.write(packet->buffer, packet->bufferLen);
          f.close();
        }

        fileMgr.read_dir();
    }

    void requestDownload(char filename[FILENAME_MAX_SIZE]){
        std::shared_ptr<Packet> packet(new Packet);
        packet->type = PacketType::DOWNLOAD;
        packet->pathLen = strlen(filename);
        strcpy(packet->filename, filename);
        sendMessageClient(packet);
    }

    void deleteFile(char filename[FILENAME_MAX_SIZE]){
        std::lock_guard<std::mutex> lck(_modifyingDirectory);
        fileMgr.delete_file(std::string(filename));
        fileMgr.read_dir();
    }

    void requestDelete(char filename[FILENAME_MAX_SIZE]){
        std::shared_ptr<Packet> packet(new Packet);
        packet->type = PacketType::DELETE;
        packet->packetNum = _packetNum;
        packet->pathLen = strlen(filename);
        _packetNum++;
        strcpy(packet->filename, filename);
        sendMessageClient(packet);
    }

    void exitSession(){
        std::shared_ptr<Packet> packet(new Packet);
        packet->type = PacketType::EXIT;
        packet->packetNum = _packetNum;
        _packetNum++;
        bool ack = false;
        while (!ack)
        {
            sendMessageClient(packet);
            ack = waitAck(packet->packetNum);
        }
        stop();
    }

    void getSyncDir(){
      fileMgr.check_dir(string("./sync_") + user);
      if (!fileMgr.is_valid())
          throw std::runtime_error("couldn't find nor create sync directory ");
    }

    void listClient(){
      if (fileMgr.is_valid()){
          map<string, STAT_t> files;
          files = fileMgr.read_dir();
          for (auto file = files.begin(); file != files.end(); file++)
          {
            std::cout << "nome: " << file->first << "\n";
            std::cout << "\t tamanho: " << file->second.st_size << std::endl;
            std::cout << "\t mtime:" << ctime(&file->second.st_mtime);
            std::cout << "\t atime:" << ctime(&file->second.st_atime);
            std::cout << "\t ctime:" << ctime(&file->second.st_ctime);
          }
      }
    }

    void requestListServer(){
      serverFiles.clear();
      std::cout << "requesting server information" << std::endl;
      std::shared_ptr<Packet> packet(new Packet);
      packet->type = PacketType::LIST;
      packet->packetNum = _packetNum;
      _packetNum++;
      bool ack = false;
      while (!ack)
      {
          sendMessageClient(packet);
          ack = waitAck(packet->packetNum);
      }
    }

    void receiveServerFileInfo(std::shared_ptr<Packet> packet){
      static uint waited_piece = 0;
      if (packet->fragmentNum == waited_piece)
      {
        string fname(packet->filename,packet->pathLen);
        STAT_t data;
        memcpy(&data, packet->buffer, packet->bufferLen);
        serverFiles[fname] = data;
        waited_piece++;
        if (waited_piece == packet->totalFragments)
        {
          std::cout << "terminated loading server files list." << std::endl;
          waited_piece = 0;
          ListServer();
        }
      }
    }

    void ListServer(){
      cout << "listing server..." << endl;
      for (auto file = serverFiles.begin(); file != serverFiles.end(); file++)
      {
        std::cout << "nome: " << file->first << "\n";
        std::cout << "\t tamanho: " << file->second.st_size << std::endl;
        std::cout << "\t mtime:" << ctime(&file->second.st_mtime);
        std::cout << "\t atime:" << ctime(&file->second.st_atime);
        std::cout << "\t ctime:" << ctime(&file->second.st_ctime);
      }
    }
};

}
