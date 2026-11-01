#pragma once
#include <memory>
#include <string>
#include <vector>
namespace uew {
struct DeviceCapabilities { std::string interfaces; std::string description; };
struct TelemetryMessage { std::string name; double value{}; std::string unit; bool simulated{true}; };
struct DeviceResult { bool ok{}; std::string message; };
class IHardwareDevice { public: virtual ~IHardwareDevice()=default; virtual std::string deviceType() const=0; virtual DeviceCapabilities capabilities() const=0; virtual DeviceResult connect()=0; virtual DeviceResult disconnect()=0; virtual TelemetryMessage readTelemetry() const=0; };
class MockDevice : public IHardwareDevice { public: explicit MockDevice(bool failFirst=false):failFirst_(failFirst){} DeviceResult connect() override; DeviceResult disconnect() override; bool connected()const{return connected_;} protected: bool connected_{false}; bool failFirst_{false}; bool failed_{false}; };
class MicrocontrollerMockAdapter final:public MockDevice {public: using MockDevice::MockDevice; std::string deviceType()const override{return "Microcontroller (SIMULATED / MOCK)";} DeviceCapabilities capabilities()const override{return {"USB serial (assumption)","Telemetry and host command channel; voltage/pinout device-specific."};} TelemetryMessage readTelemetry()const override;};
class FpgaMockAdapter final:public MockDevice {public: using MockDevice::MockDevice; std::string deviceType()const override{return "FPGA board (SIMULATED / MOCK)";} DeviceCapabilities capabilities()const override{return {"USB control/debug (assumption)","Logic status telemetry; board-specific I/O and power."};} TelemetryMessage readTelemetry()const override;};
class JtagMockAdapter final:public MockDevice {public: using MockDevice::MockDevice; std::string deviceType()const override{return "JTAG debug adapter (SIMULATED / MOCK)";} DeviceCapabilities capabilities()const override{return {"USB to JTAG (assumption)","Programming/debug interface, not a compute board."};} TelemetryMessage readTelemetry()const override;};
std::vector<std::shared_ptr<IHardwareDevice>> discoverMockDevices(bool failFirstFpga=false);
}
