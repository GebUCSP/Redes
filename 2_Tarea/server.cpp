	  /* Server code in C */
 
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

class Protocol{
public:
	Protocol(int socketFD): SocketFD(socketFD) {};

	void displayMessage(std::string nickname, std::string msg){
		std::cout << "[" << nickname << "]: " << msg << std::endl;
	}
	
	void recievePacket(){
		char buffer[256];
		
		std::string nickname;
		int nickname_size;
		std::string msg;
		int msg_size;
		
		
		std::memset(buffer, 0, sizeof(buffer));
		int n = read(SocketFD,buffer,3);
		buffer[n] = '\0';
		
		nickname_size = std::atoi(buffer);
		
		std::memset(buffer, 0, sizeof(buffer));
		n = read(SocketFD,buffer,nickname_size);
		buffer[n] = '\0';
		
		nickname = std::string(buffer);
		
		std::memset(buffer, 0, sizeof(buffer));
		n = read(SocketFD,buffer,3);
		buffer[n] = '\0';
		
		msg_size = atoi(buffer);
		
		std::memset(buffer, 0, sizeof(buffer));
		n = read(SocketFD,buffer,msg_size);
		buffer[n] = '\0';
		
		msg = std::string(buffer);  
		
		displayMessage(nickname, msg);
	}
	
	void sendPacket(std::string nickname, std::string msg){
	
		std::string nickname_size, msg_size;
		
		nickname_size = std::to_string(nickname.size());
		nickname_size.insert(0,3 - nickname_size.size(), '0');
		
		msg_size = std::to_string(msg.size());
		msg_size.insert(0,3 - msg_size.size(), '0');
		
		std::string packet = nickname_size + nickname + msg_size + msg;
		write(SocketFD, packet.c_str(), packet.size());
		
		displayMessage(nickname, msg);
	}

private:
	int SocketFD;
};

Protocol *protocol = nullptr;
 std::string own_nickname;

void readFunction(){
	for(;;){
		protocol->recievePacket();
	}
}

void writeFunction(){
	for(;;){
		std::string own_msg;
		std::getline(std::cin, own_msg);
	 	protocol->sendPacket(own_nickname, own_msg);
	}
}
 
  int main(void)
  {
    struct sockaddr_in stSockAddr;
    int SocketFD = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    char buffer[256];
    int n;
 
 	//server settings
 	own_nickname = "Server";
 	//cout << "Enter nickname: ";
 	//cin >> own_nickname; 	
 	//end config
 
 
    if(-1 == SocketFD)
    {
      perror("can not create socket");
      exit(EXIT_FAILURE);
    }
 
    memset(&stSockAddr, 0, sizeof(struct sockaddr_in));
 
    stSockAddr.sin_family = AF_INET;
    stSockAddr.sin_port = htons(8080);
    stSockAddr.sin_addr.s_addr = INADDR_ANY;
 
    bind(SocketFD,(const struct sockaddr *)&stSockAddr, sizeof(struct sockaddr_in));
 
    listen(SocketFD, 10);
 
    int ConnectFD = accept(SocketFD, NULL, NULL); 
    
    	protocol = new Protocol(ConnectFD);
    
    std::thread writeThread(writeFunction);
    std::thread readThread(readFunction);
    
    writeThread.join();
    readThread.join();
 
    close(SocketFD);
    return 0;
  }
