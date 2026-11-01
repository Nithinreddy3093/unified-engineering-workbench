#include "MockDevices.h"
#include <stdexcept>
namespace uew {
DeviceResult MockDevice::connect(){if(connected_)return {true,"Already connected."}; if(failFirst_&&!failed_){failed_=true;return {false,"SIMULATED TIMEOUT EVENT: first connection attempt represents a 3000 ms timeout; retry is safe."};}connected_=true;return {true,"Connected to simulated device."};}
DeviceResult MockDevice::disconnect(){if(!connected_)return {false,"Device is not connected."};connected_=false;return {true,"Disconnected."};}
TelemetryMessage MicrocontrollerMockAdapter::readTelemetry()const{if(!connected_)throw std::logic_error("Cannot read MCU telemetry: device is not connected.");return {"uptime",42.0,"s",true};}
TelemetryMessage FpgaMockAdapter::readTelemetry()const{if(!connected_)throw std::logic_error("Cannot read FPGA telemetry: device is not connected.");return {"clock_status",1.0,"running (simulated)",true};}
TelemetryMessage JtagMockAdapter::readTelemetry()const{if(!connected_)throw std::logic_error("Cannot read JTAG telemetry: adapter is not connected.");return {"debug_link",1.0,"connected (simulated)",true};}
std::vector<std::shared_ptr<IHardwareDevice>> discoverMockDevices(bool fail){return {std::make_shared<MicrocontrollerMockAdapter>(),std::make_shared<FpgaMockAdapter>(fail),std::make_shared<JtagMockAdapter>()};}
}
