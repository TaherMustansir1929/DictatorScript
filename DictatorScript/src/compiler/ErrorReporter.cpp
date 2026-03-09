#include "ErrorReporter.h"
#include <iostream>

std::string Diagnostic::toString() const {
    std::string prefix = (severity == Severity::ERROR)
        ? "[DictatorScript Error] "
        : "[DictatorScript Warning] ";
    return prefix + filename + ":" + std::to_string(line) + ":" +
           std::to_string(column) + " — " + message;
}

void ErrorReporter::report(Severity severity, const std::string& filename,
                           int line, int column, const std::string& message) {
    diagnostics_.push_back({severity, filename, line, column, message});
    if (severity == Severity::ERROR) {
        errorCount_++;
    } else {
        warningCount_++;
    }
}

void ErrorReporter::error(const std::string& filename, int line, int column,
                          const std::string& message) {
    report(Severity::ERROR, filename, line, column, message);
}

void ErrorReporter::warning(const std::string& filename, int line, int column,
                            const std::string& message) {
    report(Severity::WARNING, filename, line, column, message);
}

bool ErrorReporter::hasErrors() const {
    return errorCount_ > 0;
}

bool ErrorReporter::hasWarnings() const {
    return warningCount_ > 0;
}

void ErrorReporter::printAll() const {
    for (const auto& diag : diagnostics_) {
        std::cerr << diag.toString() << std::endl;
    }
}

void ErrorReporter::clear() {
    diagnostics_.clear();
    errorCount_ = 0;
    warningCount_ = 0;
}

const std::vector<Diagnostic>& ErrorReporter::getDiagnostics() const {
    return diagnostics_;
}
