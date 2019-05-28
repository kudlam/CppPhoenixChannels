#include "push.h"
#include "channel.h"


phoenix::push::push(std::shared_ptr<channel> channel,const std::string& topic, const std::string& event, const std::string& payload, const std::string& ref, const std::string& joinRef):m_timeout(5000){
    m_channel = channel;
    m_message = {topic,event,payload,ref,joinRef};
}

void phoenix::push::send()
{
    nlohmann::json json = m_message;
    m_channel->sendData(json.dump());
}

phoenix::push& phoenix::push::setTimemout(phoenix::push::duration timeout, std::shared_ptr<boost::asio::io_service> ioservice){
    m_timeout = timeout;
    m_ioservice = ioservice;
    m_timer = std::unique_ptr<boost::asio::deadline_timer>(new boost::asio::deadline_timer(*m_ioservice));
    m_timer->expires_from_now(m_timeout);
    m_timer->async_wait([](const boost::system::error_code& ){

    });
    return *this;
}

void phoenix::push::processResponse(phoenix::channelMessage &channelMessage)
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_messageReceived = true;
        m_ReceivedMessage = channelMessage;
    }
    invokeCallbacks();
}

phoenix::push &phoenix::push::receive(const std::string &status, phoenix::push::callback callback){
    m_eventToCallback.insert(std::make_pair(status,callback));
    invokeCallbacks();
    return *this;
}

void phoenix::push::invokeCallbacks()
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if(m_messageReceived){
            nlohmann::json json = nlohmann::json::parse(m_ReceivedMessage.payload);
            auto range = m_eventToCallback.equal_range(json["status"]);
            for(auto it = range.first; it != range.second; ++it){
                it->second(m_message);
            }
        }
    }
}

