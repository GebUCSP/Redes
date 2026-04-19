 /* Client code in C */
 
  #include <sys/types.h>
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #include <stdio.h>
  #include <stdlib.h>
  #include <string.h>
  #include <unistd.h>
 
  int main(void)
  {
    struct sockaddr_in stSockAddr;
    int Res;
    int SocketFD = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    int n;
 
    memset(&stSockAddr, 0, sizeof(struct sockaddr_in));
 
    stSockAddr.sin_family = AF_INET;
    stSockAddr.sin_port = htons(8080);
    Res = inet_pton(AF_INET, "10.0.2.15", &stSockAddr.sin_addr);
 
    connect(SocketFD, (const struct sockaddr *)&stSockAddr, sizeof(struct sockaddr_in));     
    char buffer[256];
    
    n = write(SocketFD,"Client is connected\n",strlen("Client is connected\n"));
    
    for(;;)
    {
    
    	bzero(buffer,256);
    	
        n = read(SocketFD,buffer,255);
    	printf("[Server]: %s",buffer);
	   
	bzero(buffer,256);
	
        printf("[Client]: ");
        fgets(buffer,255,stdin);
        
    	n = write(SocketFD,buffer,strlen(buffer));
 
    }
    
    shutdown(SocketFD, SHUT_RDWR);
    close(SocketFD);
    return 0;
  }
