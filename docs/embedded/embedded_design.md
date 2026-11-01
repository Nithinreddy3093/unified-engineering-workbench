# Embedded integration design (SIMULATED / MOCK)

```text
                  HOST COMPUTER
                  /           \
         USB serial/control   USB debug link
             /                    \
   MCU development board      FPGA development board
                                      |
                               JTAG signals (TCK/TMS/TDI/TDO)
                                      |
                               JTAG debug adapter
```

The block diagram is conceptual. The relationship between an FPGA board and a debug adapter varies by board. The adapters in `src/core/hardware` are software-only; discovery returns fixed mock devices and one FPGA mock can simulate a first-attempt timeout. No physical connection, pinout, or measurement is represented.

| Device | Purpose | Power / ground / voltage | Connector and host link | Signals / ownership |
|---|---|---|---|---|
| MCU board (representative class) | Firmware control and telemetry | Board-specific supply; common ground for connected logic; I/O levels must match | USB connector and USB serial assumed | Board owns its GPIO and firmware protocol |
| FPGA board (representative class) | Programmable logic status/compute endpoint | Board-specific supply; I/O voltage bank-specific; level shifting may be required | USB configuration/control assumed | FPGA board owns user I/O and configuration path |
| JTAG adapter | Debug/programming interface | Target reference voltage and ground requirements are adapter-specific | USB to adapter; target-side JTAG header assumed | TCK/TMS/TDI/TDO direction and pinout follow target documentation |

JTAG is a debug/programming interface, not a generic interchangeable compute board. Do not wire boards using this conceptual table: consult the exact vendor board and adapter manuals before any physical integration. All current telemetry is explicitly marked simulated.
