#pragma once

#include "Session.hpp"
#include "ElectionManager.hpp"
#include "FileManager.hpp"

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
    FileManager fileMgr;
    char* username = nullptr;
    char user[BUFFER_MAX_SIZE];
    std::thread signalThread;
    std::thread dieslowlyThread;
    uint counter;
    bool alive;

    std::mutex _loginMutex;
    ElectionManager& _electionManager;

public:
    RMSession(ElectionManager& em, UDPSocket& socket, bool primary, int server_id = -1, char*user = nullptr) :
        Session<true>(socket), _electionManager(em){
        _packetNum = 0;
        _id = 0; //com qual RM a sessão se comunica
        _server_id = server_id; //id do servidor atual;
        _primary = primary;
        _otherPrimary = false;
        _connected = false;
        _signalRunning = false;
        _packetNum = 0;
        std::cout << "RMSession created - ";
        std::cout << "primary: " << _primary << std::endl;
        originalAddress = socket.getReadingAddress();
        username = user;
        alive = true;
    }

    void stop(void){}

    void onSessionReadMessage(std::shared_ptr<Packet> packet){
        Session<true>::onSessionReadMessage(packet); // Handles ACK
        counter++;

        if(!_signalRunning){
            signalThread = std::thread(&RMSession::keepAlive,this);
            _signalRunning = true;
        }

        if (packet->type == PacketType::LOGIN_RM)
        {
            std::lock_guard<std::mutex> lck(_loginMutex);

            if (_primary && !_connected){
                std::cout << "RM recebi LOGIN_RM" << '\n';
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
                if (username == nullptr)
                  std::cout << "NULLPTR\n";

                if (username) {
                    std::cout << "Estou logado, deveria enviar coisas pros secundarios!" << '\n';
                    std::shared_ptr<Packet> loginPacket(new Packet);
                    loginPacket->type = PacketType::LOGIN;
                    loginPacket->packetNum = _packetNum;
                    strcpy(loginPacket->buffer,username);
                    bool ack1 = false;
                    while (!ack1)
                    {
                        std::cout << "RM sending ID " << _id << " to the secondary RM" << std::endl;;
                        int preturn = sendMessageServer(loginPacket);
                        ack1 = waitAck(loginPacket->packetNum);
                    }

                    sendAllFiles(username);
                }
                _connected = true;

                dieslowlyThread = std::thread(&RMSession::dieSlowly, this);
                _signalRunning = true;

            // if server is waiting for an ID
            } else if (!_connected && _server_id < 0){
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
        else if (packet->type == PacketType::SIGNAL && _primary)
        {
            counter++;
        }
        // ============
        else if(packet->type == PacketType::DATA){
            if (_primary){
                bool ack = false;
                while (!ack)
                {
                    int preturn = sendMessageServer(packet);
                    ack = waitAck(packet->packetNum);
                }
            } else{
                std::cout << "Data message!!!" << '\n';

                std::string file(packet->filename, packet->pathLen);
                uint part = packet->fragmentNum;
                uint total = packet->totalFragments;
                double percent = 100*(((double)part+1)/(double)total);
                std::cout << "Received: " << file << " (" << part+1 << " / " << total << ") - " << percent << "%" << std::endl;

                receiveFile(packet);
            }
            // _supervisor->sendPacket(_sessionAddress, packet);
            //_packetNum++;     // Should be increased when sending a message to the client

        } else if(packet->type == PacketType::LOGIN) {
            if (_primary){
                std::cout << "primario = loguei agora!" << '\n';
                bool ack = false;
                while (!ack)
                {
                    int preturn = sendMessageServer(packet);
                    ack = waitAck(packet->packetNum);
                }

                sendAllFiles(packet->buffer);

            } else if (_connected){

                std::cout << "Login message!!!" << '\n';

                std::string directory = std::string("./") + std::string(packet->buffer);
                strcpy(user, packet->buffer);
                username = user;
                fileMgr.check_dir(directory);
                if (fileMgr.is_valid()) {

                _packetNum++;
                }
                else {
                    std::cout << "Error opening/creating the user directory " << directory << std::endl;
                }
            }
        } else if (packet->type == PacketType::DELETE){
            if (_primary){
                bool ack = false;
                while (!ack)
                {
                    int preturn = sendMessageServer(packet);
                    ack = waitAck(packet->packetNum);
                }
            } else{
                std::cout << "delete " << packet->filename << std::endl;
                fileMgr.delete_file(string(packet->filename));
            }
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
            packet->packetNum = _packetNum;
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
        packet->type = PacketType::LOGIN_RM;
        packet->id = _server_id;
        packet->packetNum = 0;
        int preturn = sendMessageServer(packet);
        if(preturn < 0) std::runtime_error("Error upon sending message to client: " + std::to_string(preturn));
    }

    void keepAlive(){
        std::cout << "starting keep alive" << std::endl;
        uint last_counter;
        std::shared_ptr<Packet> packet(new Packet);
        packet->type = PacketType::SIGNAL;
        packet->packetNum = 0;
        packet->id = _server_id;
        while (_connected){
            last_counter = counter;
            int preturn = sendMessageServer(packet);
            if(preturn < 0) std::runtime_error("Error upon sending message to client: " + std::to_string(preturn));
            std::this_thread::sleep_for(std::chrono::seconds(2));
            if (last_counter == counter){
                std::cout << "the RM " << _id << " is probably dead." << std::endl;
                _connected = false;

                // If the primary RM fails, starts a new election
                if(_otherPrimary){
                    std::cout << "STARTING ELECTION" << std::endl;
                    _electionManager.election();
                }
            }
        }
    }

    void dieSlowly(){
        uint last_counter;
        std::cout << "dying slowly" << std::endl;
        while(_connected){
            std::cout << "I am alive" << std::endl;
            last_counter = counter;
            std::this_thread::sleep_for(std::chrono::seconds(4));
            if (last_counter == counter){
                std::cout << "died" << std::endl;
                alive = false;
                break;
            }
        }
    }


    bool isAlive(){
        return alive;
    }

    // Getters / Setters
    void setAddresses(AddressList addresses){
        _addresses = addresses;
    }

    int getID(){
        return _id;
    }

    // ============= ServerSession methods ===================

    std::string parsePath(char filename[FILENAME_MAX_SIZE]){
      //check if filename init by GLOBAL_TOKEN
      std::string path(filename);
      if (path.find(GLOBAL_TOKEN) != std::string::npos){
        //remove GLOBAL_TOKEN from string
        path.erase(0,std::string(GLOBAL_TOKEN).size());
      }
      return path;
    }

    void receiveFile(std::shared_ptr<Packet> packet){
      static std::string last_file;
      static uint waited_piece = 0;
      std::string fname = parsePath(packet->filename);

      if (fname == last_file && waited_piece != packet->fragmentNum){
        return;
      }

      if (fname != last_file)
      {
        last_file = fname;
        waited_piece = 0;
      }
      waited_piece++;

      if (waited_piece == packet->totalFragments)
        waited_piece = 0;
        std::cout << username << "\n";
        std::string directory = std::string("./") + std::string(username);
        std::cout << "DIRECTORY   "<<directory << "\n";
        fileMgr.check_dir(directory);

      if (fileMgr.is_valid()){
        if (packet->fragmentNum == 0){
          std::cout << "creating file " << fname << std::endl;
          fileMgr.create_file(fname, packet->buffer, packet->bufferLen);
        } else
          fileMgr.append_file(fname, packet->buffer, packet->bufferLen);
      } else{
        std::cout << "Sync directory is invalid" << std::endl;
      }
    }

    bool sendFile(char filename[FILENAME_MAX_SIZE]){
      std::string fname = parsePath(filename);

      bool readFile = false;
      char buffer[BUFFER_MAX_SIZE];
      int currentFragment = 0;

      if (!fileMgr.is_valid()){
        std::cout << "sync directory is invalid" << std::endl;
        return false;
      }

      map<std::string, STAT_t> files = fileMgr.read_dir();
      if (files.find(fname)==files.end()){
        std::cout << "No such file" << std::endl;
        return false;
      }
      size_t size = files[fname].st_size;
      double nFragments = double(size)/double(BUFFER_MAX_SIZE);
      uint ceiledFragments = ceil(nFragments);

      while(!readFile){
        int amountRead = fileMgr.read_file(fname,buffer,BUFFER_MAX_SIZE);
        if (amountRead > 0){
          bool ack = false;
          std::shared_ptr<Packet> packet(new Packet);
          packet->type = PacketType::DATA;
          packet->packetNum = _packetNum;
          packet->fragmentNum = currentFragment;
          packet->totalFragments = ceiledFragments;
          packet->bufferLen = amountRead;
          packet->pathLen = strlen(filename);
          memcpy(packet->buffer, buffer, amountRead);
          strcpy(packet->filename, filename);
          while(!ack){
            int preturn = sendMessageServer(packet);
            if(preturn < 0) std::runtime_error("Error upon sending message to server: " + std::to_string(preturn));
            ack = waitAck(packet->packetNum);
          }
          _packetNum++;
          currentFragment++;
        }
        if (amountRead < BUFFER_MAX_SIZE-1) {
          readFile = true;
        }
      }
      return true;
    }

    void sendAllFiles(char *username) {
        std::cout << "username   " << username << "\n";
        std::string directory = std::string("./") + std::string(username);
        fileMgr.check_dir(directory);
        if (fileMgr.is_valid()){
            //send the files in the user folder
            map<string, STAT_t> files;
            files = fileMgr.read_dir();
            for (auto file = files.begin(); file != files.end(); file++)
            {
              char fname[FILENAME_MAX_SIZE];
              strcpy(fname,file->first.c_str());
              sendFile(fname);
            }
            _packetNum++;
        }
        else {
            std::cout << "Error opening/creating the user directory " << directory << std::endl;
        }
    }
};


}
