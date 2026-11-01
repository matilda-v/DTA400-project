
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/traffic-control-module.h"
#include "ns3/flow-monitor-module.h"

#include <iostream>
#include <iomanip>
#include <string>

using namespace ns3;

int main(int argc, char *argv[])
{
    uint32_t queueSize = 50;

    CommandLine cmd(__FILE__);
    cmd.AddValue("queueSize", "Bottleneck queue capacity in packets",
                 queueSize);
    cmd.Parse(argc, argv);

    double appStart = 1.0;
    double appStop = 20.0;
    double simulationStop = 30.0;
    uint16_t port = 50000;

    // 1. Create sender, router, and receiver.
    NodeContainer nodes;
    nodes.Create(3);

    Ptr<Node> sender = nodes.Get(0);
    Ptr<Node> router = nodes.Get(1);
    Ptr<Node> receiver = nodes.Get(2);

    // 2. Create the sender-to-router link.
    PointToPointHelper accessLink;
    accessLink.SetDeviceAttribute(
        "DataRate", StringValue("10Mbps"));
    accessLink.SetChannelAttribute(
        "Delay", StringValue("2ms"));

    NetDeviceContainer accessDevices =
        accessLink.Install(sender, router);

    // 3. Create the slower router-to-receiver bottleneck.
    PointToPointHelper bottleneckLink;
    bottleneckLink.SetDeviceAttribute(
        "DataRate", StringValue("1Mbps"));
    bottleneckLink.SetChannelAttribute(
        "Delay", StringValue("10ms"));

    bottleneckLink.SetQueue(
    "ns3::DropTailQueue<Packet>",
    "MaxSize", StringValue("1p"));

    NetDeviceContainer bottleneckDevices =
        bottleneckLink.Install(router, receiver);

    // 4. Install the Internet protocol stack.
    InternetStackHelper stack;
    stack.Install(nodes);

    // 5. Configure the router's outgoing queue.
    TrafficControlHelper trafficControl;
    std::string maxQueueSize = std::to_string(queueSize) + "p";

    trafficControl.SetRootQueueDisc(
        "ns3::FifoQueueDisc",
        "MaxSize", StringValue(maxQueueSize));

    NetDeviceContainer routerOutgoingDevice;
    routerOutgoingDevice.Add(bottleneckDevices.Get(0));
    QueueDiscContainer queueDiscs =
    	trafficControl.Install(routerOutgoingDevice);

    // 6. Assign IP addresses to both links.
    Ipv4AddressHelper address;

    address.SetBase("10.1.1.0", "255.255.255.0");
    address.Assign(accessDevices);

    address.SetBase("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer receiverInterfaces =
        address.Assign(bottleneckDevices);

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // 7. Install a TCP receiver.
    PacketSinkHelper sinkHelper(
        "ns3::TcpSocketFactory",
        InetSocketAddress(Ipv4Address::GetAny(), port));

    ApplicationContainer sinkApps = sinkHelper.Install(receiver);
    sinkApps.Start(Seconds(appStart));
    sinkApps.Stop(Seconds(simulationStop));

    // 8. Install a TCP sender that continuously sends data.
    BulkSendHelper sourceHelper(
        "ns3::TcpSocketFactory",
        InetSocketAddress(receiverInterfaces.GetAddress(1), port));

    sourceHelper.SetAttribute("MaxBytes", UintegerValue(0));

    ApplicationContainer sourceApps = sourceHelper.Install(sender);
    sourceApps.Start(Seconds(appStart));
    sourceApps.Stop(Seconds(appStop));

    // 9. Collect network-flow statistics.
    FlowMonitorHelper flowHelper;
    Ptr<FlowMonitor> monitor = flowHelper.InstallAll();

    std::cout << "Queue capacity: " << queueSize << " packets\n";

    Simulator::Stop(Seconds(simulationStop));
    Simulator::Run();

    const auto& queueStats = queueDiscs.Get(0)->GetStats();

    std::cout << "Router FIFO queue drops: "
          << queueStats.nTotalDroppedPackets << '\n';

    Ptr<PacketSink> sink =
    	DynamicCast<PacketSink>(sinkApps.Get(0));

    monitor->CheckForLostPackets();

    Ptr<Ipv4FlowClassifier> classifier =
        DynamicCast<Ipv4FlowClassifier>(flowHelper.GetClassifier());

    const auto stats = monitor->GetFlowStats();

    for (const auto &entry : stats)
    {
        Ipv4FlowClassifier::FiveTuple tuple =
            classifier->FindFlow(entry.first);

        if (tuple.destinationPort != port || tuple.protocol != 6)
        {
            continue;
        }

        const FlowMonitor::FlowStats &flow = entry.second;
        double duration = appStop - appStart;

        double throughputMbps = sink->GetTotalRx() * 8.0
                                / (duration * 1000000.0);

        double averageDelayMs = 0.0;
        if (flow.rxPackets > 0)
        {
            averageDelayMs =
                flow.delaySum.GetSeconds() * 1000.0 / flow.rxPackets;
        }

        std::cout << std::fixed << std::setprecision(3);
        std::cout << "TCP results:\n";
        std::cout << "  Transmitted packets: " << flow.txPackets << '\n';
        std::cout << "  Received packets:    " << flow.rxPackets << '\n';
        std::cout << "  Lost packets:        " << flow.lostPackets << '\n';
        std::cout << "  Throughput:          " << throughputMbps
                  << " Mbps\n";
        std::cout << "  Average delay:       " << averageDelayMs
                  << " ms\n";
    }

    Simulator::Destroy();
    return 0;
}
