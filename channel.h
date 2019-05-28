#ifndef CHANNEL_H
#define CHANNEL_H

#include "socket.h"
#include "channel_message.h"

namespace phoenix {
class push;
class channel: public std::enable_shared_from_this<channel>{
public:
    typedef std::function<void (channelMessage&)>  callback;

    channel(const std::string& topic, std::shared_ptr<phoenix::socket> socket);
    std::shared_ptr<phoenix::push> join();
    std::shared_ptr<phoenix::push> push(const std::string& event, const std::string& payload);
    void processMessage(std::unique_ptr<channelMessage> message);
    void on(const std::string& event_name, callback  callback);
    void sendData(const std::string& data);

    const std::string& topic() const;

private:
    std::mutex m_mutex;
    std::string m_topic;
    std::string m_joinRef;
    std::unordered_multimap<std::string,callback> m_topicToCallback;
    std::unordered_map<std::string,std::shared_ptr<phoenix::push>> m_idToPush;
    std::shared_ptr<phoenix::socket> m_socket;
    std::shared_ptr<phoenix::push> m_push;

    std::string getRef();

};
}

#endif // CHANNEL_H
