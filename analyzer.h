#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

struct ZoneCount {
    std::string zone;
    long long count;
};

struct SlotCount {
    std::string zone;
    int hour;              // 0–23
    long long count;
};

struct PairHash {
    size_t operator()(const std::pair<std::string, int>& p) const noexcept {
        return std::hash<std::string>{}(p.first) ^ (std::hash<int>{}(p.second) << 1);
    }
};

class TripAnalyzer {
public:
    // Parse Trips.csv, skip dirty rows, never crash
    void ingestFile(const std::string& csvPath);

    // Top K zones: count desc, zone asc
    std::vector<ZoneCount> topZones(int k = 10) const;

    // Top K slots: count desc, zone asc, hour asc
    std::vector<SlotCount> topBusySlots(int k = 10) const;
    std::unordered_map<std::string, long long> zoneMapCount;
    std::unordered_map<std::pair<std::string, int>, long long, PairHash> slotMapCount;
    

};
