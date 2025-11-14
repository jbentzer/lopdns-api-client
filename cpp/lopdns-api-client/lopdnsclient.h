#ifndef LOPDNSCLIENT_H
#define LOPDNSCLIENT_H

#include <string>
#include <list>
#include "restclient-cpp/connection.h"
#include "restclient-cpp/restclient.h"

typedef struct Zone
{
    std::string name;
    int serial;
    int refresh;
    int retry;
    int expire;
    int minimum;
} Zone;

typedef struct Record
{
    std::string name;
    std::string type;
    std::string content;
    int ttl;
    int priority;
} Record;

typedef struct Token
{
    std::string token;
    std::string expires;
    long long epochExpires;
    std::string tzone;
} Token;

class LopDnsClient
{
public:
    LopDnsClient(const std::string& url = "https://api.lopdns.se/v2", int timeoutInSeconds = 10);
    ~LopDnsClient();

    // Methods for interacting with the API
    bool authenticate(const std::string& client_id, const int durationInSeconds);
    bool isTokenExpired(const int minTimeLeftInSeconds = 30);
    bool validateToken();
    std::list<std::string> getZones();
    std::list<Record> getRecords(const std::string& zone_name);
    Record createRecord(const std::string& zone_name, const std::string& record_name,
                                        const std::string& type, const std::string& content,
                                        int ttl = 3600, int priority = 0);
    Record updateRecord(const std::string& zone_name, const std::string& old_record_name,
                                       const std::string& matching_type, const std::string& old_content, 
                                       const std::string& new_content, int ttl = 3600, int priority = 0);
    bool deleteRecord(const std::string& zone_name, const std::string& record_name,
                                        const std::string& type, const std::string& content);

private:
    Token token;
    std::string url;
    int timeout;
    RestClient::Response makeRestCall(const std::string& method, const std::string& endpoint, bool applyAuthHeaders = true,
                      const RestClient::HeaderFields& headers = RestClient::HeaderFields(),
                      const std::map<std::string, std::string>& queryParams = std::map<std::string, std::string>(),
                      const std::string& body = "");
};

#endif // LOPDNSCLIENT_H