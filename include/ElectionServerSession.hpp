#include "Session.hpp"

namespace dropbox {

class ElectionServerSession : public Session<true> {
private:
    UDPSocket _socket;
    std::thread _listenThread;
public:
    ElectionServerSession(struct sockaddr_in addr) : _socket(addr), Session<true>(_socket){
        _listenThread = std::thread(&ElectionServerSession::listen, this);
    }

    void onSessionReadMessage(std::shared_ptr<Packet> packet){
        Session<true>::onSessionReadMessage(packet);
    }

    void listen(){
        while(1){
            std::shared_ptr<Packet> packet(new Packet);
            *packet = _socket.read();

            // Checks if there was an error upon reading the next packet
            if(packet->bufferLen < 0){
                std::cout << "Error while trying to read from socket" << std::endl;
                break;
            } else{

            }
        }
    }
};

}
