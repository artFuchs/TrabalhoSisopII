#pragma once

#include "Session.hpp"

namespace dropbox{

class RMSession : public Session<true>{
private:
    uint32_t _packetNum;
    int _server_id;
    int _id;
    bool _primary;
    bool _connected;

public:
    RMSession(UDPSocket& socket, bool primary, int server_id = -1) :
        Session<true>(socket){
        _packetNum = 0;
        _id = 0; //com qual RM a sessão se comunica
        _server_id = server_id; //id do servidor atual;
        _primary = primary;
        _connected = false;
    }

    void stop(void){}

    void onSessionReadMessage(std::shared_ptr<Packet> packet){
        Session<true>::onSessionReadMessage(packet); // Handles ACK

        if (packet->type == PacketType::LOGIN)
        {
            if (_primary){
              cout << "RM received connection request" << endl;
              _id = (int)packet->id;
              init_connection();
            }
            else if (!_connected){
              // adquire id da seção e do servidor
              _id = (int)packet->id;
              memcpy(&_server_id, packet->buffer, sizeof(_server_id));
              _connected = true;
              std::cout << "RM connection acomplished" << std::endl;
            }
            else
            {
              //get other IPs
            }
        }
        else if (packet->type == PacketType::ACK)
        {
            std::cout << "RM received ACK: " << packet->packetNum << std::endl;
        }

    }

    void init_connection(){
        bool ack = false;
        std::shared_ptr<Packet> packet(new Packet);
        packet->type = PacketType::LOGIN;
        packet->packetNum = _packetNum;
        // if primary, this will have the primary server id
        // if secondary, this will have -1
        packet->id = _server_id;
        // if primary, this will inform the RM id
        // if secondary, this will just be ignored
        memcpy(packet->buffer, &_id, sizeof(_id));
        packet->bufferLen = sizeof(_id);
        while(!ack){
            int preturn = sendMessageServer(packet);
            if(preturn < 0) std::runtime_error("Error upon sending message to server: " + std::to_string(preturn));
            ack = waitAck(packet->packetNum);
        }

        _packetNum++;
    }



};


}
