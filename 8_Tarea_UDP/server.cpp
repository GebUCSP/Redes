#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <string>
#include <cstring>
#include <iostream>
#include <thread>
#include <map>
#include <utility>


#include "action_manager.hpp"
#include "json.hpp"

using json = nlohmann::json;

using namespace std;

class ServerActionManager : public DatagramManager {
public:
    ServerActionManager(){}
    
    void writeDatagram(int socketFD, sockaddr_in& senderAddr, string packet){
        for(string datagram: wrap(packet))
            sendDatagram(socketFD, senderAddr, datagram);
    }

    string findClient(int socketFD, sockaddr_in& senderAddr, map<string,sockaddr_in>& clientsTable){
        for(auto &client: clientsTable){
            if(client.second.sin_addr.s_addr == senderAddr.sin_addr.s_addr && client.second.sin_port == senderAddr.sin_port){
                return client.first;
            }
        }
        return "";
    }

    //ACTIONS

    void loginHandler(int socketFD, sockaddr_in& senderAddr, string payload, map<string, sockaddr_in>& clientsTable){
        int nickname_size = stoi(payload.substr(0, 4));
        string nickname = payload.substr(4, nickname_size);

        if(clientsTable.count(nickname)){
            string msg = "Nickname already exists";
            string size = lengthString(msg,5);
            string packet = "E" + size + msg;
            //write(socketFD, packet.c_str(), packet.size());
            writeDatagram(socketFD, senderAddr, packet);
        }

        clientsTable[nickname] = senderAddr;
        writeDatagram(socketFD, senderAddr, "K");
    }


    void logoutHandler(int socketFD, sockaddr_in& senderAddr, map<string,sockaddr_in>& clientsTable) {
        string source = findClient(socketFD, senderAddr, clientsTable);
        if(!source.empty()){
            clientsTable.erase(source);
            cout << source << " se desconectó." << endl;
        }
    }


    void broadcastHandler(int socketFD, sockaddr_in& senderAddr, string payload, map<string,sockaddr_in>& clientsTable){

        string source = findClient(socketFD, senderAddr, clientsTable);
        int msg_size = stoi(payload.substr(0, 7));
        string msg = payload.substr(7, msg_size);

        string packet = "b" + lengthString(source, 3) + source + lengthString(msg, 7) + msg;

        for(auto& client: clientsTable){
        	writeDatagram(socketFD,client.second,packet);
        }

        displayMessage(source, msg);
    }
    
    void unibroadcastHandler(int socketFD, sockaddr_in& senderAddr, string payload, map<string,sockaddr_in>& clientsTable){
        string source = findClient(socketFD, senderAddr, clientsTable);
        int msg_size = stoi(payload.substr(0, 5));
        string msg = payload.substr(5, msg_size);
        int dest_size = stoi(payload.substr(5 + msg_size, 7));
        string dest = payload.substr(5 + msg_size + 7, dest_size);

        string packet = "u" + lengthString(source,7) + source + lengthString(msg,5) + msg;

        if(clientsTable.count(dest)){
            writeDatagram(socketFD,clientsTable[dest],packet);
        }

        displayWisper(source,dest, msg);
    }

    void listHandler(int socketFD, sockaddr_in& senderAddr, map<string,sockaddr_in>& clientsTable){
        vector<string> nicknames;

        for(const auto&[nickname, _]: clientsTable){
            nicknames.push_back(nickname);
        }

        json j = nicknames;
        string packet = "t" + lengthString(j.dump(),5) + j.dump();
        //TO DO
        writeDatagram(socketFD, senderAddr, packet);
    }

    void fileHandler(int socketFD, sockaddr_in& senderAddr, string payload, map<string,sockaddr_in>& clientsTable){
        int file_size = stoi(payload.substr(0, 5));
        string file_data = payload.substr(5, file_size);
        int filename_size = stoi(payload.substr(5 + file_size, 5));
        string filename = payload.substr(5 + file_size + 5, filename_size);
        int dest_size = stoi(payload.substr(5 + file_size + 5 + filename_size, 5));
        string dest = payload.substr(5 + file_size + 5 + filename_size + 5, dest_size);

        string source = findClient(socketFD, senderAddr, clientsTable);

        string packet = "f" + lengthString(file_data, 5) + file_data + lengthString(filename, 5) + filename + lengthString(source, 5) + source;

        if(clientsTable.count(dest)){
            writeDatagram(socketFD, clientsTable[dest],packet);
        }
    }
};

class Server {
private:
    int socketFD;
    struct sockaddr_in stSockAddr;

    ServerActionManager sam;

	map<string, sockaddr_in> clientsTable;

public:
    Server() {

        socketFD = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);

        memset(&stSockAddr, 0, sizeof(stSockAddr));
        stSockAddr.sin_family = AF_INET;
        stSockAddr.sin_port = htons(45000);
        stSockAddr.sin_addr.s_addr = INADDR_ANY;

        bind(socketFD, (const struct sockaddr *)&stSockAddr, sizeof(stSockAddr));
    }

    ~Server() {
        close(socketFD);
    }

    bool matchHandler(sockaddr_in& senderAddr, const string& packet){
        if(packet.empty())
            return false;

        char type = packet[0];
        string payload = packet.substr(1);
        
        switch (type)
        {
        case 'L':
            sam.loginHandler(socketFD, senderAddr, payload, clientsTable);
            break;
        case 'O':
            sam.logoutHandler(socketFD, senderAddr, clientsTable);
            return false;
            break;
        case 'B':
            sam.broadcastHandler(socketFD, senderAddr, payload, clientsTable);
            break;
        case 'U':
            sam.unibroadcastHandler(socketFD, senderAddr, payload, clientsTable);
            break;
        case 'T':
            sam.listHandler(socketFD, senderAddr, clientsTable);
            break;
        case 'F':
            sam.fileHandler(socketFD, senderAddr, payload, clientsTable);
            break;
        
        default:
            break;
        }

        return true;
    }

    void init() {
		for(;;){
            string buffer (500, 0);
            sockaddr_in senderAddr{};
            socklen_t len = sizeof(senderAddr);

            int n = recvfrom(socketFD, buffer.data(), buffer.size(),0,(sockaddr*)&senderAddr, &len);
            
            if(n <= 0)
                continue;

            Datagram dg = sam.parse(buffer);
            if(!dg.checksum_valid){
                cout << "datagram with lost data" << endl;
                continue;
            }

            string packet = sam.reassemble(dg);
            if(packet.empty())
                continue;

            matchHandler(senderAddr,packet);
		}

    }
};

int main() {
    Server server;
    server.init();
	
    return 0;
}