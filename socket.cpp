#include "socket.h"
#include "channel_message.h"

phoenix::socket::socket(const std::string &uri, const std::string &hostname, const std::string &cacert):m_cacert(cacert),m_hostname(hostname){

    m_onOkCallback = [](){};
    m_onErrorCallback = [](const std::string& error){};
    m_stop = false;
    m_client.set_access_channels(websocketpp::log::alevel::none);
    m_client.clear_access_channels(websocketpp::log::alevel::frame_payload);
    m_client.set_error_channels(websocketpp::log::elevel::none);
    m_client.init_asio();
    m_client.set_message_handler(bind(&socket::on_message, this, std::placeholders::_1,std::placeholders::_2));
    m_client.set_tls_init_handler(bind(&socket::on_tls_init, this, m_hostname.c_str(), std::placeholders::_1));
    m_client.set_open_handler([this](websocketpp::connection_hdl hdl){
        rejoinAllChannels();
        {
            std::unique_lock<std::mutex> lock(m_cvMutex);
            m_state = OPENED;
            m_cv.notify_one();
        }
    });
    m_client.set_close_handler([this](websocketpp::connection_hdl hdl){

        {
            std::unique_lock<std::mutex> lock(m_cvMutex);
            m_state = CLOSED;
            m_cv.notify_one();
        }
        if(!this->m_stop)
            connect();
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    });
    m_client.set_fail_handler([this](websocketpp::connection_hdl hdl){
        {
            std::unique_lock<std::mutex> lock(m_cvMutex);
            m_state = FAILED;
            m_cv.notify_one();
        }
        m_onErrorCallback("Fail in hadler");
        if(!this->m_stop)
            connect();
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    });
    m_client.start_perpetual();
    m_uri = uri;
    connect();
    m_future = std::async(std::launch::async,[this](){
        //TODO stopping mechanism
        while(!this->m_stop){
            try{
                m_client.run();
            }
            catch(const std::exception& e){
                std::cout << "Running thread failed with: "<< e.what() << std::endl;

                websocketpp::lib::error_code  ec;
                m_client.close(m_hdl, websocketpp::close::status::protocol_error, "Error", ec );
                m_onErrorCallback(e.what());
                if(ec){
                    state state;
                    {
                        std::unique_lock<std::mutex> lock(m_cvMutex);
                        state = m_state;
                    }
                    if(state != OPENED){
                        connect();
                    }
                    std::cout << "Failed closing socket: "<< ec.message() << std::endl;
                }
            }
        }
    });


    m_heartbeatFuture = std::async(std::launch::async,[this](){
        //TODO stopping
        while(!this->m_stop){
            int timeDiff = 500;
            for(int i = 0; i < m_defaultHeartbeatMs; i = i + timeDiff){
                std::this_thread::sleep_for(std::chrono::milliseconds(timeDiff));
                if(this->m_stop)
                    break;
            }

            try{
                channelMessage message = {"phoenix","heartbeat","",std::to_string(this->getRef()),""};
                nlohmann::json json = message;
                this->send(json.dump());
            }
            catch(const std::exception& e){
                std::cout << "Running heartbeat thread failed with: "<< e.what() <<  std::endl;
                websocketpp::lib::error_code  ec;
                m_client.close(m_hdl, websocketpp::close::status::protocol_error, "Error", ec );
                m_onErrorCallback(e.what());
                if(ec){
                    std::cout << "Failed closing socket in heartbeat: "<< ec.message() << std::endl;
                }
            }
        }
    });

}

phoenix::socket::~socket()
{
    m_stop = true;
    m_client.stop_perpetual();
    m_client.stop();
    m_future.wait();

}

void phoenix::socket::connect()
{
    websocketpp::lib::error_code ec;
    m_con = m_client.get_connection(m_uri, ec);
    if (ec) {
        std::cout << "Failed to get connection: " << ec << std::endl;
    }
    m_hdl = m_con->get_handle();
    m_client.connect(m_con);
}

void phoenix::socket::waitForConnection()
{
    std::unique_lock<std::mutex> lock(m_cvMutex);
    m_cv.wait(lock,[this](){return m_state == OPENED;});
}

void phoenix::socket::waitForConnectionOrFail()
{
    std::unique_lock<std::mutex> lock(m_cvMutex);
    m_cv.wait(lock,[this](){return m_state != INITIAL;});
}

void phoenix::socket::setOnOkCallback(const std::function<void ()> &onOkCallback)
{
    m_onOkCallback = onOkCallback;
}

void phoenix::socket::setOnErrorCallback(const std::function<void (const std::string &error)> &onErrorCallback)
{
    m_onErrorCallback = onErrorCallback;
}

void phoenix::socket::rejoinAllChannels()
{
    std::unique_lock<std::mutex> lock(m_toppicToChannelsMutex);
    for(auto& topicChannels : m_toppicToChannels){
        //TDO maybe something else on fail
        topicChannels.second.join().
            receive("ok",[](phoenix::channelMessage& message){
                                       std::cout << "Successfully joined channel with topic " << message.topic << std::endl;
                                   }).
            receive("error",[this](phoenix::channelMessage& message){
                    std::cout << "Failed joining to channel with topic, error: " << message.topic << "," << message.payload << std::endl;
                    m_client.close(m_hdl, websocketpp::close::status::protocol_error, "Error" );
                }).
            start(phoenix::push::duration(0));
    }
}


void phoenix::socket::send(const std::string &data){
    m_client.send(m_hdl, data, websocketpp::frame::opcode::binary);
}

phoenix::channel& phoenix::socket::getChannel(const std::string &topic)
{
    std::lock_guard<std::mutex> lock(m_toppicToChannelsMutex);
    auto iter = m_toppicToChannels.emplace(std::piecewise_construct,std::forward_as_tuple(topic),std::forward_as_tuple(topic,*this));
    return iter->second;
}

void phoenix::socket::removeChannel(const std::string &topic)
{
    std::lock_guard<std::mutex> lock(m_toppicToChannelsMutex);
    m_toppicToChannels.erase(topic);
}


uint32_t phoenix::socket::getRef(){
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_ref++;
}

void phoenix::socket::on_message(websocketpp::connection_hdl, client::message_ptr msg)
{
    //TODO some kind of serialization
    if(m_stop)
        return;
    m_onOkCallback();
    nlohmann::json json = nlohmann::json::parse(msg->get_payload());
    channelMessage channelMessage = phoenix::channelMessage(json);
    //TODO reduce lock
    {
        std::lock_guard<std::mutex> lock(m_toppicToChannelsMutex);
        auto range = m_toppicToChannels.equal_range(channelMessage.topic);
        for(auto it = range.first; it != range.second; ++it){
            it->second.processMessage(channelMessage);
        }
    }
}

phoenix::socket::context_ptr phoenix::socket::on_tls_init(const char *hostname, websocketpp::connection_hdl) {
    context_ptr ctx = websocketpp::lib::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::sslv23);

    try {
        ctx->set_options(boost::asio::ssl::context::default_workarounds |
                         boost::asio::ssl::context::no_sslv2 |
                         boost::asio::ssl::context::no_sslv3 |
                         boost::asio::ssl::context::single_dh_use);


        ctx->set_verify_mode(boost::asio::ssl::verify_peer);
        ctx->set_verify_callback(boost::asio::ssl::rfc2818_verification(hostname));

        ctx->add_certificate_authority(const_buffer(m_cacert.data(), m_cacert.size()));
    } catch (std::exception& e) {
        std::cout << e.what() << std::endl;
    }
    return ctx;
}

