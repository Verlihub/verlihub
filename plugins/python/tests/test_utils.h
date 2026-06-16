#ifndef TEST_UTILS_H
#define TEST_UTILS_H

#include <string>
#include <sstream>
#include <iomanip>

// Memory tracking helper
struct MemoryStats {
    size_t vm_size_kb = 0;   // Virtual memory size
    size_t vm_rss_kb = 0;    // Resident set size (physical memory)
    size_t vm_data_kb = 0;   // Data segment size
    
    bool read_from_proc();
    std::string to_string() const;
};

struct MemoryTracker {
    MemoryStats initial;
    MemoryStats min_stats;
    MemoryStats max_stats;
    MemoryStats current;
    int sample_count = 0;
    
    void start();
    void sample();
    void print_report() const;
};

#endif // TEST_UTILS_H
