/*
 * NewFatTree.cc
 *
 *  Created on: Mar 20, 2014
 *      Author: chris
 */

#include<iostream>
#include<fstream>
#include<string>
#include<cassert>
#include<malloc.h>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/csma-module.h"
#include "ns3/ipv4-nix-vector-helper.h"
#include "ns3/netanim-module.h"
#include "ns3/constant-position-mobility-model.h"
#include "ns3/gnuplot.h"

#include "ns3/node.h"

using namespace ns3;
using namespace std;

std::string i2s(uint i) {
	std::string s;
	std::stringstream out;
	out << i;
	s = out.str();
	return s;
}

std::map<Ptr<Node>, Ipv4Address> ServerIpMap;
std::map<Ipv4Address, Ptr<Node> > IpServerMap;

std::map<uint, std::string> id_serverLabel_map;
std::map<std::string, uint> serverLabel_id_map;
std::map<std::string, Ipv4Address> serverLabel_address_map;

//vector<pair<string, pair<string, uint> > > flows; // TCP data flows

std::vector<std::pair<std::string, std::string> > server_turning_pairs; // serverLabel-> turningSwitchLabel
std::map<std::string, std::string> flow_turning_map; // flow->turningSwitchLabel

bool isEfficientFatTree = false;

// use the LRD algorithm for comparison.
bool isLRD = false;
// flows experience failure for testing LRD. flowId = "dstLabel_turningLabel_failureSwitchLabel"
std::map<std::string, uint> LRD_failureflow_oif_map;

std::map<std::string, uint32_t> FailNode_oif_map;
extern double startTimeFatTree;
extern double stopTimeFatTree;
//extern uint32_t selectedNode;
extern double collectTime;
//extern double failTimeFatTree;
const uint Port_num = 8; // set the # of ports at a switch. k=8

class Flow {
public:
	Flow(std::string srcLabel, std::string dstLabel, uint dst_port);
	std::string getFlowId(bool isAck = false);
	void ShowSinkResult();

	Ptr<BulkSendApplication> srcApp;
	Ptr<PacketSink> sinkApp;

	std::string srcLabel, dstLabel;
	uint dst_port;
};

Flow::Flow(std::string srcLabel, std::string dstLabel, uint dst_port) {
	this->srcLabel = srcLabel;
	this->dstLabel = dstLabel;
	this->dst_port = dst_port;
}

std::string Flow::getFlowId(bool isAck) {
	if (!isAck) { // data flow
		return srcLabel + "->" + dstLabel + ":" + i2s(dst_port);
	} else { // ack flow
		return dstLabel + ":" + i2s(dst_port) + "->" + srcLabel;
	}
}

void Flow::ShowSinkResult() {
	Ptr<PacketSink> sink = this->sinkApp;
	std::cout << "-----------flow id = " << this->getFlowId() << "------------"
			<< std::endl;
//	std::cout << "sinkApp = " << sink << "; sourceApp=" << this->srcApp
//			<< std::endl;
	std::cout << "total size : " << sink->GetTotalRx() << " B" << std::endl;
	std::cout << "total number of packets : " << sink->packetN << std::endl;
	std::cout << "total delay is : " << sink->totalDelay << " s" << std::endl;
	std::cout << "average delay is : " << sink->totalDelay / sink->packetN
			<< " s" << std::endl;
	std::cout << "Goodput is : "
			<< (sink->GetTotalRx()
					/ (stopTimeFatTree - startTimeFatTree - collectTime)) * 8.0
					/ 1000.0 << " kbps" << std::endl << std::endl;
}

vector<Flow> flows;

void setFailure(void) {
	// link 001:1 <-> 002:5
	FailNode_oif_map["001"] = 1;
	FailNode_oif_map["002"] = 5;
}
void clearFailure(NodeContainer switchAll) {
	FailNode_oif_map.clear();
	for (uint i = 0; i < switchAll.GetN(); i++) {
		switchAll.Get(i)->reRoutingMap.clear();
	}

}
NS_LOG_COMPONENT_DEFINE("first");

Ptr<BulkSendApplication> newBulkSendApp(Ptr<Node> srcNode, Ptr<Node> dstNode,
		uint dstPort, uint maxBytes = 0) {
	BulkSendHelper source("ns3::TcpSocketFactory",
			(InetSocketAddress(ServerIpMap[dstNode], dstPort)));
	source.SetAttribute("MaxBytes", UintegerValue(maxBytes));
	return DynamicCast<BulkSendApplication>(source.Install(srcNode).Get(0));
}

Ptr<PacketSink> newPacketSinkApp(Ptr<Node> dstNode, uint dstPort) {
	PacketSinkHelper sink = PacketSinkHelper("ns3::TcpSocketFactory",
			InetSocketAddress(Ipv4Address().GetAny(), dstPort));
	return DynamicCast<PacketSink>(sink.Install(dstNode).Get(0));
}

void addFlow(std::string srcLabel, std::string dstLabel, uint dst_port,
		std::string turningSwitchLabel) {
	Flow flow(srcLabel, dstLabel, dst_port);
	string dataFlowId = flow.getFlowId();
	string ackFlowId = flow.getFlowId(true);

	if (flow_turning_map.find(dataFlowId) != flow_turning_map.end()) {
		std::cout << "Duplicate flow: " << dataFlowId << std::endl;
		assert(false);
	}
	flows.push_back(flow);
	flow_turning_map[dataFlowId] = turningSwitchLabel; // data flow -> turning switch label
	flow_turning_map[ackFlowId] = turningSwitchLabel; // ack flow -> turning switch label
}

void showSinkResult(string flowId, Ptr<PacketSink> sink) {
	std::cout << "-----------flow id = " << flowId << "------------"
			<< std::endl;
	std::cout << "total size : " << sink->GetTotalRx() << std::endl;
	std::cout << "total number of packets : " << sink->packetN << std::endl;
	std::cout << "total delay is : " << sink->totalDelay << std::endl;
	std::cout << "average delay is : " << sink->totalDelay / sink->packetN
			<< std::endl;
	std::cout << "Goodput is : "
			<< (sink->GetTotalRx()
					/ (stopTimeFatTree - startTimeFatTree - collectTime)) * 8.0
					/ 1000.0 << " kbps" << std::endl << std::endl;
}

int main(int argc, char *argv[]) {
	/*-------------------parameter setting----------------------*/
	startTimeFatTree = 0.0;
	stopTimeFatTree = 40.0;
	collectTime = 10.0;
//	selectedNode = 129; //Id 167 is the aggregate switch for flow(18->5) and flow(20->113)
	//Id 171 is the aggregate switch for flow(35->5) and flow(40->100)
	//Id 129 is the edge switch for flow(18->5) and flow(35->5)

//	Simulator::Schedule(Seconds(10.0), clearFailure, switchAll);
//	Simulator::Schedule(Seconds(50.0), setFailure);

//Changes seed from default of 1 to a specific number

	SeedManager::SetSeed(6);
//Changes run number (scenario number) from default of 1 to a specific runNumber
	SeedManager::SetRun(5);

//	Config::SetDefault("ns3::OnOffApplication::PacketSize", UintegerValue(210));
//	Config::SetDefault("ns3::OnOffApplication::DataRate",
//			StringValue("448kb/s"));
	Config::SetDefault("ns3::DropTailQueue::Mode",
			EnumValue(ns3::Queue::QUEUE_MODE_BYTES));
//
//	Config::SetDefault("ns3::DropTailQueue::Mode",
//			EnumValue(ns3::Queue::QUEUE_MODE_PACKETS));

	// output-queued switch buffer size (max bytes of each fifo queue)
	Config::SetDefault("ns3::DropTailQueue::MaxBytes", UintegerValue(20000));
//	Config::SetDefault("ns3::DropTailQueue::MaxPackets", UintegerValue(25));
//	Config::SetDefault("ns3::RedQueue::Mode",
//			EnumValue(ns3::Queue::QUEUE_MODE_BYTES));
//
////	 output-queued switch buffer size (max bytes of each fifo queue)
//	Config::SetDefault("ns3::RedQueue::MinTh", DoubleValue(5000.0));
//	Config::SetDefault("ns3::RedQueue::MaxTh", DoubleValue(15000.0));
//	Config::SetDefault("ns3::RedQueue::QueueLimit", UintegerValue(15000));
	/*-------------------------------------------------------------------------*/
	CommandLine cmd;
	bool enableMonitor = false;
	cmd.AddValue("EnableMonitor", "Enable Flow Monitor", enableMonitor);
	cmd.AddValue("LRD", "Use LRD algorithm for rerouting", isLRD);
	cmd.AddValue("collectTime", "Time to start collecting", collectTime);
	cmd.AddValue("stopTime", "Time to stop simulation", stopTimeFatTree);

	cmd.Parse(argc, argv);

	Ipv4InterfaceContainer p2p_ip;
	InternetStackHelper internet;
	Ipv4InterfaceContainer server_ip;
	Ipv4AddressHelper address;
	NS_LOG_INFO("Create Nodes");
	// Create nodes for servers
	NodeContainer node_server;
	node_server.Create(Port_num * Port_num * Port_num / 4);
	internet.Install(node_server);
	// Create nodes for Level-2 switches
	NodeContainer node_l2switch;
	node_l2switch.Create(Port_num * Port_num / 2);
	internet.Install(node_l2switch);
	// Create nodes for Level-1 switches
	NodeContainer node_l1switch;
	node_l1switch.Create(Port_num * Port_num / 2);
	internet.Install(node_l1switch);
	// Create nodes for Level-0 switches
	NodeContainer node_l0switch;
	node_l0switch.Create(Port_num * Port_num / 4);
	internet.Install(node_l0switch);
	// Point to Point Helper to all the links in fat-tree include the server subnets
	PointToPointHelper p2p;
	p2p.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
	p2p.SetChannelAttribute("Delay", StringValue("1ms"));
//	p2p.SetQueue(
//			"ns3::RedQueue");
//		p2p.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
//		p2p.SetChannelAttribute("Delay", StringValue("1ms"));
//	// First create four set of Level-0 subnets
	uint32_t ip1;

	//-------------------------------------- Construct Topology------------------------------------------------

	for (uint i = 0; i < Port_num; i++) {     // cycling with pods
		for (uint j = 0; j < Port_num / 2; j++) { // cycling within every pod among Level 2 switches
			node_l2switch.Get(i * (Port_num / 2) + j)->SetId_FatTree(i, j, 2); // labeling the switch
			for (uint k = 0; k < Port_num / 2; k++) { // cycling within every level 2 switch

				NetDeviceContainer link;
				NodeContainer node;
				Ptr<Node> serverNode = node_server.Get(
						i * (Port_num * Port_num / 4) + j * (Port_num / 2) + k);
				node.Add(serverNode);
				//node_server.Get(
//						i * (Port_num * Port_num / 4) + j * (Port_num / 2) + k)->SetId_FatTree(
//						i, j, k); // labling the host server

				serverNode->nodeId_FatTree = NodeId(i, j, k); // labeling the host server

				std::string serverTag = i2s(i) + i2s(j) + i2s(k);
				serverLabel_id_map[serverTag] = serverNode->GetId();
				id_serverLabel_map[serverNode->GetId()] = serverTag;

				node.Add(node_l2switch.Get(i * (Port_num / 2) + j));
				link = p2p.Install(node);
				// assign the ip address of the two device.
				ip1 = (10 << 24) + (i << 16) + (3 << 14) + (j << 8) + (k << 4);
				//std::cout<< "assign ip is "<< Ipv4Address(ip1)<<std::endl;
				address.SetBase(Ipv4Address(ip1), "255.255.255.248");
				server_ip = address.Assign(link);

				ServerIpMap[serverNode] = server_ip.GetAddress(0);
				IpServerMap[server_ip.GetAddress(0)] = serverNode;
				serverLabel_address_map[serverTag] = server_ip.GetAddress(0);
			}

		}
	} // End of creating servers at level-0 and connecting it to first layer switch
	  // Now connect first layer switch with second layer
	for (uint i = 0; i < Port_num; i++) { //cycling among pods
		for (uint j = 0; j < Port_num / 2; j++) {  //cycling among switches
			for (uint k = 0; k < Port_num / 2; k++) { //cycling among ports on one switch
				NetDeviceContainer link;
				NodeContainer node;
				node.Add(node_l2switch.Get(i * (Port_num / 2) + j));
				node.Add(node_l1switch.Get(i * (Port_num / 2) + k));
				node_l1switch.Get(i * (Port_num / 2) + k)->SetId_FatTree(i, k,
						1);
				link = p2p.Install(node);
				ip1 = (10 << 24) + (i << 16) + (2 << 14) + (j << 8) + (k << 4);
				address.SetBase(Ipv4Address(ip1), "255.255.255.248");
				p2p_ip = address.Assign(link);
			}
		}
	}  //end of the connections construct between l2 switches and l1 switches
	   // Now connect the core switches and the level-2 switches

	   // Efficient fat-tree (k=8) construction (shuffle pattern between core and aggregation layers)
	uint efficient_Pod[Port_num][Port_num * Port_num / 4] = { { 1, 2, 3, 4, 5,
			6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 }, { 2, 3, 4, 5, 6, 7, 8, 9,
			10, 11, 12, 13, 14, 15, 16, 1 }, { 3, 4, 5, 6, 7, 8, 9, 10, 11, 12,
			13, 14, 15, 16, 1, 2 }, { 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
			16, 1, 2, 3 }, { 1, 5, 9, 13, 2, 6, 10, 14, 3, 7, 11, 15, 4, 8, 12,
			16 }, { 5, 9, 13, 2, 6, 10, 14, 3, 7, 11, 15, 4, 8, 12, 16, 1 }, {
			9, 13, 2, 6, 10, 14, 3, 7, 11, 15, 4, 8, 12, 16, 1, 5 }, { 13, 2, 6,
			10, 14, 3, 7, 11, 15, 4, 8, 12, 16, 1, 5, 9 } };

	// Regular fat-tree (k=8) construction (shuffle pattern between core and aggregation layers)
	uint regular_Pod[Port_num][Port_num * Port_num / 4] = { { 1, 2, 3, 4, 5, 6,
			7, 8, 9, 10, 11, 12, 13, 14, 15, 16 }, { 1, 2, 3, 4, 5, 6, 7, 8, 9,
			10, 11, 12, 13, 14, 15, 16 }, { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
			12, 13, 14, 15, 16 }, { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13,
			14, 15, 16 }, { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
			16 }, { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 }, {
			1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 }, { 1, 2, 3,
			4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 } };

	//uint temp = 0;
	for (uint i = 0; i < Port_num; i++) {	//cycling among the pods
		for (uint j = 0; j < Port_num / 2; j++) {//cycling among the pod switches
			for (uint k = 0; k < Port_num / 2; k++) {//cycling among the ports on each switch
				uint temp;
				if (isEfficientFatTree) {
					temp = efficient_Pod[i][j * (Port_num / 2) + k];
				} else {
					temp = regular_Pod[i][j * (Port_num / 2) + k];
				}
				NetDeviceContainer link;
				NodeContainer node;
				node.Add(node_l1switch.Get(i * (Port_num / 2) + j));
				node.Add(node_l0switch.Get(temp - 1));
				link = p2p.Install(node);
				ip1 = (10 << 24) + (i << 16) + (1 << 14) + (j << 8) + (k << 4);
				address.SetBase(Ipv4Address(ip1), "255.255.255.248");
				p2p_ip = address.Assign(link);
			}

		}
	}

	for (uint i = 0; i < Port_num / 2; i++)    	// labeling the level 0 switches
			{
		for (uint j = 0; j < Port_num / 2; j++) {
			node_l0switch.Get(i * (Port_num / 2) + j)->SetId_FatTree(i, j, 0);
		}
	}

	Ipv4GlobalRoutingHelper::PopulateRoutingTables();

	uint16_t dst_port;
	dst_port = 20;

	NodeContainer switchAll;
	switchAll.Add(node_l2switch);
	switchAll.Add(node_l1switch);
	switchAll.Add(node_l0switch);

//	p2p.EnablePcap("server tracing 35", node_server.Get(35)->GetDevice(1),
//			true);
//	p2p.EnablePcap("server tracing 18", node_server.Get(18)->GetDevice(1),
//			true);
//	p2p.EnablePcap("server tracing 20", node_server.Get(20)->GetDevice(1),
//			true);
//	p2p.EnablePcap("server tracing 40", node_server.Get(40)->GetDevice(1),
//			true);
//	p2p.EnablePcap("server tracing 23", node_server.Get(23)->GetDevice(1),
//			true);
//	p2p.EnablePcap("server tracing 21", node_server.Get(21)->GetDevice(1),
//			true);

//---------------------------------Flows: BulkSend and Sink Application-------------------------------//

	ApplicationContainer srcApps;
	ApplicationContainer sinkApps;

	// Add flow path
//	addFlow("000", "001", dst_port, "00x");
//	addFlow("000", "010", dst_port, "001");
	addFlow("000", "101", dst_port, "000"); // 6-hop flow goes through the failure link

	addFlow("200", "300", dst_port, "300"); // 6-hop flow goes through no failure link.

	// add Source&Sink Apps (Traffic Generator&Receiver) for each flow
	for (uint i = 0; i < flows.size(); i++) {
		Flow* flow = &flows[i];
		std::string src_label = flow->srcLabel;
		std::string dst_label = flow->dstLabel;
		uint port = flow->dst_port;

		flow->srcApp = newBulkSendApp(
				node_server.Get(serverLabel_id_map[src_label]),
				node_server.Get(serverLabel_id_map[dst_label]), port);
		flow->sinkApp = newPacketSinkApp(
				node_server.Get(serverLabel_id_map[dst_label]), port);

		srcApps.Add(flow->srcApp);
		sinkApps.Add(flow->sinkApp);
	}

	srcApps.Start(Seconds(startTimeFatTree));
	srcApps.Stop(Seconds(stopTimeFatTree));

	sinkApps.Start(Seconds(startTimeFatTree));
	sinkApps.Stop(Seconds(stopTimeFatTree));

//----------------------------------Simulation result-----------------------------------------------//
	// set failure
//	double failTimeFatTree = collectTime + (stopTimeFatTree - collectTime) / 2;
	double failTimeFatTree = collectTime;
	Simulator::Schedule(Seconds(failTimeFatTree), setFailure);

	std::cout << "collectTime=" << collectTime << "; failureTime="
			<< failTimeFatTree << "; stopTime=" << stopTimeFatTree << "; isLRD="
			<< isLRD << std::endl;
	Simulator::Stop(Seconds(stopTimeFatTree + 0.01));
	Simulator::Run();
	Simulator::Destroy();

	// print results
	for (uint i = 0; i < flows.size(); i++) {
//		string flowId = flows[i].first + "->" + flows[i].second.first;
//		showSinkResult(flowId, flow_sink_map[flowId]);
		flows[i].ShowSinkResult();

//		std::cout << "the target one(35->5): --------------------------------"
//				<< std::endl;
//		std::cout << "total size : " << sinkStatisticTarget->GetTotalRx()
//				<< std::endl;
//		std::cout << "total number of packets : "
//				<< sinkStatisticTarget->packetN << std::endl;
//		std::cout << "total delay is : " << sinkStatisticTarget->totalDelay
//				<< std::endl;
//		std::cout << "average delay is : "
//				<< sinkStatisticTarget->totalDelay
//						/ sinkStatisticTarget->packetN << std::endl;
//		std::cout << "Goodput is : "
//				<< (sinkStatisticTarget->GetTotalRx()
//						/ (stopTimeFatTree - startTimeFatTree - collectTime))
//						* 8.0 / 1000.0 << " kbps" << std::endl;
//		/*--------------------------------Compare 1------------------------------------------*/
//		std::cout
//				<< "the Target two (same Dst with Target one)(18->5) : --------------------------------"
//				<< std::endl;
//		std::cout << "total size : " << sinkStatisticCompare1->GetTotalRx()
//				<< std::endl;
//		std::cout << "total number of packets : "
//				<< sinkStatisticCompare1->packetN << std::endl;
//		std::cout << "total delay is : " << sinkStatisticCompare1->totalDelay
//				<< std::endl;
//		std::cout << "average delay is : "
//				<< sinkStatisticCompare1->totalDelay
//						/ sinkStatisticCompare1->packetN << std::endl;
//		std::cout << "Goodput is : "
//				<< (sinkStatisticCompare1->GetTotalRx()
//						/ (stopTimeFatTree - startTimeFatTree - collectTime))
//						* 8.0 / 1000.0 << " kbps" << std::endl;
//		/*--------------------------------Compare 2------------------------------------------*/
//		std::cout
//				<< "the compare one (share same path in the Dst Pod(23->6), used in LRD Algorithm) : -----------------------"
//				<< std::endl;
//		std::cout << "total size : " << sinkStatisticCompare2->GetTotalRx()
//				<< std::endl;
//		std::cout << "total number of packets : "
//				<< sinkStatisticCompare2->packetN << std::endl;
//		std::cout << "total delay is : " << sinkStatisticCompare2->totalDelay
//				<< std::endl;
//		std::cout << "average delay is : "
//				<< sinkStatisticCompare2->totalDelay
//						/ sinkStatisticCompare2->packetN << std::endl;
//		std::cout << "Goodput is : "
//				<< (sinkStatisticCompare2->GetTotalRx()
//						/ (stopTimeFatTree - startTimeFatTree - collectTime))
//						* 8.0 / 1000.0 << " kbps" << std::endl;
//		/*--------------------------------Compare 3------------------------------------------*/
//		std::cout
//				<< "the compare two (share same path in the Src Pod(20->113), used in LRB Algorithm) : -----------------------"
//				<< std::endl;
//		std::cout << "total size : " << sinkStatisticCompare3->GetTotalRx()
//				<< std::endl;
//		std::cout << "total number of packets : "
//				<< sinkStatisticCompare3->packetN << std::endl;
//		std::cout << "total delay is : " << sinkStatisticCompare3->totalDelay
//				<< std::endl;
//		std::cout << "average delay is : "
//				<< sinkStatisticCompare3->totalDelay
//						/ sinkStatisticCompare3->packetN << std::endl;
//		std::cout << "Goodput is : "
//				<< (sinkStatisticCompare3->GetTotalRx()
//						/ (stopTimeFatTree - startTimeFatTree - collectTime))
//						* 8.0 / 1000.0 << " kbps" << std::endl;
//		/*--------------------------------Compare 4------------------------------------------*/
//		std::cout
//				<< "the compare two (share same path in the Src Pod(21->50), used in LRB Algorithm) : -----------------------"
//				<< std::endl;
//		std::cout << "total size : " << sinkStatisticCompare5->GetTotalRx()
//				<< std::endl;
//		std::cout << "total number of packets : "
//				<< sinkStatisticCompare5->packetN << std::endl;
//		std::cout << "total delay is : " << sinkStatisticCompare5->totalDelay
//				<< std::endl;
//		std::cout << "average delay is : "
//				<< sinkStatisticCompare5->totalDelay
//						/ sinkStatisticCompare5->packetN << std::endl;
//		std::cout << "Goodput is : "
//				<< (sinkStatisticCompare5->GetTotalRx()
//						/ (stopTimeFatTree - startTimeFatTree - collectTime))
//						* 8.0 / 1000.0 << " kbps" << std::endl;
//		/*--------------------------------Compare 5------------------------------------------*/
//		std::cout
//				<< "the compare three (share same path in the Src Pod(40->100), used in LRB Algorithm) : -----------------------"
//				<< std::endl;
//		std::cout << "total size : " << sinkStatisticCompare4->GetTotalRx()
//				<< std::endl;
//		std::cout << "total number of packets : "
//				<< sinkStatisticCompare4->packetN << std::endl;
//		std::cout << "total delay is : " << sinkStatisticCompare4->totalDelay
//				<< std::endl;
//		std::cout << "average delay is : "
//				<< sinkStatisticCompare4->totalDelay
//						/ sinkStatisticCompare4->packetN << std::endl;
//		std::cout << "Goodput is : "
//				<< (sinkStatisticCompare4->GetTotalRx()
//						/ (stopTimeFatTree - startTimeFatTree - collectTime))
//						* 8.0 / 1000.0 << " kbps" << std::endl;
	}
//	delete []Pod;
//	//Pod = NULL;
	NS_LOG_INFO("Done");
	std::cout << "\n ALL done!\n" << std::endl;
	return 0;
}        // End of program

