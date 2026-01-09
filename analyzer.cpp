#include <string>
#include <vector>
#include <iostream>
#include <algorithm>
#include <unordered_map>
#include <array>
#include <string_view>
#include <cctype>
#include <ifstream>
 
using namespace std;
 
// ===================== analyzer.h (inlined) =====================
struct ZoneCount {
    std::string zone;
    long long count;
};
 
struct SlotCount {
    std::string zone;
    int hour;              // 0-23
    long long count;
};
 
class TripAnalyzer {
public:
    // Parse Trips.csv, skip dirty rows, never crash
    void ingestFile(const string& csvPath);
 
    // Top K zones: count desc, zone asc
    std::vector<ZoneCount> topZones(int k = 10) const;
 
    // Top K slots: count desc, zone asc, hour asc
    std::vector<SlotCount> topBusySlots(int k = 10) const;
 
private:
    // Transparent hashing so we can find() with string_view without allocating a string each row
    struct TransparentHash {
        using is_transparent = void;
        size_t operator()(std::string_view sv) const noexcept {
            return std::hash<std::string_view>{}(sv);
        }
        size_t operator()(const std::string& s) const noexcept {
            return std::hash<std::string_view>{}(std::string_view{s});
        }
    };
    struct TransparentEq {
        using is_transparent = void;
        bool operator()(std::string_view a, std::string_view b) const noexcept { return a == b; }
        bool operator()(const std::string& a, std::string_view b) const noexcept { return std::string_view{a} == b; }
        bool operator()(std::string_view a, const std::string& b) const noexcept { return a == std::string_view{b}; }
        bool operator()(const std::string& a, const std::string& b) const noexcept { return a == b; }
    };
 
    // zone string -> index
    std::unordered_map<std::string, int, TransparentHash, TransparentEq> zoneIndex;
    std::vector<std::string> zones;                 // index -> zone id string
    std::vector<long long> zoneCounts;              // index -> count
    std::vector<std::array<long long, 24>> hourCounts; // index -> [hour] count
};
 
// ===================== analyzer.cpp (inlined) =====================
 
// fast hour parse: expects "YYYY-MM-DD HH:MM" (at least ".... ....:..")
static inline int fastParseHour(const char* p, size_t len) noexcept {
    // Need at least positions 10=' ' 11,12 = digits
    if (len < 13) return -1;
    if (p[10] != ' ') return -1;
    unsigned char c1 = static_cast<unsigned char>(p[11]);
    unsigned char c2 = static_cast<unsigned char>(p[12]);
    if (!std::isdigit(c1) || !std::isdigit(c2)) return -1;
    int h = (p[11] - '0') * 10 + (p[12] - '0');
    return (h >= 0 && h <= 23) ? h : -1;
}
 
// find next comma starting from pos (no exceptions)
static inline size_t findComma(const std::string& s, size_t start) noexcept {
    return s.find(',', start);
}
 
void TripAnalyzer::ingestFile(const string& csvPath) {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
 
    
    // reset (fresh start)
    zoneIndex.clear();
    zones.clear();
    zoneCounts.clear();
    hourCounts.clear();
    ifstream file(csvPath);
    std::string line;
    // Skip header
    if (!std::getline(file, line)) return;
 
    // Optional: if you expect many zones, reserving helps.
    // You can tweak these based on dataset size.
    zoneIndex.reserve(4096);
    zones.reserve(4096);
    zoneCounts.reserve(4096);
    hourCounts.reserve(4096);
 
    while (std::getline(file, line)) {
        if (line.empty()) continue;
 
        // Need at least 6 fields => at least 5 commas.
        // Fields: 0 TripID, 1 PickupZoneID, 2 DropoffZoneID, 3 PickupDateTime, 4 DistanceKm, 5 FareAmount
        size_t c0 = findComma(line, 0);
        if (c0 == std::string::npos) continue;
        size_t c1 = findComma(line, c0 + 1);
        if (c1 == std::string::npos) continue;
        size_t c2 = findComma(line, c1 + 1);
        if (c2 == std::string::npos) continue;
        size_t c3 = findComma(line, c2 + 1);
        if (c3 == std::string::npos) continue;
        size_t c4 = findComma(line, c3 + 1);
        if (c4 == std::string::npos) continue;
 
        // pickupZone is between c0+1 and c1-1
        size_t zStart = c0 + 1;
        size_t zLen = (c1 > zStart) ? (c1 - zStart) : 0;
        if (zLen == 0) continue;
 
        std::string_view zoneSv(line.data() + zStart, zLen);
 
        // pickupDateTime is between c2+1 and c3-1
        size_t dtStart = c2 + 1;
        size_t dtLen = (c3 > dtStart) ? (c3 - dtStart) : 0;
        if (dtLen == 0) continue;
 
        int hour = fastParseHour(line.data() + dtStart, dtLen);
        if (hour < 0) continue; // dirty row
 
        // Find or create zone index (no allocation for lookup)
        auto it = zoneIndex.find(zoneSv);
        int idx;
        if (it == zoneIndex.end()) {
            idx = (int)zones.size();
            zones.emplace_back(zoneSv);     // allocate only once per new zone
            zoneCounts.push_back(0);
            hourCounts.push_back({});
            hourCounts.back().fill(0);
            zoneIndex.emplace(zones.back(), idx);
        } else {
            idx = it->second;
        }
 
        // Aggregate
        zoneCounts[idx] += 1;
        hourCounts[idx][hour] += 1;
    }
}
 
std::vector<ZoneCount> TripAnalyzer::topZones(int k) const {
    std::vector<ZoneCount> result;
    result.reserve(zones.size());
 
    for (size_t i = 0; i < zones.size(); ++i) {
        result.push_back({zones[i], zoneCounts[i]});
    }
 
    auto cmp = [](const ZoneCount& a, const ZoneCount& b) {
        if (a.count != b.count) return a.count > b.count;
        return a.zone < b.zone;
    };
 
    if (k <= 0) return {};
    if ((int)result.size() <= k) {
        std::sort(result.begin(), result.end(), cmp);
        return result;
    }
 
    // Top-k without full sort
    std::nth_element(result.begin(), result.begin() + k, result.end(), cmp);
    result.resize(k);
    std::sort(result.begin(), result.end(), cmp);
    return result;
}
 
std::vector<SlotCount> TripAnalyzer::topBusySlots(int k) const {
    std::vector<SlotCount> all;
    all.reserve(zones.size() * 4); // heuristic; many hours will be zero
 
    for (size_t i = 0; i < zones.size(); ++i) {
        const auto& arr = hourCounts[i];
        for (int h = 0; h < 24; ++h) {
            long long c = arr[h];
            if (c != 0) all.push_back({zones[i], h, c});
        }
    }
 
    auto cmp = [](const SlotCount& a, const SlotCount& b) {
        if (a.count != b.count) return a.count > b.count;
        if (a.zone != b.zone) return a.zone < b.zone;
        return a.hour < b.hour;
    };
 
    if (k <= 0) return {};
    if ((int)all.size() <= k) {
        std::sort(all.begin(), all.end(), cmp);
        return all;
    }
 
    std::nth_element(all.begin(), all.begin() + k, all.end(), cmp);
    all.resize(k);
    std::sort(all.begin(), all.end(), cmp);
    return all;
}
 
// ===================== main (UNCHANGED as required) =====================
int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
 
    TripAnalyzer analyzer;
    analyzer.ingestStdin();
 
    cout << "TOP_ZONES\n";
    for (auto& z : analyzer.topZones())
        cout << z.zone << "," << z.count << "\n";
 
    cout << "TOP_SLOTS\n";
    for (auto& s : analyzer.topBusySlots())
        cout << s.zone << "," << s.hour << "," << s.count << "\n";
 
    return 0;
}
