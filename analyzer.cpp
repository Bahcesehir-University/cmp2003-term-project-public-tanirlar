#include "analyzer.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <unordered_map>
#include <memory>

// Implementation class (hidden from header)
class TripAnalyzerImpl {
public:
    std::unordered_map<std::string, long long> zoneCounts;
    std::unordered_map<std::string, long long> slotCounts;
};

// Global map to associate TripAnalyzer instances with their implementation
static std::unordered_map<const TripAnalyzer*, std::unique_ptr<TripAnalyzerImpl>> implMap;

// Helper: Get or create impl for a TripAnalyzer instance
static TripAnalyzerImpl* getImpl(const TripAnalyzer* ta) {
    auto it = implMap.find(ta);
    if (it == implMap.end()) {
        implMap[ta] = std::make_unique<TripAnalyzerImpl>();
        return implMap[ta].get();
    }
    return it->second.get();
}

// Helper: Clean up old entries (simple heuristic: if map is too large, clear it)
static void cleanupIfNeeded() {
    // If we have too many entries, it likely means old test objects are lingering
    // Clear entries for objects that might be destroyed
    if (implMap.size() > 100) {
        implMap.clear();
    }
}

// Helper function to parse hour from datetime string
// Format: "YYYY-MM-DD HH:MM"
static int parseHour(const std::string& datetime) {
    size_t spacePos = datetime.find(' ');
    if (spacePos == std::string::npos) return -1;

    std::string timePart = datetime.substr(spacePos + 1);
    size_t colonPos = timePart.find(':');
    if (colonPos == std::string::npos) return -1;

    try {
        int hour = std::stoi(timePart.substr(0, colonPos));
        if (hour < 0 || hour > 23) return -1;
        return hour;
    } catch (...) {
        return -1;
    }
}

void TripAnalyzer::ingestFile(const std::string& csvPath) {
    cleanupIfNeeded();
    TripAnalyzerImpl* impl = getImpl(this);

    // Clear existing data for this instance (fresh start)
    impl->zoneCounts.clear();
    impl->slotCounts.clear();

    std::ifstream file(csvPath);
    if (!file.is_open()) return;

    std::string line;
    // Skip header
    if (!std::getline(file, line)) return;

    while (std::getline(file, line)) {
        if (line.empty()) continue;

        std::stringstream ss(line);
        std::string field;
        std::vector<std::string> fields;

        while (std::getline(ss, field, ',')) {
            fields.push_back(field);
        }

        // Need at least 6 fields: TripID,PickupZoneID,DropoffZoneID,PickupDateTime,DistanceKm,FareAmount
        if (fields.size() < 6) continue;

        std::string pickupZone = fields[1];
        std::string pickupDateTime = fields[3];

        if (pickupZone.empty() || pickupDateTime.empty()) continue;

        int hour = parseHour(pickupDateTime);
        if (hour == -1) continue;  // Invalid hour

        // Aggregate zone counts
        impl->zoneCounts[pickupZone]++;

        // Aggregate slot counts (zone + hour)
        std::string slotKey = pickupZone + "_" + std::to_string(hour);
        impl->slotCounts[slotKey]++;
    }

    file.close();
}

std::vector<ZoneCount> TripAnalyzer::topZones(int k) const {
    TripAnalyzerImpl* impl = getImpl(this);
    std::vector<ZoneCount> result;

    for (const auto& pair : impl->zoneCounts) {
        result.push_back({pair.first, pair.second});
    }

    // Sort: count desc, then zone asc
    std::sort(result.begin(), result.end(), [](const ZoneCount& a, const ZoneCount& b) {
        if (a.count != b.count) return a.count > b.count;
        return a.zone < b.zone;
    });

    // Return top k
    if (result.size() > static_cast<size_t>(k)) {
        result.resize(k);
    }

    return result;
}

std::vector<SlotCount> TripAnalyzer::topBusySlots(int k) const {
    TripAnalyzerImpl* impl = getImpl(this);
    std::vector<SlotCount> result;

    for (const auto& pair : impl->slotCounts) {
        // Parse back the zone and hour from key "zone_hour"
        size_t lastUnderscore = pair.first.find_last_of('_');
        std::string zone = pair.first.substr(0, lastUnderscore);
        int hour = std::stoi(pair.first.substr(lastUnderscore + 1));

        result.push_back({zone, hour, pair.second});
    }

    // Sort: count desc, then zone asc, then hour asc
    std::sort(result.begin(), result.end(), [](const SlotCount& a, const SlotCount& b) {
        if (a.count != b.count) return a.count > b.count;
        if (a.zone != b.zone) return a.zone < b.zone;
        return a.hour < b.hour;
    });

    // Return top k
    if (result.size() > static_cast<size_t>(k)) {
        result.resize(k);
    }

    return result;
}
