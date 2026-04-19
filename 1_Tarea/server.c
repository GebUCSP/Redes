	  /* Server code in C */
 
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
    int SocketFD = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    char buffer[256];
    int n;
 
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
    
    for(;;)
    {
 
      bzero(buffer,256);
      
      n = read(ConnectFD,buffer,255);
      printf("[Client]: %s",buffer);
     
      bzero(buffer,256);
     
      printf("[Server]: ");
      fgets(buffer,255,stdin);
        
      n = write(ConnectFD,buffer,strlen(buffer));
      
      
    }
 
    close(SocketFD);
    return 0;
  }
