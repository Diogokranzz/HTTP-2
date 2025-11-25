#pragma once
#include <string>
#include <string_view>
#include <vector>
#include <sstream>

namespace http {

class Json {
public:
    static std::string serialize(const std::vector<std::pair<std::string, std::string>>& fields) {
        std::stringstream ss;
        ss << "{";
        for (size_t i = 0; i < fields.size(); ++i) {
            ss << "\"" << fields[i].first << "\": \"" << fields[i].second << "\"";
            if (i < fields.size() - 1) ss << ", ";
        }
        ss << "}";
        return ss.str();
    }

    
    static std::string get_value(std::string_view json, std::string_view key) {
        std::string key_str = "\"" + std::string(key) + "\"";
        size_t key_pos = json.find(key_str);
        if (key_pos == std::string_view::npos) return "";

        size_t pos = key_pos + key_str.length();
        
        
        while (pos < json.length() && (json[pos] == ' ' || json[pos] == ':' || json[pos] == '\t' || json[pos] == '\n' || json[pos] == '\r')) {
            pos++;
        }

       
        if (pos >= json.length() || json[pos] != '\"') return "";
        pos++; 

        size_t end = pos;
        while (end < json.length() && json[end] != '\"') {
            if (json[end] == '\\' && end + 1 < json.length()) end++;
            end++;
        }
        
        return std::string(json.substr(pos, end - pos));
    }
};

}
