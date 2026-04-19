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

class PackageManager {
public:
    PackageManager(){}

    void displayMessage(std::string nickname, std::string msg) {
        std::cout << "[" << nickname << "]: " << msg << std::endl;
    }

    std::pair<std::string, std::string> receivePacket(int socketFD) {
        for (;;) {
            char buffer[256];

            std::string nickname;
            int nickname_size;
            std::string msg;
            int msg_size;

            std::memset(buffer, 0, sizeof(buffer));
            int n = read(socketFD, buffer, 3);
            buffer[n] = '\0';
            nickname_size = std::atoi(buffer);

            std::memset(buffer, 0, sizeof(buffer));
            n = read(socketFD, buffer, nickname_size);
            buffer[n] = '\0';
            nickname = std::string(buffer);

            std::memset(buffer, 0, sizeof(buffer));
            n = read(socketFD, buffer, 3);
            buffer[n] = '\0';
            msg_size = std::atoi(buffer);

            std::memset(buffer, 0, sizeof(buffer));
            n = read(socketFD, buffer, msg_size);
            buffer[n] = '\0';
            msg = std::string(buffer);


        	displayMessage(nickname, msg);

			return std::pair<std::string,std::string>(nickname,msg);
        }
    }

    void sendPacket(int socketFD, std::string nickname, std::string msg) {
        std::string nickname_size, msg_size;

        nickname_size = std::to_string(nickname.size());
        nickname_size.insert(0, 3 - nickname_size.size(), '0');

        msg_size = std::to_string(msg.size());
        msg_size.insert(0, 3 - msg_size.size(), '0');

        std::string packet = nickname_size + nickname + msg_size + msg;
        write(socketFD, packet.c_str(), packet.size());

    }
};

class Server {
private:
    int SocketFD;
    int ConnectFD;
    struct sockaddr_in stSockAddr;

    std::string nickname;
    PackageManager packageManager;

	std::map<std::string, int> clientsTable;

public:
    Server(std::string _nickname = "Server") {
        nickname = _nickname;

        SocketFD = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

        std::memset(&stSockAddr, 0, sizeof(stSockAddr));
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

    void readFunction(int socketFD) {
		for(;;){
        	std::pair<std::string, std::string> package = packageManager.receivePacket(socketFD);
			for(auto& client: clientsTable){
                if (client.first == package.first)
				    packageManager.sendPacket(client.second, package.first, package.second);
			}
		}
    }

    void writeFunction(int socketFD) {
        for (;;) {
            std::string msg;
            std::getline(std::cin, msg);
            packageManager.sendPacket(socketFD, nickname, msg);
        }
    }

    void init() {
		for(;;){
			ConnectFD = accept(SocketFD, NULL, NULL);

			std::pair<std::string,std::string> new_client = packageManager.receivePacket(ConnectFD);
			clientsTable[new_client.first] = ConnectFD;

			std::thread readThread(&Server::readFunction, this, ConnectFD);

			readThread.detach();
		}

    }
};

int main() {
    Server server("Server");
    server.init();
	
    return 0;
}