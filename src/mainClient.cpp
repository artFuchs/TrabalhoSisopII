#include <iostream>
#include <string>

#include "Client.hpp"
#include "Frontend.hpp"

#define PORT 3999;

int main(int argc, char* argv[]){

    if (argc < 4){
      std::cout << "USO: " << std::endl;
      std::cout << "./client <username> <server ip adress> <port> [<frontendPort>]" << std::endl;
      std::cout << "frontendPort pode ser necessário caso esteja rodando mais do que um cliente" << std::endl;
      return 0;
    }

    int port = PORT;
    if (argc == 5)
        port = std::stoi(argv[4]);
    dropbox::Frontend frontend(port);
    port = frontend.getPort();
    if (port < 0)
        std::cout << "Frontend socket not valid" << std::endl;
    int frontPort = std::stoi(argv[3]);
    frontend.setServerAddress(argv[2], frontPort);
    std::cout << "FRONTEND criado com port:" << port << std::endl;


    dropbox::Client client(argv[1], "localhost", port);

    std::string cstr;
    char *token;
    char *arg;

        client.start();

        while (1) {
           getline(std::cin, cstr);
           char* cmd = &cstr[0u];
           if((token = strtok(cmd," \t")) != NULL ){
             if ((strcmp(token, "upload")) == 0){
               if((arg = strtok(NULL," \t")) != NULL)
                 client.upload(arg);
               else
                 std::cout << "Argument missing\n";
             } else if ((strcmp(token, "download")) == 0){
                 if((arg = strtok(NULL," \t")) != NULL)
                   client.download(arg);
                 else
                   std::cout << "Argument missing\n";
             } else if ((strcmp(token, "delete")) == 0){
                 if((arg = strtok(NULL," \t")) != NULL)
                   client.deleteFile(arg);
                 else
                   std::cout << "Argument missing\n";
             } else if ((strcmp(token, "exit")) == 0){
                   client.stop();
                   break;
             } else if ((strcmp(token, "list_server")) == 0){
                   client.listSrv();
             } else if ((strcmp(token, "list_client")) == 0){
                   client.listCli();
             } else if ((strcmp(token, "get_sync_dir")) == 0){
                   client.getSyncDir();
             }
           }

         }


    return 0;
}
