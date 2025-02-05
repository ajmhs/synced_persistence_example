# Persistence Service Demonstrator

This document serves as a guide for setting up and demonstrating how multiple services can write identical data while utilizing multiple synchronized RTI Persistence Service instances for handling synchronization within load-balanced cloud services.

## Configuration Considerations

In addition to employing the Persistence Service, it is crucial to ensure that the DESTINATION_ORDER QoS policy is properly configured:

- **DDS_DestinationOrderQosPolicyKind**: `DDS_BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS`
- **DDS_DestinationOrderQosPolicyScopeKind**: `DDS_INSTANCE_SCOPE_DESTINATIONORDER_QOS`

When the scope is set to `DDS_INSTANCE_SCOPE_DESTINATIONORDER_QOS` (default), the sample's source timestamp determines the most recent information within each instance. This configuration ensures that, in cases where multiple DataWriters with the same strength update the same instance concurrently, all DataReaders receive a consistent final value. If a DataReader encounters a sample with an older source timestamp than the last received, the sample is discarded. The total number of dropped samples due to outdated timestamps can be monitored using the `old_source_timestamp_dropped_sample_count` field in `DATA_READER_CACHE_STATUS`. However, neither the `SAMPLE_REJECTED` nor `SAMPLE_LOST` status will be updated in such cases.

## Installation

The RTI Persistence Service Docker image is available on Docker Hub at:

[RTI Persistence Service Docker Image](https://hub.docker.com/r/rticom/persistence-service)

## Usage

To demonstrate the functionality, at least two persistence service instances must be started and configured to synchronize with each other. Once the publisher and subscriber are running, the flow of data through the persistence services can be observed by stopping and restarting the services and monitoring their effect on the subscriber.

With the persistence services active, the publisher and subscriber discover each other, and all instances are relayed through the persistence services. If one persistence service is stopped, the subscriber will continue to receive data. If both persistence services are stopped, the subscriber will no longer receive data. Once the services are restarted, the subscriber will catch up to the latest instance.

This demonstration operates on **domain 12**.

## Network Configuration

The Docker containers use bridged networking. To ensure consistent addressing across different systems, a dedicated Docker network must be created:

```bash
docker network create --subnet=10.1.0.0/24 tmpnet
```

### Assigning Network Addresses

Each persistence service container should be assigned a unique network address:
- **Persistence_Service_1**: `10.1.0.10`
- **Persistence_Service_2**: `10.1.0.20`

In the persistence service configuration file, synchronization is enabled, and each persistence service is configured with the other's address as an initial peer.

## Starting Persistence Service Containers

### Starting Instance A

```bash
docker run -it --rm --network tmpnet --ip 10.1.0.10 --hostname ps_a \
-v $RTI_LICENSE_FILE:/opt/rti.com/rti_connext_dds-7.3.0/rti_license.dat:ro \
-v $PWD/persistenceservice.xml:/home/rtiuser/rti_workspace/7.3.0/user_config/persistence_service/USER_PERSISTENCE_SERVICE.xml:ro \
--name=persistence_service_a \
rticom/persistence-service:latest \
-cfgName persistence_service_a -verbosity 4
```

### Starting Instance B

```bash
docker run -it --rm --network tmpnet --ip 10.1.0.20 --hostname ps_b \
-v $RTI_LICENSE_FILE:/opt/rti.com/rti_connext_dds-7.3.0/rti_license.dat:ro \
-v $PWD/persistenceservice.xml:/home/rtiuser/rti_workspace/7.3.0/user_config/persistence_service/USER_PERSISTENCE_SERVICE.xml:ro \
--name=persistence_service_b \
rticom/persistence-service:latest \
-cfgName persistence_service_b -verbosity 4
```

### Docker Run Command Breakdown

- `docker run` - Starts a new container.
- `-it` - Runs in interactive mode.
- `--rm` - Automatically removes the container upon exit.
- `--network tmpnet` - Uses the predefined Docker network.
- `--ip` - Assigns a static IP address.
- `--hostname` - Defines the container's hostname.
- `-v` - Mounts required configuration files in read-only mode.
- `-cfgName` - Specifies the persistence service configuration name.
- `-verbosity 4` - Enables detailed logging.

## Quality of Service (QoS) Configuration

Each persistence service is configured to:
- Use persistent storage.
- Operate with UDPv4 only (no shared memory).
- Synchronize with any discovered instances.
- Define initial peers to ensure quick discovery.
- Utilize source timestamp destination order for QoS.

The persistence service configurations are contained within the `persistenceservice.xml` file, each defined inside the `<persistence_service>` element.

### Storage Configuration

The `<persistent_storage>` XML element defines persistent storage settings, including:
- `<directory>` - Specifies the storage location (e.g., `tmp`).
- `<file_prefix>` - Defines a prefix (`ps_`) for stored files.

### Destination Order QoS Policy

The destination order QoS policy controls the sequence in which samples are delivered to DataReaders. The `DDS_BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS` setting ensures that messages are ordered based on their source timestamps, while the `INSTANCE_SCOPE_DESTINATIONORDER_QOS` ensures order is maintained per instance.

### Initial Peers

Each persistence service is configured with a list of initial peers, which include each other. Additionally:
- The publisher defines both persistence service IP addresses as its initial peers.
- The subscriber also defines both persistence services and sets `<accept_unknown_peers>` to `false`, ensuring it only communicates through the persistence services.

### Direct Communication Restriction

The `<durability>` element contains the `<direct_communication>` tag, which is set to `false`. This setting ensures that subscribers will only receive data relayed through the persistence services, preventing direct publisher-to-subscriber communication.

## Test Applications

Both the publisher and subscriber applications use the `Pattern.Status` QoS profile. This profile ensures reliable, durable communication with appropriate history settings and liveliness monitoring. 

### Key QoS Aspects:

1. **Reliability** - Ensures all status updates are reliably delivered.
2. **Durability** - Uses transient local durability for late-joining subscribers.
3. **History** - Keeps a limited number of samples to manage memory efficiently.
4. **Liveliness** - Ensures active monitoring of entities.

The publisher generates a **red square** on **domain 12**. The standard Shapes Demo can be used as a subscriber to visualize data flow. The square is published at a slow rate, allowing for verification of X and Y values between the publisher and subscriber.

---
This document provides a structured approach for setting up and validating the RTI Persistence Service in a synchronized multi-instance environment. For additional details, refer to the RTI documentation or support channels.
