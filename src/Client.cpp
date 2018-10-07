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

    _monitoringThread = std::thread([&]{
        std::string dir_name = std::string("./sync_") + std::string(username);
        FileMonitor fm(dir_name);
        if (!fm.is_valid())
        {
            throw std::runtime_error("couldn't find nor create sync directory ");
        }
        else
        {
            while(_running){
                map<string, FILE_MOD_t> m;
                m = fm.diff_dir();
                if (!m.empty())
                {
                    cout << "CHANGES!" << endl;
                    for (auto it = m.begin(); it!=m.end(); it++)
                    {
                        cout << it->first;
                        switch (it->second.mod){
                        case MOVED:
                            cout << " moved/created" << endl;
                            break;
                        case MODIFIED:
                            cout << " modified" << endl;
                            break;
                        case ERASED:
                            cout << " erased" << endl;
                            break;
                        }
                        if (it->second.mod!=ERASED){
                            cout << "\t tamanho: " << it->second.file_stat.st_size << endl;
                            cout << "\t mtime:" << ctime(&it->second.file_stat.st_mtime);
                            cout << "\t atime:" << ctime(&it->second.file_stat.st_atime);
                            cout << "\t ctime:" << ctime(&it->second.file_stat.st_ctime);
                        }
                    }
                    cout << endl;
                    sleep(3);
                }
            }
        }
    });

    _clientSession.start();
}

void Client::start(void){
}

void Client::stop(void){
    _running = false;
    _connected = false;

    if(_listeningThread.joinable()){
        _listeningThread.join();
    }
    if(_monitoringThread.joinable()){
        _monitoringThread.join();
    }
}


}
