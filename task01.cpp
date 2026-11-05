#include <iostream>
#include <iomanip>
#include <map>
#include <memory>
#include <string>
#include <vector>

class Bill {
protected:
    std::string patient;
public:
    explicit Bill(const std::string& p) : patient(p) {}
    virtual ~Bill() = default;
    virtual double total() const = 0;
    virtual std::string type() const { return "Bill"; }
    const std::string& patientName() const { return patient; }
    virtual void print() const {
        std::cout << type() << ' ' << patient << ": " << total() << '\n';
    }
};

class OutPatient : public Bill {
    double consultation;
    double tests;
public:
    OutPatient(const std::string& p, double c, double t)
        : Bill(p), consultation(c), tests(t) {}
    double total() const override { return consultation + tests; }
    std::string type() const override { return "OP"; }
};

class InPatient : public Bill {
    int days;
    double perDay;
    double proceduresAndMedicines;
public:
    InPatient(const std::string& p, int d, double rate, double extras)
        : Bill(p), days(d), perDay(rate), proceduresAndMedicines(extras) {}
    double total() const override { return days * perDay + proceduresAndMedicines; }
    std::string type() const override { return "IP"; }
};

class DayCareBill : public Bill {
    double packagePrice;
public:
    DayCareBill(const std::string& p, double price)
        : Bill(p), packagePrice(price) {}
    double total() const override { return packagePrice; }
    std::string type() const override { return "DayCare"; }
};

class EmergencyBill : public Bill {
    double baseCharge;
    double hourlyRate;
    int hours;
public:
    EmergencyBill(const std::string& p, double base, double rate, int h)
        : Bill(p), baseCharge(base), hourlyRate(rate), hours(h) {}
    double total() const override { return baseCharge + hourlyRate * hours; }
    std::string type() const override { return "Emergency"; }
};

class Payer {
public:
    virtual ~Payer() = default;
    virtual double amountCovered(const Bill& bill) const = 0;
    virtual std::string name() const = 0;
};

class SelfPay : public Payer {
public:
    double amountCovered(const Bill&) const override { return 0.0; }
    std::string name() const override { return "SelfPay"; }
};

class Insurance : public Payer {
    double coverLimit;
    double coPaymentPercent;
public:
    Insurance(double limit, double copay)
        : coverLimit(limit), coPaymentPercent(copay) {}
    double amountCovered(const Bill& bill) const override {
        const double eligible = bill.total() < coverLimit ? bill.total() : coverLimit;
        return eligible * (100.0 - coPaymentPercent) / 100.0;
    }
    std::string name() const override { return "Insurance"; }
};

class GovernmentScheme : public Payer {
    std::map<std::string, double> packageRates;
public:
    explicit GovernmentScheme(std::map<std::string, double> rates)
        : packageRates(std::move(rates)) {}
    double amountCovered(const Bill& bill) const override {
        const auto it = packageRates.find(bill.type());
        if (it == packageRates.end()) return 0.0;
        return it->second < bill.total() ? it->second : bill.total();
    }
    std::string name() const override { return "GovernmentScheme"; }
};

int main() {
    std::cout << std::fixed << std::setprecision(2);
    std::vector<std::unique_ptr<Bill>> bills;
    bills.push_back(std::make_unique<OutPatient>("Ravi", 500, 1200));
    bills.push_back(std::make_unique<InPatient>("Meena", 3, 2500, 40000));
    bills.push_back(std::make_unique<DayCareBill>("Arun", 18000));
    bills.push_back(std::make_unique<EmergencyBill>("Sara", 3000, 750, 2));

    SelfPay self;
    Insurance insurance(50000, 20);
    GovernmentScheme scheme({{"OP", 1500}, {"IP", 30000},
                             {"DayCare", 16000}, {"Emergency", 4000}});
    std::vector<const Payer*> payers{&self, &insurance, &scheme};

    double dayTotal = 0.0;
    for (const auto& bill : bills) {
        bill->print();
        dayTotal += bill->total();
        for (const Payer* payer : payers) {
            std::cout << "  " << payer->name() << " covers "
                      << payer->amountCovered(*bill) << '\n';
        }
    }
    std::cout << "Day total: " << dayTotal << '\n';
    return 0;
}
