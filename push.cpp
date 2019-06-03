#include "push.h"
#include "channel.h"
#include <iostream>


phoenix::push::push(channel& channel,const std::string& topic, const std::string& event, const std::string& payload, const std::string& ref, const std::string& joinRef):
    m_channel(channel), m_deatlineTimer(std::unique_ptr<boost::asio::deadline_timer>()){
    m_message = {topic,event,payload,ref,joinRef};
    m_ref = ref;
}

phoenix::push::push(phoenix::push &&other):m_ref(std::move(other.m_ref)),
    m_mutex(),
    m_channel(other.m_channel),
    m_message(std::move(other.m_message)),
    m_eventToCallback(std::move(other.m_eventToCallback)),
    m_deatlineTimer(std::move(other.m_deatlineTimer))
{

}

phoenix::push::~push()
{
    if (m_deatlineTimer){
        m_deatlineTimer->cancel();
        m_deatlineTimer.reset();
    }
    //std::cout << "Destructing push with m_ref: " << m_ref << std::endl;
}

void phoenix::push::send()
{
    nlohmann::json json = m_message;
    m_channel.sendData(json.dump());
}

void phoenix::push::start(phoenix::push::duration timeout)
{
    send();
    m_deatlineTimer = m_channel.setTimeout(timeout);

}

void phoenix::push::processResponse(phoenix::channelMessage &channelMessage)
{
    invokeCallbacks(channelMessage);

}

phoenix::push &phoenix::push::receive(const std::string &status, phoenix::push::callback callback){
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_eventToCallback.insert(std::make_pair(status,callback));
    }
    return *this;
}

void phoenix::push::invokeCallbacks(phoenix::channelMessage &channelMessage)
{
    {
        nlohmann::json json = nlohmann::json::parse(channelMessage.payload);
        //TODO logg error on missing
        auto status = json["status"];
        std::lock_guard<std::mutex> lock(m_mutex);
        auto range = m_eventToCallback.equal_range(status);
        for(auto it = range.first; it != range.second; ++it){
            it->second(channelMessage);
        }
    }
}

