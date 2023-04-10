#include <mymuduo/TcpServer.h> // 在/usr/include/下查找
#include <mymuduo/Logger.h>

#include <string>
#include <functional>

class EchoServer
{
public:
    EchoServer(EventLoop *loop,
            const InetAddress &addr,
            const std::string name)
        : server_(loop, addr, name)
        , loop_(loop)
    {
        // 注册回调
        server_.setConnectionCallback(
            std::bind(&EchoServer::onConnection, this, std::placeholders::_1)
        );

        server_.setMessageCallback(
            std::bind(&EchoServer::onMessage, this,
                std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
        );

        // 设置合适的loop线程数量，subloop
        server_.setThreadNum(3);
    }

    void start()
    {
        server_.start();
    }
private:
    // 连接建立或断开的回调
    void onConnection(const TcpConnectionPtr &conn)
    {
        if(conn->connected())
        {
            LOG_INFO("Connection UP: %s", conn->peerAddress().toIpPort().c_str());
        }
        else 
        {
            LOG_INFO("Connection DOWN: %s", conn->peerAddress().toIpPort().c_str());
        }
    }
    
    // 读写事件到来的回调
    void onMessage(const TcpConnectionPtr &conn,
                Buffer *buf, // 指针或引用符和变量名连在一起
                Timestamp time)
    {
        std::string msg = buf->retriveAllAsString();
        conn->send(msg); // echo
        conn->shutdown(); // 发完数据再关，优雅
    }

    EventLoop *loop_;
    TcpServer server_;
};

int main()
{
    EventLoop loop;
    InetAddress addr(8888);
    EchoServer server(&loop, addr, "myEchoServer"); // Acceptor non-blocking listenfd create bind
    server.start(); // listen loopthread listenfd->acceptChannel->mainLoop->
    loop.loop(); // 启动底层EpollPoller中的 epoll_wait
    return 0;
}