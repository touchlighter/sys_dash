#pragma once

#include <string>
#include <vector>

// High-level system stats
struct SysStats {
    std::string cpu_brand;
    float cpu_usage_percent;
    float memory_used_gb;
    float memory_total_gb;
    float memory_percent;
    float uptime_seconds;
    float disk_used_gb;
    float disk_total_gb;
    float disk_percent;
    std::string battery_status; // e.g. "Charging", "Discharging", "Full"
    int battery_percent;
};

// Core API
namespace sysinfo {
    SysStats collect();
    void initialize();
    void shutdown();
}

