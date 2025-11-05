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
#include <args.hxx>
#include <restclient-cpp/connection.h>
#include <restclient-cpp/restclient.h>
#include "plog/Log.h"
#include "plog/Init.h"
#include "plog/Formatters/TxtFormatter.h"
#include "plog/Appenders/ColorConsoleAppender.h"
#include "lopdnsclient.h"


const std::string URL = "https://api.lopdns.se/v2";

typedef enum ActionType {
    ACTION_GET_ZONES,
    ACTION_GET_RECORDS,
    ACTION_UPDATE_RECORD
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
    {"update-record", ACTION_UPDATE_RECORD}
};

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
    std::string record_type = "A";
    std::string current_content;
    std::string replace_content;
    std::string new_content;
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

int main(int argc, char* argv[])
{
  static plog::ColorConsoleAppender<plog::TxtFormatter> consoleAppender;
  plog::init(plog::info, &consoleAppender);
  
  try
  {

    Settings settings;

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
    args::ValueFlag<std::string> current_content(parser, "current_content", "Current content for DNS tasks (regex search)", {'u', "current-content"}, "");
    args::ValueFlag<std::string> replace_content(parser, "replace_content", "Content to be replaced by <new_content> (regex replace)", {'x', "replace-content"}, "");
    args::ValueFlag<std::string> new_content(parser, "new_content", "New content to replace <replace_content>", {'w', "new-content"}, "");
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
        return 0;
    }
    catch (args::ParseError const& e)
    {
        LOG_ERROR << e.what();
        LOG_ERROR << parser;
        return 1;
    }
    catch (args::ValidationError const& e)
    {
        LOG_ERROR << e.what();
        LOG_ERROR << parser;
        return 1;
    }

    if (log_level) {
        std::string logLevelStr = args::get(log_level);
        trim(logLevelStr);
        auto it = logLevelMap.find(logLevelStr);
        if (it == logLevelMap.end()) {
            std::cerr << "Invalid log level specified: " << logLevelStr << ". Valid levels are: error, warning, info, debug.\n";
            return 1;
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
            return 1;
        }
    } else {
        LOG_ERROR << "Action is required.";
        return 1;
    }
    if (client_id) {
        std::string clientIdStr = args::get(client_id);
        trim(clientIdStr);
        settings.client_id = clientIdStr;
    } else {
        LOG_ERROR << "Client ID is required.";
        return 1;
    }
    if (record_type) {
        std::string recordTypeStr = args::get(record_type);
        trim(recordTypeStr);
        settings.record_type = recordTypeStr;
    } else if (settings.action == ACTION_UPDATE_RECORD) {
        LOG_ERROR << "Record type is required.";
        return 1;
    }
    if (zone) {
        std::string zoneStr = args::get(zone);
        trim(zoneStr);
        settings.zone = zoneStr;
    } else if (settings.action == ACTION_UPDATE_RECORD) {
        LOG_ERROR << "Zone is required.";
        return 1;
    }
    if (record_name) {
        std::string recordNameStr = args::get(record_name);
        trim(recordNameStr);
        settings.record_name = recordNameStr;
    } else if (settings.action == ACTION_UPDATE_RECORD) {
        LOG_ERROR << "Record name is required.";
        return 1;
    }
    if (current_content) {
        std::string oldContentStr = args::get(current_content);
        trim(oldContentStr);
        settings.current_content = oldContentStr;
    } else if (settings.action == ACTION_UPDATE_RECORD && !settings.all_records) {
        LOG_ERROR << "Current content is required.";
        return 1;
    }
    if (replace_content) {
        std::string replaceContentStr = args::get(replace_content);
        trim(replaceContentStr);
        settings.replace_content = replaceContentStr;
    }
    if (new_content) {
        std::string newContentStr = args::get(new_content);
        trim(newContentStr);
        settings.new_content = newContentStr;
    } else if (settings.action == ACTION_UPDATE_RECORD) {
        LOG_ERROR << "New content is required.";
        return 1;
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
    LOG_DEBUG << "  Current Content: " << settings.current_content;
    LOG_DEBUG << "  Replace Content: " << settings.replace_content;
    LOG_DEBUG << "  New Content: " << settings.new_content;
    LOG_DEBUG << "  All Records: " << (settings.all_records ? "true" : "false");
    LOG_DEBUG << "  Dry Run: " << (settings.dry_run ? "true" : "false");

    LopDnsClient client(settings.base_url, settings.timeout);

    if (!client.authenticate(settings.client_id, settings.token_duration_sec)) {
        LOG_ERROR << "Authentication failed.";
        return 1;
    }

    std::list<std::string> zones = client.getZones();
    if (!settings.zone.empty()) {
        if (std::find(zones.begin(), zones.end(), settings.zone) == zones.end()) {
            LOG_ERROR << "Specified zone not found: " << settings.zone;
            return 1;
        }
        zones = {settings.zone};
    }
    else {
        if (zones.empty()) {
            LOG_ERROR << "No zones available to retrieve records from.";
            return 1;
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
        case ACTION_UPDATE_RECORD:
        {
            auto records = client.getRecords(settings.zone);
            bool found = false;
            for (const auto& record : records) {
                std::cmatch regex_match;
                if (record.name == settings.record_name && 
                    record.type == settings.record_type && 
                        ((settings.all_records && settings.current_content.empty()) || 
                        std::regex_search(record.content.c_str(), regex_match, std::regex(settings.current_content)))) {
                    std::string new_content;
                    if (settings.replace_content.empty()) {
                        new_content = settings.new_content;
                    }
                    else {
                        new_content = std::regex_replace(record.content, std::regex(settings.replace_content), settings.new_content);
                    }
                    if (new_content == record.content) {
                        LOG_INFO << "Record content is already up to date for record: " << record.name << "\n";
                        continue;
                    }
                    
                    Record updatedRecord;

                    if (settings.dry_run) {
                        LOG_INFO << "[Dry Run] Would update record:\n";
                        updatedRecord = record;
                        updatedRecord.content = new_content;
                    }
                    else {
                        updatedRecord = client.updateRecord(settings.zone, settings.record_name,
                                                                    settings.record_type, record.content,
                                                                    settings.new_content);
                        LOG_INFO << "Updated record:\n";
                    }
                    found = true;
                    LOG_INFO << "  Name: " << updatedRecord.name << ", Type: " << updatedRecord.type
                              << ", Content: " << updatedRecord.content << ", TTL: " << updatedRecord.ttl
                              << ", Priority: " << updatedRecord.priority << "\n";
                    if (!settings.all_records) {
                        break;
                    }
                }
            }
            if (!found) {
                LOG_INFO << "No records updated for name: " << settings.record_name
                          << " and type: " << settings.record_type << "\n";
                return 1;
            }
            break;
        }
        default:
            LOG_ERROR << "Unknown action.";
            return 1;
    }
  }
  catch (std::exception& e)
  {
    LOG_ERROR << "Exception: " << e.what();
  }

  return 0;
}
