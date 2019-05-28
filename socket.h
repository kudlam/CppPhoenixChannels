#ifndef SOCKET_H
#define SOCKET_H
#include <functional>
#include <string>
#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>
#include <future>
#include <unordered_map>
#include <memory>
namespace phoenix {
class channel;
class socket: public std::enable_shared_from_this<phoenix::socket>{
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
    void send(const std::string& data);
    std::shared_ptr<channel> getChannel(const std::string& topic);
    void registerChannel(std::shared_ptr<channel> channel);
    int32_t getRef();

private:

    client m_client;
    std::string m_cacert;
    std::string m_hostname;
    std::string m_uri;
    websocketpp::connection_hdl m_hdl;
    std::mutex m_mutex;
    std::mutex m_cvMutex;
    std::condition_variable m_cv;
    int32_t m_ref;
    std::future<void> m_future;
    int m_defaultHeartbeatMs = 5000;
    std::future<void> m_heartbeatFuture;
    state m_state = INITIAL;
    std::unordered_multimap<std::string,std::shared_ptr<channel>> m_toppicToChannels;

    void connect();
    void waitForConnection();
    void on_message(websocketpp::connection_hdl, client::message_ptr msg);
    context_ptr on_tls_init(const char * hostname, websocketpp::connection_hdl);
    bool verify_certificate(const char * hostname, bool preverified, boost::asio::ssl::verify_context& ctx);
    bool verify_subject_alternative_name(const char * hostname, X509 * cert);
    bool verify_common_name(const char * hostname, X509 * cert);

};

}
#endif // SOCKET_H
