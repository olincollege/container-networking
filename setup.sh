#!/bin/bash
# This script sets up a bridge interface on a Linux system.

SERVER_PID=$1
CLIENT_PID=$2
if [ -z "$CLIENT_PID" ] || [ -z "$SERVER_PID" ]; then
    echo "Usage: $0 <server_pid> <client_pid>"
    exit 1
fi

# Remove existing network interfaces and bridge if they exist
# This is to ensure that the script can be run multiple times without errors
echo "Removing existing network interfaces and bridge..."
sudo ip link del veth-client-br >/dev/null 2>&1
sudo ip link del veth-server-br >/dev/null 2>&1
sudo ip link del br0 >/dev/null 2>&1
sudo ip link del veth-client >/dev/null 2>&1
sudo ip link del veth-server >/dev/null 2>&1
echo "Existing network interfaces and bridge removed."

# Create a bridge interface
echo "Creating bridge interface..."
sudo ip link add name br0 type bridge
sudo ip addr add 192.168.100.1/24 dev br0
sudo ip link set br0 up
echo "Bridge interface created and set up."

# Setup first container (server)
echo "Setting up first (server) container..."
sudo ip link add veth-server type veth peer name veth-server-br
sudo ip link set veth-server-br master br0
sudo ip link set veth-server netns "$SERVER_PID"
sudo nsenter --net=/proc/"$SERVER_PID"/ns/net ip addr add 192.168.100.3/24 dev veth-server
sudo nsenter --net=/proc/"$SERVER_PID"/ns/net ip link set veth-server up
sudo ip link set veth-server-br up
echo "Server container set up with PID $SERVER_PID."

# Setup second container (client)
echo "Setting up second (client) container..."
sudo ip link add veth-client type veth peer name veth-client-br
sudo ip link set veth-client-br master br0
sudo ip link set veth-client netns "$CLIENT_PID"
sudo nsenter --net=/proc/"$CLIENT_PID"/ns/net ip addr add 192.168.100.2/24 dev veth-client
sudo nsenter --net=/proc/"$CLIENT_PID"/ns/net ip link set veth-client up
sudo ip link set veth-client-br up
echo "Client container set up with PID $CLIENT_PID."

echo "Network setup complete. Containers are now connected to the bridge."
