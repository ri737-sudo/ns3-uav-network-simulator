# UAV Network Simulator (ns-3)

This project simulates communication between multiple UAVs using ns-3.

## Features
- User-defined number of UAVs
- WiFi communication
- TCP / UDP traffic
- UAV-to-UAV messaging in terminal
- Adjustable distance and simulation time

## Run the simulation

```bash
cd ~/ns-3-dev
./ns3 run "scratch/uav-network-sim --nUavs=5 --distance=100 --interval=2 --simTime=20"

Save:

