#include "util.h"
#include "SpookyV2.h"

bool Globals::trace = false;
bool Globals::debugSave = false;

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



