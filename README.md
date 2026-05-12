# UAV Network Simulator (ns-3)

This project simulates UAV-to-UAV communication using ns-3.

## Features
- Dynamic topology selection based on drone count
- UDP (normal mode) / TCP (emergency mode)
- WiFi ad-hoc UAV network
- Real-time console logs:
  - UAV X → UAV Y : MESSAGE SENT
  - UAV Y RECEIVED MESSAGE

## How to Run

Inside ns-3 directory:

```bash
./ns3 run "scratch/uav-network-sim \
--nUav=5 \
--altitude=120 \
--emergency=no \
--terrain=less"
