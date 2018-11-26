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
                packet->packetNum = _packetNum;
                _id = packet->id;  // packet ID must carry the id of session
                packet->id = _server_id;  // packet ID tells what RM sended it
                memcpy(packet->buffer, &_id, sizeof(_id));  // set an ID to the secondary RM
                packet->bufferLen = sizeof(_id);

                // sends the message until get ACK
                bool ack = false;
                while (!ack)
                {
                    std::cout << "sending ID " << _id << " to the secondary RM" << std::endl;;
                    int preturn = sendMessageServer(packet);
                    ack = waitAck(packet->packetNum);
                }
                _packetNum++;
            }else{
                _id = packet->id; // ID of the RM that this RMSession is communicating with
                memcpy(&_server_id, packet->buffer, sizeof(_id)); // this server ID
                std::cout << "received ID: " << _server_id << std::endl;
            }
        }
        else if (packet->type == PacketType::ACK)
        {
            std::cout << "RM received ACK: " << packet->packetNum << std::endl;
        }

    }

    //[nonblocking] try to send a LOGIN message to an primary RM
    void connect(){
        std::shared_ptr<Packet> packet(new Packet);
        packet->type = PacketType::LOGIN;
        packet->id = _server_id;
        int preturn = sendMessageServer(packet);
        if(preturn < 0) std::runtime_error("Error upon sending message to client: " + std::to_string(preturn));
    }

    int getServerID(){
        return _server_id;
    }


};


}
