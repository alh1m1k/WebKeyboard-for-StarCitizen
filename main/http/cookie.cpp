#include "cookie.h"

#include "string"
#include "util.h"
#include "generated.h"


namespace http {

    using namespace std::literals;

    cookie::cookie(): name(), value(), httpOnly(false) {};

    cookie::cookie(const std::string& name, const std::string& value, bool httpOnly ): name(name), value(value), httpOnly(httpOnly) {};

    cookie::cookie(const std::string&& name, const std::string& value, bool httpOnly ): name(name), value(value), httpOnly(httpOnly) {};

    cookie::cookie(const std::string& cookies) {
        size_t      pos = 0;
        uint16_t    index = 0;
        size_t      cSize = cookies.size();

        infoIf(LOG_COOKIE_BUILD, "cookie::cookie", cookies.c_str());

        while(pos < cSize) {
            size_t wordEndPos   = std::min(cookies.find(';', pos), cSize);
            size_t nameEndPos   = std::min(cookies.find('=', pos), wordEndPos);

            std::string name = cookies.substr(pos, nameEndPos-pos);
            trim(name);

            if (nameEndPos == wordEndPos) {
                if (index == 0) {
                    break;
                } else {
                    populate(std::move(name), "", index++);
                }
            } else {
                pos = nameEndPos+1;
                while(pos < cSize && cookies[pos] == ' ') pos++;

                if (pos == cSize) {
                    break;
                }

                std::string value = cookies.substr(pos, wordEndPos-pos);
                trim(value);
                if (value[0] == '"' && value[value.size()-1] == '"') {
                    value = value.substr(1, value.size()-2);
                }
                populate(std::move(name), std::move(value), index++);
            }

            pos = wordEndPos + 1;
            while(pos < cookies.size() && cookies[pos] == ' ') pos++;
        }

    };

    cookie::cookie(const cookie& copy):  name(copy.name), value(copy.value), path(copy.path) {
        maxAge = copy.maxAge;
        httpOnly = copy.httpOnly;
        if (copy._heap != nullptr) {
            _heap = std::make_unique<std::unordered_map<std::string, std::string>>(*copy._heap);
        }
    }

    cookie::cookie(cookie&& move): _heap(std::move(move._heap)), name(std::move(move.name)), value(std::move(move.value)), path(std::move(move.path)) {
        maxAge = move.maxAge;
        httpOnly = move.httpOnly;
    }

    cookie& cookie::operator=(const cookie& copy) {
        name     = copy.name;
        value    = copy.value;
        path     = copy.path;
        maxAge   = copy.maxAge;
        httpOnly = copy.httpOnly;
        if (copy._heap != nullptr) {
            _heap = std::make_unique<std::unordered_map<std::string, std::string>>(*copy._heap);
        }
        return *this;
    }

    cookie& cookie::operator=(cookie&& move) {
        name     = std::move(move.name);
        value    = std::move(move.value);
        path     = std::move(move.path);
        maxAge   = move.maxAge;
        httpOnly = move.httpOnly;
        _heap    = std::move(move._heap);
        return *this;
    }

    bool cookie::populate(std::string&& name_, std::string&& value_, uint16_t index) {

        debugIf(LOG_COOKIE_BUILD, "cookie::populate", name_.c_str(), " ", value_.c_str(), " ", value_.size(), " ", index);

        if (index == 0) {
            name    = name_;
            value   = value_;
            return true;
        }

        if (name_ == "Path"s) {
            path = value_;
            return true;
        } else if (name_ == "HttpOnly"s) {
            httpOnly = true;
            return true;
        } else if (name_ == "Max-Age"s) {

            try {
                maxAge = std::stoi(value_);
            } catch (const std::invalid_argument& e) {
                error("cookie parse ", e.what());
            } catch (const std::out_of_range& e) {
                error("cookie parse ", e.what());
            }
            return true;
        } else {
            debug("emplace", name_.c_str(), " ", value_.c_str(), " ", index);
            rest().emplace(std::move(name_), std::move(value_));
            return true;
        }

        return false;
    }

    std::unordered_map<std::string, std::string>& cookie::rest() const {
        if (_heap == nullptr) {
            _heap = std::make_unique<std::unordered_map<std::string, std::string>>();
        }
        return *_heap;
    }

    std::string cookie::string() const {
        //"session_id=12345; Path=/; HttpOnly; Max-Age=3600"
        std::string buffer = {};
        if (name.size() && value.size()) {
            //todo encoding
            buffer = name + "=" + value;
        } else {
            return buffer;
        }
        if (path.size()) {
            buffer += "; Path=" + path;
        }
        if (httpOnly) {
            buffer += "; HttpOnly";
        }
        if (maxAge >= 0) {
            buffer += "; Max-Age=" + std::to_string(maxAge);
        }

        if (_heap != nullptr) {
            for (auto it = _heap->begin(); it != _heap->end(); ++it) {
                //todo encoding
                if (it->second == "") {
                    buffer += "; " + it->first;
                } else {
                    buffer += "; " + it->first + "=" + it->second;
                }
            }
        }

        return buffer;
    }

    bool cookie::valid() const {
        if (!name.size() || !value.size()) {
            return false;
        }
        return true;
    }

}
