#include"iomanager.h"
#include<iostream>
#include<sys/socket.h>//recv,send
#include<netinet/in.h>
#include<arpa/inet.h>
#include<fcntl.h>
using namespace sylar;
int sock;
char buf[4096];
//GET / HTTP/1.0\r\n\r\n
char send_data[]="GET / HTTP/1.0\r\n\r\n";
void func1(){
    recv(sock,buf,4096,0);
    std::cout<<buf<<std::endl<<std::endl;
}
void func2(){
    send(sock,send_data,sizeof(send_data),0);
}
int main(){
    IOManager manager(2);
    sock=socket(AF_INET,SOCK_STREAM,0);
    sockaddr_in addr;
    addr.sin_family=AF_INET;
    addr.sin_port=htons(80);
    addr.sin_addr.s_addr=inet_addr("103.235.46.96");
    fcntl(sock,F_SETFL,O_NONBLOCK);
    connect(sock,(sockaddr*)&addr,sizeof(addr));
    manager.addEvent(sock,IOManager::WRITE,&func2);
    manager.addEvent(sock,IOManager::READ,&func1);
    std::cout<<"event has been posted\n\n";
    return 0;
}