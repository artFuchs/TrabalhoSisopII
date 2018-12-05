#include "ElectionManager.hpp"
#include "Server.hpp"

void dropbox::ElectionManager::onElectionWon(){
    _server.onElectionWon();
}
