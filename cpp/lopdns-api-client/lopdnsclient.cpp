#include <nlohmann/json.hpp>
#include <restclient-cpp/connection.h>
#include <restclient-cpp/restclient.h>

#include "lopdnsclient.h"
#include <iostream>

using json = nlohmann::json;

LopDnsClient::LopDnsClient(const std::string& url, int timeoutInSeconds, bool verbose)
{
    RestClient::init();

    this->url = url;
    this->timeout = timeoutInSeconds;
    this->verbose = verbose;
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
        if (verbose) {
            std::cout << "Authentication successful. Response: " << response.body << std::endl;
        }
        // Parse response and set token details
        json responseBody = json::parse(response.body);
        this->token.token = responseBody["token"];
        this->token.expires = responseBody["expires"];
        this->token.epochExpires = responseBody["epochExpires"].template get<long long>();
        this->token.tzone = responseBody["tz"];

        return true;
    }
    else {
        std::cerr << "Authentication call failed with code: " << response.code << std::endl;
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
        if (verbose) {
            std::cout << "Token validation successful. Response: " << response.body << std::endl;
        }
        return true;
    }
    else {
        std::cerr << "Token validation call failed with code: " << response.code << std::endl;
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
        if (verbose) {
            std::cout << "Retrieved " << zones.size() << " zones." << std::endl;
        }   
        
        return zones;
    }
    else {
        std::cerr << "Token validation call failed with code: " << response.code << std::endl;
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
            if (verbose) {
                std::cout << "Retrieved " << responseBody.size() << " records for zone: " << zone_name << std::endl;
            }
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
        std::cerr << "Token validation call failed with code: " << response.code << std::endl;
    }
    return {};
}

Record LopDnsClient::updateRecord(const std::string& zone_name, const std::string& old_record_name,
                                   const std::string& matching_type, const std::string& old_content, const std::string& new_content)
{
    // Implementation for updating a DNS record
    json bodyJson;
    bodyJson["oldName"] = old_record_name;
    bodyJson["matchingType"] = matching_type;
    bodyJson["oldValue"] = old_content;
    bodyJson["newValue"] = new_content;

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
        std::cerr << "Token validation call failed with code: " << response.code << std::endl;
    }
    return Record{};
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

    if (verbose) {
        std::cout << "[DEBUG] Making " << method << " request to '" << url << "'" << std::endl;
    }   

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

        if (verbose) {
            std::cout << "[DEBUG] " << method << " " << url << std::endl;
            for (const auto& header : conn->GetHeaders()) {
                std::cout << "[DEBUG] Call Header: " << header.first << ": " << header.second << std::endl;
            }
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

    if (verbose) {
        std::cout << "[DEBUG] " << method << " " << this->url << uri << " Response Code: " << response.code << " Data: " << response.body << std::endl;
        for (const auto& header : response.headers) {
            std::cout << "[DEBUG] Response Header: " << header.first << ": " << header.second << std::endl;
        }
    }
        
    delete conn;

    return response;
}
