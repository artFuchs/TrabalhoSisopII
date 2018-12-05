#pragma once

#include <map>
#include <mutex>
#include <thread>
#include <atomic>

#include "Socket.hpp"


namespace dropbox{

class Server;

typedef std::map<int, struct sockaddr_in> AddressList;

class ElectionManager{

private:
    int& _id;
    AddressList& _addresses;
    UDPSocket& _socket;

    Server& _server;

    bool _runningElection;
    std::thread _electionThread;
    std::mutex _electionMutex;

    // If we receive any answer, we give up on the ElectionManager
    // So we don't need to know which RM has answered us
    // On the other hand, we do need to know which RM is the new coordinator
    int _coordinatorID;
    std::atomic<bool> _receivedCoordinator;
    std::atomic<bool> _receivedAnswer;

    // In milliseconds
    unsigned int _waitAnswerTimeout;
    unsigned int _waitCoordinatorTimeout;
    unsigned int _afterElectionWaitTime;    // To avoid receiving election message
                                            // just after finishing the election

public:
    ElectionManager(Server& server, UDPSocket& socket, AddressList& al, int& id) :
    _server(server), _socket(socket), _addresses(al), _id(id){
        _receivedAnswer = false;
        _receivedCoordinator = false;
        _runningElection = false;

        _waitAnswerTimeout = 500;
        _waitCoordinatorTimeout = 500;
        _afterElectionWaitTime = 500;

        _electionThread = std::thread([&]{
                while(true){
                    // Waits until _runningElection is set to true
                    while(!_runningElection);
                    bool hasWon = false;

                    while(true){
                        // Edge case where there is no RM with smaller id
                        if(checkSmallerRMID()){
                            sendCoordinator();
                            hasWon = true;
                            break;
                        }

                        // Send election to the other RMs
                        sendElection();

                        if(waitAnswer()){
                            // Received an answer, waits for a coordinator
                            // If no message was received, reinitiates the election
                            if(waitCoordinator()){
                                break;
                            }
                        } else{
                            // No answer received, this RM is the new coordinator
                            sendCoordinator();
                            hasWon = true;
                            break;
                        }
                    }

                    if(hasWon){
                        onElectionWon();
                    }

                    // Doesn't seem necessary
                    // std::this_thread::sleep_for(std::chrono::milliseconds(_afterElectionWaitTime));
                    _receivedAnswer = false;
                    _receivedCoordinator = false;
                    _runningElection = false;
                }
            });
    }

    // TODO: receive the id of the RM that has failed to handle
    // the failure of multiple coordinators while _electionThread
    // is running an election (unlikely)
    void election(){
        _runningElection = true;
    }

    void sendElection(){
        for(auto it : _addresses){
            // Only send message to the RMs with smaller id
            if(it.first < _id){
                sendElection(it.second);
            }
        }
    }

    void sendCoordinator(){
        for(auto it : _addresses){
            sendCoordinator(it.second);
        }
    }

    void sendAnswer(int id){
        auto it = _addresses.find(id);

        if(it != _addresses.end()){
            sendAnswer(it->second);
        }
    }

    void sendElection(struct sockaddr_in address){
        std::shared_ptr<Packet> packet(new Packet);
        packet->type = PacketType::ELECTION;
        packet->id = _id;
        _socket.send(packet, &address);
    }

    void sendAnswer(struct sockaddr_in address){
        std::shared_ptr<Packet> packet(new Packet);
        packet->type = PacketType::ANSWER;
        packet->id = _id;
        _socket.send(packet, &address);
    }

    void sendCoordinator(struct sockaddr_in address){
        std::shared_ptr<Packet> packet(new Packet);
        packet->type = PacketType::COORDINATOR;
        packet->id = _id;
        _socket.send(packet, &address);
    }

    void onElection(std::shared_ptr<Packet> packet){
        sendAnswer(packet->id);

        if(!_runningElection){
            election();
        }
    }

    void onAnswer(std::shared_ptr<Packet> packet){
        _receivedAnswer = true;
    }

    void onCoordinator(std::shared_ptr<Packet> packet){
        _coordinatorID = packet->id;
        _receivedCoordinator = true;
    }

    bool waitAnswer(){
        std::this_thread::sleep_for(std::chrono::milliseconds(_waitAnswerTimeout));
        return _receivedAnswer;
    }

    bool waitCoordinator(){
        std::this_thread::sleep_for(std::chrono::milliseconds(_waitCoordinatorTimeout));
        return _receivedCoordinator;
    }

    // Returns true if there is no RM with smaller id, false otherwise
    bool checkSmallerRMID(){
        for(auto it : _addresses){
            if(it.first < _id){
                return false;
            }
        }

        return true;
    }

    void onElectionWon();
};

};
