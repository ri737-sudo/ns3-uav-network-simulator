Step 1 — Initialize Simulation Parameters
Start the program.
Set default values:
Number of UAVs = 7
Flying altitude = 100 m
Terrain type = less obstacles
Message = "Swarm Ready"
Allow the user to modify these parameters through command-line inputs.

Step 2 — Create UAV Nodes
Create N virtual UAV nodes in the simulator.
These nodes represent drones in the swarm.

Step 3 — Set UAV Positions
Assign a fixed position mobility model.
Place UAVs in a straight line.
Maintain constant altitude for all UAVs.
Keep 100 m spacing between consecutive UAVs.

This forms a basic UAV communication chain.

Step 4 — Configure Wireless Channel
Create a Wi-Fi communication channel.
If terrain = more obstacles:
Enable signal fading model (Nakagami).
Enable distance-based signal loss model.
Otherwise, use default free-space propagation.

This step simulates real-world radio behaviour.

Step 5 — Install Wi-Fi Devices
Configure Wi-Fi standard (802.11ac).
Set the communication mode to Ad-hoc.
Install Wi-Fi radios on all UAVs.

Ad-hoc mode allows UAVs to talk directly without a base station.

Step 6 — Install Internet Protocol Stack
Install TCP/IP stack on every UAV.
Assign each UAV a unique IP address.

Now every UAV behaves like a network device.

Step 7 — Start Flow Monitoring
Enable the FlowMonitor module.
Begin tracking network performance metrics:
Packets sent
Packets received
Delay
Throughput
Packet loss

Step 8 — Create Receiving Sockets
Open a UDP receiving port on every UAV.
Attach a receive callback function.
Whenever a packet arrives, print the event.

Each UAV now has an open “mailbox”.

Step 9 — Schedule Packet Transmission
For each UAV from 1 → N-1:
Create a sending socket.
Schedule packet transmission.
Each UAV sends a message to the next UAV.
Transmission occurs sequentially with a time delay.

This creates a multi-hop communication chain.

Step 10 — Run Simulation
Start simulation clock.
Simulate for 15 seconds.
Stop the simulation automatically.

Step 11 — Collect Performance Results
For each communication flow:
Calculate:
Total transmitted packets
Total received packets
Packet loss
Average delay
Network throughput
Print results to the terminal.
Step 12 — End Simulation
Free simulator memory.
Terminate program.
