#include "ns3/aodv-module.h"
#include "ns3/dsr-module.h"
#include "ns3/dsdv-module.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/wifi-module.h" 
#include "ns3/v4ping-helper.h"
#include "ns3/position-allocator.h"

#include "ns3/gauss-markov-mobility-model.h"

#include "ns3/propagation-loss-model.h"
#include "ns3/applications-module.h"
#include "ns3/v4ping.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/udp-client-server-helper.h"
#include "ns3/udp-server.h"
#include <iostream>
#include <cmath>
#include <string>
#include <fstream>

using namespace ns3;
using namespace std;
using namespace dsr;

class RouteExample 
{
public:
  RouteExample ();
  /// Configure script parameters, \return true on successful configuration
  bool Configure (int argc, char **argv);
  /// Run simulation
  void Run ();
  /// Report results
  void Report (std::ostream & os);

private:

  // parameters
  /// Number of nodes
  uint32_t size;
  /// Simulation time, seconds
  double simTime;
  /// Write per-device PCAP traces if true
  bool pcap;
  /// Print routes if true
  bool printRoutes;
  
  string topology = "/home/yh3155/workspace/ns-allinone-3.37/ns-3.37/scratch/manet100.csv";
  
  double txrange = 30;  //네트워크 사이즈?
//  double txrange = 50;  //네트워크 사이즈?
//  double txrange = 100;  //네트워크 사이즈?

  uint32_t interval = 10;
  
  bool verbose = false;
  
  bool tracing = false;
  
  char* outputFilename = (char *)"manet";

  // network
  NodeContainer nodes;
  NetDeviceContainer devices;
  Ipv4InterfaceContainer interfaces;
  Address serverAddress[50];
  UdpServer sermon;
  YansWifiPhyHelper wifiPhy;
  
  
  WifiMacHelper wifiMac;

private:
  void CreateNodes ();
  void CreateDevices ();
  void InstallInternetStack ();
  void InstallApplications ();
};

NS_LOG_COMPONENT_DEFINE ("ManetTest");

int main (int argc, char **argv)
{
  RouteExample test;
  if (!test.Configure (argc, argv))
    NS_FATAL_ERROR ("Configuration failed. Aborted.");
  
  test.Run ();
  test.Report (std::cout);


  return 0;
}

//-----------------------------------------------------------------------------
RouteExample::RouteExample () :
//  size (10),    //노드 개수
  size (50),    //노드 개수

  simTime (50),
  pcap (false),
  printRoutes (false)
{
}

bool
RouteExample::Configure (int argc, char **argv)
{
  // Enable AODV logs by default. Comment this if too noisy
  // LogComponentEnable("AodvRoutingProtocol", LOG_LEVEL_ALL);

  SeedManager::SetSeed (12345);
  CommandLine cmd;

  cmd.AddValue ("pcap", "Write PCAP traces.", pcap);
  cmd.AddValue ("printRoutes", "Print routing table dumps.", printRoutes);
  cmd.AddValue ("size", "Number of nodes.", size);
  cmd.AddValue ("simTime", "Simulation time, in seconds.", simTime);
  cmd.AddValue ("outputFilename", "Output filename", outputFilename);
  cmd.AddValue ("topology", "Topology file.", topology);
  cmd.AddValue ("txrange", "Transmission range per node, in meters.", txrange);
  cmd.AddValue ("interval", "Interval between each iteration.", interval);
  cmd.AddValue ("verbose", "Verbose tracking.", verbose);
  cmd.AddValue ("tracing", "Enable pcap tracing", tracing);
  cmd.AddValue ("outputFilename", "Output filename", outputFilename);

  cmd.Parse (argc, argv);
  
  if (verbose)
  {
    LogComponentEnable ("UdpSocket", LOG_LEVEL_INFO);
    LogComponentEnable ("UdpServer", LOG_LEVEL_INFO);
  }
  
  return true;
}

void
RouteExample::Run ()
{
//  Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", UintegerValue (1)); // enable rts cts all the time.
  CreateNodes ();
  CreateDevices ();
  InstallInternetStack ();
  InstallApplications ();

  //std::cout << "Starting simulation for " << simTime << " s ...\n";

  Simulator::Stop (Seconds (simTime));
  Simulator::Run ();
  Simulator::Destroy ();
}

void
RouteExample::Report (std::ostream &)
{ 
}

void
RouteExample::CreateNodes ()
{
  std::cout << "Creating " << (unsigned)size << " nodes with transmission range " << txrange << "m.\n";
  nodes.Create (size);
  // Name nodes
  for (uint32_t i = 0; i < size; ++i)
    {
      std::ostringstream os;
      os << "node-" << i;
      Names::Add (os.str (), nodes.Get (i));
    }
  
  Ptr<ListPositionAllocator> positionAllocS = CreateObject<ListPositionAllocator> ();
  
  
  std::string line;
  ifstream file(topology);
  
  uint16_t i = 0																																																																																																																																																																																																																																																																														;
  double vec[3];
  
  if(file.is_open())
  {
	while(getline(file,line))
	{
		
		//std::cout<<line<< '\n';
		char seps[] = ",";
		char *token;

		token = strtok(&line[0], seps);
		
		//std::cout << token << "\n";
		
		while(token != NULL)
		{
			//printf("[%s]\n", token);
			vec[i] = atof(token);
			i++;
			token = strtok (NULL, ",");
			if(i == 3)
			{
				//std::cout << "\n" << vec[0] << "  " << vec[1] << "   " << vec[2] << "\n";
				positionAllocS->Add(Vector(vec[1], vec[2], 0.0));
				i = 0;
			}
        }

	  }
	  file.close();
	}
	else
	{
	  std::cout<<"Error in csv file"<< '\n';
	}

	
  
  MobilityHelper mobilityS;
  mobilityS.SetPositionAllocator(positionAllocS);
//  mobilityS.SetMobilityModel("ns3::ConstantPositionMobilityModel"); //whatever it is
  mobilityS.SetMobilityModel("ns3::GaussMarkovMobilityModel"); //whatever it is

  mobilityS.Install(nodes);
}

void
RouteExample::CreateDevices ()
{
  
  wifiMac.SetType ("ns3::AdhocWifiMac");
  
  YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();
  wifiChannel.AddPropagationLoss("ns3::RangePropagationLossModel",
								   "MaxRange", DoubleValue (txrange));
  wifiPhy.SetChannel (wifiChannel.Create ());
  WifiHelper wifi;
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager", "DataMode", StringValue ("OfdmRate6Mbps"), "RtsCtsThreshold", UintegerValue (0));
  devices = wifi.Install (wifiPhy, wifiMac, nodes); 

  if (pcap)
    {
      wifiPhy.EnablePcapAll (std::string ("aodv"));
    }
}

void
RouteExample::InstallInternetStack ()
{
  
  AodvHelper aodv;
  DsrHelper dsr;
  DsdvHelper dsdv;
  DsrMainHelper dsrMain;
  // you can configure AODV attributes here using aodv.Set(name, value)
  InternetStackHelper stack;
//  stack.SetRoutingHelper (aodv); // has effect on the next Install ()
  stack.SetRoutingHelper (dsdv); // has effect on the next Install ()

  stack.Install (nodes);
//  dsrMain.Install(dsr,nodes);     //dsr은 flowmonitor가 제대로 동작하지 않는듯 함(broadcast로 인해)

  Ipv4AddressHelper address;
  address.SetBase ("10.0.0.0", "255.0.0.0");
  interfaces = address.Assign (devices);
  
  for(uint32_t i = 0; i < (size / 2); i++)
  {
	serverAddress[i] = Address (interfaces.GetAddress (i));
  }
  
/*
  if (printRoutes)
    {
      Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper> ("aodv.routes", std::ios::out);
      aodv.PrintRoutingTableAllAt (Seconds (8), routingStream);
    }
    */
}

void
RouteExample::InstallApplications ()
{
  int i = 0;
  int j = 0;
  int k = 0;
  int port = 4000;
  UdpServerHelper server (port);
  ApplicationContainer apps;
  
  for(i = 0; i < (size / 2); i++)
  {
	  apps = server.Install (nodes.Get (i));
  }
  
  apps.Start (Seconds (1.0));
  apps.Stop (Seconds (simTime));
  
  //Ptr<UdpServer> sermon = server.GetServer();
  

  uint32_t MaxPacketSize = 1024;
//  uint32_t MaxPacketSize = 2048;

  Time interPacketInterval = Seconds (0.01);
  uint32_t maxPacketCount = 3; //트래픽
//  uint32_t maxPacketCount = 30; //트래픽
  double interval_start = 2.0, interval_end = interval_start + interval;

  for(i = 0; i < (size / 2); i++)
  {
    UdpClientHelper client (serverAddress[i], port);
    client.SetAttribute ("MaxPackets", UintegerValue (maxPacketCount));
    client.SetAttribute ("Interval", TimeValue (interPacketInterval));
    client.SetAttribute ("PacketSize", UintegerValue (MaxPacketSize));
    
    apps = client.Install (nodes.Get ((size / 2) + i));

    interval_start = interval_end + 1.0;
    interval_end = interval_start + interval;
  }

  apps.Start (Seconds (interval_start));
  apps.Stop (Seconds (interval_end));

  uint32_t rxPacketsum = 0;
  double Delaysum = 0; 
  uint32_t txPacketsum = 0;
  uint32_t txBytessum = 0;
  uint32_t rxBytessum = 0;
  uint32_t txTimeFirst = 0;
  uint32_t rxTimeLast = 0;
  uint32_t lostPacketssum = 0;
  
  FlowMonitorHelper flowmon;
  Ptr<FlowMonitor> monitor = flowmon.InstallAll();
  

  Simulator::Stop (Seconds (simTime)); 
  Simulator::Run ();
  
  
  monitor->CheckForLostPackets ();
  
  std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();
  
  for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i)
    {
      rxPacketsum += i->second.rxPackets;
      txPacketsum += i->second.txPackets;
      txBytessum += i->second.txBytes;
      rxBytessum += i->second.rxBytes;
      Delaysum += i->second.delaySum.GetSeconds();
      lostPacketssum += i->second.lostPackets;
    }
  
  

  std::cout << "\n\n";
  std::cout << "  Total Tx Packets: " << txPacketsum << "\n";
  std::cout << "  Total Rx Packets: " << rxPacketsum << "\n";
  std::cout << "  Total Packets Lost: " << lostPacketssum << "\n";
  std::cout << "  Throughput: " << ((rxBytessum * 8.0) / simTime)/1024<<" Kbps"<<"\n";
  std::cout << "  Packets Delivery Ratio: " << (((txPacketsum - lostPacketssum) * 100) /txPacketsum) << "%" << "\n";
  
  std::cout << "  Average E2E Delay: " << Delaysum/rxPacketsum <<"\n";
  flowmon.SerializeToXmlFile("flow.xml", false, false);


  ofstream writeFile;
  writeFile.open("result.csv",std::ios_base::out | std::ios_base::app);
  writeFile<<txPacketsum<<','<<rxPacketsum<<','<<lostPacketssum<<','<<((rxBytessum * 8.0) / simTime)/1024<<','<<(((txPacketsum - lostPacketssum) * 100) /txPacketsum)<<','<<Delaysum/rxPacketsum<<'\n';
  writeFile.close();


  Simulator::Destroy ();
  
}

