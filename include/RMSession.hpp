#pragma once

#include "Session.hpp"

namespace dropbox{

class RMSession : public Session<true>{
private:
    uint32_t _packetNum;
    int _id;
    bool _primary;
    bool _connected;

public:
    RMSession(UDPSocket& socket, bool primary) :
        Session<true>(socket){
        _packetNum = 0;
        _id = 0;
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
              init_connection();
            }
            else if (!_connected){
              _id = stoi(packet->buffer);
              _connected = true;
              cout << "RM connection acomplished" << endl;
            }
            else
            {
              //get other IPs
            }
        }
        else if (packet->type == PacketType::ACK)
        {
            cout << "RM received ACK: " << packet->packetNum << endl;
        }

    }

    void init_connection(){
        bool ack = false;
        std::shared_ptr<Packet> packet(new Packet);

        packet->type = PacketType::LOGIN;
        packet->packetNum = _packetNum;
        std::string id = std::to_string(_id);
        strcpy(packet->buffer, id.c_str());
        while(!ack){
            cout << "Sending message" << endl;
            int preturn = sendMessageServer(packet);
            if(preturn < 0) std::runtime_error("Error upon sending message to server: " + std::to_string(preturn));
            ack = waitAck(packet->packetNum);
        }
        _packetNum++;
    }



};


}
