#include <iomanip>
#include <iostream>
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
    virtual std::string type() const = 0;
};

class OutPatient : public Bill {
    double consultation, tests;
public:
    OutPatient(const std::string& p, double c, double t)
        : Bill(p), consultation(c), tests(t) {}
    double total() const override { return consultation + tests; }
    std::string type() const override { return "OP"; }
};

class InPatient : public Bill {
    int days;
    double rate, extras;
public:
    InPatient(const std::string& p, int d, double r, double e)
        : Bill(p), days(d), rate(r), extras(e) {}
    double total() const override { return days * rate + extras; }
    std::string type() const override { return "IP"; }
};

// New bill type: added without modifying existing bill classes.
class MaternityBill : public Bill {
    double deliveryPackage;
    double newbornCare;
public:
    MaternityBill(const std::string& p, double package, double care)
        : Bill(p), deliveryPackage(package), newbornCare(care) {}
    double total() const override { return deliveryPackage + newbornCare; }
    std::string type() const override { return "Maternity"; }
};

class Payer {
public:
    virtual ~Payer() = default;
    virtual double amountCovered(const Bill& bill) const = 0;
    virtual std::string name() const = 0;
};

class Insurance : public Payer {
    double limit;
    double copay;
public:
    Insurance(double l, double c) : limit(l), copay(c) {}
    double amountCovered(const Bill& bill) const override {
        const double eligible = bill.total() < limit ? bill.total() : limit;
        return eligible * (100.0 - copay) / 100.0;
    }
    std::string name() const override { return "Insurance"; }
};

// New payer type: added without modifying the existing payer classes.
class CorporatePayer : public Payer {
    double coveragePercent;
    double maximum;
public:
    CorporatePayer(double percent, double maxCover)
        : coveragePercent(percent), maximum(maxCover) {}
    double amountCovered(const Bill& bill) const override {
        const double percentageAmount = bill.total() * coveragePercent / 100.0;
        return percentageAmount < maximum ? percentageAmount : maximum;
    }
    std::string name() const override { return "CorporatePayer"; }
};

int main() {
    std::cout << std::fixed << std::setprecision(2);
    std::vector<std::unique_ptr<Bill>> bills;
    bills.push_back(std::make_unique<OutPatient>("Ravi", 500, 1200));
    bills.push_back(std::make_unique<InPatient>("Meena", 3, 2500, 40000));
    bills.push_back(std::make_unique<MaternityBill>("Latha", 65000, 12000));

    Insurance insurance(50000, 20);
    CorporatePayer corporate(70, 60000);
    const Payer* payers[] = {&insurance, &corporate};

    for (const auto& bill : bills) {
        std::cout << bill->type() << " " << bill->total() << '\n';
        for (const Payer* payer : payers) {
            std::cout << payer->name() << " covers "
                      << payer->amountCovered(*bill) << '\n';
        }
    }
    return 0;
}
