#pragma once

#include <memory>

#include "Socket.hpp"

namespace dropbox{

// We could use this class to handle keep-alive

///
/// \brief Session is the abstraction of a UDP connection
///
template<bool isServer>
class Session{

private:
    UDPSocket& _socket;
    sockaddr_in _receiverAddress;
    std::vector<uint32_t> _receivedAcks;        // It's a vector in case we want to make the send() asynchronous
                                                // In this case we should store the packet also

    bool checkAck(uint32_t packetNum){
        for(auto it = _receivedAcks.begin(); it != _receivedAcks.end(); it++){
            if(*it == packetNum){
                //std::cout << *it << "\n";
                _receivedAcks.erase(it);
                return true;
            }
        }

        return false;
    }

protected:

    bool waitAck(uint32_t packetNum, std::chrono::milliseconds totalSleepTime = std::chrono::milliseconds(1000)){
        const std::chrono::milliseconds sleepTime = std::chrono::milliseconds(10);
        bool receivedAck = false;

        while((not receivedAck) and (totalSleepTime > std::chrono::milliseconds(0))){
            std::this_thread::sleep_for(sleepTime);
            totalSleepTime -= sleepTime;
            receivedAck = checkAck(packetNum);
        }

        return receivedAck;
    }

    int sendMessageServer(std::shared_ptr<Packet> packet){
        return _socket.send(packet, &_receiverAddress);
    }

    int sendMessageClient(std::shared_ptr<Packet> packet){
        return _socket.send(packet);
    }



public:

    Session(UDPSocket& socket) : _socket(socket){
    }

    void setReceiverAddress(sockaddr_in& receiverAddress){
        _receiverAddress = receiverAddress;
    }

    virtual void onSessionReadMessage(std::shared_ptr<Packet> packet){
        if(packet->type == PacketType::ACK){
            _receivedAcks.push_back(packet->packetNum);
        } else{
            std::shared_ptr<Packet> ackPacket(new Packet);
            ackPacket->type = PacketType::ACK;
            ackPacket->packetNum = packet->packetNum;
            if(isServer){
                std::cout << "sending ACK" << std::endl;
                sendMessageServer(ackPacket);
            } else{
                sendMessageClient(ackPacket);
            }
        }
    }

};

}
