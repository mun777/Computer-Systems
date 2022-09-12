#include "TCPRequestChannel.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include<netinet/in.h>
#include <iostream>


TCPRequestChannel::TCPRequestChannel (const std::string _ip_address, const std::string _port_no) {
    //if server 
    //  create a socket on the speicified 
    //      -specify domain type and protocol
    //  bind socket to the add to set up listening 
    //  mark socket as listening
    //only client has ip



    if(_ip_address == "")
    {
        int status;
        struct addrinfo holder;
        struct addrinfo *serverinfo;
        memset(&holder,0,sizeof(holder));
        holder.ai_family = AF_INET;
        holder.ai_socktype = SOCK_STREAM;
        holder.ai_flags = AI_PASSIVE;
        holder.ai_protocol = 0;
        if((status = getaddrinfo(NULL, _port_no.c_str(), &holder, &serverinfo))!=0)
        {
            std::cout<<"Failed to get address"<<std::endl;
            exit(1);    
        }
        if((sockfd = socket(serverinfo->ai_family, serverinfo->ai_socktype, serverinfo->ai_protocol)) == -1)
        {
            
            std::cout<<"socket failed"<<std::endl;
            exit(1);  
        }
        if((status= bind(sockfd,serverinfo->ai_addr,serverinfo->ai_addrlen))==-1)
        {
            perror("bind");
            std::cout<<"bind failed"<<std::endl;
            exit(1);
        }
        freeaddrinfo(serverinfo);
        if((status= listen(sockfd,SOMAXCONN))==-1)
        {
            std::cout<<"listen failed"<<std::endl;
            exit(1);
        }
        std::cout<<"server connected"<<std::endl;

    }
    //if client 
    //  create a socket on the speicifeid 
    //      -specify domain type and protocol 
    //  connect socket to the ip add of the server
    else{
        int status;
        struct addrinfo holder;
        struct addrinfo *serverinfo;
        memset(&holder,0,sizeof(holder));
        holder.ai_family = AF_INET;
        holder.ai_socktype = SOCK_STREAM;
        holder.ai_flags = AI_PASSIVE;
        holder.ai_protocol = 0;
        if((status = getaddrinfo(_ip_address.c_str(), _port_no.c_str(), &holder, &serverinfo))!=0)
        {
            std::cout<<"Failed to get address"<<std::endl;
            exit(1);    
        }
        if((sockfd = socket(serverinfo->ai_family, serverinfo->ai_socktype, serverinfo->ai_protocol)) == -1)
        {
            std::cout<<"socket failed"<<std::endl;
            exit(1);  
        }
        if((status= connect(sockfd,serverinfo->ai_addr,serverinfo->ai_addrlen))==-1)
        {
            perror("connect");
            std::cout<<"connect failed"<<std::endl;
            exit(1);
        }
        freeaddrinfo(serverinfo);

        std::cout<<"client connected"<<std::endl;
    }


}

TCPRequestChannel::TCPRequestChannel (int _sockfd) {
    this->sockfd = _sockfd;

}

TCPRequestChannel::~TCPRequestChannel () {
    //close the sockfd
    close(sockfd);


}

int TCPRequestChannel::accept_conn () {

    //struct sockaddr_storage
    //implementing accept (...) retval the sockfd of the cient
    struct sockaddr_storage their_addr;
    socklen_t addr_size;
    addr_size = sizeof(their_addr);

    int new_fd;
    new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &addr_size);
    return new_fd;
}
// read/write, rec/send
int TCPRequestChannel::cread (void* msgbuf, int msgsize) {

    int bytes_rec;
    bytes_rec = recv(sockfd,msgbuf,msgsize,0);
    return bytes_rec;

}

int TCPRequestChannel::cwrite (void* msgbuf, int msgsize) {
    int bytes_sent;
    bytes_sent = send(sockfd,msgbuf,msgsize,0);
    return bytes_sent;
}
