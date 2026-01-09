#include "analyzer.h"
#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <utility>
#include <algorithm>
#include <array>
using namespace std;
// Students may use ANY data structure internally

void TripAnalyzer::ingestFile(const std::string& csvPath) {
    // TODO:
    // - open file
    // - skip header
    // - skip malformed rows
    // - extract PickupZoneID and pickup hour
    // - aggregate counts
    zoneMapCount.clear();
    slotMapCount.clear();
    ifstream inputFile(csvPath); 
    string line;

    getline(inputFile, line); // skip header
    while (getline(inputFile, line)){
        size_t firstComma = line.find(',');
        if (firstComma == string::npos) continue;
        size_t secondComma = line.find(',', firstComma + 1);
        if (secondComma == string::npos) continue;
        size_t thirdComma = line.find(',', secondComma + 1);
        if (thirdComma == string::npos) continue;
        size_t fourthComma = line.find(',', thirdComma + 1);
        if (fourthComma == string::npos) continue;
        size_t fifthComma = line.find(',', fourthComma + 1);
        if (fifthComma == string::npos) continue;
         
        //extracting zoneID
        string zoneID = line.substr(firstComma + 1, secondComma - firstComma - 1);
        if (zoneID.empty()) continue;

        //extracting hour
        string dateHour = line.substr(thirdComma + 1, fourthComma - thirdComma -1);
        if (dateHour.size() < 16) continue; //malformed rows
        string pickUpHourString = dateHour.substr(11, 2);
        int pickUpHour = (pickUpHourString[0] - '0') * 10 + (pickUpHourString[1] - '0');

         
        zoneMapCount[zoneID]++; //mapping 
        slotMapCount[zoneID][pickUpHour]++;
    }
}

std::vector<ZoneCount> TripAnalyzer::topZones(int k) const {
    // TODO:
    // - sort by count desc, zone asc
    // - return first kvector<ZoneCount> result;
    vector<ZoneCount> result;
    result.reserve(zoneMapCount.size());
    for (const auto& entry : zoneMapCount) {
        result.push_back({entry.first, entry.second});
    }
    if (result.empty() || k <= 0) return {};
    int kk = std::min(k, (int)result.size());
    nth_element(result.begin(), result.begin() + kk, result.end(),
        [](const ZoneCount& a, const ZoneCount& b) {
            if (a.count != b.count)
                return a.count > b.count; 
            return a.zone < b.zone;     
        });
    sort(result.begin(), result.begin() + kk,
        [](const ZoneCount& a, const ZoneCount& b) {
            if (a.count != b.count)
                return a.count > b.count; 
            return a.zone < b.zone;     
        });
    result.resize(kk);
    return result;
}


std::vector<SlotCount> TripAnalyzer::topBusySlots(int k) const {
    // TODO:
    // - sort by count desc, zone asc, hour asc
    // - return first k
    std::vector<SlotCount> result;


    result.reserve(slotMapCount.size() * 24);
    for (const auto& entry : slotMapCount) {
        const auto& zone = entry.first;
        const auto& hourArray = entry.second;
        for (int hour = 0; hour < 24; ++hour) {
            if (hourArray[hour] > 0) {
                result.push_back({zone, hour, hourArray[hour]});
            }
        }
    }

    if (result.empty() || k <= 0) return {};
    int kk = min(k, (int)result.size());
    nth_element(result.begin(), result.begin() + kk, result.end(),
        [](const SlotCount& a, const SlotCount& b) {
            if (a.count != b.count)
                return a.count > b.count; 
            if (a.zone != b.zone)
                return a.zone < b.zone; 
            return a.hour < b.hour;     
        });
    sort(result.begin(), result.begin() + kk,
        [](const SlotCount& a, const SlotCount& b) {
            if (a.count != b.count)
                return a.count > b.count; 
            if (a.zone != b.zone)
                return a.zone < b.zone; 
            return a.hour < b.hour;     
        });

    result.resize(kk);
    return result;
}






