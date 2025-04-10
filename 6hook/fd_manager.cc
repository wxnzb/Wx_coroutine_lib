#include "fd_manager.h"
#include<sys/stat.h>
#include<fcntl.h>
FdCtx::FdCtx(int fd){
    m_fd=fd;
    init();
}
FdCtx::~FdCtx(){

}
bool FdCtx::init(){
   struct stat st;
   if(fstat(m_fd,&st)==-1){
    m_isInit=false;
    m_isSocket=false;//这个为什么要设置
   }else{
    m_isInit=true;
    m_isSocket=S_ISSOCK(st.st_mode);
   }
   if(m_isSocket){
    //看这个套子节是否是非阻塞的
    int flags=fcntl(m_fd,F_GETFL,0);
    if(!(flags)&O_NONBLOCK){
        fcntl(m_fd,F_SETFD,flags|O_NONBLOCK);
        m_sysNonblock=true;
    }else{
        m_sysNonblock=false;
    }
   }
   return m_isInit;
}