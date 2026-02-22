#include "test_utils.h"
#include <fstream>
#include <iostream>

bool MemoryStats::read_from_proc() {
    std::ifstream status("/proc/self/status");
    if (!status.is_open()) return false;
    
    std::string line;
    while (std::getline(status, line)) {
        if (line.find("VmSize:") == 0) {
            sscanf(line.c_str(), "VmSize: %zu kB", &vm_size_kb);
        } else if (line.find("VmRSS:") == 0) {
            sscanf(line.c_str(), "VmRSS: %zu kB", &vm_rss_kb);
        } else if (line.find("VmData:") == 0) {
            sscanf(line.c_str(), "VmData: %zu kB", &vm_data_kb);
        }
    }
    return vm_size_kb > 0;
}

std::string MemoryStats::to_string() const {
    std::ostringstream oss;
    oss << "VmSize: " << std::setw(8) << vm_size_kb << " KB, "
        << "VmRSS: " << std::setw(8) << vm_rss_kb << " KB, "
        << "VmData: " << std::setw(8) << vm_data_kb << " KB";
    return oss.str();
}

void MemoryTracker::start() {
    initial.read_from_proc();
    min_stats = initial;
    max_stats = initial;
    current = initial;
    sample_count = 1;
}

void MemoryTracker::sample() {
    current.read_from_proc();
    sample_count++;
    
    // Track minimums
    if (current.vm_size_kb < min_stats.vm_size_kb) min_stats.vm_size_kb = current.vm_size_kb;
    if (current.vm_rss_kb < min_stats.vm_rss_kb) min_stats.vm_rss_kb = current.vm_rss_kb;
    if (current.vm_data_kb < min_stats.vm_data_kb) min_stats.vm_data_kb = current.vm_data_kb;
    
    // Track maximums
    if (current.vm_size_kb > max_stats.vm_size_kb) max_stats.vm_size_kb = current.vm_size_kb;
    if (current.vm_rss_kb > max_stats.vm_rss_kb) max_stats.vm_rss_kb = current.vm_rss_kb;
    if (current.vm_data_kb > max_stats.vm_data_kb) max_stats.vm_data_kb = current.vm_data_kb;
}

void MemoryTracker::print_report() const {
    std::cout << "\n=== Memory Usage Report ===" << std::endl;
    std::cout << "Samples taken: " << sample_count << std::endl;
    std::cout << "\nInitial:  " << initial.to_string() << std::endl;
    std::cout << "Minimum:  " << min_stats.to_string() << std::endl;
    std::cout << "Maximum:  " << max_stats.to_string() << std::endl;
    std::cout << "Final:    " << current.to_string() << std::endl;
    
    // Calculate deltas
    long delta_vmsize = (long)current.vm_size_kb - (long)initial.vm_size_kb;
    long delta_vmrss = (long)current.vm_rss_kb - (long)initial.vm_rss_kb;
    long delta_vmdata = (long)current.vm_data_kb - (long)initial.vm_data_kb;
    
    std::cout << "\nMemory Growth (Final - Initial):" << std::endl;
    std::cout << "  VmSize: " << std::setw(8) << delta_vmsize << " KB";
    if (delta_vmsize > 0) std::cout << " (↑ increased)";
    else if (delta_vmsize < 0) std::cout << " (↓ decreased)";
    std::cout << std::endl;
    
    std::cout << "  VmRSS:  " << std::setw(8) << delta_vmrss << " KB";
    if (delta_vmrss > 0) std::cout << " (↑ increased)";
    else if (delta_vmrss < 0) std::cout << " (↓ decreased)";
    std::cout << std::endl;
    
    std::cout << "  VmData: " << std::setw(8) << delta_vmdata << " KB";
    if (delta_vmdata > 0) std::cout << " (↑ increased)";
    else if (delta_vmdata < 0) std::cout << " (↓ decreased)";
    std::cout << std::endl;
    
    // Peak memory increase from initial
    long peak_vmsize = (long)max_stats.vm_size_kb - (long)initial.vm_size_kb;
    long peak_vmrss = (long)max_stats.vm_rss_kb - (long)initial.vm_rss_kb;
    long peak_vmdata = (long)max_stats.vm_data_kb - (long)initial.vm_data_kb;
    
    std::cout << "\nPeak Memory Growth (Max - Initial):" << std::endl;
    std::cout << "  VmSize: " << std::setw(8) << peak_vmsize << " KB" << std::endl;
    std::cout << "  VmRSS:  " << std::setw(8) << peak_vmrss << " KB" << std::endl;
    std::cout << "  VmData: " << std::setw(8) << peak_vmdata << " KB" << std::endl;
    
    // Leak detection heuristic
    std::cout << "\n=== Memory Leak Analysis ===" << std::endl;
    const long LEAK_THRESHOLD_KB = 1024; // 1 MB threshold
    
    bool potential_leak = false;
    if (delta_vmrss > LEAK_THRESHOLD_KB) {
        std::cout << "⚠️  WARNING: VmRSS increased by " << delta_vmrss 
                  << " KB (>" << LEAK_THRESHOLD_KB << " KB threshold)" << std::endl;
        potential_leak = true;
    }
    if (delta_vmdata > LEAK_THRESHOLD_KB) {
        std::cout << "⚠️  WARNING: VmData increased by " << delta_vmdata 
                  << " KB (>" << LEAK_THRESHOLD_KB << " KB threshold)" << std::endl;
        potential_leak = true;
    }
    
    if (!potential_leak) {
        std::cout << "✓ No significant memory growth detected" << std::endl;
        std::cout << "  All memory deltas are within acceptable threshold (< " 
                  << LEAK_THRESHOLD_KB << " KB)" << std::endl;
    } else {
        std::cout << "\nNote: Some memory growth is normal for:" << std::endl;
        std::cout << "  - Python interpreter caches" << std::endl;
        std::cout << "  - GIL state management" << std::endl;
        std::cout << "  - Message parser buffers" << std::endl;
        std::cout << "Large sustained growth across iterations indicates a leak." << std::endl;
    }
}
