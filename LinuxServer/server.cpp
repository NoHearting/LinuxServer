/*
 * @Descripttion:
 * 服务器主程序 
 * @Version: 1.0
 * @Author: zsj
 * @Date: 2020-05-02 11:56:47
 * @LastEditors: zsj
 * @LastEditTime: 2020-05-04 12:49:52
 */

#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<errno.h>
#include<string.h>
#include<fcntl.h>
#include<stdlib.h>
#include<cassert>
#include<sys/epoll.h>

#include"locker.h"
#include"threadpool.h"
#include"http_conn.h"
// #include"utils.h"

#define MAX_FD 65535
#define MAX_EVENT_NUMBER  10000

extern int addfd(int epollfd,int fd,bool one_shot);
extern int removefd(int epollfd,int fd);

void addsig(int sig,void(*handler)(int),bool restart = true){
    struct sigaction sa;
    memset(&sa,0,sizeof(sa));
    sa.sa_handler = handler;
    if(restart){
        sa.sa_flags |= SA_RESTART;
    }
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig,&sa,NULL) != -1);
}

void show_error(int connfd,const char * info){
    printf_zsj(__FILE__,__LINE__,"%s",info);
    send(connfd,info,strlen(info),0);
    close(connfd);
}

int main(int argc,char * argv[])
{
    if(argc < 2){
        printf_zsj(__FILE__,__LINE__,"Usage: %s <port>\n",argv[0]);
    }

    //忽略SIGPIPE信号
    addsig(SIGPIPE,SIG_IGN);

    //创建线程池
    threadpool<http_conn>*pool = NULL;
    try{
        pool = new threadpool<http_conn>;
    }
    catch(...){
        return 1;
    }

    //预先为每个可能的客户连接分配一个http_conn对象
    http_conn * users = new http_conn[MAX_FD];
    assert(users);
    int user_count = 0;

    int listenfd = socket(PF_INET,SOCK_STREAM,0);
    assert(listenfd != -1);
    struct linger tmp = {1,0};
    setsockopt(listenfd,SOL_SOCKET,SO_LINGER,&tmp,sizeof(tmp));

    int ret = 0;
    sockaddr_in address;
    bzero(&address,sizeof(address));
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_family = AF_INET;
    address.sin_port = htons(atoi(argv[1]));

    ret = bind(listenfd,(sockaddr*)&address,sizeof(address));
    assert(ret != -1);

    ret = listen(listenfd,5);
    assert(ret != -1);

    epoll_event events[MAX_EVENT_NUMBER];
    int epollfd = epoll_create(5);
    assert(epollfd != -1);
    addfd(epollfd,listenfd,false);

    http_conn::m_epollfd = epollfd;

    while(true){
        int number = epoll_wait(epollfd,events,MAX_EVENT_NUMBER,-1);
        if((number < 0) && (errno != EINTR)){
            printf_zsj(__FILE__,__LINE__,"epoll failure\n");
            break;
        }
        for(int i = 0;i<number;i++){
            int sockfd = events[i].data.fd;
            if(sockfd == listenfd){   //接受连接请求
                sockaddr_in clnt_addr;
                socklen_t clnt_addr_sz = sizeof(clnt_addr);
                int connfd = accept(listenfd,(sockaddr*)&clnt_addr,&clnt_addr_sz);
                if(connfd < 0){
                    printf_zsj(__FILE__,__LINE__,"errno is: %d\n",errno);
                    continue;
                }
                if(http_conn::m_user_count >= MAX_FD){
                    show_error(connfd,"Internal server busy");
                    continue;
                }
                users[connfd].init(connfd,clnt_addr);  //初始化逻辑处理的类
            }
            else if(events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)){
                //如果有异常,直接关闭客户连接
                users[sockfd].close_conn();
            }
            else if(events[i].events & EPOLLIN){
                //根据读到的结果,决定是将任务添加到线程池,还是关闭连接
                if(users[sockfd].read()){
                    pool->append(users + sockfd);
                }
                else{
                    users[sockfd].close_conn();
                }
            }
            else if(events[i].events & EPOLLOUT){
                
                //根据写的结果,决定是否关闭连接
                if(!users[sockfd].write()){
                    users[sockfd].close_conn();
                }
            }
            else{

            }

        }
    }
    close(epollfd);
    close(listenfd);
    delete[] users;
    delete pool;

}

/*****************************************************************
                         服务器运行大致流程
 1. 首先建立一个监听socket,一个全局的epoll 
 2. 然后监听客户端的连接请求,如果有则初始化一个连接
 3. 然后监听客户端是否发送数据,如果发送数据,则将这个连接添加到任务队列
 4. 线程池中的线程到任务队列中取任务来执行(调用process)
 5. 


*/ 