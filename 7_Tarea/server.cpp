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

class ServerActionManager: ActionManager {
public:
    ServerActionManager(){}
    
    //ACTIONS

    void error(int socketFD,string msg = "Unknown"){
        string out = "E" + lengthString(msg,5) + msg;
        write(socketFD, out.data(),out.size());
    }

    void ok(int socketFD){
        write(socketFD,"K",1);
    }

    void loginHandler(int socketFD, map<string, int>& clientsTable){
        int nickname_size = readInt(socketFD,4);
        string nickname = readString(socketFD,nickname_size);

        if(clientsTable.count(nickname)){
            string msg = "Nickname already exists";
            string size = lengthString(msg,5);
            string packet = "E" + size + msg;
            write(socketFD, packet.c_str(), packet.size());
            close(socketFD);
            return;
        }

        clientsTable[nickname] = socketFD;
        ok(socketFD);
    }

    void logoutHandler(int socketFD, map<string,int>& clientsTable) {
        string source;
        
        for(auto client: clientsTable){
            if(socketFD == client.second){
                source = client.first;
                clientsTable.erase(client.first);
                break;
            }
        }

        // FEAT AVISAR DESCONEXION
        close(socketFD);

        cout << source << " se desconectó." << endl;
    }

    void broadcastHandler(int socketFD, map<string,int>& clientsTable){

        string source;
        for(auto client: clientsTable){
            if(socketFD == client.second){
                source = client.first;
                break;
            }
        }

        int msg_size = readInt(socketFD,7);
        string msg = readString(socketFD, msg_size);

        string packet = "b" + lengthString(source, 3) + source + lengthString(msg, 7) + msg;

        for(auto& client: clientsTable){
        	write(client.second, packet.data(), packet.size());
        }

        displayMessage(source, msg);
    }
    
    void unibroadcastHandler(int socketFD, map<string,int>& clientsTable){
        string source;
        for(auto client: clientsTable){
            if(socketFD == client.second){
                source = client.first;
                break;
            }
        }

        int msg_size = readInt(socketFD,5);
        string msg = readString(socketFD, msg_size);
        int dest_size = readInt(socketFD,7);
        string dest = readString(socketFD, dest_size);

        string packet = "u" + lengthString(source,7) + source + lengthString(msg,5) + msg;

        for(auto& client: clientsTable){
            if(client.first == dest){
                write(client.second, packet.data(), packet.size());
                break;
            }
        }

        displayWisper(source,dest, msg);
    }

    void listHandler(int socketFD, map<string,int>& clientsTable){
        vector<string> nicknames;

        for(const auto&[nickname, _]: clientsTable){
            nicknames.push_back(nickname);
        }

        json j = nicknames;
        string packet = "t" + lengthString(j.dump(),5) + j.dump();
        write(socketFD,packet.data(),packet.size());
    }

    void fileHandler(int socketFD, map<string,int>& clientsTable){
        int file_size = readInt(socketFD, 5);
        string file_data = readString(socketFD, file_size);
        int filename_size = readInt(socketFD, 5);
        string filename = readString(socketFD, filename_size);
        int dest_size = readInt(socketFD, 5);
        string dest = readString(socketFD, dest_size);

        string source;
        for(auto client: clientsTable){
            if(socketFD == client.second){
                source = client.first;
                break;
            }
        }

        string packet = "f" + lengthString(file_data, 5) + file_data + lengthString(filename, 5) + filename + lengthString(source, 5) + source;

        for(auto& client: clientsTable){
            if(client.first == dest){
                write(client.second, packet.data(), packet.size());
                break;
            }
        }
    }
};

class Server {
private:
    int SocketFD;
    int ConnectFD;
    struct sockaddr_in stSockAddr;

    string nickname;
    ServerActionManager sam;

	map<string, int> clientsTable;

public:
    Server(string _nickname = "Server") {
        nickname = _nickname;

        SocketFD = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

        memset(&stSockAddr, 0, sizeof(stSockAddr));
        stSockAddr.sin_family = AF_INET;
        stSockAddr.sin_port = htons(45000);
        stSockAddr.sin_addr.s_addr = INADDR_ANY;

        bind(SocketFD, (const struct sockaddr *)&stSockAddr, sizeof(stSockAddr));
        listen(SocketFD, 10);

    }

    ~Server() {
        shutdown(ConnectFD, SHUT_RDWR);
        close(ConnectFD);
        close(SocketFD);
    }

    bool matchHandler(int socketFD){
        string buffer(1,0);
        int n = read(socketFD, buffer.data(), 1);


        if (n == 0) {
            sam.logoutHandler(socketFD, clientsTable);
            return false;
        }

        if (n < 0) {
            perror("read");
            return false;
        }

        char type = buffer[0];
        
        switch (type)
        {
        case 'L':
            sam.loginHandler(socketFD,clientsTable);
            break;
        case 'O':
            sam.logoutHandler(socketFD,clientsTable);
            return false;
            break;
        case 'B':
            sam.broadcastHandler(socketFD, clientsTable);
            break;
        case 'U':
            sam.unibroadcastHandler(socketFD,clientsTable);
            break;
        case 'T':
            sam.listHandler(socketFD,clientsTable);
            break;
        case 'F':
            sam.fileHandler(socketFD,clientsTable);
            break;
        
        default:
            break;
        }

        return true;
    }



    void readFunction(int socketFD) {
		for(;;){
            if (!matchHandler(socketFD)) break;
		}
    }

    void init() {
		for(;;){
			ConnectFD = accept(SocketFD, NULL, NULL);

			matchHandler(ConnectFD);

			thread readThread(&Server::readFunction, this, ConnectFD);

			readThread.detach();
		}

    }
};

int main() {
    Server server("Server");
    server.init();
	
    return 0;
}