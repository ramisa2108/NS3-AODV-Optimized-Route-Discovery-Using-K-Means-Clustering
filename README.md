# NS3 AODV Optimized Route Discovery Using K-Means Clustering

In AODV routing, route discovery is done by the flooding method - which is broadcasting route request (RREQ) packtes to all the nodes in the transmission range of a sender. It often results in unnecessary re-transmissions of RREQ packets and the reply (RREP) packets generated in response, resulting in packet collisions and congestion in the network. In this project, I have proposed an optimized route discovering method for AODV. The key idea is to use K-Means clustering algorithm for selecting the best cluster of RREQ packet forwarders instead of broadcasting. The objective of this method is to reduce unnecessary control packet transmission in the network, thereby reducing congestion and end to end delay of the network.

The network is simulated using ns3.35.


### Features used in K-Means clustering:  

- distance to destination 
- number of transmission errors
- free buffer space

Optimal cluster is chosen based on these features of the neighbours.

### Evaluation of clusters:

Evaluation of the clusters are done by comparing to an ideal forwarder with features:

- distance to destination = 0
- number of transmission errors = 0
- free buffer space = maximum buffer size

### Modifications in the code base: 

- **`aodvKmeans-packet.h`**  
    
    new fields added to `RREPHeader` class : `m_txErrorCount`, `m_freeSpace`, `m_positionX`, `m_positionY`
- **`aodvKmeans-rtable.h`**  
    
    new fields added to `RoutingTableEntry` class : `m_txErrorCount`, `m_freeSpace`, `m_positionX`, `m_positionY`
- **`aodvKmeans-routing-protocol.h`**  
    
    - `m_position` :  
     vector containing this node's position
    - `m_lastKnowPosition` :  
     map containing last known physical position of IP addresses
    - `m_lastKnownCluster` :  
     map containing last known clusters for forwarding to IP addresses (cleared periodically)

- **`aodvKmeans-rtable.cc`**  
    - `Kmeans` : 
    runs K-Means clustering algorithm on neighbouring nodes to find optimal cluster of forwarders for given destination


- **`aodvKmeans-routing-protocol.cc`**  
    
    - `SendHello` :  
        current node's position, free buffer space, transmission error count sent through `RREPHeader`
    
    - `RecvReply` :  
        last known positions updated
    
    - Features in `RoutingTable` updated with incoming `RREPHeader` information 
    
    - `NotifyTxError` :  
        current node's tranmission error count incremented
    
    - `SendRequest` and `RecvRequest` :  
     if `m_lastKnownPosition` contains the IP address for destination, then instead of broadcasting, `RREQ` is forwarded to optimal cluster obtained from `m_lastKnownClusters` or by running K-Means 


### Comparison with AODV

![](/Results/nodes-del_ratio.png)
![](/Results/pps-del_ratio.png)
![](/Results/nodes-delay.png)
![](/Results/pps-delay.png)


- Number of nodes : 10, 20, 30, 40, 50  
- Number of packets per second : 10, 20, 30, 40, 50

From the figures, we can see that the **delivery ratio of the modified algorithm is slightly less than AODV**. But the difference in delivery ratio is not very significant considering the modified approach uses substantially less number of forwarders for route discovery. On the other hand, we can see that the **end to end delay in the modified approach is a lot lower than using only AODV**. This is due to using lower number of RREQ and RREP packets in the modified approach which makes the route discovery process much faster. So, even though the delivery ratio sufferes a little due to selective forwarding using K-Means clustering, the modified approach reduces the end to end delay of the network.


### Steps for running the examples
1. Download and install [ns3.35](https://www.nsnam.org/releases/ns-3-35/)
2. Copy the folder ![aodvKmeans](/aodvKmeans) to **ns3/src** or **ns3/contrib**
3. Build the new module **aodvKmeans** [Instructions](https://www.nsnam.org/docs/manual/html/new-modules.html)
4. Add the examples from ![scratch](/scratch) to **ns3/scratch**

