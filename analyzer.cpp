#include <string>
#include <vector>
#include <iostream>
#include <algorithm>
#include <unordered_map>
#include <utility>
#include <fstream>
#include <array>
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
    void ingestStdin(const std::string& csvPath);
    std::vector<ZoneCount> topZones(int k = 10) const;
    // Top K zones: count desc, zone asc
    std::vector<SlotCount> topBusySlots(int k = 10) const;
    std::unordered_map<std::string, long long> zoneMapCount;
    unordered_map<string, array<long long, 24>> slotMapCount;
    
    

};

// ===================== analyzer.cpp (inlined) =====================


void TripAnalyzer::ingestStdin(const std::string& csvPath) {
    zoneMapCount.clear();
    slotMapCount.clear();
    ifstream inputFile(csvPath);
    if (!inputFile.is_open()) return;
    string line;

    getline(inputFile, line); // skip header
    while (getline(inputFile, line)){
        size_t firstComma = line.find(',');
        if (firstComma == string::npos) continue; //malformed rows
        size_t secondComma = line.find(',', firstComma + 1);
        if (secondComma == string::npos) continue;
        size_t thirdComma = line.find(',', secondComma + 1);
        if (thirdComma == string::npos) continue;
        size_t fourthComma = line.find(',', thirdComma + 1);  //firstComma-secondComma pickupZoneId; thirdComma-fourthComma pickUpDate
        if (fourthComma == string::npos) continue;
        size_t fifthComma = line.find(',', fourthComma + 1);
        if (fourthComma == string::npos) continue;

        //check malformed rows
        bool malformed = false;
        string ID = line.substr(0, firstComma); 
        for (unsigned char c : ID){
            if (!isdigit(c)){
                malformed = true;
                break;
            }
        }
        if (malformed) continue;
        
        string distance = line.substr(fourthComma + 1, fifthComma - fourthComma - 1);
        for (unsigned char c : distance){
            if (c == '.') continue;
            else if (!isdigit(c)) {malformed = true; break;}
        }
        if (malformed) continue;

        string fareAmount = line.substr(fifthComma + 1,fifthComma + 1);
        for (unsigned char c : fareAmount){
            if (c == '.') continue;
            else if (!isdigit(c)) {malformed = true; break;}
        }
        if (malformed) continue;

        //extracting zoneID
        string zoneID = line.substr(firstComma + 1, secondComma - firstComma - 1);
        if (zoneID.empty()) continue;

        //extracting hour
        string dateHour = line.substr(thirdComma + 1, fourthComma - thirdComma -1);
        if (dateHour.size() < 16) continue; //malformed rows
        string pickUpHourString = dateHour.substr(11, 2);
        int pickUpHour = stoi(pickUpHourString);

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
    partial_sort(result.begin(), result.begin() + kk, result.end(),
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
    result.reserve(slotMapCount.size());
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
    int kk = std::min(k, (int)result.size());
    partial_sort(result.begin(), result.begin() + kk, result.end(),
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

int main() {
   auto t0 = std::chrono::high_resolution_clock::now();

    
    TripAnalyzer analyzer;
    analyzer.ingestStdin("SmallTrips.csv");

    cout << "TOP_ZONES\n";
    for (auto& z : analyzer.topZones())
        cout << z.zone << "," << z.count << "\n";

    cout << "TOP_SLOTS\n";
    for (auto& s : analyzer.topBusySlots())
        cout << s.zone << "," << s.hour << "," << s.count << "\n";

    auto t1 = std::chrono::high_resolution_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();

    std::cout << "EXEC_MS\n" << ms << "\n";
    return 0;
}
