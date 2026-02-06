#include "SystemMonitor.h"
#include "network/HttpSender.h"
#include "database/DatabaseManager.h"
#include <QStorageInfo>
#include <QJsonDocument>
#include <QDateTime>
#include <QDebug>

#ifdef Q_OS_LINUX
#include <unistd.h>
#include <fstream>
#include <sstream>
#endif

#ifdef Q_OS_MAC
#include <sys/types.h>
#include <sys/sysctl.h>
#include <mach/mach.h>
#include <mach/processor_info.h>
#include <mach/mach_host.h>
#endif

#ifdef Q_OS_WIN
#include <windows.h>
#include <psapi.h>
#endif

const QString SystemMonitor::HARDWARE_MONITOR_UPDATE_TIME = "HARDWARE_MONITOR_UPDATE_TIME";
const QString SystemMonitor::MAX_MEMORY_SIZE = "MAX_MEMORY_SIZE";
const QString SystemMonitor::MAX_DISK_SIZE = "MAX_DISK_SIZE";

SystemMonitor::SystemMonitor(QObject* parent)
    : QObject(parent),
      m_sender(nullptr),
      m_timer(new QTimer(this)),
      m_updateInterval(10000) {
    
    connect(m_timer, &QTimer::timeout, this, &SystemMonitor::collectAndReport);
}

SystemMonitor::~SystemMonitor() {
    stop();
}

void SystemMonitor::initialize(HttpSender* sender) {
    m_sender = sender;
    
    DatabaseManager* db = DatabaseManager::instance();
    
    QStorageInfo storage = QStorageInfo::root();
    qint64 totalDiskSpace = storage.bytesTotal();
    db->saveKeyValue(MAX_DISK_SIZE, totalDiskSpace);
    
#ifdef Q_OS_MAC
    int mib[2];
    int64_t physical_memory;
    size_t length;
    
    mib[0] = CTL_HW;
    mib[1] = HW_MEMSIZE;
    length = sizeof(int64_t);
    sysctl(mib, 2, &physical_memory, &length, NULL, 0);
    
    db->saveKeyValue(MAX_MEMORY_SIZE, (qint64)physical_memory);
#elif defined(Q_OS_LINUX)
    std::ifstream meminfo("/proc/meminfo");
    std::string line;
    qint64 totalMemory = 0;
    
    while (std::getline(meminfo, line)) {
        if (line.find("MemTotal:") == 0) {
            std::istringstream iss(line);
            std::string label;
            iss >> label >> totalMemory;
            totalMemory *= 1024;
            break;
        }
    }
    db->saveKeyValue(MAX_MEMORY_SIZE, totalMemory);
#elif defined(Q_OS_WIN)
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);
    db->saveKeyValue(MAX_MEMORY_SIZE, (qint64)memInfo.ullTotalPhys);
#endif
    
    QVariant updateTimeVar = db->getKeyValue(HARDWARE_MONITOR_UPDATE_TIME);
    if (!updateTimeVar.isValid() || updateTimeVar.toLongLong() < 5000) {
        m_updateInterval = 10000;
        db->saveKeyValue(HARDWARE_MONITOR_UPDATE_TIME, m_updateInterval);
    } else {
        m_updateInterval = updateTimeVar.toLongLong();
    }
    
    qInfo() << "系统监控初始化完成，更新间隔:" << m_updateInterval << "ms";
}

void SystemMonitor::start() {
    if (m_timer->isActive()) {
        return;
    }
    
    qInfo() << "启动系统监控";
    m_timer->start(m_updateInterval);
    collectAndReport();
}

void SystemMonitor::stop() {
    if (m_timer->isActive()) {
        qInfo() << "停止系统监控";
        m_timer->stop();
    }
}

void SystemMonitor::collectAndReport() {
    getSystemInfo();
    
    if (!m_sender || !m_sender->isInitialized()) {
        return;
    }
    
    QJsonObject data = m_currentData.toJson();
    QString jsonStr = QJsonDocument(data).toJson(QJsonDocument::Compact);
    
    m_sender->sendCommand("monitorInfo", jsonStr);
    
}

void SystemMonitor::getSystemInfo() {
    m_currentData.collectTimestamp = QDateTime::currentMSecsSinceEpoch();
    m_currentData.cpu = getCpuUsage();
    m_currentData.mem = getMemoryUsage();
    m_currentData.disk = getDiskUsage();
}

double SystemMonitor::getCpuUsage() {
#ifdef Q_OS_MAC
    host_cpu_load_info_data_t cpuinfo;
    mach_msg_type_number_t count = HOST_CPU_LOAD_INFO_COUNT;
    
    if (host_statistics(mach_host_self(), HOST_CPU_LOAD_INFO, (host_info_t)&cpuinfo, &count) == KERN_SUCCESS) {
        unsigned long long totalTicks = 0;
        for (int i = 0; i < CPU_STATE_MAX; i++) {
            totalTicks += cpuinfo.cpu_ticks[i];
        }
        
        unsigned long long idleTicks = cpuinfo.cpu_ticks[CPU_STATE_IDLE];
        
        static unsigned long long _previousTotalTicks = 0;
        static unsigned long long _previousIdleTicks = 0;
        
        unsigned long long totalTicksSinceLastTime = totalTicks - _previousTotalTicks;
        unsigned long long idleTicksSinceLastTime = idleTicks - _previousIdleTicks;
        
        double usage = 1.0 - ((totalTicksSinceLastTime > 0) ? ((double)idleTicksSinceLastTime) / totalTicksSinceLastTime : 0);
        
        _previousTotalTicks = totalTicks;
        _previousIdleTicks = idleTicks;
        
        return usage;
    }
#elif defined(Q_OS_LINUX)
    static unsigned long long lastTotalUser = 0, lastTotalUserLow = 0, lastTotalSys = 0, lastTotalIdle = 0;
    
    std::ifstream file("/proc/stat");
    std::string line;
    std::getline(file, line);
    
    unsigned long long totalUser, totalUserLow, totalSys, totalIdle;
    std::istringstream iss(line);
    std::string cpu;
    iss >> cpu >> totalUser >> totalUserLow >> totalSys >> totalIdle;
    
    if (lastTotalUser != 0) {
        unsigned long long total = (totalUser - lastTotalUser) + (totalUserLow - lastTotalUserLow) + (totalSys - lastTotalSys);
        unsigned long long totalAll = total + (totalIdle - lastTotalIdle);
        
        double usage = (totalAll > 0) ? (double)total / totalAll : 0.0;
        
        lastTotalUser = totalUser;
        lastTotalUserLow = totalUserLow;
        lastTotalSys = totalSys;
        lastTotalIdle = totalIdle;
        
        return usage;
    }
    
    lastTotalUser = totalUser;
    lastTotalUserLow = totalUserLow;
    lastTotalSys = totalSys;
    lastTotalIdle = totalIdle;
#elif defined(Q_OS_WIN)
    static ULARGE_INTEGER lastCPU, lastSysCPU, lastUserCPU;
    static int numProcessors = 0;
    static HANDLE self = GetCurrentProcess();
    
    if (numProcessors == 0) {
        SYSTEM_INFO sysInfo;
        GetSystemInfo(&sysInfo);
        numProcessors = sysInfo.dwNumberOfProcessors;
        
        FILETIME ftime, fsys, fuser;
        GetSystemTimeAsFileTime(&ftime);
        memcpy(&lastCPU, &ftime, sizeof(FILETIME));
        
        GetProcessTimes(self, &ftime, &ftime, &fsys, &fuser);
        memcpy(&lastSysCPU, &fsys, sizeof(FILETIME));
        memcpy(&lastUserCPU, &fuser, sizeof(FILETIME));
    }
    
    FILETIME ftime, fsys, fuser;
    ULARGE_INTEGER now, sys, user;
    
    GetSystemTimeAsFileTime(&ftime);
    memcpy(&now, &ftime, sizeof(FILETIME));
    
    GetProcessTimes(self, &ftime, &ftime, &fsys, &fuser);
    memcpy(&sys, &fsys, sizeof(FILETIME));
    memcpy(&user, &fuser, sizeof(FILETIME));
    
    double percent = (sys.QuadPart - lastSysCPU.QuadPart) + (user.QuadPart - lastUserCPU.QuadPart);
    percent /= (now.QuadPart - lastCPU.QuadPart);
    percent /= numProcessors;
    
    lastCPU = now;
    lastUserCPU = user;
    lastSysCPU = sys;
    
    return percent;
#endif
    
    return 0.0;
}

double SystemMonitor::getMemoryUsage() {
#ifdef Q_OS_MAC
    vm_size_t page_size;
    mach_port_t mach_port;
    mach_msg_type_number_t count;
    vm_statistics64_data_t vm_stats;
    
    mach_port = mach_host_self();
    count = sizeof(vm_stats) / sizeof(natural_t);
    
    if (KERN_SUCCESS == host_page_size(mach_port, &page_size) &&
        KERN_SUCCESS == host_statistics64(mach_port, HOST_VM_INFO, (host_info64_t)&vm_stats, &count)) {
        
        long long free_memory = (int64_t)vm_stats.free_count * (int64_t)page_size;
        long long used_memory = ((int64_t)vm_stats.active_count +
                                 (int64_t)vm_stats.inactive_count +
                                 (int64_t)vm_stats.wire_count) * (int64_t)page_size;
        
        DatabaseManager* db = DatabaseManager::instance();
        qint64 totalMemory = db->getKeyValue(MAX_MEMORY_SIZE, 0).toLongLong();
        
        if (totalMemory > 0) {
            return (double)used_memory / totalMemory;
        }
    }
#elif defined(Q_OS_LINUX)
    std::ifstream meminfo("/proc/meminfo");
    std::string line;
    qint64 totalMemory = 0, freeMemory = 0, buffers = 0, cached = 0;
    
    while (std::getline(meminfo, line)) {
        if (line.find("MemTotal:") == 0) {
            std::istringstream(line) >> line >> totalMemory;
        } else if (line.find("MemFree:") == 0) {
            std::istringstream(line) >> line >> freeMemory;
        } else if (line.find("Buffers:") == 0) {
            std::istringstream(line) >> line >> buffers;
        } else if (line.find("Cached:") == 0) {
            std::istringstream(line) >> line >> cached;
        }
    }
    
    qint64 usedMemory = totalMemory - freeMemory - buffers - cached;
    return (totalMemory > 0) ? (double)usedMemory / totalMemory : 0.0;
#elif defined(Q_OS_WIN)
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);
    
    DWORDLONG totalMemory = memInfo.ullTotalPhys;
    DWORDLONG freeMemory = memInfo.ullAvailPhys;
    
    return (totalMemory > 0) ? (double)(totalMemory - freeMemory) / totalMemory : 0.0;
#endif
    
    return 0.0;
}

double SystemMonitor::getDiskUsage() {
    QStorageInfo storage = QStorageInfo::root();
    
    if (storage.isValid() && storage.isReady()) {
        qint64 total = storage.bytesTotal();
        qint64 available = storage.bytesAvailable();
        
        if (total > 0) {
            return (double)(total - available) / total;
        }
    }
    
    return 0.0;
}

SystemMonitorData SystemMonitor::getCurrentData() const {
    return m_currentData;
}
