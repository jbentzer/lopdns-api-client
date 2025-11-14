#include <nlohmann/json.hpp>
#include <restclient-cpp/connection.h>
#include <restclient-cpp/restclient.h>
#include "plog/Log.h"

#include "lopdnsclient.h"
#include <iostream>

using json = nlohmann::json;

LopDnsClient::LopDnsClient(const std::string& url, int timeoutInSeconds)
{
    RestClient::init();

    this->url = url;
    this->timeout = timeoutInSeconds;
}

LopDnsClient::~LopDnsClient()
{
    // Destructor implementation (if needed)
}

bool LopDnsClient::authenticate(const std::string& client_id, const int durationInSeconds)
{
    // Implementation for authenticating the client
    RestClient::HeaderFields headers;
    headers["x-clientid"] = client_id;

    std::map<std::string, std::string> queryParams;
    queryParams["duration"] = std::to_string(durationInSeconds);

    RestClient::Response response = makeRestCall("GET", "/auth/token", false, headers, queryParams);
    if (response.code >= 200 && response.code < 300)
    {
        LOG_DEBUG << "Authentication successful. Response: " << response.body;
        // Parse response and set token details
        json responseBody = json::parse(response.body);
        this->token.token = responseBody["token"];
        this->token.expires = responseBody["expires"];
        this->token.epochExpires = responseBody["epochExpires"].template get<long long>();
        this->token.tzone = responseBody["tz"];

        return true;
    }
    else {
        LOG_ERROR << "Authentication call failed with code: " << response.code;
    }
    return false;
}

bool LopDnsClient::isTokenExpired(const int minTimeLeftInSeconds)
{
    // Implementation for checking if the token is expired
    auto current_time = int(time(nullptr));
    if (!token.epochExpires)
    {
        return true;
    }
    return current_time > token.epochExpires - minTimeLeftInSeconds;
}

bool LopDnsClient::validateToken()
{
    // Implementation for validating the token
    if (this->isTokenExpired(30))
    {
        return false;
    }

    RestClient::Response response = makeRestCall("GET", "/auth/validate");
    if (response.code >= 200 && response.code < 300)
    {
        LOG_DEBUG << "Token validation successful. Response: " << response.body;
        return true;
    }
    else {
        LOG_ERROR << "Token validation call failed with code: " << response.code;
    }

    return false;
}

std::list<std::string> LopDnsClient::getZones()
{
    // Implementation for getting the list of zones
    RestClient::Response response = makeRestCall("GET", "/zones");
    if (response.code >= 200 && response.code < 300)
    {
        // Parse response and return list of zones
        std::list<std::string> zones;
        json responseBody = json::parse(response.body);
        if (responseBody.is_array()) {
            zones.assign(responseBody.begin(), responseBody.end());
        } else {
            throw std::runtime_error("expected top-level array");
        }
        /*
        for (const auto& zoneData : responseBody) {
            std::cout << "Zone: " << zoneData.at("name").get<std::string>() << std::endl;
            
            zones.emplace_back(Zone{
                zoneData.at("name").get<std::string>(),
                zoneData.at("serial").get<int>(),
                zoneData.at("refresh").get<int>(),
                zoneData.at("retry").get<int>(),
                zoneData.at("expire").get<int>(),
                zoneData.at("minimum").get<int>()
            });
        */
        LOG_DEBUG << "Retrieved " << zones.size() << " zones.";

        return zones;
    }
    else {
        LOG_ERROR << "Token validation call failed with code: " << response.code;
    }

    return {};
}

std::list<Record> LopDnsClient::getRecords(const std::string& zone_name)
{
    // Implementation for getting the list of records for a zone
    RestClient::Response response = makeRestCall("GET", "/records/" + zone_name);
    if (response.code >= 200 && response.code < 300)
    {
        // Parse response and return list of records
        std::list<Record> records;
        json responseBody = json::parse(response.body);
        if (responseBody.is_array()) {
            LOG_DEBUG << "Retrieved " << responseBody.size() << " records for zone: " << zone_name;
        } else {
            throw std::runtime_error("expected top-level array");
        }
        for (const auto& recordData : responseBody)
        {
            Record record;
            record.name = recordData["name"];
            record.type = recordData["type"];
            record.content = recordData["content"];
            record.ttl = recordData["ttl"].template get<int>();
            record.priority = recordData["prio"].template get<int>();
            records.push_back(record);
        }
        return records;
    }
    else {
        LOG_ERROR << "Token validation call failed with code: " << response.code;
    }
    return {};
}

Record LopDnsClient::createRecord(const std::string& zone_name, const std::string& record_name,
                                   const std::string& type, const std::string& content, int ttl, int priority)
{
    // Implementation for adding a DNS record
    json bodyJson;
    bodyJson["name"] = record_name;
    bodyJson["type"] = type;
    bodyJson["value"] = content;
    bodyJson["ttl"] = ttl;
    bodyJson["priority"] = priority;

    RestClient::Response response = makeRestCall("POST", "/records/" + zone_name, true, RestClient::HeaderFields(), std::map<std::string, std::string>(), bodyJson.dump());
    if (response.code >= 200 && response.code < 300)
    {
        // Parse response and return updated record
        json responseBody = json::parse(response.body);
        Record record;
        record.name = responseBody["name"];
        record.type = responseBody["type"];
        record.content = responseBody["data"];
        record.ttl = responseBody["ttl"];
        record.priority = responseBody["priority"];
        return record;
    }
    else {
        LOG_ERROR << "Call failed with code: " << response.code;
    }
    return Record{};
}

Record LopDnsClient::updateRecord(const std::string& zone_name, const std::string& old_record_name,
                                   const std::string& matching_type, const std::string& old_content, 
                                   const std::string& new_content, int ttl, int priority)
{
    // Implementation for updating a DNS record
    json bodyJson;
    bodyJson["oldName"] = old_record_name;
    bodyJson["matchingType"] = matching_type;
    bodyJson["oldValue"] = old_content;
    bodyJson["newValue"] = new_content;
    bodyJson["ttl"] = ttl;
    bodyJson["priority"] = priority;
    
    RestClient::Response response = makeRestCall("PUT", "/records/" + zone_name, true, RestClient::HeaderFields(), std::map<std::string, std::string>(), bodyJson.dump());
    if (response.code >= 200 && response.code < 300)
    {
        // Parse response and return updated record
        json responseBody = json::parse(response.body);
        Record record;
        record.name = responseBody["name"];
        record.type = responseBody["type"];
        record.content = responseBody["data"];
        record.ttl = responseBody["ttl"];
        record.priority = responseBody["priority"];
        return record;
    }
    else {
        LOG_ERROR << "Token validation call failed with code: " << response.code;
    }
    return Record{};
}

bool LopDnsClient::deleteRecord(const std::string& zone_name, const std::string& record_name,
                                   const std::string& type, const std::string& content)
{
    // Implementation for deleting a DNS record
    json bodyJson;
    bodyJson["name"] = record_name;
    bodyJson["type"] = type;
    bodyJson["value"] = content;

    RestClient::Response response = makeRestCall("DELETE", "/records/" + zone_name, true, RestClient::HeaderFields(), std::map<std::string, std::string>(), bodyJson.dump());
    if (response.code >= 200 && response.code < 300)
    {
        LOG_DEBUG << "Record deleted successfully. Response: " << response.body;
        return true;
    }
    else {
        LOG_ERROR << "Deleting record call failed with code: " << response.code;
    }

    return false;
}

RestClient::Response LopDnsClient::makeRestCall(const std::string& method, const std::string& endpoint, bool applyAuthHeaders,
                      const RestClient::HeaderFields& headers, const std::map<std::string, std::string>& queryParams,
                      const std::string& body)
{
    // Implementation for making a REST API call
    std::string queryString = "";
    for (const auto& param : queryParams) {
        queryString += (queryString.empty() ? "?" : "&");
        queryString += param.first + "=" + param.second;
    }

    std::string uri = endpoint + queryString;
    RestClient::Connection* conn = new RestClient::Connection(this->url);
    RestClient::Response response;
    response.code = -1;

    LOG_DEBUG << "Making " << method << " request to '" << url << "'";

    try
    {
        conn->SetTimeout(this->timeout);
        conn->FollowRedirects(true);

        conn->SetUserAgent("LopDnsClientCpp/1.0");
        
        conn->SetHeaders(headers);
        conn->AppendHeader("Content-Type", "application/json");
        conn->AppendHeader("Accept", "application/json");
        if (applyAuthHeaders) {
            conn->AppendHeader("x-token", this->token.token);
        }

        LOG_DEBUG << method << " " << url << uri << " Headers:";
        for (const auto& header : conn->GetHeaders()) {
            LOG_DEBUG << "Call Header: " << header.first << ": " << header.second;
        }

        if (method == "GET") {
            response = conn->get(uri);
        } else if (method == "POST") {
            response = conn->post(uri, body);
        } else if (method == "PUT") {
            response = conn->put(uri, body);
        } else if (method == "DELETE") {
            response = conn->del(uri);
        }
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        delete conn;
        throw;
    }

    LOG_DEBUG << method << " " << this->url << uri << " Response Code: " << response.code << " Data: " << response.body;
    for (const auto& header : response.headers) {
        LOG_DEBUG << "Response Header: " << header.first << ": " << header.second;
    }
        
    delete conn;

    return response;
}
