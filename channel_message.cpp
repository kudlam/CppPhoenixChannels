#include "channel_message.h"

void phoenix::to_json(nlohmann::json &j, const phoenix::channelMessage &message) {
    j = nlohmann::json{{"topic", message.topic}, {"event", message.event}, {"payload", message.payload}, {"ref", message.ref}, {"join_ref", message.join_ref}};
}

void phoenix::from_json(const nlohmann::json &j, phoenix::channelMessage &message) {
    j.at("topic").get_to(message.topic);
    j.at("event").get_to( message.event);
    message.payload = j.at("payload").dump();
    if(j.find("ref") != j.end())
        j.at("ref").get_to(message.ref);
    if(j.find("join_ref") != j.end())
        j.at("join_ref").get_to(message.join_ref);
}
