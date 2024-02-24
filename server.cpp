#include "src/client_http.hpp"
#include "src/server_http.hpp"

// Added for the json-example
#define BOOST_SPIRIT_THREADSAFE
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

// Added for the default_resource example
#include <algorithm>
#include <boost/filesystem.hpp>
#include <fstream>
#include <iostream>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <ctime>
#include <cctype>

using HttpServer = SimpleWeb::Server<SimpleWeb::HTTP>;
using HttpClient = SimpleWeb::Client<SimpleWeb::HTTP>;

static std::unordered_map<unsigned int, std::unordered_map<unsigned int, struct daily_stats *>> sensor_stats; // key=sensor_id
std::mutex mtx;

struct daily_stats {
    int min;
    int max;
    int num_of_sample;
    int sum_of_sample;
    std::tm *inputDate;
};

bool is_string_to_int_valid(const std::string &str, int &num) {
    try {
        num = std::stoi(str);
    }
    catch (std::invalid_argument const& ex) {
        return false;
    }
    return true;
}

// Function to get the day of the week and check if it's within the last 7 days
bool is_within_past_week(std::tm* date) {
    if (!date) {
        return false;
    }
    std::time_t currentTime = std::time(nullptr);
    std::time_t inputTime = std::mktime(date);
    std::time_t sevenDaysAgo = currentTime - (7 * 24 * 60 * 60);

    return (inputTime >= sevenDaysAgo && inputTime <= currentTime);
}

// function to convert string day to int day 
int day_to_int(std::string day) {
    if (day == "SUN") return 0;
    if (day == "MON") return 1;
    if (day == "TUE") return 2;
    if (day == "WED") return 3;
    if (day == "THU") return 4;
    if (day == "FRI") return 5;
    if (day == "SAT") return 6;

    return -1;
}

// function to translate a date to a int week day
int date_to_day(const std::tm* date) {
    char dayBuffer[20];
    std::strftime(dayBuffer, sizeof(dayBuffer), "%A", date);
    std::string day = dayBuffer;
    day = day.substr(0,3);
    std::transform(day.begin(), day.end(), day.begin(), ::toupper);
    
    return day_to_int(day);
}

// function to insert a 
void insert_data(const int sen_id, const int temp, std::tm *date) {
    int day_id = date_to_day(date);
    if (day_id < 0 || day_id > 6) {
        return;
    }

    // a lock is needed in order to block other threads from tempering data during write
    mtx.lock();
    auto sen_table = sensor_stats.find(sen_id);
    // sensor doesnt exist in map, create and insert it
    if (sen_table == sensor_stats.end()) { 
        std::unordered_map<unsigned int, struct daily_stats *> map;
        struct daily_stats *daily = new daily_stats{temp, temp, 1, temp, date};
        map.insert({day_id, daily});
        sensor_stats.insert({sen_id, map});
    // sensor exist in map
    } else {
        auto daily = sen_table->second.find(day_id);
        // day doesnt exist in inner map, create and insert it
        if (daily == sen_table->second.end()) { 
            struct daily_stats *daily_st = new daily_stats{temp, temp, 1, temp, date};
            sen_table->second.insert({day_id, daily_st});
        // day exist in map
        } else {
            // current day is from past week, add stats
            if (is_within_past_week(daily->second->inputDate) == true) {
                if (temp < daily->second->min) daily->second->min = temp;
                if (temp > daily->second->max) daily->second->max = temp;
                daily->second->num_of_sample++;
                daily->second->sum_of_sample += temp;
            // current day isnt from past week, repopulate stats 
            } else {
                daily->second->inputDate = date;
                daily->second->min = temp;
                daily->second->max = temp;
                daily->second->sum_of_sample = temp;
                daily->second->num_of_sample = 1;
            }
        }
    }
    mtx.unlock();
}

// fuction to parse sensor id, temperature and date and insert if valid
void parse_sensor_input(const std::string &in, const char sep) {
    if (in.size() == 0) {
        return;
    }
    std::string sensor_id, temperature, time_stamp, temp_str;
    for (size_t i=0; i<in.length(); i++) {
        if (in[i] == sep) {
            if (sensor_id.size() == 0) {
                sensor_id = temp_str;
            } else if (temperature.size() == 0) {
                temperature = temp_str;
            } else if (time_stamp.size() == 0) {
                time_stamp = temp_str;
            } else {
                return;
            }
            temp_str = "";
            continue;
        }
        temp_str += in[i];
    }
    if (sensor_id.size() == 0 || temperature.size() == 0) {
        return;
    }
    if (time_stamp.size() == 0) {
        time_stamp = temp_str;
    }

    int sen_id, temp, day, month, year;
    bool is_valid = is_string_to_int_valid(sensor_id, sen_id);
    if (!is_valid || sen_id < 0) {
        return;
    }
    is_valid = is_string_to_int_valid(temperature, temp);
    if (!is_valid) {
        return;
    }
    // parse date and check if valid and is within past week
    sscanf (time_stamp.c_str(),"%d/%d/%d",&day, &month, &year);
    std::tm *inputDate = new std::tm();
    inputDate->tm_year = year - 1900;   // Year minus 1900
    inputDate->tm_mon = month - 1;      // Month is 0-based
    inputDate->tm_mday = day;           // Day of the month
    is_valid = is_within_past_week(inputDate);
    if (!is_valid) {
        delete inputDate;
        return;
    }
    insert_data(sen_id, temp, inputDate);
}

// function to retrive daily stats for a specific sensor
struct daily_stats *get_sensor_stats_per_day(const int sen_id, const std::string day) {
    int day_id = day_to_int(day);
    auto sen_table = sensor_stats.find(sen_id);
    // no sensor found to show stats
    if (sen_table == sensor_stats.end()) {
        return nullptr;
    }
    // required day has no stats
    if (!sen_table->second[day_id]) {
        return nullptr;
    }
    bool is_valid = is_within_past_week(sen_table->second[day_id]->inputDate);
    // stats are not from past week
    if (!is_valid) {
        return nullptr;
    }
    return sen_table->second[day_id];
}

// function to calculate weekly stats for a specific sensor
void get_sensor_stats_of_past_week(const int sen_id, struct daily_stats &daily_st) {
    auto sen_table = sensor_stats.find(sen_id);
    if (sen_table == sensor_stats.end()) {
        return;
    }
    bool is_first_max = true, is_first_min = true;
    for (auto daily : sen_table->second) {
        if (is_within_past_week(daily.second->inputDate)) {
            daily_st.num_of_sample += daily.second->num_of_sample;
            daily_st.sum_of_sample += daily.second->sum_of_sample;
            if (is_first_max == true) {
                daily_st.max = daily.second->max;
                is_first_max = false;
            }
            else if (daily.second->max > daily_st.max) {
                daily_st.max = daily.second->max;
            }
            if (is_first_min == true) {
                daily_st.min = daily.second->min;
                is_first_min = false;
            }
            else if (daily.second->min < daily_st.min) {
                daily_st.min = daily.second->min;
            }
        } else {

        }
    }
    return;
}

// function to calculate weekly stats for all sensors
void get_all_sensor_stats_of_past_week(struct daily_stats &total_stats) {
    bool is_first = true;
    for (auto sensor : sensor_stats) {
        struct daily_stats daily{0, 0, 0, 0, nullptr};
        get_sensor_stats_of_past_week(sensor.first, daily);
        if (is_first == true) {
            total_stats.max = daily.max;
            total_stats.min = daily.min;
            total_stats.num_of_sample = daily.num_of_sample;
            total_stats.sum_of_sample = daily.sum_of_sample;
            is_first = false;
        } else {
            if (daily.max > total_stats.max) {
                total_stats.max = daily.max;
            }
            if (daily.min < total_stats.min) {
                total_stats.min = daily.min;
            }
            total_stats.sum_of_sample += daily.sum_of_sample;
            total_stats.num_of_sample += daily.num_of_sample;
        }
    }
    return;
}

// function to add print to output stream
void print_stats(const struct daily_stats *daily, std::shared_ptr<HttpServer::Response> response, std::stringstream &stream) {
    if (daily->num_of_sample == 0) {
        return;
    }
    stream << "max temperature: " << daily->max << std::endl;
    stream << "min temperature: " << daily->min << std::endl;
    stream << "average temperature: " << (daily->sum_of_sample / daily->num_of_sample) << std::endl;
    response->write(stream);
}

// function to print to output stream
void get_stats(const std::string &sensor_id, const std::string day, std::shared_ptr<HttpServer::Response> response) {
    std::stringstream stream;
    if (sensor_id != "ALL") {
        int sen_id;
        bool is_valid = is_string_to_int_valid(sensor_id, sen_id);
        if (!is_valid || sen_id < 0) {
            return;
        }
        // stats for specific sensor for specific day
        if (day != "ALL") {
            struct daily_stats *daily = get_sensor_stats_per_day(sen_id, day);
            if (!daily) {
                return;
            }
            stream << "stats for sensor " << sen_id << " for day:" << day << std::endl;
            print_stats(daily, response, stream);
        // stats for specific sensor in past week
        } else {    
            struct daily_stats daily{0, 0, 0, 0, nullptr};
            get_sensor_stats_of_past_week(sen_id, daily);
            if (daily.num_of_sample == 0) {
                return;
            }
            stream << "stats for sensor " << sen_id << " for past week:" << std::endl;
            print_stats(&daily, response, stream);
        }
    // stats for all sensors in past week
    } else {    
        if (sensor_stats.size() == 0) {
            return;
        }
        struct daily_stats daily{0, 0, 0, 0, nullptr};
        get_all_sensor_stats_of_past_week(daily);
        stream << "stats for all sensors for past week:" << std::endl;
        print_stats(&daily, response, stream);
    }
}

// function to parse sensor id, day to show required stats
void parse_stats_input(const std::string &in, const char sep, std::shared_ptr<HttpServer::Response> response) {
    if(in.size() == 0) {
        return;
    }
    std::string sensor_id, day, temp_str;
    for (size_t i=0; i<in.length(); i++) {
        if (in[i] == sep) {
            if (sensor_id.size() == 0) {
                sensor_id = temp_str;
            } else if (day.size() == 0) {
                day = temp_str;
            } else {
                return;
            }
            temp_str = "";
            continue;
        }
        temp_str += in[i];
    }
    if (sensor_id.size() == 0) {
        return;
    }
    if (day.size() == 0) {
        day = temp_str;
    }
    get_stats(sensor_id, day, response);
}

int main() {
    // HTTP-server at port 8080
    HttpServer server;
    server.config.port = 8080;

    // GET /stats activate stats collection and response
    server.resource["^/stats$"]["GET"] = [](std::shared_ptr<HttpServer::Response> response, std::shared_ptr<HttpServer::Request> request) {
    std::stringstream stream;
    std::string query_string = request->query_string.c_str();
    parse_stats_input(query_string, ',', response);
    response->write(stream);
  };

  // POST /string expect string to be parsed and entered into the data base
  server.resource["^/string$"]["POST"] = [](std::shared_ptr<HttpServer::Response> response, std::shared_ptr<HttpServer::Request> request) {
    std::string content = request->content.string();

    // use threads to allow concurrent requests
    std::thread worker(parse_sensor_input, content, ',');
    if(worker.joinable()) {
        worker.detach();
    }
    content = "";
    response->write(content);
  };

    // Start server and receive assigned port when server is listening for requests
    std::promise<unsigned short> server_port;
    std::thread server_thread([&server, &server_port]() {
        // Start server
        server.start([&server_port](unsigned short port) {
            server_port.set_value(port);
        });
    });
    std::cout << "Server listening on port " << server_port.get_future().get() << std::endl;
    std::cout << "For adding sensor stats use:  curl -X POST http://localhost:8080/string -d \"sensor_id,temparture,date\"" << std::endl;
    std::cout << "For example:  curl -X POST http://localhost:8080/string -d \"2,30,20/2/2024\"" << std::endl;
    std::cout << "For recieving stats use:  curl -X GET http://localhost:8080/stats?<sensor_id/ALL>,SUN/MON/TUE/WED/THU/FRI/SAT/ALL>" << std::endl;
    std::cout << "For example:  curl -X GET http://localhost:8080/stats?2,MON" << std::endl;

    server_thread.join();
}
