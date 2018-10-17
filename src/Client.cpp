#include <iostream>
#include <exception>

#include "Client.hpp"
#include "FileMonitor.hpp"

namespace dropbox{

  Client::Client(const char* username, const char* hostname, int port) : _listenSocket(hostname, port), _clientSession(_listenSocket){
      _running = true;
      _connected = false;

      _clientSession.setReceiverAddress(_listenSocket.getReadingAddress());

      _listeningThread = std::thread([&] {

          while(_running){
              std::shared_ptr<Packet> packet(new Packet);
              sockaddr_in ser;

              *packet = _listenSocket.read(&ser);

              // Checks if there was an error upon reading the next packet
              if(packet->bufferLen < 0){
                  std::cout << "Error while trying to read from socket" << std::endl;
                  break;
              } else{
                  if(!_connected){
                      _connected = true;
                  }

                  _clientSession.onSessionReadMessage(packet);
              }
          }
      });

      _clientSession.connect(username);   // We need to be receiving messages before attempting to connect


      _clientSession.start();
  }

void Client::start(void){

}

void Client::stop(void){
    _running = false;
    _connected = false;

    _clientSession.exitSession();

    if(_listeningThread.joinable()){
        _listeningThread.join();
    }

}

void Client::upload(char filename[FILENAME_MAX_SIZE]){
  std::cout << "upload signal send" << std::endl;
  char _filename[FILENAME_MAX_SIZE];
  std::string fn = GLOBAL_TOKEN + std::string(filename);
  strcpy(_filename, fn.c_str());
  _clientSession.uploadFile(_filename);
}

void Client::download(char filename[]){
  char _filename[FILENAME_MAX_SIZE];
  std::string fn = GLOBAL_TOKEN + std::string(filename);
  strcpy(_filename, fn.c_str());
  _clientSession.requestDownload(_filename);
}

void Client::deleteFile(char filename[]){
  char _filename[FILENAME_MAX_SIZE];
  std::string fn = GLOBAL_TOKEN + std::string(filename);
  strcpy(_filename, fn.c_str());
  _clientSession.deleteFile(_filename);
}

void Client::listSrv(){
  _clientSession.listServer();
}

void Client::listCli(){
  _clientSession.listClient();
}

void Client::getSyncDir(){
  _clientSession.getSyncDir();
}

}
