#include "channel.h"
#include "json.hpp"
#include "push.h"

phoenix::channel::channel(const std::string &topic, std::shared_ptr<phoenix::socket> socket):m_topic(topic),m_socket(socket){
    m_joinRef = std::to_string(m_socket->getRef());
}

std::shared_ptr<phoenix::push> phoenix::channel::join(){
    return push( JOIN, "" );
}

std::shared_ptr<phoenix::push> phoenix::channel::push( const std::string& event, const std::string& payload){
    auto ref = getRef();
    auto push = std::make_shared<phoenix::push>(shared_from_this(),m_topic, event,payload, ref, m_joinRef);
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_idToPush.insert(std::make_pair(ref,push));
    }
    push->send();
    return push;
}

void phoenix::channel::processMessage(std::unique_ptr<phoenix::channelMessage> message)
{
    if(message->join_ref != "" && message->join_ref != m_joinRef )
        return;
    auto range = m_topicToCallback.equal_range(message->event);
    for(auto it = range.first;it != range.second;++it){
        it->second(*message);
    }
    std::vector<std::shared_ptr<phoenix::push>> pushes;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto rangePushes = m_idToPush.equal_range(message->ref);
        for(auto it = rangePushes.first; it != rangePushes.second;++it)
            pushes.push_back(it->second);
        m_idToPush.erase(message->ref);
    }
    for(auto& push: pushes){
        push->processResponse(*message);
    }

}

void phoenix::channel::on(const std::string &event_name, phoenix::channel::callback callback){
    m_topicToCallback.insert(std::make_pair(event_name,callback));
}

void phoenix::channel::sendData(const std::string &data)
{
    m_socket->send(data);
}

std::string phoenix::channel::getRef()
{
    return std::to_string(m_socket->getRef());
}

const std::string& phoenix::channel::topic() const
{
    return m_topic;
}

