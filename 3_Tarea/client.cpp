#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <string>
#include <cstring>
#include <iostream>
#include <thread>

class PackageManager {
public:
    PackageManager() {};

	void displayMessage(std::string nickname, std::string msg){
		std::cout << "[" << nickname << "]: " << msg << std::endl;
	}

    void receivePacket(int socketFD) {
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
            msg_size = atoi(buffer);

            std::memset(buffer, 0, sizeof(buffer));
            n = read(socketFD, buffer, msg_size);
            buffer[n] = '\0';
            msg = std::string(buffer);

            displayMessage(nickname, msg);
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

class Client {
private:
    std::string nickname;
    struct sockaddr_in stSockAddr;
    PackageManager packageManager;
    
    int SocketFD;
    int Res;

public:
    Client(std::string _nickname) {
        SocketFD = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

        memset(&stSockAddr, 0, sizeof(struct sockaddr_in));
        stSockAddr.sin_family = AF_INET;
        stSockAddr.sin_port = htons(45000);

        Res = inet_pton(AF_INET, "10.0.2.15", &stSockAddr.sin_addr);

        connect(SocketFD, (const struct sockaddr *)&stSockAddr, sizeof(struct sockaddr_in));

        nickname = _nickname;

        packageManager.sendPacket(SocketFD, nickname, "Conectado");
    }

	~Client(){
		shutdown(SocketFD, SHUT_RDWR);
		close(SocketFD);
	}

    void writeFunction(int socketFD) {
        for (;;) {
            std::string msg;
            std::getline(std::cin, msg);
            packageManager.sendPacket(socketFD, nickname, msg);
        }
    }

    void readFunction(int socketFD) {
        for (;;) {
            packageManager.receivePacket(socketFD);
        }
    }

    void init() {
        std::thread readThread(&Client::readFunction, this, SocketFD);
        std::thread writeThread(&Client::writeFunction, this, SocketFD);

        readThread.join();
        writeThread.join();
    }
};

int main(void) {
    std::string client_nickname;
    std::cout << "Enter nickname: ";
    std::getline(std::cin, client_nickname);

	Client client(client_nickname);
    client.init();

    return 0;
}