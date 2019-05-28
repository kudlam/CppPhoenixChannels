#ifndef PUSH_H
#define PUSH_H
#include <chrono>
#include "channel_message.h"
#include "websocketpp/common/asio.hpp"
#include <memory>
#include <mutex>

namespace phoenix {
class channel;
class push
{
public:
    typedef std::function<void(channelMessage&)> callback;
    typedef boost::posix_time::millisec duration;
    push(std::shared_ptr<channel> channel,const std::string& topic, const std::string& event, const std::string& payload, const std::string& ref,const std::string& joinRef);
    void send();
    push& setTimemout(duration timeout, std::shared_ptr<boost::asio::io_service> ioservice);
    void processResponse(channelMessage& channelMessage);
    push& receive(const std::string& status,callback callback);
private:
       std::mutex m_mutex;
       std::shared_ptr<channel> m_channel;
       channelMessage m_message;
       bool m_messageReceived = false;
       channelMessage m_ReceivedMessage;
       duration m_timeout;
       std::unique_ptr<channelMessage> m_response;
       std::shared_ptr<boost::asio::io_service> m_ioservice;
       std::unordered_multimap<std::string,callback> m_eventToCallback;
       std::unique_ptr<boost::asio::deadline_timer> m_timer;

       void invokeCallbacks();



};
}


#endif // PUSH_H
