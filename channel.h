#ifndef CHANNEL_H
#define CHANNEL_H

#include <mutex>
#include "channel_message.h"
#include "push.h"

namespace phoenix {
class push;
class socket;
class channel{
public:
    typedef std::function<void (channelMessage&)>  callback;

    channel(const std::string& topic, phoenix::socket& socket);
    channel(channel&& other);
    channel(const channel& other)=delete;
    ~channel();
    phoenix::push& join();
    phoenix::push& push(const std::string& event, const std::string& payload);
    void processMessage(channelMessage& message);
    void on(const std::string& eventName, callback  callback);
    void off(const std::string& eventName);
    void offId(const std::string& redId);
    std::unique_ptr<boost::asio::deadline_timer> setTimeout(std::chrono::milliseconds duration);
    void sendData(const std::string& data);

    const std::string& topic() const;

private:
    std::mutex m_mutex;
    std::string m_topic;
    std::string m_joinRef;
    std::unordered_multimap<std::string,callback> m_topicToCallback;
    typedef std::unordered_map<std::string,phoenix::push> idToPush_t;
    idToPush_t m_idToPush;
    phoenix::socket& m_socket;
    boost::asio::io_service m_ioService;

    std::string getRef();

};
}

#endif // CHANNEL_H
