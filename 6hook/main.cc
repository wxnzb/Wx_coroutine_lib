#include<iostream>
#include<sys/socket.h>
#include<netinet/in.h>
#include<unistd.h>
#include<fcntl.h>
#include<cstring>
#include "iomanager.h"
static int sockfd=-1;
void test_accept(){
    struct sockaddr_in client_addr;
    memset(&client_addr,0,sizeof(client_addr));
    socklen_t client_addr_len=sizeof(client_addr);
    int newsockfd=accept(sockfd,(struct sockaddr*)&client_addr,&client_addr_len);
    if(newsockfd<0){
        std::cerr<<"accept error"<<std::endl;
        return;
    }else{
        std::cout<<"accept success"<<std::endl;
        sylar::IOManager::GetThis()->addEvent(newsockfd,sylar::IOManager::READ,[newsockfd](){
        char buffer[1024];
        //设置非阻塞
        fcntl(newsockfd,F_SETFL,O_NONBLOCK);
        while(true){
            ssize_t n=recv(newsockfd,buffer,sizeof(buffer),0);
            if(n<=0){
                std::cerr<<"recv error"<<std::endl;
                close(newsockfd);
                break;
            }else{
                std::cout<<"Received: "<<buffer<<std::endl;
                const char* response="HTTP/1.1 200 OK\r\n"
                                           "Content-Type: text/plain\r\n"
                                           "Content-Length: 13\r\n"
                                           "Connection: keep-alive\r\n"
                                           "\r\n"
                                           "Hello, World!";
                send(newsockfd,response,strlen(response),0);
                close(newsockfd);
                break;
            }
        }
    });
    }
    sylar::IOManager::GetThis()->addEvent(newsockfd,sylar::IOManager::READ,test_accept);
   // std::cout<<"Accepted connection from "<<inet_ntoa(client_addr.sin_addr)<<":"<<ntohs(client_addr.sin_port)<<std::endl;
}
void test_iomanager(){
    int portno=8080;
    struct sockaddr_in server_addr;
    memset(&server_addr,0,sizeof(server_addr));
    sockfd=socket(AF_INET,SOCK_STREAM,0);
    if(sockfd<0){
        std::cerr<<"socket error"<<std::endl;
        return;
    }
    //解决addresss already in use的问题
    int opt=1;
    setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));
    server_addr.sin_family=AF_INET;
    server_addr.sin_addr.s_addr=INADDR_ANY;
    server_addr.sin_port=htons(portno);
    if(bind(sockfd,(struct sockaddr*)&server_addr,sizeof(server_addr))<0){
        std::cerr<<"bind error"<<std::endl;
        return;
    }
    if(listen(sockfd,1024)<0){
        std::cerr<<"listen error"<<std::endl;
        return;
    }
    std::cout<<"Server started on port "<<portno<<std::endl;
    //不阻塞
    fcntl(sockfd,F_SETFL,O_NONBLOCK);
    sylar::IOManager iom(9);
    iom.addEvent(sockfd,sylar::IOManager::READ,test_accept);
}
int main(){
    test_iomanager();
    return 0;
}
