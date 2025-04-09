# Brooke Notes

## Container Networking Basics

[Container Networking Basics](https://www.flockport.com/guides/container-networking-basics)

- Linux kernel can create a private NAT network
- Can use `brcrl addbr br0` with the `bridge-utils` package
- Can use `ip link add br0 type bridge` to create a bridge network too

## Types of Docker Networks

1. **bridge:** If you build a container without specifying the kind of driver, the container will only be created in the bridge network, which is the default network. 
2. **host:** Containers will not have any IP address they will be directly created in the system network which will remove isolation between the docker host and containers. 
3. **none:** IP addresses won’t be assigned to containers. These containments are not accessible to us from the outside or from any other container.
4. **overlay:** overlay network will enable the connection between multiple Docker demons and make different Docker swarm services communicate with each other.
5. **ipvlan:** Users have complete control over both IPv4 and IPv6 addressing by using the IPvlan driver.
6. **macvlan:** macvlan driver makes it possible to assign MAC addresses to a container.
