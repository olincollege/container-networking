#!/bin/bash
# This script sets up a bridge interface on a Linux system.

CLIENT_PID=$1
SERVER_PID=$2
if [ -z "$CLIENT_PID" ] || [ -z "$SERVER_PID" ]; then
    echo "Usage: $0 <client_pid> <server_pid>"
    exit 1
fi

sudo ip link del veth-client-br >/dev/null 2>&1
sudo ip link del veth-server-br >/dev/null 2>&1
sudo ip link del br0 >/dev/null 2>&1
sudo ip link del veth-client >/dev/null 2>&1
sudo ip link del veth-server >/dev/null 2>&1

sudo ip link add name br0 type bridge
sudo ip addr add 192.168.100.1/24 dev br0
sudo ip link set br0 up

# Setup first container
sudo ip link add veth-client type veth peer name veth-client-br
sudo ip link set veth-client-br master br0
sudo ip link set veth-client netns "$CLIENT_PID"
sudo nsenter --net=/proc/"$CLIENT_PID"/ns/net ip addr add 192.168.100.2/24 dev veth-client
sudo nsenter --net=/proc/"$CLIENT_PID"/ns/net ip link set veth-client up
sudo ip link set veth-client-br up

# Setup second container
sudo ip link add veth-server type veth peer name veth-server-br
sudo ip link set veth-server-br master br0
sudo ip link set veth-server netns "$SERVER_PID"
sudo nsenter --net=/proc/"$SERVER_PID"/ns/net ip addr add 192.168.100.3/24 dev veth-server
sudo nsenter --net=/proc/"$SERVER_PID"/ns/net ip link set veth-server up
sudo ip link set veth-server-br up
