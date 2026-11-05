#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <variant>

class Bill {
public:
    virtual ~Bill() = default;
    virtual double total() const = 0;
};

class OutPatientBill : public Bill {
    double consultation;
    double tests;
public:
    OutPatientBill(double c, double t) : consultation(c), tests(t) {}
    double total() const override { return consultation + tests; }
};

class InPatientBill : public Bill {
    int days;
    double rate;
    double extras;
public:
    InPatientBill(int d, double r, double e) : days(d), rate(r), extras(e) {}
    double total() const override { return days * rate + extras; }
};

struct BillRecord {
    int type; // 0 = OP, 1 = IP
    double consultation;
    double tests;
    int days;
    double dailyRate;
    double extras;
};

double switchTotal(const BillRecord& b) {
    switch (b.type) {
        case 0: return b.consultation + b.tests;
        case 1: return b.days * b.dailyRate + b.extras;
        default: throw std::invalid_argument("unknown bill type");
    }
}

struct OPVariant { double consultation; double tests; };
struct IPVariant { int days; double dailyRate; double extras; };
using BillVariant = std::variant<OPVariant, IPVariant>;

double variantTotal(const BillVariant& b) {
    return std::visit([](const auto& x) -> double {
        using T = std::decay_t<decltype(x)>;
        if constexpr (std::is_same_v<T, OPVariant>)
            return x.consultation + x.tests;
        else
            return x.days * x.dailyRate + x.extras;
    }, b);
}

template <typename F>
double measureMs(F&& fn) {
    const auto start = std::chrono::steady_clock::now();
    const double result = fn();
    const auto stop = std::chrono::steady_clock::now();
    volatile double sink = result;
    (void)sink;
    return std::chrono::duration<double, std::milli>(stop - start).count();
}

bool validateByStatus(double amount) {
    return amount >= 0.0 && amount <= 1000000.0;
}

void validateByException(double amount) {
    if (amount < 0.0) throw std::invalid_argument("negative");
    if (amount > 1000000.0) throw std::out_of_range("too large");
}

int main() {
    std::cout << std::fixed << std::setprecision(3);
    OutPatientBill op(500, 1200);
    InPatientBill ip(3, 2500, 40000);
    Bill* ptrs[] = {&op, &ip};
    std::cout << "Virtual totals: " << ptrs[0]->total()
              << " " << ptrs[1]->total() << '\n';

    const BillRecord records[] = {
        {0, 500, 1200, 0, 0, 0},
        {1, 0, 0, 3, 2500, 40000}
    };
    std::cout << "Switch totals: " << switchTotal(records[0])
              << " " << switchTotal(records[1]) << '\n';

    const std::uint64_t sizes[] = {1000ULL, 1000000ULL, 10000000ULL};
    for (const auto n : sizes) {
        const double virtualMs = measureMs([&]() {
            double sum = 0;
            for (std::uint64_t i = 0; i < n; ++i)
                sum += (i & 1U) ? ptrs[1]->total() : ptrs[0]->total();
            return sum;
        });
        const double switchMs = measureMs([&]() {
            double sum = 0;
            for (std::uint64_t i = 0; i < n; ++i)
                sum += switchTotal(records[i & 1U]);
            return sum;
        });
        const BillVariant variants[] = {OPVariant{500, 1200},
                                        IPVariant{3, 2500, 40000}};
        const double variantMs = measureMs([&]() {
            double sum = 0;
            for (std::uint64_t i = 0; i < n; ++i)
                sum += variantTotal(variants[i & 1U]);
            return sum;
        });
        std::cout << n << " bills: virtual=" << virtualMs
                  << " ms, switch=" << switchMs
                  << " ms, variant=" << variantMs << " ms\n";
    }

    const int invalidPercentages[] = {0, 1, 50};
    constexpr int count = 1000000;
    for (const int percent : invalidPercentages) {
        const int invalidEvery = percent == 0 ? count + 1 : 100 / percent;
        const double statusMs = measureMs([&]() {
            int rejected = 0;
            for (int i = 0; i < count; ++i) {
                const double amount = (i % invalidEvery == 0 && percent != 0)
                                      ? -1.0 : 100.0;
                if (!validateByStatus(amount)) ++rejected;
            }
            return static_cast<double>(rejected);
        });
        const double exceptionMs = measureMs([&]() {
            int rejected = 0;
            for (int i = 0; i < count; ++i) {
                const double amount = (i % invalidEvery == 0 && percent != 0)
                                      ? -1.0 : 100.0;
                try {
                    validateByException(amount);
                } catch (const std::exception&) {
                    ++rejected;
                }
            }
            return static_cast<double>(rejected);
        });
        std::cout << percent << "% invalid: status=" << statusMs
                  << " ms, exceptions=" << exceptionMs << " ms\n";
    }
    return 0;
}
