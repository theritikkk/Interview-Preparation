// ============================================================
// DRILL 1 — THE OOP LIVE-CODE QUESTION (the #1 scored item)
// Shows: compile-time polymorphism (overloading) +
//        runtime polymorphism (overriding + virtual) +
//        virtual destructor + delete through base pointer
//
// COMPILE & RUN:  g++ -std=c++17 01_vehicle_car.cpp -o v && ./v
//
// PREDICTED OUTPUT (say it aloud BEFORE running):
//   DL-01 @ Rs.50/km
//   DL-02 @ Rs.55/km
//   DL-03 @ Rs.45/km
//   50 | 75
// ============================================================

#include <iostream>
#include <string>
using namespace std;

class Vehicle {
private:
    string regNo;                        // hidden state = encapsulation
public:
    Vehicle(string regNo) {              // constructor runs at creation
        this->regNo = regNo;             // this-> = the member; plain name = the parameter
    }
    virtual ~Vehicle() { }               // virtual destructor: safe delete via base pointer

    virtual double farePerKm() {         // virtual = runtime dispatch through vtable
        return 20;
    }
    void describe() {                    // shared behaviour
        cout << regNo << " @ Rs." << farePerKm() << "/km" << endl;
    }                                    // farePerKm() inside is virtual -> real type decides
};

class Car : public Vehicle {             // inheritance: Car IS-A Vehicle
private:
    int seats;
public:
    Car(string regNo, int seats) : Vehicle(regNo) {   // ": Vehicle(regNo)" builds the base part FIRST
        this->seats = seats;
    }
    double farePerKm() override {        // same signature = OVERRIDING (runtime); override = safety check
        return 30 + 5 * seats;
    }
    double farePerKm(double surge) {     // OVERLOADING (compile-time): same name, extra parameter
        // NOTE: this overload lives HERE in Car on purpose — a derived class's
        // function name HIDES the base class's overloads of the same name,
        // so c.farePerKm(1.5) would fail if the overload existed only in Vehicle.
        return farePerKm() * surge;      // reuses the override above -> 50 * 1.5 = 75
    }
};

class Truck : public Vehicle {
public:
    Truck(string regNo) : Vehicle(regNo) {}
    double farePerKm() override { return 45; }
};

int main() {
    Car c("DL-01", 4);
    c.describe();                                   // static binding: compiler knows c is a Car

    Vehicle* v1 = new Car("DL-02", 5);              // base pointer -> derived object
    Vehicle* v2 = new Truck("DL-03");
    v1->describe();                                 // dynamic binding -> Car's fare (55)
    v2->describe();                                 // dynamic binding -> Truck's fare (45)

    cout << c.farePerKm() << " | " << c.farePerKm(1.5) << endl;   // overload pair

    delete v1;   // virtual destructor -> Car part destroyed, then Vehicle part. No leak.
    delete v2;
    return 0;
}
