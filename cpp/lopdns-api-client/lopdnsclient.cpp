#include "nlohmann/json.hpp"
#include "plog/Log.h"

#include "lopdnsclient.h"
#include <iostream>

using json = nlohmann::json;

const std::string USER_AGENT = "lopdns-api-client/1.0";
const std::string ENCODING = "application/json";
const std::string API_VERSION = "v2";

LopDnsClient::LopDnsClient(const std::string& url, int timeoutInSeconds)
{
    this->url = url;
    this->timeout = timeoutInSeconds;
}

LopDnsClient::~LopDnsClient()
{
    // Cleanup if necessary
}

bool LopDnsClient::authenticate(const std::string& client_id, const int durationInSeconds)
{
    // Implementation for authenticating the client
    Headers headers;
    headers["x-clientid"] = client_id;

    QueryParams queryParams;
    queryParams["duration"] = std::to_string(durationInSeconds);

    Response response = makeRestCall("GET", "/auth/token", false, headers, queryParams);
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
        LOG_ERROR << "Authentication call failed with code: " << response.code << " body: " << response.body;
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

    Response response = makeRestCall("GET", "/auth/validate");
    if (response.code >= 200 && response.code < 300)
    {
        LOG_DEBUG << "Token validation successful. Response: " << response.body;
        return true;
    }
    else {
        LOG_ERROR << "Token validation call failed with code: " << response.code << " body: " << response.body;
    }

    return false;
}

bool LopDnsClient::invalidateToken()
{
    if (this->isTokenExpired())
    {
        return true;
    }

    // Implementation for invalidating the token
    Response response = makeRestCall("GET", "/auth/invalidate");
    if (response.code >= 200 && response.code < 300)
    {
        LOG_DEBUG << "Token invalidation successful. Response: " << response.body;
        return true;
    }
    else {
        LOG_ERROR << "Token invalidation call failed with code: " << response.code;
    }

    return false;
}

std::list<std::string> LopDnsClient::getZones()
{
    // Implementation for getting the list of zones
    Response response = makeRestCall("GET", "/zones");
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
        LOG_ERROR << "Token validation call failed with code: " << response.code << " body: " << response.body;
    }

    return {};
}

std::list<Record> LopDnsClient::getRecords(const std::string& zone_name)
{
    // Implementation for getting the list of records for a zone
    Response response = makeRestCall("GET", "/records/" + zone_name);
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
        LOG_ERROR << "Token validation call failed with code: " << response.code << " body: " << response.body;
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

    Response response = makeRestCall("POST", "/records/" + zone_name, true, Headers(), QueryParams(), bodyJson.dump());
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
                                   const std::optional<std::string>& new_record_name, const std::optional<std::string>& new_type,
                                   const std::optional<std::string>& new_content, const std::optional<int>& new_ttl, 
                                   const std::optional<int>& new_priority)
{
    // Implementation for updating a DNS record
    json bodyJson;
    bodyJson["oldName"] = old_record_name;
    bodyJson["matchingType"] = matching_type;
    bodyJson["oldValue"] = old_content;
    
    if (new_record_name.has_value()) {
        bodyJson["newName"] = new_record_name.value();
    }
    if (new_type.has_value()) {
        bodyJson["newType"] = new_type.value();
    }
    if (new_content.has_value()) {
        bodyJson["newValue"] = new_content.value();
    }
    if (new_ttl.has_value()) {
        bodyJson["newTtl"] = new_ttl.value();
    }
    if (new_priority.has_value()) {
        bodyJson["newPriority"] = new_priority.value();
    }
    
    Response response = makeRestCall("PUT", "/records/" + zone_name, true, Headers(), QueryParams(), bodyJson.dump());
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
        LOG_ERROR << "Token validation call failed with code: " << response.code << " body: " << response.body;
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

    Response response = makeRestCall("DELETE", "/records/" + zone_name, true, Headers(), QueryParams(), bodyJson.dump());
    if (response.code >= 200 && response.code < 300)
    {
        LOG_DEBUG << "Record deleted successfully. Response: " << response.body;
        return true;
    }
    else {
        LOG_ERROR << "Deleting record call failed with code: " << response.code << " body: " << response.body;
    }

    return false;
}

Response LopDnsClient::makeRestCall(const std::string& method, const std::string& endpoint, bool applyAuthHeaders,
                      const Headers& headers, const QueryParams& queryParams,
                      const std::string& body)
{
    // Implementation for making a REST API call
    std::string uri =  "/" + API_VERSION + endpoint;

    std::stringstream logData;

    logData << "Making " << method << " request to '" << url << "':";
    logData << "\n\tURI: " << uri;

    if (!queryParams.empty()) {
        logData << "\n\tQuery Parameters:";
        for (const auto& param : queryParams) {
            logData << "\n\t\t" << param.first << ": " << param.second;
        }
    }
    if (applyAuthHeaders) {
        logData << "\n\tHeaders (auth):";
        logData << "\n\t\tx-token: " << this->token.token;
    }
    if (headers.size() > 0) {
        logData << "\n\tHeaders (additional):";
        for (const auto& header : headers) {
            logData << "\n\t\t" << header.first << ": " << header.second;
        }
    }
    if (!body.empty()) {
        logData << "\n\tBody: " << body;
    }
    LOG_DEBUG << logData.str();


    httplib::Result httpResult;
    httplib::SSLClient client(url.c_str());

    try
    {

        LOG_DEBUG << "Setting timeouts to " << timeout << " seconds.";
        client.set_connection_timeout(timeout, 0); 
        client.set_read_timeout(timeout, 0); // 5 seconds
        client.set_write_timeout(timeout, 0); // 5 seconds

        LOG_DEBUG << "Setting connection properties ";
        client.set_follow_location(true);
        client.enable_server_certificate_verification(true);

        LOG_DEBUG << "Preparing HTTP " << method << " request to '" << client.host() << uri << "' on port " << client.port();
        LOG_DEBUG << "Adding headers";
        httplib::Headers httpHeaders;
        for (const auto& header : headers) {
            httpHeaders.insert(header);
        }
        if (applyAuthHeaders) {
            httpHeaders.insert({"x-token", this->token.token});
        }
        httpHeaders.insert({"User-Agent", USER_AGENT.c_str()});
        
        LOG_DEBUG << "Adding parameters";
        httplib::Params httpParams;
        for (const auto& param : queryParams) {
            httpParams.insert(param);
        }
        
        LOG_DEBUG << "Sending HTTP " << method << " request";
        if (method == "GET") {
            httpResult = client.Get(uri, httpParams, httpHeaders);
        } else if (method == "POST") {
            httpResult = body.empty() ? client.Post(uri, httpHeaders, httpParams) : client.Post(uri, httpHeaders, body, ENCODING);
        } else if (method == "PUT") {
            httpResult = body.empty() ? client.Put(uri, httpHeaders, httpParams) : client.Put(uri, httpHeaders, body, ENCODING);
        } else if (method == "PATCH") {
            httpResult = body.empty() ? client.Patch(uri, httpHeaders, httpParams) : client.Patch(uri, httpHeaders, body, ENCODING);
        } else if (method == "DELETE") {
            httpResult = body.empty() ? client.Delete(uri, httpHeaders, httpParams) : client.Delete(uri, httpHeaders, body, ENCODING);
        } else {
            throw std::runtime_error("Unsupported HTTP method: " + method);
        }
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        throw;
    }

    Response response;
    response.code = -1;

    if (!httpResult) {
        auto err = httplib::to_string(httpResult.error());
        LOG_ERROR << "HTTP request failed: " << err;
        auto sslResult = client.get_openssl_verify_result();
        if (sslResult) {
            LOG_ERROR << "SSL verify error: " << X509_verify_cert_error_string(sslResult);
        }
        response.body = err;
        return response;
    }

    LOG_DEBUG << "Handling HTTP response";
    response.code = httpResult->status;
    response.body = httpResult->body;

    logResponse(uri, method, httpResult);

    return response;
}

void LopDnsClient::logResponse(const std::string& uri, const std::string& method, const httplib::Result& httpResult)
{
    std::stringstream logData;
    logData << "Response Code: " << httpResult->status << "\n";
    logData << "Response Body: " << httpResult->body << "\n";
    for (const auto& header : httpResult->headers) {
        logData << "Response Header: " << header.first << ": " << header.second << "\n";
    }
    if (httpResult->status >= 400) {
        LOG_ERROR << logData.str();
    } else {
        LOG_DEBUG << logData.str();
    }
}