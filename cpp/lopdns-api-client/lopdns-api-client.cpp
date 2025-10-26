//
// lopdns-api-client.cpp
// ~~~~~~~~~~~~~~~
//

#include <iostream>
#include <istream>
#include <ostream>
#include <string>
#include <map>
#include <list>
#include <ctime>
#include <exception>
#include <args.hxx>
#include "restclient-cpp/connection.h"
#include "restclient-cpp/restclient.h"
#include "lopdnsclient.h"


const std::string URL = "https://api.lopdns.se/v2";

typedef enum ActionType {
    ACTION_GET_ZONES,
    ACTION_GET_RECORDS,
    ACTION_UPDATE_RECORD
} ActionType;

const std::map<std::string, ActionType> actionMap = {
    {"get-zones", ACTION_GET_ZONES},
    {"get-records", ACTION_GET_RECORDS},
    {"update-record", ACTION_UPDATE_RECORD}
};

struct Settings
{
    std::string base_url = URL;
    int timeout = 10;
    int token_duration_sec = 3600;
    std::string client_id;
    bool verbose = false;
    ActionType action = ACTION_GET_ZONES;
    std::string zone;
    std::string record_name;
    std::string record_type = "A";
    std::string current_content;
    std::string new_content;
    bool all_records = false;
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
    args::Flag verbose(parser, "verbose", "Enable verbose logging", {'v', "verbose"}, false);
    args::ValueFlag<std::string> zone(parser, "zone", "The DNS zone to update", {'z', "zone"}, "");
    args::ValueFlag<std::string> record_type(parser, "record_type", "The type of the DNS record", {'r', "record-type"}, "A");
    args::ValueFlag<std::string> record_name(parser, "record_name", "The name of the DNS record", {'n', "record-name"}, "");
    args::ValueFlag<std::string> current_content(parser, "current_content", "Current content for DNS tasks", {'u', "current-content"}, "");
    args::ValueFlag<std::string> new_content(parser, "new_content", "New content for DNS tasks", {'w', "new-content"}, "");
    args::ValueFlag<std::string> action(parser, "action", "The action to perform", {'a', "action"}, "");
    args::Flag all_records(parser, "all_records", "Flag to indicate that all applicable records should be processed", {'A', "all-records"}, false);

    try
    {
        parser.ParseCLI(argc, argv);
    }
    catch (args::Help)
    {
        std::cout << parser;
        return 0;
    }
    catch (args::ParseError e)
    {
        std::cerr << e.what() << std::endl;
        std::cerr << parser;
        return 1;
    }
    catch (args::ValidationError e)
    {
        std::cerr << e.what() << std::endl;
        std::cerr << parser;
        return 1;
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
            std::cerr << "Invalid action specified: " << actionStr << ".\n";
            std::cout << "Valid actions are:\n";
            for (const auto& pair : actionMap) {
                std::cout << "  " << pair.first << "\n";
            }
            return 1;
        }
    } else {
        std::cerr << "Action is required.\n";
        return 1;
    }
    if (client_id) {
        std::string clientIdStr = args::get(client_id);
        trim(clientIdStr);
        settings.client_id = clientIdStr;
    } else {
        std::cerr << "Client ID is required.\n";
        return 1;
    }
    if (record_type) {
        std::string recordTypeStr = args::get(record_type);
        trim(recordTypeStr);
        settings.record_type = recordTypeStr;
    } else if (settings.action == ACTION_UPDATE_RECORD) {
        std::cerr << "Record type is required.\n";
        return 1;
    }
    if (zone) {
        std::string zoneStr = args::get(zone);
        trim(zoneStr);
        settings.zone = zoneStr;
    } else if (settings.action == ACTION_UPDATE_RECORD) {
        std::cerr << "Zone is required.\n";
        return 1;
    }
    if (record_name) {
        std::string recordNameStr = args::get(record_name);
        trim(recordNameStr);
        settings.record_name = recordNameStr;
    } else if (settings.action == ACTION_UPDATE_RECORD) {
        std::cerr << "Record name is required.\n";
        return 1;
    }
    if (current_content) {
        std::string oldContentStr = args::get(current_content);
        trim(oldContentStr);
        settings.current_content = oldContentStr;
    } else if (settings.action == ACTION_UPDATE_RECORD && !settings.all_records) {
        std::cerr << "Current content is required.\n";
        return 1;
    }
    if (new_content) {
        std::string newContentStr = args::get(new_content);
        trim(newContentStr);
        settings.new_content = newContentStr;
    } else if (settings.action == ACTION_UPDATE_RECORD) {
        std::cerr << "New content is required.\n";
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
    if (verbose) {
        settings.verbose = args::get(verbose);
    }
    if (settings.verbose) {
        std::cout << "Settings:" << std::endl;
        std::cout << "  Base URL: " << settings.base_url << std::endl;
        std::cout << "  Timeout: " << settings.timeout << " seconds" << std::endl;
        std::cout << "  Token Duration: " << settings.token_duration_sec << " seconds" << std::endl;
        std::cout << "  Client ID: " << settings.client_id << std::endl;
        std::cout << "  Action: ";
        for (const auto& pair : actionMap) {
            if (pair.second == settings.action) {
                std::cout << pair.first << std::endl;
                break;
            }
        }
        std::cout << "  Zone: " << settings.zone << std::endl;
        std::cout << "  Record Type: " << settings.record_type << std::endl;
        std::cout << "  Record Name: " << settings.record_name << std::endl;
        std::cout << "  Current Content: " << settings.current_content << std::endl;
        std::cout << "  New Content: " << settings.new_content << std::endl;
    }


    LopDnsClient client(settings.base_url, settings.timeout, settings.verbose);

    if (!client.authenticate(settings.client_id, settings.token_duration_sec)) {
        std::cerr << "Authentication failed.\n";
        return 1;
    }

    std::list<std::string> zones = client.getZones();
    if (!settings.zone.empty()) {
        if (std::find(zones.begin(), zones.end(), settings.zone) == zones.end()) {
            std::cerr << "Specified zone not found: " << settings.zone << "\n";
            return 1;
        }
        zones = {settings.zone};
    }
    else {
        if (zones.empty()) {
            std::cerr << "No zones available to retrieve records from.\n";
            return 1;
        }
    }

    switch (settings.action) {
        case ACTION_GET_ZONES:
        {
            std::cout << "Zones:\n";
            for (const auto& zone : zones) {
                std::cout << "  " << zone << "\n";
            }
            break;
        }
        case ACTION_GET_RECORDS:
        {
            // Retrieve and print records for each zone
            for (const auto& zone : zones) {
                std::cout << "Records in zone " << zone << ":\n";
                auto records = client.getRecords(zone);
                for (const auto& record : records) {
                    std::cout << "  Name: " << record.name << ", Type: " << record.type
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
                if (record.name == settings.record_name && 
                    record.type == settings.record_type && 
                        ((settings.all_records && settings.current_content.empty()) || 
                        record.content == settings.current_content)) {
                    if (settings.new_content == record.content) {
                        std::cout << "Record content is already up to date for record: " << record.name << "\n";
                        continue;
                    }
                    Record updatedRecord = client.updateRecord(settings.zone, settings.record_name,
                                                                settings.record_type, record.content,
                                                                settings.new_content);
                    std::cout << "Updated Record:\n";
                    std::cout << "  Name: " << updatedRecord.name << ", Type: " << updatedRecord.type
                                << ", Content: " << updatedRecord.content << ", TTL: " << updatedRecord.ttl
                                << ", Priority: " << updatedRecord.priority << "\n";
                    found = true;
                    if (!settings.all_records) {
                        break;
                    }
                }
            }
            if (!found) {
                std::cout << "No records updated for name: " << settings.record_name
                            << " and type: " << settings.record_type << "\n";
                return 1;
            }
            break;
        }
        default:
            std::cerr << "Unknown action.\n";
            return 1;
    }
  }
  catch (std::exception& e)
  {
    std::cout << "Exception: " << e.what() << "\n";
  }

  return 0;
}