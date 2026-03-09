#ifndef DICTATORSCRIPT_ERROR_REPORTER_H
#define DICTATORSCRIPT_ERROR_REPORTER_H

#include <string>
#include <vector>

// Severity levels for diagnostic messages.
enum class Severity { ERROR, WARNING };

// A single diagnostic message with source location.
struct Diagnostic {
    Severity severity;
    std::string filename;
    int line;
    int column;
    std::string message;

    std::string toString() const;
};

// Central error/warning reporter used by all compiler stages.
// Collects diagnostics and provides a hasErrors() check to abort the pipeline.
class ErrorReporter {
public:
    void report(Severity severity, const std::string& filename,
                int line, int column, const std::string& message);

    void error(const std::string& filename, int line, int column,
               const std::string& message);
    void warning(const std::string& filename, int line, int column,
                 const std::string& message);

    bool hasErrors() const;
    bool hasWarnings() const;

    // Print all collected diagnostics to stderr.
    void printAll() const;

    // Clear all diagnostics (useful between files).
    void clear();

    const std::vector<Diagnostic>& getDiagnostics() const;

private:
    std::vector<Diagnostic> diagnostics_;
    int errorCount_ = 0;
    int warningCount_ = 0;
};

#endif // DICTATORSCRIPT_ERROR_REPORTER_H
