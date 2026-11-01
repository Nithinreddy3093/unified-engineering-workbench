#include "SystemInfo.h"
#include <sstream>
#include <thread>
#include <vector>
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#include <winternl.h>
#endif
namespace uew {
std::string systemInformation(){std::ostringstream s;
#ifdef _WIN32
    using RtlGetVersionFn=LONG(WINAPI*)(PRTL_OSVERSIONINFOW);
    auto ntdll=GetModuleHandleW(L"ntdll.dll");
    auto rtl=reinterpret_cast<RtlGetVersionFn>(GetProcAddress(ntdll,"RtlGetVersion"));
    RTL_OSVERSIONINFOW ver{};ver.dwOSVersionInfoSize=sizeof(ver);
    if(rtl&&rtl(&ver)==0)s<<"Operating system: Windows "<<ver.dwMajorVersion<<'.'<<ver.dwMinorVersion<<" build "<<ver.dwBuildNumber<<"\n";
    else s<<"Operating system: Windows (version unavailable)\n";
    wchar_t cpu[256]{};DWORD cpuBytes=sizeof(cpu);HKEY key{};
    if(RegOpenKeyExW(HKEY_LOCAL_MACHINE,L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",0,KEY_QUERY_VALUE,&key)==ERROR_SUCCESS){
        if(RegQueryValueExW(key,L"ProcessorNameString",nullptr,nullptr,reinterpret_cast<BYTE*>(cpu),&cpuBytes)==ERROR_SUCCESS){
            int n=WideCharToMultiByte(CP_UTF8,0,cpu,-1,nullptr,0,nullptr,nullptr);std::string name(n?static_cast<std::size_t>(n):0,'\0');
            if(n>0){WideCharToMultiByte(CP_UTF8,0,cpu,-1,name.data(),n,nullptr,nullptr);name.resize(static_cast<std::size_t>(n-1));s<<"CPU: "<<name<<"\n";}
        }RegCloseKey(key);
    }
    DWORD bytesNeeded=0;GetLogicalProcessorInformationEx(RelationProcessorCore,nullptr,&bytesNeeded);std::vector<unsigned char> storage(bytesNeeded);unsigned physical=0;
    if(bytesNeeded&&GetLogicalProcessorInformationEx(RelationProcessorCore,reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(storage.data()),&bytesNeeded)){
        auto p=reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(storage.data());auto end=storage.data()+bytesNeeded;
        while(reinterpret_cast<unsigned char*>(p)<end){++physical;p=reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(reinterpret_cast<unsigned char*>(p)+p->Size);}
    }
    SYSTEM_INFO si{};GetSystemInfo(&si);s<<"Physical cores: "<<(physical?std::to_string(physical):"unavailable")<<"\nLogical processors: "<<si.dwNumberOfProcessors<<"\n";
    MEMORYSTATUSEX mem{};mem.dwLength=sizeof(mem);if(GlobalMemoryStatusEx(&mem))s<<"RAM: "<<(mem.ullTotalPhys/(1024ull*1024ull))<<" MiB\n";
#else
    s<<"Operating system: non-Windows platform\n";
    s<<"Logical processors: "<<std::thread::hardware_concurrency()<<"\n";
    s<<"CPU model, physical cores, and RAM: unavailable from portable core API.\n";
#endif
    s<<"Compiler: ";
#if defined(_MSC_VER)
    s<<"MSVC "<<_MSC_VER;
#elif defined(__clang__)
    s<<"Clang "<<__clang_version__;
#elif defined(__GNUC__)
    s<<"GCC "<<__VERSION__;
#else
    s<<"unknown";
#endif
#ifdef NDEBUG
    s<<"\nBuild type: Release\n";
#else
    s<<"\nBuild type: Debug\n";
#endif
    return s.str();
}
}
