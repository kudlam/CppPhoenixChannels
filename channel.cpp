#include "channel.h"
#include "json.hpp"
#include "socket.h"

phoenix::channel::channel(const std::string &topic, phoenix::socket& socket):m_topic(topic),m_socket(socket){
    m_joinRef = std::to_string(m_socket.getRef());
}

phoenix::channel::channel(phoenix::channel &&other):m_mutex(),m_topic(std::move(other.m_topic)),m_joinRef(std::move(other.m_joinRef)),m_topicToCallback(std::move(other.m_topicToCallback)),
    m_idToPush(std::move(other.m_idToPush)),m_socket(other.m_socket)
{
}

phoenix::channel::~channel()
{
    //std::cout << "Destructing channel" << std::endl;
}

phoenix::push& phoenix::channel::join(){
    return push(JOIN, "" );
}

phoenix::push& phoenix::channel::push( const std::string& event, const std::string& payload){
    auto ref = getRef();
    std::pair<idToPush_t::iterator,bool> it;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        it = m_idToPush.emplace(std::piecewise_construct,std::forward_as_tuple(ref),std::forward_as_tuple(*this,m_topic, event,payload, ref, m_joinRef));
    }
    if(!it.second){
        throw std::runtime_error("Can start push");
    }
    it.first->second.send();
    return it.first->second;
}

void phoenix::channel::processMessage(phoenix::channelMessage& message)
{
    if(message.join_ref != "" && message.join_ref != m_joinRef )
        return;
    std::vector<callback> callbacks;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto range = m_topicToCallback.equal_range(message.event);
        for(auto it = range.first;it != range.second;++it){
            callbacks.push_back(it->second);
        }
    }
    for(auto& callback:callbacks){
        callback(message);
    }
    std::vector<phoenix::push> pushes;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto rangePushes = m_idToPush.equal_range(message.ref);
        for(auto it = rangePushes.first; it != rangePushes.second;++it)
            pushes.push_back(std::move(it->second));
        m_idToPush.erase(message.ref);
    }
    for(auto& push: pushes){
        push.processResponse(message);
    }

}

void phoenix::channel::on(const std::string &eventName, phoenix::channel::callback callback){
    std::lock_guard<std::mutex> lock(m_mutex);
    m_topicToCallback.insert(std::make_pair(eventName,callback));
}

void phoenix::channel::off(const std::string &eventName)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_topicToCallback.erase(eventName);
}

void phoenix::channel::offId(const std::string &redId)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_idToPush.erase(redId);
}

std::unique_ptr<boost::asio::deadline_timer> phoenix::channel::setTimeout(std::chrono::milliseconds duration)
{
    std::unique_ptr<boost::asio::deadline_timer> timer = std::unique_ptr<boost::asio::deadline_timer>(new boost::asio::deadline_timer(m_ioService));
    timer->expires_from_now(boost::posix_time::milliseconds(duration.count()));
    timer->async_wait([this](const boost::system::error_code& ){
        channelMessage message{"","","{\"status\":\"timeout\"}","",""};
        //TODO this could be faster
        this->processMessage(message);
    });
    return timer;
}

void phoenix::channel::sendData(const std::string &data)
{
    m_socket.send(data);
}

std::string phoenix::channel::getRef()
{
    return std::to_string(m_socket.getRef());
}

const std::string& phoenix::channel::topic() const
{
    return m_topic;
}

