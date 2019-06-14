#ifndef PUSH_H
#define PUSH_H
#include <chrono>
#include "channel_message.h"
#include "websocketpp/websocketpp/common/asio.hpp"
#include <memory>
#include <mutex>

namespace phoenix {
class channel;
class push
{
public:
    typedef std::function<void(channelMessage&)> callback;
    typedef std::chrono::milliseconds duration;
    push(channel& channel,const std::string& topic, const std::string& event, const std::string& payload, const std::string& ref,const std::string& joinRef);
    push(const push& push) = delete;
    push(push&& other);
    ~push();
    void send();
    void start(duration timeout);
    void processResponse(channelMessage& channelMessage);
    push& receive(const std::string& status,callback callback);
private:
       std::string m_ref;
       std::mutex m_mutex;
       channel& m_channel;
       channelMessage m_message;
       std::unordered_multimap<std::string,callback> m_eventToCallback;
       std::unique_ptr<boost::asio::deadline_timer> m_deatlineTimer;

       void invokeCallbacks(phoenix::channelMessage &channelMessage);



};
}


#endif // PUSH_H
