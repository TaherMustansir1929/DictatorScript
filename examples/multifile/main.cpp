#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <cctype>

int add(int a, int b);
int subtract(int a, int b);
int factorial(int n);

int add(int a, int b) {
    return a + b;
}

int subtract(int a, int b) {
    return a - b;
}

int factorial(int n) {
    if (n <= 1) {
        return 1;
    }
    return n * factorial(n - 1);
}

int main() {
    int result = add(3, 4);
    std::cout << "3 + 4 = ";
    std::cout << result;
    std::cout << "\n";
    int fact = factorial(5);
    std::cout << "5! = ";
    std::cout << fact;
    std::cout << "\n";

    return 0;
}

