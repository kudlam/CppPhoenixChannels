#include <iostream>
#include "../socket.h"
#include "../push.h"
#include <thread>
#include <boost/date_time.hpp>

using namespace std;

int main()
{
    cout << "Starting channel" << endl;
    std::string cacert("-----BEGIN CERTIFICATE-----\n"
                       "MIIDqDCCApCgAwIBAgIUQ34DIgMDegmeuTohSOaAFRkFzpAwDQYJKoZIhvcNAQEL\n"
                       "BQAweDELMAkGA1UEBhMCWFgxDDAKBgNVBAgMA04vQTEMMAoGA1UEBwwDTi9BMSAw\n"
                       "HgYDVQQKDBdTZWxmLXNpZ25lZCBjZXJ0aWZpY2F0ZTErMCkGA1UEAwwiMTIwLjAu\n"
                       "MC4xOiBTZWxmLXNpZ25lZCBjZXJ0aWZpY2F0ZTAeFw0yMDExMjAxMjM2NTlaFw0y\n"
                       "MjExMjAxMjM2NTlaMHgxCzAJBgNVBAYTAlhYMQwwCgYDVQQIDANOL0ExDDAKBgNV\n"
                       "BAcMA04vQTEgMB4GA1UECgwXU2VsZi1zaWduZWQgY2VydGlmaWNhdGUxKzApBgNV\n"
                       "BAMMIjEyMC4wLjAuMTogU2VsZi1zaWduZWQgY2VydGlmaWNhdGUwggEiMA0GCSqG\n"
                       "SIb3DQEBAQUAA4IBDwAwggEKAoIBAQCqKNeIA+i0YAgXsB1nXhKAWrqOKhZXM+Z8\n"
                       "T8Dawx927ufUeRloz3RbxU13uyMyRx2Icekf6WTQCOVb5TAmksC3Fviht4CVL5zy\n"
                       "idAUMzCLJp046w6Zr8EYz/DnTaROCXNfp6DnC/n2lkfCnFDXy5yubC/Sjo6xUbtG\n"
                       "LfC6L3d6Sx4ih6mNzS5mV33f3r3vs55mUj+un7k1heWv1uWUKxJegyG8To/9zHrl\n"
                       "byO+75OWL2m4G9oo7SJh/HzvctDnBoZj3pD6TnpXCsO1RU2WCaZvVACvEgqLQXz1\n"
                       "JpefYiWXTWzyV/VaJFLJYMHsJbaPXqMvALrNhiRmEDbl/ZEGI36zAgMBAAGjKjAo\n"
                       "MCYGA1UdEQQfMB2HBH8AAAGHBMCoWAqHBMCoCGqCCWxvY2FsaG9zdDANBgkqhkiG\n"
                       "9w0BAQsFAAOCAQEAPWdxMQ0KZumowQjn+T3wWFMIM2aRuMv8Bo4CJc1YfzHmMs/Z\n"
                       "/ezT/7D4qP2n7NSdGA480I1gbvc0GdhUjSgadSFTjICFnrW32JZprDrB9JnOXq5i\n"
                       "ZO+js4PAa6iZ+9386SHwlESPTIZwiKlnzzZzFUDY2IAt+SWia/Uu6HVx4zGUmqs5\n"
                       "ST119LQ6E3KK+0IoFeyprKMKt7nxs5nVIYE4e4RgXgJdYx5AL/2A8at5Bw2R5F/o\n"
                       "tbZP6NitRncsKqffRn22RveOOLGdufVu6dDVmxI7vlxuWG7GwWZjlLJf6SuArtB3\n"
                       "PxWzTcUjiwuiLBVI3C7TB+wK9uz+wvrplVyNZg==\n"
                       "-----END CERTIFICATE-----");
    phoenix::socket s("wss://localhost:4002/socket/websocket","localhost",cacert);
    //phoenix::socket s("ws://localhost:4003/socket/websocket","localhost",cacert);
    cout << "Socket created" << endl;
    s.setOnOkCallback([](){std::cout << "OK" << std::endl;});
    s.setOnErrorCallback([](const std::string& error){std::cout << "Error: " << error <<  std::endl;});
    int counter{0};
    std::mutex m;
    auto& channel = s.getChannel("telemetry:lobby");
    auto okCallback = [&counter,&m](phoenix::channelMessage& message){std::lock_guard<std::mutex> lock(m);counter++;};
    auto timeoutCallback = [](phoenix::channelMessage& ){std::cout << "Timeout callback" << std::endl;};
    auto errorHandler = [](phoenix::channelMessage& message){std::cout <<  "Received expected error:" << message.event << std::endl;};
    s.waitForConnection();
    channel.join().receive("ok", okCallback).receive("error",errorHandler).start(phoenix::push::duration(0));
    this_thread::sleep_for(std::chrono::milliseconds(2000));
    channel.on("event",okCallback);

    auto push = [&channel,errorHandler,okCallback,timeoutCallback](int id){
        std::string data;
        data.reserve(200);
        boost::posix_time::ptime ptime{boost::posix_time::microsec_clock::universal_time()};
        data.append("{\"id\":");
        data.append(std::to_string(id));
        data.append(",\"time\":\"");
        data.append(boost::posix_time::to_iso_extended_string(ptime));
        data.append("\"");
        data.append(",\"position\":");
        data.append("{\"movementId\":\"1\",\"locations\":");
        data.append("[{\"time\":\"" + boost::posix_time::to_iso_extended_string(ptime) + "\",\"latitude\":10,\"longitude\":10,\"altitude\":10}]}");
        data.append("}");
        channel.push("state",data).receive("error",errorHandler).receive("ok",okCallback).receive("timeout",timeoutCallback).start(phoenix::push::duration(0));
        boost::posix_time::ptime ptimeEnd{boost::posix_time::microsec_clock::universal_time()};
        std::cout << "Sending took: " <<  (ptimeEnd-ptime).total_microseconds() << "us" << std::endl;
    };
    for(int i=0;i<10000;i++){
        try{
           push(i);

        }
        catch(const std::exception& e){
            std::cout << "Error during sending: " << i << ", "<< e.what() << std::endl;
        }
        this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    //std::cout << "Waiting for sleep" << std::endl;
    this_thread::sleep_for(std::chrono::milliseconds(1000));
    std::cout << "Finished: " << counter << std::endl;


    return 0;
}
