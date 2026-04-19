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

class ClientActionManager: ActionManager {
private:
    int socketFD;

public:
    ClientActionManager(int _socketFD): socketFD(_socketFD) {};


    string readString(int size){
        return ActionManager::readString(socketFD,size);
    }

    int readInt(int size){
        return ActionManager::readInt(socketFD,size);
    }

    // ACTIONS 

    void error(){
        int msg_size = readInt(5);
        string msg = readString(msg_size);
        cout << msg << endl;
    }

    void ok(){
        cout << "ok" << endl;
    }

    void login(string nickname){
        string size = lengthString(nickname,4);
        string login_msg = "L" + size + nickname;
        write(socketFD, login_msg.data(), login_msg.size());
    }

    void logout(){
        string out = "O";
        write(socketFD, out.data(), out.size());
        exit(0);
    }

    void broadcast(){
        string msg;
        cout << "Msg: ";
        getline(cin,msg);

        string msg_size = lengthString(msg,7);
        string packet = "B" + msg_size + msg;
        write(socketFD, packet.c_str(), packet.size());
    }

    void broadcastHandler(){
        int source_size = readInt(3);
        string source = readString(source_size);
        int msg_size = readInt(7);
        string msg = readString(msg_size);

        displayMessage(source,msg);
    }

    void unicast(){
        string dest,msg;
        cout << "Dest: ";
        getline(cin,dest);
        cout << "Msg: ";
        getline(cin,msg);

        string packet = "U" + lengthString(msg,5) + msg + lengthString(dest,7) + dest;
        write(socketFD, packet.c_str(), packet.size());
    }

    void unicastHandler(){
        int source_size = readInt(7);
        string source = readString(source_size);
        int msg_size = readInt(5);
        string msg = readString(msg_size);

        displayWisper(source,"me",msg);
    }

    void list(){
        string packet = "T";
        write(socketFD,packet.data(),packet.size());
    }

    void listHandler(){
        int list_size = readInt(5);
        string list = readString(list_size);
        json j = json::parse(list);

        cout << "Online: " << endl;

        for(int i = 0; i < j.size(); i++){
            cout << (i + 1) << ". " << j[i].get<string>() << endl;
        }
    }

    void file() {
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

        write(socketFD, packet.data(), packet.size());

        cout << "File sent: " << file_name << endl;
    }

    void fileHandler() {
        int content_size = readInt(5);
        string content = readString(content_size);
        int file_name_size = readInt(5);
        string file_name = readString(file_name_size);
        int source_size = readInt(5);
        string source = readString(source_size);

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
        SocketFD = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

        memset(&stSockAddr, 0, sizeof(struct sockaddr_in));
        stSockAddr.sin_family = AF_INET;
        stSockAddr.sin_port = htons(45000);

        Res = inet_pton(AF_INET, "127.0.0.1", &stSockAddr.sin_addr);

        connect(SocketFD, (const struct sockaddr *)&stSockAddr, sizeof(struct sockaddr_in));

        //FIX PARA LOGIN
        nickname = _nickname;

        cam = make_unique<ClientActionManager>(SocketFD);

        cam->login(nickname);
        matchHandler();
    }

	~Client(){
		shutdown(SocketFD, SHUT_RDWR);
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

        switch (action)
        {
        case 0:
            cam->logout();
            running = false;
            break;
        case 1:
            cam->broadcast();
            break;
        case 2:
            cam->unicast();
            break;
        case 3:
            cam->list();
            break;
        case 4:
            cam->file();
            break;
        default:
            cout << "Unknown action" << endl;
            break;
        }
    }

    void matchHandler(){
        string buffer(1,0);
        int n = read(SocketFD, buffer.data(), 1);
        char type = buffer[0];
        
        switch (type)
        {
        case 'K':
            cam->ok();
            running = true;
            break;
        case 'E':
            cam->error();
            break;
        case 'b':
            cam->broadcastHandler();
            break;
        case 'u':
            cam->unicastHandler();
            break;
        case 't':
            cam->listHandler();
            break;
        case 'f':
            cam->fileHandler();
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