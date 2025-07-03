#include "sysinfo.h"
#include <mach/mach.h>
#include <sys/sysctl.h>
#include <sys/mount.h>
#include <IOKit/ps/IOPowerSources.h>
#include <IOKit/ps/IOPSKeys.h>
#include <CoreFoundation/CoreFoundation.h>
#include <chrono>
#include <thread>
#include <cstring>
#include <vector>

namespace sysinfo {

static uint64_t last_idle = 0, last_total = 0;

void initialize() {
    // Capture initial CPU tick counts
    host_cpu_load_info_data_t cpuinfo;
    mach_msg_type_number_t count = HOST_CPU_LOAD_INFO_COUNT;
    host_statistics(mach_host_self(), HOST_CPU_LOAD_INFO, (host_info_t)&cpuinfo, &count);
    last_idle = cpuinfo.cpu_ticks[CPU_STATE_IDLE];
    last_total = 0;
    for (int i = 0; i < CPU_STATE_MAX; ++i)
        last_total += cpuinfo.cpu_ticks[i];
}

float get_cpu_usage() {
    host_cpu_load_info_data_t cpuinfo;
    mach_msg_type_number_t count = HOST_CPU_LOAD_INFO_COUNT;
    host_statistics(mach_host_self(), HOST_CPU_LOAD_INFO, (host_info_t)&cpuinfo, &count);
    uint64_t idle = cpuinfo.cpu_ticks[CPU_STATE_IDLE];
    uint64_t total = 0;
    for (int i = 0; i < CPU_STATE_MAX; ++i)
        total += cpuinfo.cpu_ticks[i];
    uint64_t delta_idle = idle - last_idle;
    uint64_t delta_total = total - last_total;
    last_idle = idle;
    last_total = total;
    if (delta_total == 0) return 0.0f;
    return (1.0f - (float)delta_idle / delta_total) * 100.0f;
}

std::string get_cpu_brand() {
    char buf[256];
    size_t len = sizeof(buf);
    sysctlbyname("machdep.cpu.brand_string", &buf, &len, nullptr, 0);
    return std::string(buf);
}

void get_memory(float &used_gb, float &total_gb, float &percent) {
    mach_msg_type_number_t count = HOST_VM_INFO64_COUNT;
    vm_statistics64_data_t vmstat;
    host_statistics64(mach_host_self(), HOST_VM_INFO64, (host_info64_t)&vmstat, &count);
    int64_t pagesize;
    size_t size = sizeof(pagesize);
    sysctlbyname("hw.pagesize", &pagesize, &size, nullptr, 0);
    int64_t free = vmstat.free_count + vmstat.inactive_count;
    int64_t used = vmstat.active_count + vmstat.wire_count;
    int64_t total = free + used;
    total_gb = total * pagesize / 1e9;
    used_gb = used * pagesize / 1e9;
    percent = (used_gb / total_gb) * 100.0f;
}

float get_uptime() {
    struct timeval boottime;
    size_t size = sizeof(boottime);
    int mib[2] = {CTL_KERN, KERN_BOOTTIME};
    sysctl(mib, 2, &boottime, &size, nullptr, 0);
    time_t bsec = boottime.tv_sec, now = time(nullptr);
    return static_cast<float>(now - bsec);
}

void get_disk(float &used_gb, float &total_gb, float &percent) {
    struct statfs stats;
    statfs("/", &stats);
    uint64_t total = stats.f_blocks * stats.f_bsize;
    uint64_t free = stats.f_bfree * stats.f_bsize;
    total_gb = total / 1e9;
    used_gb = (total - free) / 1e9;
    percent = (used_gb / total_gb) * 100.0f;
}

void get_battery(std::string &status, int &percent) {
    CFTypeRef info = IOPSCopyPowerSourcesInfo();
    CFArrayRef sources = IOPSCopyPowerSourcesList(info);
    if (CFArrayGetCount(sources) == 0) {
        status = "Unknown";
        percent = -1;
        return;
    }
    CFDictionaryRef desc = IOPSGetPowerSourceDescription(info, CFArrayGetValueAtIndex(sources, 0));
    if (!desc) {
        status = "Unknown";
        percent = -1;
        return;
    }
    CFStringRef state = (CFStringRef)CFDictionaryGetValue(desc, CFSTR(kIOPSPowerSourceStateKey));
    CFNumberRef capacity = (CFNumberRef)CFDictionaryGetValue(desc, CFSTR(kIOPSCurrentCapacityKey));
    CFNumberRef max = (CFNumberRef)CFDictionaryGetValue(desc, CFSTR(kIOPSMaxCapacityKey));
    int cur = 0, maxval = 0;
    CFNumberGetValue(capacity, kCFNumberIntType, &cur);
    CFNumberGetValue(max, kCFNumberIntType, &maxval);
    percent = (int)((float)cur / maxval * 100.0f);
    status = CFStringGetCStringPtr(state, kCFStringEncodingUTF8);
    CFRelease(sources);
    CFRelease(info);
}

SysStats collect() {
    SysStats stats;
    stats.cpu_brand = get_cpu_brand();
    stats.cpu_usage_percent = get_cpu_usage();
    get_memory(stats.memory_used_gb, stats.memory_total_gb, stats.memory_percent);
    stats.uptime_seconds = get_uptime();
    get_disk(stats.disk_used_gb, stats.disk_total_gb, stats.disk_percent);
    get_battery(stats.battery_status, stats.battery_percent);
    return stats;
}

void shutdown() {
    // No cleanup necessary
}

}

