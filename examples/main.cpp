#include <iostream>
#include "../socket.h"
#include "../push.h"
#include <thread>

using namespace std;

int main()
{
    cout << "Starting channel" << endl;
    std::string cacert("-----BEGIN CERTIFICATE-----\n"
                       "MIICqzCCAZOgAwIBAgIBATANBgkqhkiG9w0BAQUFADAAMB4XDTE5MDYwMjE5MjYw\n"
                       "MFoXDTIwMDYwMjE5MjYwMFowADCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoC\n"
                       "ggEBANGBUtG4+Wqx2uw0YQxGuZTXU2mPDlzshfLxuipji1RAliR/eC/mpPhsrhsZ\n"
                       "qZt3SK7uOcq7porBOW/6WjRD3VPgArHqAmjEGFv3rafIA+fjrXq1xbPqp4Ac6x5G\n"
                       "co1RghNAjg6IOLJ5qb5zTdOiHy2NZnsPsIFx/Dt5++d7sAPiQn86M7oa7xesqaMg\n"
                       "/KsMGk/Nr2kUErJmipfj53xsqHAUbTCeRJHcIMQg9htOKXbhqGMeJATKhtr8mheK\n"
                       "RgTB8uGzbARKPa4fFAV8t+b8tkq5madx+mRkB9r6+VHpOzDV8Wz/rW7Dqk1Sc3zA\n"
                       "1BRrY1JuwTzlMroQTwDfPXmYAakCAwEAAaMwMC4wDAYDVR0TBAUwAwEB/zALBgNV\n"
                       "HQ8EBAMCAoQwEQYJYIZIAYb4QgEBBAQDAgIEMA0GCSqGSIb3DQEBBQUAA4IBAQCw\n"
                       "DKu0XJCoM+Jm9Ggj8My96t6j/dQLA6UjQHJvAlB3Dg1zPmM9w5jzhJUT/U7792JT\n"
                       "uN5iYL1rH4OZlCtn+AM/8+tXtP556fOB9s2whVvvkAYh45H7y5QGlQ4+pA/2e4S7\n"
                       "osiV1vD//uMvxn1CIk9I6nFx02sqS6K2kBhZyaDHwiEMV+CdAXFc8cSCgmxSQtzL\n"
                       "LmVFKiQwrXN6Vni6hajDkc2gtPgCBuIa7d+MEj5jRzp8kwczj3rr7waUW8j0Kw3V\n"
                       "pRVAkjckgvq92SsAE5AemC8LHezxCFO0RFQgL4YAX08gDnBnkNy95LsfZQr/nvDu\n"
                       "mlpzC+EpkDBIrvGpwbHn\n"
                       "-----END CERTIFICATE-----");
    phoenix::socket s("wss://localhost:4002/socket/websocket","localhost",cacert);
    cout << "Socket created" << endl;
    int counter{0};
    std::mutex m;
    auto& channel = s.getChannel("telemetry:lobby");
    auto okCallback = [&counter,&m](phoenix::channelMessage& message){std::lock_guard<std::mutex> lock(m);counter++;};
    auto timeoutCallback = [](phoenix::channelMessage& ){std::cout << "Timeout callback" << std::endl;};
    auto errorHandler = [](phoenix::channelMessage& message){std::cout <<  "Received expected error:" << message.event << std::endl;};
    s.waitForConnection();
    channel.join().receive("ok", okCallback).receive("error",errorHandler).start(phoenix::push::duration(0));

    auto push = [&channel,errorHandler,okCallback,timeoutCallback](int id){channel.push("state","{\"id\":" + std::to_string(id)+"}").receive("error",errorHandler).receive("ok",okCallback).receive("timeout",timeoutCallback).start(phoenix::push::duration(0));};
    for(int i=0;i<1000;i++){
        try{
            push(i);
        }
        catch(const std::exception& e){
            std::cout << "Error during sending: " << i << ", "<< e.what() << std::endl;
        }
        this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
    //std::cout << "Waiting for sleep" << std::endl;
    this_thread::sleep_for(std::chrono::milliseconds(10000));
    std::cout << "Finished: " << counter << std::endl;


    return 0;
}
