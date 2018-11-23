#include <iostream>
#include <string>

#include "Server.hpp"

#define PORT 4000
#define RMPORT 4001

//server <port> <RMport> [-S <IP> <port>]
int main(int argc, char* argv[]){

    if (argc < 3){
        std::cout << "USO: " << std::endl;
        std::cout << "./server <port> <RMport> [-S <IP> <port>]" << std::endl;
    }

    else
    {
      int port = std::stoi(argv[1]);
      int RMport = std::stoi(argv[2]);

      if (argc == 3)
      {
          cout << "primary server" << endl;
          dropbox::Server server(port, RMport);
          std::string dummy;
          server.run();
          // Waits for user input to stop the server
          std::cin >> dummy;
          server.stop();
      }
      else if (strcmp(argv[3], "-S")==0)
      {
          cout << "secondary Server" << endl;
          int priPort = stoi(argv[5]);
          dropbox::Server server(port, RMport, argv[4], priPort);
          std::string dummy;
          server.run();
          // Waits for user input to stop the server
          std::cin >> dummy;
          server.stop();
      }
    }
    return 0;
}
