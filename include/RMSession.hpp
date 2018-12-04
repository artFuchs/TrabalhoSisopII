#pragma once

#include "Session.hpp"
#include "ElectionManager.hpp"

namespace dropbox{

class RMSession : public Session<true>{
private:
    uint32_t _packetNum;
    int _server_id;
    int _id;
    bool _primary;
    bool _otherPrimary;
    bool _connected;
    AddressList _addresses;
    struct sockaddr_in originalAddress;

    bool _signalRunning;
    std::thread signalThread;
    uint counter;

    ElectionManager& _electionManager;

public:
    RMSession(ElectionManager& em, UDPSocket& socket, bool primary, int server_id = -1) :
        Session<true>(socket), _electionManager(em){
        _packetNum = 0;
        _id = 0; //com qual RM a sessão se comunica
        _server_id = server_id; //id do servidor atual;
        _primary = primary;
        _otherPrimary = false;
        _connected = false;
        _signalRunning = false;
        std::cout << "RMSession created - ";
        std::cout << "primary: " << _primary << std::endl;
        originalAddress = socket.getReadingAddress();
    }

    void stop(void){}

    void onSessionReadMessage(std::shared_ptr<Packet> packet){
        Session<true>::onSessionReadMessage(packet); // Handles ACK
        counter++;
        
        if(!_signalRunning){
            signalThread = std::thread(&RMSession::keepAlive,this);
            _signalRunning = true;
        }
        
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
                    std::cout << "RM sending ID " << _id << " to the secondary RM" << std::endl;;
                    int preturn = sendMessageServer(packet);
                    ack = waitAck(packet->packetNum);
                }
                _packetNum++;

                // sends addresses from another RMs;
                sendAddressesList();
            // if server is waiting for an ID
            }else if (!_connected && _server_id < 0){
                _id = packet->id; // ID of the RM that this RMSession is communicating with
                memcpy(&_server_id, packet->buffer, sizeof(_id)); // this server ID
                std::cout << "RM received ID: " << _server_id << " from RM " << _id << std::endl;
                _connected = true;
            // if server has already an ID, smaller than the connecting server
            }else if (!_connected && _server_id < packet->id){ //
                _id = packet->id; // ID of the RM that this RMSession is communicating with
                std::cout << "RM received connecting request from RM " << _id << std::endl;
                packet->id = _server_id;
                packet->packetNum = _packetNum;

                bool ack = false;
                while (!ack)
                {
                    std::cout << "RM sending ID " << _id << " to the secondary RM" << std::endl;;
                    int preturn = sendMessageServer(packet);
                    ack = waitAck(packet->packetNum);
                }
                _connected = true;
                _packetNum++;
            }else if (!_connected){
                _id = packet->id;
                std::cout << "connected with RM " << _id << std::endl;
                _connected = true;
            }
            else{
                std::cout << "Strange... this session is already connected... but just sent a LOGIN request..." << std::endl;
                std::cout << "This is a work to Sherlock Holmes." << std::endl;
            }

        }
        else if (packet->type == PacketType::LIST)
        {
            int id;
            struct sockaddr_in address;
            memcpy(&id, packet->buffer, sizeof(id));
            memcpy(&address, packet->buffer+sizeof(id), sizeof(address));
            std::string address_str = std::to_string(address.sin_addr.s_addr) + ":" + std::to_string(address.sin_port);
            std::cout << "received {ID: " << id << ", ADDRESS: " << address_str << "}" << std::endl;
            _addresses[id]=address;
            // TODO: need to think of something to have certain that the other RM received the LOGIN message
            setReceiverAddress(address);
            connect(id);
            setReceiverAddress(originalAddress);
            _otherPrimary = true;
        }
        else if (packet->type == PacketType::ELECTION)
        {
            _electionManager.onElection(packet);
        }
        else if (packet->type == PacketType::ANSWER)
        {
            _electionManager.onAnswer(packet);
        }
        else if (packet->type == PacketType::COORDINATOR)
        {
            _electionManager.onCoordinator(packet);
            _otherPrimary = true;
        }
        else if (packet->type == PacketType::ACK)
        {
            std::cout << "RM received ACK: " << packet->packetNum << std::endl;
        }

    }

    void sendAddressesList()
    {
        std::shared_ptr<Packet> packet(new Packet);
        packet->type = PacketType::LIST;
        packet->packetNum = _packetNum;
        packet->id = _server_id;
        packet->totalFragments = _addresses.size();
        packet->fragmentNum = 0;

        for (auto it = _addresses.begin(); it!=_addresses.end(); it++)
        {
            bool ack = false;
            int id = it->first;
            struct sockaddr_in address =  it->second;

            memcpy(packet->buffer, &id, sizeof(id));
            memcpy(packet->buffer+sizeof(id), &address, sizeof(address));
            packet->bufferLen = sizeof(id) + sizeof(address);
            std::string address_str = std::to_string(address.sin_addr.s_addr) + ":" + std::to_string(address.sin_port);
            std::cout << "sending {ID: " << id << ", ADDRESS: " << address_str <<  "}" << std::endl;
            while(!ack){
                int preturn = sendMessageServer(packet);
                if(preturn < 0) std::runtime_error("Error upon sending message to client: " + std::to_string(preturn));
                ack = waitAck(_packetNum);
            }
            _packetNum++;
        }


    }

    // [nonblocking] try to send a LOGIN message to an primary RM
    void connect(int id = 0){
        std::shared_ptr<Packet> packet(new Packet);
        packet->type = PacketType::LOGIN;
        packet->id = _server_id;
        packet->packetNum = 0;
        int preturn = sendMessageServer(packet);
        if(preturn < 0) std::runtime_error("Error upon sending message to client: " + std::to_string(preturn));
    }

    void keepAlive(){
        cout << "starting keep alive" << endl;
        uint last_counter;
        std::shared_ptr<Packet> packet(new Packet);
        packet->type = PacketType::SIGNAL;
        packet->packetNum = 0;
        packet->id = _server_id;
        while (_connected){
            last_counter = counter;
            int preturn = sendMessageServer(packet);
            if(preturn < 0) std::runtime_error("Error upon sending message to client: " + std::to_string(preturn));
            std::this_thread::sleep_for(std::chrono::seconds(1));
            if (last_counter == counter){
                std::cout << "the RM " << _id << " is probably dead." << std::endl;
                _connected = false;

                // If the primary RM fails, starts a new election
                if(_otherPrimary){
                    _electionManager.election();
                }
            }
        }

    }

    // Getters / Setters
    void setAddresses(AddressList addresses){
        _addresses = addresses;
    }
};


}
