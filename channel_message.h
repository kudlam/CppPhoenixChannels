#ifndef CHANNELMESSAGE_H
#define CHANNELMESSAGE_H
#include <string>
#include "json.hpp"

namespace phoenix {
static std::string CLOSE = "phx_close";
static std::string ERROR = "phx_error";
static std::string JOIN="phx_join";
static std::string REPLY="phx_reply";
static std::string LEAVE="phx_leave";
struct channelMessage{
public:
    std::string topic;
    std::string event;
    std::string payload;
    std::string ref;
    std::string join_ref;
};
void to_json(nlohmann::json& j, const channelMessage& message);

void from_json(const nlohmann::json& j, channelMessage& message);

}
#endif // CHANNELMESSAGE_H
