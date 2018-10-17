#include <iostream>
#include <string>

#include "Client.hpp"
#define PORT 4000

int main(int argc, char* argv[]){

    if (argc != 4){
      std::cout << "USO: " << std::endl;
      std::cout << "./client <username> <server ip adress> <port>" << std::endl;
      return 0;
    }

    int port = std::stoi(argv[3]);

    dropbox::Client client(argv[1], argv[2], port);
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
