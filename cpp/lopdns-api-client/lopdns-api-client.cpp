//
// lopdns-api-client.cpp
// ~~~~~~~~~~~~~~~
//

#include <iostream>
#include <istream>
#include <ostream>
#include <string>
#include <regex>
#include <map>
#include <list>
#include <ctime>
#include <exception>
#include <optional>
#include <sstream>
#include "args.hxx"
#include "plog/Log.h"
#include "plog/Init.h"
#include "plog/Formatters/TxtFormatter.h"
#include "plog/Appenders/ColorConsoleAppender.h"
#include "lopdnsclient.h"


const std::string URL = "api.lopdns.se";

constexpr int defaultDnsRecordTtl = 3600;
constexpr int defaultDnsRecordPriority = 0;

typedef enum ActionType {
    ACTION_GET_ZONES,
    ACTION_GET_RECORDS,
    ACTION_CREATE_RECORD,
    ACTION_UPDATE_RECORD,
    ACTION_CREATE_OR_UPDATE_RECORD,
    ACTION_DELETE_RECORD
} ActionType;

typedef enum LogLevelType {
    LOGLEVEL_ERROR,
    LOGLEVEL_WARNING,
    LOGLEVEL_INFO,
    LOGLEVEL_DEBUG
} LogLevelType;

const std::map<std::string, ActionType> actionMap = {
    {"get-zones", ACTION_GET_ZONES},
    {"get-records", ACTION_GET_RECORDS},
    {"create-record", ACTION_CREATE_RECORD},
    {"update-record", ACTION_UPDATE_RECORD},
    {"createorupdate-record", ACTION_CREATE_OR_UPDATE_RECORD},
    {"delete-record", ACTION_DELETE_RECORD}};

const std::map<std::string, LogLevelType> logLevelMap = {
    {"error", LOGLEVEL_ERROR},
    {"warning", LOGLEVEL_WARNING},
    {"info", LOGLEVEL_INFO},
    {"debug", LOGLEVEL_DEBUG}
};

struct Settings
{
    std::string base_url = URL;
    int timeout = 10;
    int token_duration_sec = 3600;
    std::string client_id;
    ActionType action = ACTION_GET_ZONES;
    std::string zone;
    std::string record_name;
    std::string record_type;
    std::string current_record_content;
    std::string replace_record_content_regex;
    
    // New content for create or update
    std::optional<std::string> new_record_content;
    std::optional<std::string> new_record_name;
    std::optional<std::string> new_record_type;
    std::optional<int> new_record_ttl;
    std::optional<int> new_record_priority;

    bool all_records = false;
    LogLevelType log_level = LOGLEVEL_INFO;
    bool dry_run = false;
};

void trim(std::string& s) {
    auto not_space = [](unsigned char c){ return !std::isspace(c); };

    // erase the spaces at the front
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), not_space));
    // erase the spaces at the back
    s.erase(std::find_if(s.rbegin(), s.rend(), not_space).base(), s.end());
}

std::list<Record> getRecords(LopDnsClient& client, const Settings& settings)
{
    std::list<Record> matchedRecords;

    auto records = client.getRecords(settings.zone);
    for (const auto& record : records) {
        std::cmatch regex_match;
        if (record.name == settings.record_name && 
            record.type == settings.record_type && 
                ((settings.all_records && settings.current_record_content.empty()) || 
                std::regex_search(record.content.c_str(), regex_match, std::regex(settings.current_record_content)))) {
            matchedRecords.push_back(record);
        }
    }

    return matchedRecords;
}

bool createRecord(LopDnsClient& client, const Settings& settings, Record& outRecord)
{
    std::string contentValue;
    std::string nameValue;
    std::string typeValue;
    if (settings.new_record_content.has_value())
        contentValue = settings.new_record_content.value();
    else {
        contentValue = settings.current_record_content;
    }
    if (settings.new_record_name.has_value()) {
        nameValue = settings.new_record_name.value();
    }
    else {
        nameValue = settings.record_name;
    }
    if (settings.new_record_type.has_value()) {
        typeValue = settings.new_record_type.value();
    }
    else {
        typeValue = settings.record_type;
    }
    int ttlValue = settings.new_record_ttl.value_or(defaultDnsRecordTtl);
    int priorityValue = settings.new_record_priority.value_or(defaultDnsRecordPriority);
    if (settings.dry_run) {
        LOG_INFO << "[Dry Run] Would create record:" << std::endl;
        outRecord.name = nameValue;
        outRecord.type = typeValue;
        outRecord.content = contentValue;
        outRecord.ttl = ttlValue;
        outRecord.priority = priorityValue;
    }
    else {
        outRecord = client.createRecord(settings.zone, settings.record_name,
                                                settings.record_type,
                                                contentValue,
                                                ttlValue,
                                                priorityValue);
        if (outRecord.name.empty()) {
            LOG_ERROR << "Failed to create record.";
            return false;
        }
    }

    std::stringstream logData;
    logData << "Created record:" << std::endl;
    logData << "  Name: " << outRecord.name << ", Type: " << outRecord.type
                << ", Content: " << outRecord.content << ", TTL: " << outRecord.ttl
                << ", Priority: " << outRecord.priority << std::endl;
    LOG_INFO << logData.str();

    return true;
}

bool updateRecord(LopDnsClient& client, const Settings& settings, const Record& record, 
    const std::optional<std::string>& new_record_content, Record& outRecord)
{
    std::stringstream logData;
    if (settings.dry_run) {
        outRecord = record;
        logData << "[Dry Run] Would update record:" << std::endl;
        if (new_record_content.has_value()) {
            outRecord.content = new_record_content.value();
        }
        if (settings.new_record_name.has_value()) {
            outRecord.name = settings.new_record_name.value();
        }
        if (settings.new_record_type.has_value()) {
            outRecord.type = settings.new_record_type.value();
        }
        if (settings.new_record_ttl.has_value()) {
            outRecord.ttl = settings.new_record_ttl.value();
        }
        if (settings.new_record_priority.has_value()) {
            outRecord.priority = settings.new_record_priority.value();
        }
    }
    else {
        outRecord = client.updateRecord(
            settings.zone,
            record.name,
            record.type,
            record.content,
            settings.new_record_name,
            settings.new_record_type,
            new_record_content,
            settings.new_record_ttl,
            settings.new_record_priority);
        if (outRecord.name.empty()) {
            LOG_ERROR << "Failed to update record.";
            return false;
        }

        logData << "Updated record:" << std::endl;
    }
    logData << "Changed record data:" << std::endl;
    if (settings.new_record_name.has_value()) {
        logData << "  Name: " << outRecord.name << " (was '" << record.name << "')" << std::endl;
    }
    if (settings.new_record_type.has_value()) {
        logData << "  Type: " << outRecord.type << " (was '" << record.type << "')" << std::endl;
    }
    if (new_record_content.has_value()) {
        logData << "  Content: " << outRecord.content << " (was '" << record.content << "')" << std::endl;
    }
    if (settings.new_record_ttl.has_value()) {
        logData << "  TTL: " << outRecord.ttl << " (was '" << record.ttl << "')" << std::endl;
    }
    if (settings.new_record_priority.has_value()) {
        logData << "  Priority: " << outRecord.priority << " (was '" << record.priority << "')" << std::endl;
    }

    LOG_INFO << logData.str();

    return true;
}


bool deleteRecord(LopDnsClient& client, const Settings& settings, const Record& record)
{
    if (settings.dry_run) {
        std::stringstream logData;
        logData << "[Dry Run] Would delete record:" << std::endl;
        logData << "  Name: " << record.name << ", Type: " << record.type
                  << ", Content: " << record.content << std::endl;
        LOG_INFO << logData.str();
        return true;
    }
    
    bool success = client.deleteRecord(settings.zone, record.name, record.type, record.content);
    if (!success) {
        LOG_ERROR << "Failed to delete record.";
        return false;
    }

    std::stringstream logData;
    logData << "Record deleted:" << std::endl;
    logData << "  Name: " << record.name << ", Type: " << record.type
                << ", Content: " << record.content << std::endl;
    LOG_INFO << logData.str();

    return true;
}

bool handleArgs(int argc, char* argv[], Settings& settings)
{
    std::string actionMsg = "The action to perform. Valid actions are: ";
    for (const auto& pair : actionMap) {
        actionMsg += pair.first + ", ";
    }
    args::ArgumentParser parser("This application updates DNS records using the LOP DNS API (https://api.lopdns.se/v2)", actionMsg);
    args::HelpFlag help(parser, "help", "Display this help menu", {'h', "help"});
    args::ValueFlag<std::string> base_url(parser, "base_url", "The base URL for the API", {'b', "base-url"}, URL);
    args::ValueFlag<int> timeout(parser, "timeout", "The timeout for the API requests", {'t', "timeout"}, 10);
    args::ValueFlag<int> token_duration_sec(parser, "token_duration_sec", "The token duration in seconds", {'d', "token-duration-sec"}, 3600);
    args::ValueFlag<std::string> client_id(parser, "client_id", "The client ID for authentication", {'c', "client-id"}, "");
    args::ValueFlag<std::string> zone(parser, "zone", "The DNS zone to update", {'z', "zone"}, "");
    args::ValueFlag<std::string> record_type(parser, "record_type", "The type of the DNS record", {'r', "record-type"}, "A");
    args::ValueFlag<std::string> record_name(parser, "record_name", "The name of the DNS record", {'n', "record-name"}, "");
    args::ValueFlag<std::string> current_record_content(parser, "current_record_content", "Current record content for DNS tasks (regex search)", {'u', "current-record-content"}, "");
    args::ValueFlag<std::string> replace_record_content_regex(parser, "replace_record_content_regex", "Regular expression to be used in updates (regex replace)", {'x', "replace-record-content-regex"}, "");
    args::ValueFlag<std::string> new_record_content(parser, "new_record_content", "New content for DNS records", {'w', "new-record-content"}, "");
    args::ValueFlag<std::string> new_record_type(parser, "new_record_type", "The type of the DNS record", {'R', "new-record-type"}, "");
    args::ValueFlag<std::string> new_record_name(parser, "new_record_name", "New record name to be updated with", {'N', "new-record-name"}, "");
    args::ValueFlag<int> new_record_ttl(parser, "new_record_ttl", "The new TTL for the DNS record", {'T', "new-record-ttl"}, 0);
    args::ValueFlag<int> new_record_priority(parser, "new_record_priority", "The new priority for the DNS record", {'p', "new-record-priority"}, 0);
    args::ValueFlag<std::string> action(parser, "action", "The action to perform", {'a', "action"}, "");
    args::Flag all_records(parser, "all_records", "Flag to indicate that all applicable records should be processed", {'A', "all-records"}, false);
    args::ValueFlag<std::string> log_level(parser, "log_level", "The logging level (error, warning, info, debug)", {'l', "log-level"}, "info");
    args::Flag dry_run(parser, "dry_run", "Flag to indicate that no changes should be made", {'D', "dry-run"}, false);

    try
    {
        parser.ParseCLI(argc, argv);
    }
    catch (args::Help const&)
    {
        std::cout << parser;
        exit(0);
    }
    catch (args::ParseError const& e)
    {
        LOG_ERROR << e.what();
        LOG_ERROR << parser;
        return false;
    }
    catch (args::ValidationError const& e)
    {
        LOG_ERROR << e.what();
        LOG_ERROR << parser;
        return false;
    }

    if (log_level) {
        std::string logLevelStr = args::get(log_level);
        trim(logLevelStr);
        auto it = logLevelMap.find(logLevelStr);
        if (it == logLevelMap.end()) {
            std::cerr << "Invalid log level specified: " << logLevelStr << ". Valid levels are: error, warning, info, debug.\n";
            return false;
        }
        settings.log_level = it->second;
    }
    switch (settings.log_level) {
        case LOGLEVEL_ERROR:
            plog::get()->setMaxSeverity(plog::error);
            break;
        case LOGLEVEL_WARNING:
            plog::get()->setMaxSeverity(plog::warning);
            break;
        case LOGLEVEL_INFO:
            plog::get()->setMaxSeverity(plog::info);
            break;
        case LOGLEVEL_DEBUG:
            plog::get()->setMaxSeverity(plog::debug);
            break;
        default:
            plog::get()->setMaxSeverity(plog::info);
            break;
    }
    if (all_records) {
        settings.all_records = args::get(all_records);
    }
    if (action) {
        std::string actionStr = args::get(action);
        trim(actionStr);
        auto it = actionMap.find(actionStr);
        if (it != actionMap.end()) {
            settings.action = it->second;
        } else {
            LOG_ERROR << "Invalid action specified: " << actionStr << ".";
            LOG_INFO << "Valid actions are:";
            for (const auto& pair : actionMap) {
                LOG_INFO << "  " << pair.first;
            }
            return false;
        }
    } else {
        LOG_ERROR << "Action is required.";
        return false;
    }
    if (client_id) {
        std::string clientIdStr = args::get(client_id);
        trim(clientIdStr);
        settings.client_id = clientIdStr;
    } else {
        LOG_ERROR << "Client ID is required.";
        return false;
    }
    if (record_type) {
        std::string recordTypeStr = args::get(record_type);
        trim(recordTypeStr);
        settings.record_type = recordTypeStr;
    } else if (
        settings.action == ACTION_UPDATE_RECORD
        || settings.action == ACTION_CREATE_RECORD
        || settings.action == ACTION_CREATE_OR_UPDATE_RECORD
        || settings.action == ACTION_DELETE_RECORD
        ) {
        LOG_ERROR << "Record type is required.";
        return false;
    }
    if (zone) {
        std::string zoneStr = args::get(zone);
        trim(zoneStr);
        settings.zone = zoneStr;
    } else if (
        settings.action == ACTION_UPDATE_RECORD
        || settings.action == ACTION_CREATE_RECORD
        || settings.action == ACTION_CREATE_OR_UPDATE_RECORD
        || settings.action == ACTION_GET_RECORDS
        || settings.action == ACTION_DELETE_RECORD
    ) {
        LOG_ERROR << "Zone is required.";
        return false;
    }
    if (record_name) {
        std::string recordNameStr = args::get(record_name);
        trim(recordNameStr);
        settings.record_name = recordNameStr;
    } else if (
        settings.action == ACTION_UPDATE_RECORD
        || settings.action == ACTION_CREATE_RECORD
        || settings.action == ACTION_CREATE_OR_UPDATE_RECORD
        || settings.action == ACTION_DELETE_RECORD
        ) {
        LOG_ERROR << "Record name is required.";
        return false;
    }
    if (current_record_content) {
        std::string oldContentStr = args::get(current_record_content);
        trim(oldContentStr);
        settings.current_record_content = oldContentStr;
    } else if (settings.action == ACTION_UPDATE_RECORD && !settings.all_records) {
        LOG_ERROR << "Current content is required.";
        return false;
    }
    if (replace_record_content_regex) {
        std::string replaceContentStr = args::get(replace_record_content_regex);
        trim(replaceContentStr);
        settings.replace_record_content_regex = replaceContentStr;
    }
    if (new_record_content) {
        std::string newContentStr = args::get(new_record_content);
        trim(newContentStr);
        settings.new_record_content = newContentStr;
    } else if (settings.action == ACTION_UPDATE_RECORD) {
        LOG_ERROR << "New content is required.";
        return false;
    }
    if (new_record_ttl) {
        settings.new_record_ttl = args::get(new_record_ttl);
    }
    if (new_record_priority) {
        settings.new_record_priority = args::get(new_record_priority);
    }
    if (base_url) {
        std::string baseUrlStr = args::get(base_url);
        trim(baseUrlStr);
        settings.base_url = baseUrlStr;
    }
    if (timeout) {
        settings.timeout = args::get(timeout);
    }
    if (token_duration_sec) {
        settings.token_duration_sec = args::get(token_duration_sec);
    }
    if (dry_run) {
        settings.dry_run = args::get(dry_run);
    }
    return true;
}

void logSettings(const Settings& settings)
{
    LOG_DEBUG << "Settings:";
    LOG_DEBUG << "  Base URL: " << settings.base_url;
    LOG_DEBUG << "  Timeout: " << settings.timeout << " seconds";
    LOG_DEBUG << "  Token Duration: " << settings.token_duration_sec << " seconds";
    LOG_DEBUG << "  Client ID: " << settings.client_id;
    LOG_DEBUG << "  Action: ";
    for (const auto& pair : actionMap) {
        if (pair.second == settings.action) {
            LOG_DEBUG << pair.first;
            break;
        }
    }
    LOG_DEBUG << "  Zone: " << settings.zone;
    LOG_DEBUG << "  Record Type: " << settings.record_type;
    LOG_DEBUG << "  Record Name: " << settings.record_name;
    LOG_DEBUG << "  Current Content: " << settings.current_record_content;
    LOG_DEBUG << "  Replace Content: " << settings.replace_record_content_regex;
    if (settings.new_record_content.has_value()) {
        LOG_DEBUG << "  New Content: " << settings.new_record_content.value();
    } else {
        LOG_DEBUG << "  New Content: (not set)";
    }
    if (settings.new_record_ttl.has_value()) {
        LOG_DEBUG << "  TTL: " << settings.new_record_ttl.value();
    } else {
        LOG_DEBUG << "  TTL: (not set)";
    }
    if (settings.new_record_priority.has_value()) {
        LOG_DEBUG << "  Priority: " << settings.new_record_priority.value();
    } else {
        LOG_DEBUG << "  Priority: (not set)";
    }
    LOG_DEBUG << "  All Records: " << (settings.all_records ? "true" : "false");
    std::stringstream ll; 
    ll << "  Log Level: ";
    for (const auto& pair : logLevelMap) {
        if (pair.second == settings.log_level) {
            ll << pair.first;
            break;
        }
    }
    LOG_DEBUG << ll.str();
    LOG_DEBUG << "  Dry Run: " << (settings.dry_run ? "true" : "false");
}

void exitWithError(const std::string& message, int exitCode = 1, LopDnsClient* client = nullptr)
{
    LOG_ERROR << message;
    if (client != nullptr) {
        client->invalidateToken();
    }
    exit(exitCode);
}

int main(int argc, char* argv[])
{
  static plog::ColorConsoleAppender<plog::TxtFormatter> consoleAppender;
  plog::init(plog::info, &consoleAppender);
  
  try
  {
    Settings settings;

    if (!handleArgs(argc, argv, settings)) {
        return 1;
    }

    logSettings(settings);

    LopDnsClient client(settings.base_url, settings.timeout);

    if (!client.authenticate(settings.client_id, settings.token_duration_sec)) {
        exitWithError("Authentication failed.");
    }

    std::list<std::string> zones = client.getZones();
    if (!settings.zone.empty()) {
        if (std::find(zones.begin(), zones.end(), settings.zone) == zones.end()) {
            exitWithError("Specified zone not found: " + settings.zone, 2, &client);
        }
        zones = {settings.zone};
    }
    else {
        if (zones.empty()) {
            exitWithError("No zones available to retrieve records from.", 3, &client);
        }
    }

    switch (settings.action) {
        case ACTION_GET_ZONES:
        {
            LOG_INFO << "Zones:";
            for (const auto& zone : zones) {
                LOG_INFO << "  " << zone;
            }
            break;
        }
        case ACTION_GET_RECORDS:
        {
            // Retrieve and print records for each zone
            for (const auto& zone : zones) {
                LOG_INFO << "Records in zone " << zone << ":";
                auto records = client.getRecords(zone);
                for (const auto& record : records) {
                    LOG_INFO << "  Name: " << record.name << ", Type: " << record.type
                              << ", Content: " << record.content << ", TTL: " << record.ttl
                              << ", Priority: " << record.priority << "\n";
                }
            }
            break;
        }
        case ACTION_CREATE_RECORD:
        {
            Record newRecord;
            if (!createRecord(client, settings, newRecord)) {
                exitWithError("Failed to create record.", 4, &client);
            }
            break;
        }
        case ACTION_UPDATE_RECORD:
        case ACTION_CREATE_OR_UPDATE_RECORD:
        {         
            auto records = getRecords(client, settings);
            bool updated = false;
            for (const auto& record : records) {
                std::optional<std::string> new_content;
                if (settings.replace_record_content_regex.empty()) {
                    if (settings.new_record_content.has_value()) {
                        new_content = settings.new_record_content;
                    }
                }
                else if (settings.new_record_content.has_value()) {
                    new_content = std::regex_replace(record.content, std::regex(settings.replace_record_content_regex), settings.new_record_content.value());
                }
                
                Record updatedRecord;
                if (updateRecord(client, settings, record, new_content, updatedRecord))
                {
                    updated = true;
                } else {
                    exitWithError("Failed to update record.", 5, &client);
                }   

                if (!settings.all_records) {
                    break;
                }
            }
            if (!updated) {
                if (settings.action == ACTION_UPDATE_RECORD) {
                    LOG_INFO << "No records updated for name: " << settings.record_name
                          << " and type: " << settings.record_type << "\n";
                    exitWithError("No records updated for name: " + settings.record_name + " and type: " + settings.record_type, 6, &client);
                }
                else if (settings.action == ACTION_CREATE_OR_UPDATE_RECORD) {
                    LOG_INFO << "No matching records found. Creating new record." << std::endl;
                    Record newRecord;
                    if (!createRecord(client, settings, newRecord)) {
                        exitWithError("Failed to create record.", 4, &client);
                    }
                }
            }
            break;
        }
        case ACTION_DELETE_RECORD:
        {
            auto records = getRecords(client, settings);
            if (records.empty()) {
                LOG_INFO << "No matching records found to delete." << std::endl;
                exitWithError("No matching records found to delete.", 7, &client);
            }
            for (const auto& record : records) {
                if (!deleteRecord(client, settings, record)) {
                    exitWithError("Failed to delete record.", 8, &client);
                }
                if (!settings.all_records) {
                    if (records.size() > 1) {
                        LOG_INFO << "Multiple matching records found, but 'all_records' flag is not set. Stopping after first deletion." << std::endl;
                    }
                    break;
                }
            }
            break;
        }
        default:
            LOG_ERROR << "Unknown action.";
            exitWithError("Unknown action.", 9, &client);
    }
  }
  catch (std::exception& e)
  {
    exitWithError("Unhandled exception occurred: " + std::string(e.what()), 99);
  }

  return 0;
}
