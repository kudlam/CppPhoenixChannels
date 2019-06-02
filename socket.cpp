#include "socket.h"
#include "channel_message.h"

phoenix::socket::socket(const std::string &uri, const std::string &hostname, const std::string &cacert):m_cacert(cacert),m_hostname(hostname){
    m_stop = false;
    m_client.init_asio();
    m_client.set_message_handler(bind(&socket::on_message, this, std::placeholders::_1,std::placeholders::_2));
    m_client.set_tls_init_handler(bind(&socket::on_tls_init, this, m_hostname.c_str(), std::placeholders::_1));
    m_client.set_open_handler([this](websocketpp::connection_hdl hdl){
        std::unique_lock<std::mutex> lock(m_cvMutex);
        m_state = OPENED;
        m_cv.notify_one();
    });
    m_client.set_close_handler([this](websocketpp::connection_hdl hdl){
        std::unique_lock<std::mutex> lock(m_cvMutex);
        m_state = CLOSED;
        m_cv.notify_one();
        connect();
    });
    m_client.set_fail_handler([this](websocketpp::connection_hdl hdl){
        std::unique_lock<std::mutex> lock(m_cvMutex);
        m_state = FAILED;
        m_cv.notify_one();
        connect();
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    });
    m_uri = uri;
    connect();
    m_future = std::async(std::launch::async,[this](){
        //TODO stopping mechanism
        while(!this->m_stop){
            try{
                m_client.run();
            }
            catch(const std::exception& e){
                std::cout << "Running thread failed with: "<< e.what() <<  std::endl;
            }
        }
    });
    waitForConnection();


    m_heartbeatFuture = std::async(std::launch::async,[this](){
        //TODO stopping
        while(!this->m_stop){
            try{
                channelMessage message = {"phoenix","heartbeat","",std::to_string(this->getRef()),""};
                nlohmann::json json = message;
                this->send(json.dump());
                std::this_thread::sleep_for(std::chrono::milliseconds(m_defaultHeartbeatMs));
            }
            catch(const std::exception& e){
                std::cout << "Running thread failed with: "<< e.what() <<  std::endl;
            }
        }
    });

}

phoenix::socket::~socket()
{
    m_stop = true;
    m_client.stop();
}

void phoenix::socket::connect()
{
    websocketpp::lib::error_code ec;
    client::connection_ptr con = m_client.get_connection(m_uri, ec);
    if (ec) {
        std::cout << "Failed to get connection: " << ec << std::endl;
    }
    m_hdl = con->get_handle();
    m_client.connect(con);
}

void phoenix::socket::waitForConnection()
{
    std::unique_lock<std::mutex> lock(m_cvMutex);
    m_cv.wait(lock,[this](){return m_state != INITIAL;});
}

void phoenix::socket::send(const std::string &data){
    m_client.send(m_hdl, data, websocketpp::frame::opcode::text);
}

phoenix::channel& phoenix::socket::getChannel(const std::string &topic)
{
    std::lock_guard<std::mutex> lock(m_toppicToChannelsMutex);
    auto iter = m_toppicToChannels.emplace(std::piecewise_construct,std::forward_as_tuple(topic),std::forward_as_tuple(channel(topic,*this)));
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
        ctx->set_verify_callback(bind(&socket::verify_certificate, this, hostname, std::placeholders::_1, std::placeholders::_2));


        ctx->add_certificate_authority(const_buffer(m_cacert.data(), m_cacert.size()));
    } catch (std::exception& e) {
        std::cout << e.what() << std::endl;
    }
    return ctx;
}

bool phoenix::socket::verify_certificate(const char *hostname, bool preverified, boost::asio::ssl::verify_context &ctx) {

    int depth = X509_STORE_CTX_get_error_depth(ctx.native_handle());

    if (depth == 0 ) {
        X509* cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());

        if (verify_subject_alternative_name(hostname, cert)) {
            return true;
        } else if (verify_common_name(hostname, cert)) {
            return true;
        } else {
            return false;
        }
    }

    return preverified;
}

bool phoenix::socket::verify_subject_alternative_name(const char *hostname, X509 *cert) {
    STACK_OF(GENERAL_NAME) * san_names = NULL;

    san_names = (STACK_OF(GENERAL_NAME) *) X509_get_ext_d2i(cert, NID_subject_alt_name, NULL, NULL);
    if (san_names == NULL) {
        return false;
    }

    int san_names_count = sk_GENERAL_NAME_num(san_names);

    bool result = false;

    for (int i = 0; i < san_names_count; i++) {
        const GENERAL_NAME * current_name = sk_GENERAL_NAME_value(san_names, i);

        if (current_name->type != GEN_DNS) {
            continue;
        }

        char * dns_name = (char *) ASN1_STRING_data(current_name->d.dNSName);

        if (ASN1_STRING_length(current_name->d.dNSName) != strlen(dns_name)) {
            break;
        }

        result = (strcasecmp(hostname, dns_name) == 0);
    }
    sk_GENERAL_NAME_pop_free(san_names, GENERAL_NAME_free);

    return result;
}

bool phoenix::socket::verify_common_name(const char *hostname, X509 *cert) {

    int common_name_loc = X509_NAME_get_index_by_NID(X509_get_subject_name(cert), NID_commonName, -1);
    if (common_name_loc < 0) {
        return false;
    }

    X509_NAME_ENTRY * common_name_entry = X509_NAME_get_entry(X509_get_subject_name(cert), common_name_loc);
    if (common_name_entry == NULL) {
        return false;
    }

    ASN1_STRING * common_name_asn1 = X509_NAME_ENTRY_get_data(common_name_entry);
    if (common_name_asn1 == NULL) {
        return false;
    }

    char * common_name_str = (char *) ASN1_STRING_data(common_name_asn1);

    if (ASN1_STRING_length(common_name_asn1) != strlen(common_name_str)) {
        return false;
    }

    return (strcasecmp(hostname, common_name_str) == 0);
}
