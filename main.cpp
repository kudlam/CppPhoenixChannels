#include <iostream>
#include "socket.h"
#include "push.h"
#include <thread>

using namespace std;

int main()
{
    cout << "Starting channel" << endl;
    std::string cacert("MIIDfTCCAmWgAwIBAgIJAMZe1wV7LEOvMA0GCSqGSIb3DQEBCwUAMEMxGjAYBgNVBAoMEVBob2VuaXggRnJhbWV3b3JrMSUwIwYDVQQDDBxTZWxmLXNpZ25lZCB0ZXN0IGNlcnRpZmljYXRlMB4XDTE5MDUxMjAwMDAwMFoXDTIwMDUxMjAwMDAwMFowQzEaMBgGA1UECgwRUGhvZW5peCBGcmFtZXdvcmsxJTAjBgNVBAMMHFNlbGYtc2lnbmVkIHRlc3QgY2VydGlmaWNhdGUwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQDPwx4W+kEhNp3KrNjLI6Wopqurd7rZ+T1haeJUYpzkQxMI686ftS3PmOaJKe4ni+y5tl/Dd1DxSFR3l1rm3ogj63k4s300VOTRET1bJj4i4/WTy3dqRLFErfqDzHi0OUD0Zk8D5RgZF6oZj6d3kFOHRe3y3M4mEunTLPG6Dg+ty/vfvhTwf0zM08nffPoQGdsUy33Ryxn7QAMPF76XUQMvca4BvPynnKnk6G/3XPQsuuUQAbf3Y30ysUcO5pnWVLjbh1WeebmxieQeuyLK7A7TfrWM2S/jcWngzUEnO35Ja3lfYOMwQuioyKznus9CV1PUZyu4X1PZIzNF2hwz2MadAgMBAAGjdDByMAwGA1UdEwEB/wQCMAAwDgYDVR0PAQH/BAQDAgWgMB0GA1UdJQQWMBQGCCsGAQUFBwMBBggrBgEFBQcDAjAdBgNVHQ4EFgQUOMIC+w3UP2LGrDRSzhhzVYElDjEwFAYDVR0RBA0wC4IJbG9jYWxob3N0MA0GCSqGSIb3DQEBCwUAA4IBAQBOf+nn2kg+OAV1ovT1XSdhr3EsH16M/86CT1ct8MbyNQOxJYmSWKsFuN2Kav9A+73+eqhgp1zAyz226bJpG9QLEsxmf/gxFZPIOj1mMejgtaAUWoIWvvDDmrY+EGGpLtjewJZ8k+b19BEXdXPnVjNPFBhfwtF8JS/7OwAiiFtpnsFw2db//194rHmjpZ+WoWgWBr7/LB7n4Bg3XpQqsDKUcEqTGNJqD4nV0rPc3UArDeCnhGcq+hnGc9jcgPsjj0A+puVa1iBH7pMHWoKoPwiNoUDbpo/kX01EhORFsq4vVrhj/KC6X4/EdXqFvwlEwDhgQAvGKU4hhxBkGuf8564j");

    phoenix::socket s("wss://localhost:4002/socket/websocket","localhost",cacert);
    cout << "Socket created" << endl;
    auto& channel = s.getChannel("telemetry:lobby");
    auto okCallback = [](phoenix::channelMessage& message){std::cout <<  "Processing message:" << message.event << std::endl;};
    auto timeoutCallback = [](phoenix::channelMessage& ){std::cout << "Timeout callback" << std::endl;};
    auto errorHandler = [](phoenix::channelMessage& message){std::cout <<  "Received expected error:" << message.event << std::endl;};
    channel.join().receive("ok", okCallback).receive("error",errorHandler);

    auto push = [&channel,errorHandler,okCallback,timeoutCallback](){channel.push("ping","").receive("error",errorHandler).receive("ok",okCallback).receive("timeout",timeoutCallback).start(phoenix::push::duration(10));};
    for(int i=0;i<100;i++)
        push();
    std::cout << "Waiting for sleep" << std::endl;
    this_thread::sleep_for(std::chrono::milliseconds(5000));


    return 0;
}
