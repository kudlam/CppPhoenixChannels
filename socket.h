#ifndef SOCKET_H
#define SOCKET_H
#include <functional>
#include <string>
#include "websocketpp/websocketpp/config/asio_client.hpp"
#include "websocketpp/websocketpp/client.hpp"
#include <future>
#include <unordered_map>
#include <memory>
#include <atomic>
#include "channel.h"
namespace phoenix {
class socket{
public:
    enum state{
        INITIAL,
        OPENED,
        CLOSED,
        FAILED
    };
    typedef websocketpp::client<websocketpp::config::asio_tls_client> client;
    typedef websocketpp::lib::shared_ptr<websocketpp::lib::asio::ssl::context> context_ptr;
    typedef boost::asio::const_buffer const_buffer;
    socket(const std::string& uri, const std::string& hostname, const std::string& cacert);
    ~socket();
    void send(const std::string& data);
    phoenix::channel& getChannel(const std::string& topic);
    void removeChannel(const std::string& topic);
    uint32_t getRef();

private:

    client m_client;
    std::string m_cacert;
    std::string m_hostname;
    std::string m_uri;
    websocketpp::connection_hdl m_hdl;
    std::mutex m_mutex;
    std::mutex m_cvMutex;
    std::condition_variable m_cv;
    uint32_t m_ref = 0;
    std::future<void> m_future;
    int m_defaultHeartbeatMs = 5000;
    std::future<void> m_heartbeatFuture;
    state m_state = INITIAL;
    std::mutex m_toppicToChannelsMutex;
    typedef std::unordered_multimap<std::string,channel> toppicToChannels_t;
    toppicToChannels_t m_toppicToChannels;
    std::atomic<bool> m_stop;

    void connect();
    void waitForConnection();
    void on_message(websocketpp::connection_hdl, client::message_ptr msg);
    context_ptr on_tls_init(const char * hostname, websocketpp::connection_hdl);

};

}
#endif // SOCKET_H
