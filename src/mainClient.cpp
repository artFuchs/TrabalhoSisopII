#include <iostream>
#include <string>

#include "Client.hpp"

#define PORT 4000

int main(void){
    dropbox::Client client("username", "localhost", PORT);
    std::string dummy;

    client.start();

    client.download("alo.txt");
    
    client.upload("alo.txt");
    std::cin >> dummy;
    
    client.stop();

    return 0;
}
