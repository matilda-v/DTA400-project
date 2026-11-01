  GNU nano 8.7.1                                    scratch/queue-study.cc

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
                                                  [ Read 171 lines ]
^G Help          ^O Write Out     ^F Where Is      ^K Cut           ^T Execute       ^C Location      M-U Undo
^X Exit          ^R Read File     ^\ Replace       ^U Paste         ^J Justify       ^/ Go To Line    M-E Redo