// ============================================================
// DRILL 6 — PARKING LOT (LLD, fresher-level, runnable)
// Escalations to SAY (he will ask):
//   1. "Multiple floors?" -> vector of floors, each a vector of slots;
//      keep ONE queue of free slot-ids so finding a slot is O(1).
//   2. "Bike vs car vs truck sizes?" -> slot has a type; park only in
//      a matching-or-bigger free slot.
//   3. "Nearest free slot?" -> maintain the free-slot ids in a min-heap/queue
//      instead of scanning.
//   4. "Two cars park at the same time?" -> the check-and-mark must be
//      atomic: one mutex around park/unpark (say it, no need to code).
//   5. "Fee slabs?" -> fee table per hour range instead of flat rate.
// COMPILE & RUN:  g++ -std=c++17 06_parking_lot.cpp -o pk && ./pk
// PREDICTED OUTPUT:
//   car DL-01 parked at slot 1
//   bike HR-77 parked at slot 2
//   free: 1/3
//   DL-01 unparked from slot 1, fee Rs.60
//   free: 2/3
//   XX-00 not found
// ============================================================

#include <iostream>
#include <string>
#include <vector>
using namespace std;

class Vehicle {
public:
    string number;
    string type;                       // "car" / "bike"
    Vehicle(string n, string t) { number = n; type = t; }
};

class Slot {
public:
    int id;
    bool occupied;
    string vehicleNumber;
    int entryHour;                     // for fee calculation
    Slot(int i) { id = i; occupied = false; vehicleNumber = ""; entryHour = 0; }
};

class ParkingLot {
    vector<Slot> slots;
    int ratePerHour;
public:
    ParkingLot(int n, int rate) {
        ratePerHour = rate;
        for (int i = 1; i <= n; i++) slots.push_back(Slot(i));
    }
    int freeCount() {
        int c = 0;
        for (int i = 0; i < (int)slots.size(); i++)
            if (!slots[i].occupied) c++;
        return c;
    }
    bool full() { return freeCount() == 0; }

    int park(Vehicle v, int entryHour) {
        if (full()) { cout << "FULL: " << v.number << " rejected\n"; return -1; }
        for (int i = 0; i < (int)slots.size(); i++) {
            if (!slots[i].occupied) {                     // first free slot
                slots[i].occupied = true;
                slots[i].vehicleNumber = v.number;
                slots[i].entryHour = entryHour;
                cout << v.type << " " << v.number << " parked at slot " << slots[i].id << endl;
                return slots[i].id;
            }
        }
        return -1;
    }

    int unpark(string number, int exitHour) {
        for (int i = 0; i < (int)slots.size(); i++) {
            if (slots[i].occupied && slots[i].vehicleNumber == number) {
                int hours = exitHour - slots[i].entryHour;
                if (hours < 1) hours = 1;                 // minimum billing = 1 hour
                int fee = hours * ratePerHour;
                cout << number << " unparked from slot " << slots[i].id
                     << ", fee Rs." << fee << endl;
                slots[i].occupied = false;
                slots[i].vehicleNumber = "";
                return fee;
            }
        }
        cout << number << " not found\n";
        return -1;
    }

    void status() { cout << "free: " << freeCount() << "/" << slots.size() << endl; }
};

int main() {
    ParkingLot lot(3, 20);                                // 3 slots, Rs.20/hour
    lot.park(Vehicle("DL-01", "car"), 10);
    lot.park(Vehicle("HR-77", "bike"), 11);
    lot.status();
    lot.unpark("DL-01", 13);                              // 3 hours -> Rs.60
    lot.status();
    lot.unpark("XX-00", 5);                               // unknown vehicle
    return 0;
}
