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

#include <fstream>
#include <filesystem>

#include "action_manager.hpp"
#include "json.hpp"

using json = nlohmann::json;

using namespace std;

class ClientActionManager : public DatagramManager{
private:
    int socketFD;
    sockaddr_in& senderAddr;
public:
    ClientActionManager(int _socketFD, sockaddr_in& _senderAddr): socketFD(_socketFD), senderAddr(_senderAddr) {};

    void sendDatagram(string datagram){
        DatagramManager::sendDatagram(socketFD, senderAddr, datagram);
    }

    string readDatagram(){
        return DatagramManager::readDatagram(socketFD);
    }

    // ACTIONS 

    void error(string payload){
        int msg_size = stoi(payload.substr(0,5));
        string msg = payload.substr(5,msg_size);
        cout << msg << endl;
    }

    void ok(){
        //write(socketFD, packet.data(), 1);
        cout << "ok" << endl;
    }

    string login(string nickname){
        string size = lengthString(nickname,4);
        string login_msg = "L" + size + nickname;
        //write(socketFD, login_msg.data(), login_msg.size());
        return login_msg;
    }

    string logout(){
        string out = "O";
        //write(socketFD, out.data(), out.size());
        return out;
    }

    string broadcast(){
        string msg;
        cout << "Msg: ";
        getline(cin,msg);

        string msg_size = lengthString(msg,7);
        string packet = "B" + msg_size + msg;
        //write(socketFD, packet.c_str(), packet.size());
        return packet;
    }

    void broadcastHandler(string payload){
        int source_size = stoi(payload.substr(0,3));
        string source = payload.substr(3,source_size);
        int msg_size = stoi(payload.substr(3 + source_size, 7));
        string msg = payload.substr(3 + source_size + 7, msg_size);

        displayMessage(source,msg);
    }

    string unicast(){
        string dest,msg;
        cout << "Dest: ";
        getline(cin,dest);
        cout << "Msg: ";
        getline(cin,msg);

        string packet = "U" + lengthString(msg,5) + msg + lengthString(dest,7) + dest;
        //write(socketFD, packet.c_str(), packet.size());
        return packet;
    }

    void unicastHandler(string payload){
        int source_size = stoi(payload.substr(0, 7));
        string source = payload.substr(7, source_size);
        int msg_size = stoi(payload.substr(7 + source_size, 5));
        string msg = payload.substr(7 + source_size + 5, msg_size);

        displayWisper(source,"me",msg);
    }

    string list(){
        string packet = "T";
        //write(socketFD,packet.data(),packet.size());
        return packet;
    }

    void listHandler(string payload){
        int list_size = stoi(payload.substr(0, 5));
        string list = payload.substr(5, list_size);
        json j = json::parse(list);

        cout << "Online: " << endl;

        for(int i = 0; i < j.size(); i++){
            cout << (i + 1) << ". " << j[i].get<string>() << endl;
        }
    }

    string file() {
        string dest,path;
        cout << "Dest: ";
        getline(cin,dest);
        cout << "File path: ";
        getline(cin, path);

        ifstream file(path, ios::binary | ios::ate);

        size_t size = static_cast<size_t>(file.tellg());
        file.seekg(0);
        string content(size, '\0');
        file.read(content.data(), size);

        string file_name = filesystem::path(path).filename().string();

        string packet = "F" + lengthString(content,5) + content + lengthString(file_name,5) + file_name + lengthString(dest,5) + dest;

        cout << "File sent: " << file_name << endl;
        //write(socketFD, packet.data(), packet.size());
        return packet;
    }

    void fileHandler(string payload) {
        int content_size = stoi(payload.substr(0, 5));
        string content = payload.substr(5, content_size);
        int file_name_size = stoi(payload.substr(5 + content_size, 5));
        string file_name = payload.substr(5 + content_size + 5, file_name_size);
        int source_size = stoi(payload.substr(5 + content_size + 5 + file_name_size, 5));
        string source = payload.substr(5 + content_size + 5 + file_name_size + 5, source_size);

        string out_path = "recive_file_" + file_name;
        ofstream(out_path, ios::binary).write(content.data(),content_size);

        displayWisper(source, "me", "File received: " + file_name);
    }
};

class Client {
private:
    string nickname;
    struct sockaddr_in stSockAddr;
    unique_ptr<ClientActionManager> cam;
    
    int SocketFD;
    int Res;

    bool running = false;

public:
    Client(string _nickname) {
        SocketFD = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);

        memset(&stSockAddr, 0, sizeof(struct sockaddr_in));
        stSockAddr.sin_family = AF_INET;
        stSockAddr.sin_port = htons(45000);

        Res = inet_pton(AF_INET, "127.0.0.1", &stSockAddr.sin_addr);

        nickname = _nickname;

        cam = make_unique<ClientActionManager>(SocketFD, stSockAddr);

        string packet = cam->login(nickname);
        for(string dg: cam->wrap(packet))
            cam->sendDatagram(dg);
        matchHandler();
    }

	~Client(){
		close(SocketFD);
	}

    void matchLocalAction(){
        cout << "-----------------------------" << endl;
        cout << "             MENU            " << endl;
        cout << "0. Logout" << endl;
        cout << "1. Broadcast" << endl;
        cout << "2. Unicast" << endl;
        cout << "3. List" << endl;
        cout << "4. Send File" << endl;

        cout << ": ";
 
        int action;
        cin >> action;
        cin.ignore();

        string packet;

        switch (action)
        {
        case 0:
            packet = cam->logout();
            running = false;
            break;
        case 1:
            packet = cam->broadcast();
            break;
        case 2:
            packet = cam->unicast();
            break;
        case 3:
            packet = cam->list();
            break;
        case 4:
            packet = cam->file();
            break;
        default:
            cout << "Unknown action" << endl;
            break;
        }

        for(string dg: cam->wrap(packet))
            cam->sendDatagram(dg);
    }

    void matchHandler(){
        string datagram = cam->readDatagram();
        Datagram dg = cam->parse(datagram);

        if(!dg.checksum_valid){
            cout << "Lost data" << endl;
            return;
        }

        string packet = cam->reassemble(dg);
        if (packet.empty())
            return;

        char type = packet[0];

        string payload = packet.substr(1);

        switch (type)
        {
        case 'K':
            cam->ok();
            running = true;
            break;
        case 'E':
            cam->error(payload);
            break;
        case 'b':
            cam->broadcastHandler(payload);
            break;
        case 'u':
            cam->unicastHandler(payload);
            break;
        case 't':
            cam->listHandler(payload);
            break;
        case 'f':
            cam->fileHandler(payload);
            break;

        
        default:
            break;
        }
    }

    void writeFunction() {
        for (;running;) {
            matchLocalAction();
        }
    }

    void readFunction() {
        for (;running;) {
            matchHandler();
        }
    }

    void init() {
        thread readThread(&Client::readFunction, this);
        thread writeThread(&Client::writeFunction, this);

        readThread.join();
        writeThread.join();
    }
};

int main(void) {
    string client_nickname;
    cout << "Enter nickname: ";
    getline(cin, client_nickname);

	Client client(client_nickname);
    client.init();

    return 0;
}