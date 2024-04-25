#include "util.h"
#include "SpookyV2.h"
#include "debug.h"

#include <fmt/core.h>
#include <chrono>

namespace lurp {

bool Globals::debugSave = false;

uint32_t Random::getTime()
{
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::time_point_cast<std::chrono::milliseconds>(now).time_since_epoch().count();
    return static_cast<uint32_t>(ms);
}

std::vector<std::string_view> splitSV(const std::string& str, char delim)
{
    std::vector<std::string_view> result;
    std::string_view sv(str);
    size_t pos = 0;
    while ((pos = sv.find(delim)) != std::string_view::npos) {
        if (pos > 0)
            result.push_back(sv.substr(0, pos));
        sv.remove_prefix(pos + 1);
    }
    if (!sv.empty())
        result.push_back(sv);
    return result;
}

std::vector<std::string> split(const std::string& str, char delim)
{
    std::vector<std::string> result;
    size_t pos = 0;
    while (pos < str.size()) {
		size_t next = str.find(delim, pos);
        if (next == std::string::npos)
			next = str.size();
		result.push_back(str.substr(pos, next - pos));
        pos = next + 1;
	}
    return result;
}

uint64_t Hash(const std::string& a, const std::string& b, const std::string& c, const std::string& d)
{
    uint64_t hash = 0, unused = 0;
    SpookyHash spooky;
    spooky.Init(0, 0);
    spooky.Update(a.data(), a.size());
    spooky.Update(b.data(), b.size());
    spooky.Update(c.data(), c.size());
    spooky.Update(d.data(), d.size());
    spooky.Final(&hash, &unused);
    return hash;
}

size_t parseRegion(const std::string& str, size_t start, char open, char close)
{
    assert(str[start] == open);
    int count = 1;
    size_t end = start + 1;

    while (count && end < str.size()) {
        if (str[end] == close)
            count--;
        else if (str[end] == open)
            count++;
        end++;
    }
    assert(end > 0);
    if (str[end - 1] != close) {
        std::string msg = fmt::format("Mismatched brackets in string '{}' starting at '{}'",
            str.substr(0, 12), str.substr(start, 12));
        FatalError(msg);
    }
    return end;
}

std::pair<std::string, std::string> parseKV(const std::string& str, char* sepOut)
{
    size_t sep = str.find('=');
    if (sep == std::string::npos) {
		std::string msg = fmt::format("Invalid key-value pair: '{}'", str);
		FatalError(msg);
	}
    std::string key = trim(str.substr(0, sep));
    std::string value = trim(str.substr(sep + 1));

    if (key.empty() || value.empty()) {
        std::string msg = fmt::format("Invalid key-value pair: '{}'", str);
        FatalError(msg);
    }
    if (value[0] == '{' || value[0] == '\'' || value[0] == '"') {
		char open = value[0];
		char close = (open == '{') ? '}' : open;
		size_t end = parseRegion(value, 0, open, close);
		value = value.substr(1, end - 2);
        if (sepOut)
			*sepOut = open;
	}    
	return std::make_pair(key, value);
}

std::vector<std::string> parseKVPairs(const std::string& str)
{
    std::vector<std::string> result;
	size_t pos = 0;
    while (pos < str.size()) {
		size_t next = str.find(',', pos);
		if (next == std::string::npos)
			next = str.size();
        std::string subStr = str.substr(pos, next - pos);
        if (!subStr.empty())
			result.push_back(trim(subStr));
		pos = next + 1;
	}
	return result;
}

} // namespace lurp
