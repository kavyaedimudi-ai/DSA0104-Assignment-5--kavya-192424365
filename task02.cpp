#include <iostream>
#include <limits>
#include <new>
#include <stdexcept>
#include <string>

class BillingError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class ClaimError : public BillingError {
public:
    using BillingError::BillingError;
};

double approve(double billed, double cover) {
    if (billed < 0 || cover < 0)
        throw std::invalid_argument("negative amount");
    if (billed > cover)
        throw ClaimError("exceeds cover");
    return billed;
}

double settle(double billed, double cover) {
    try {
        return approve(billed, cover);
    } catch (const ClaimError& e) {
        std::cout << "audit: " << e.what() << '\n';
        throw; // rethrow the same exception object
    }
}

class ResourceGuard {
    std::string label;
public:
    explicit ResourceGuard(std::string s) : label(std::move(s)) {}
    ~ResourceGuard() noexcept {
        std::cout << "released " << label << '\n';
    }
};

void parseAmount(const std::string& input) {
    const double value = std::stod(input);
    std::cout << "parsed " << value << '\n';
}

void testAllocationFailure() {
    // Deterministic demonstration of the bad_alloc handler.
    // A real allocation failure is implementation/environment dependent.
    throw std::bad_alloc();
}

int main() {
    const double cases[][2] = {
        {40000, 50000},
        {60000, 50000},
        {-1, 50000}
    };

    for (const auto& item : cases) {
        try {
            const double paid = settle(item[0], item[1]);
            std::cout << "approved " << paid << '\n';
        } catch (const ClaimError& e) {
            std::cout << "claim rejected: " << e.what() << '\n';
        } catch (const std::exception& e) {
            std::cout << "error: " << e.what() << '\n';
        }
    }

    try {
        ResourceGuard guard("claim-file");
        parseAmount("not-a-number");
    } catch (const std::invalid_argument& e) {
        std::cout << "input error: " << e.what() << '\n';
    }

    try {
        testAllocationFailure();
    } catch (const std::bad_alloc&) {
        std::cout << "allocation failure handled\n";
    }
    return 0;
}
