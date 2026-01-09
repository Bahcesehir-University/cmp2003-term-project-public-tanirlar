#pragma once
#include <string>
#include <vector>
#include <unordered_map>
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
    void ingestStdin(const std::string& csvPath);
    std::vector<ZoneCount> topZones(int k = 10) const;
    // Top K zones: count desc, zone asc
    std::vector<SlotCount> topBusySlots(int k = 10) const;
    std::unordered_map<std::string, long long> zoneMapCount;
    unordered_map<string, array<long long, 24>> slotMapCount;
    
    

};
