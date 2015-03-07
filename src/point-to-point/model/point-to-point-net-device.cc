/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2007, 2008 University of Washington
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "ns3/log.h"
#include "ns3/queue.h"
#include "ns3/simulator.h"
#include "ns3/mac48-address.h"
#include "ns3/llc-snap-header.h"
#include "ns3/error-model.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/uinteger.h"
#include "ns3/pointer.h"
#include "ns3/mpi-interface.h"
#include "point-to-point-net-device.h"
#include "point-to-point-channel.h"
#include "ppp-header.h"

#include "ns3/ipv4-header.h"
#include "ns3/node.h"
#include "ns3/udp-l4-protocol.h"
#include "ns3/tcp-l4-protocol.h"
#include "src/network/model/node.h"
#include <cassert>

NS_LOG_COMPONENT_DEFINE("PointToPointNetDevice");

std::map<std::string, uint32_t> FailNode_oif_map;
bool isEfficientFatTree;
bool isLRD;
std::map<std::string, uint> LRD_failureflow_oif_map;
const uint Port_num = 8;

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED(PointToPointNetDevice);

TypeId PointToPointNetDevice::GetTypeId(void) {
	static TypeId tid =
			TypeId("ns3::PointToPointNetDevice").SetParent<NetDevice>().AddConstructor<
					PointToPointNetDevice>().AddAttribute("Mtu",
					"The MAC-level Maximum Transmission Unit",
					UintegerValue(DEFAULT_MTU),
					MakeUintegerAccessor(&PointToPointNetDevice::SetMtu,
							&PointToPointNetDevice::GetMtu),
					MakeUintegerChecker<uint16_t>()).AddAttribute("Address",
					"The MAC address of this device.",
					Mac48AddressValue(Mac48Address("ff:ff:ff:ff:ff:ff")),
					MakeMac48AddressAccessor(&PointToPointNetDevice::m_address),
					MakeMac48AddressChecker()).AddAttribute("DataRate",
					"The default data rate for point to point links",
					DataRateValue(DataRate("32768b/s")),
					MakeDataRateAccessor(&PointToPointNetDevice::m_bps),
					MakeDataRateChecker()).AddAttribute("ReceiveErrorModel",
					"The receiver error model used to simulate packet loss",
					PointerValue(),
					MakePointerAccessor(
							&PointToPointNetDevice::m_receiveErrorModel),
					MakePointerChecker<ErrorModel>()).AddAttribute(
					"InterframeGap",
					"The time to wait between packet (frame) transmissions",
					TimeValue(Seconds(0.0)),
					MakeTimeAccessor(&PointToPointNetDevice::m_tInterframeGap),
					MakeTimeChecker())

			//
			// Transmit queueing discipline for the device which includes its own set
			// of trace hooks.
			//
			.AddAttribute("TxQueue",
					"A queue to use as the transmit queue in the device.",
					PointerValue(),
					MakePointerAccessor(&PointToPointNetDevice::m_queue),
					MakePointerChecker<Queue>())

			//
			// Trace sources at the "top" of the net device, where packets transition
			// to/from higher layers.
			//
			.AddTraceSource("MacTx",
					"Trace source indicating a packet has arrived for transmission by this device",
					MakeTraceSourceAccessor(
							&PointToPointNetDevice::m_macTxTrace)).AddTraceSource(
					"MacTxDrop",
					"Trace source indicating a packet has been dropped by the device before transmission",
					MakeTraceSourceAccessor(
							&PointToPointNetDevice::m_macTxDropTrace)).AddTraceSource(
					"MacPromiscRx",
					"A packet has been received by this device, has been passed up from the physical layer "
							"and is being forwarded up the local protocol stack.  This is a promiscuous trace,",
					MakeTraceSourceAccessor(
							&PointToPointNetDevice::m_macPromiscRxTrace)).AddTraceSource(
					"MacRx",
					"A packet has been received by this device, has been passed up from the physical layer "
							"and is being forwarded up the local protocol stack.  This is a non-promiscuous trace,",
					MakeTraceSourceAccessor(
							&PointToPointNetDevice::m_macRxTrace))
#if 0
					// Not currently implemented for this device
					.AddTraceSource("MacRxDrop",
							"Trace source indicating a packet was dropped before being forwarded up the stack",
							MakeTraceSourceAccessor(&PointToPointNetDevice::m_macRxDropTrace))
#endif
					//
					// Trace souces at the "bottom" of the net device, where packets transition
					// to/from the channel.
					//
					.AddTraceSource("PhyTxBegin",
					"Trace source indicating a packet has begun transmitting over the channel",
					MakeTraceSourceAccessor(
							&PointToPointNetDevice::m_phyTxBeginTrace)).AddTraceSource(
					"PhyTxEnd",
					"Trace source indicating a packet has been completely transmitted over the channel",
					MakeTraceSourceAccessor(
							&PointToPointNetDevice::m_phyTxEndTrace)).AddTraceSource(
					"PhyTxDrop",
					"Trace source indicating a packet has been dropped by the device during transmission",
					MakeTraceSourceAccessor(
							&PointToPointNetDevice::m_phyTxDropTrace))
#if 0
					// Not currently implemented for this device
					.AddTraceSource("PhyRxBegin",
							"Trace source indicating a packet has begun being received by the device",
							MakeTraceSourceAccessor(&PointToPointNetDevice::m_phyRxBeginTrace))
#endif
					.AddTraceSource("PhyRxEnd",
					"Trace source indicating a packet has been completely received by the device",
					MakeTraceSourceAccessor(
							&PointToPointNetDevice::m_phyRxEndTrace)).AddTraceSource(
					"PhyRxDrop",
					"Trace source indicating a packet has been dropped by the device during reception",
					MakeTraceSourceAccessor(
							&PointToPointNetDevice::m_phyRxDropTrace))

			//
			// Trace sources designed to simulate a packet sniffer facility (tcpdump).
			// Note that there is really no difference between promiscuous and
			// non-promiscuous traces in a point-to-point link.
			//
			.AddTraceSource("Sniffer",
					"Trace source simulating a non-promiscuous packet sniffer attached to the device",
					MakeTraceSourceAccessor(
							&PointToPointNetDevice::m_snifferTrace)).AddTraceSource(
					"PromiscSniffer",
					"Trace source simulating a promiscuous packet sniffer attached to the device",
					MakeTraceSourceAccessor(
							&PointToPointNetDevice::m_promiscSnifferTrace));
	return tid;
}

PointToPointNetDevice::PointToPointNetDevice() :
		m_txMachineState(READY), m_channel(0), m_linkUp(false), m_currentPkt(0) {
	NS_LOG_FUNCTION(this);
	reRoutDistributor = 0;
}

PointToPointNetDevice::~PointToPointNetDevice() {
	NS_LOG_FUNCTION_NOARGS();
}

void PointToPointNetDevice::AddHeader(Ptr<Packet> p, uint16_t protocolNumber) {
	NS_LOG_FUNCTION_NOARGS();
	PppHeader ppp;
	ppp.SetProtocol(EtherToPpp(protocolNumber));
	p->AddHeader(ppp);
}

bool PointToPointNetDevice::ProcessHeader(Ptr<Packet> p, uint16_t& param) {
	NS_LOG_FUNCTION_NOARGS();
	PppHeader ppp;
	p->RemoveHeader(ppp);
	param = PppToEther(ppp.GetProtocol());
	return true;
}

void PointToPointNetDevice::DoDispose() {
	NS_LOG_FUNCTION_NOARGS();
	m_node = 0;
	m_channel = 0;
	m_receiveErrorModel = 0;
	m_currentPkt = 0;
	NetDevice::DoDispose();
}

void PointToPointNetDevice::SetDataRate(DataRate bps) {
	NS_LOG_FUNCTION_NOARGS();
	m_bps = bps;
}

void PointToPointNetDevice::SetInterframeGap(Time t) {
	NS_LOG_FUNCTION_NOARGS();
	m_tInterframeGap = t;
}

bool PointToPointNetDevice::TransmitStart(Ptr<Packet> p) {
	NS_LOG_FUNCTION(this << p);NS_LOG_LOGIC("UID is " << p->GetUid() << ")");

	//
	// This function is called to start the process of transmitting a packet.
	// We need to tell the channel that we've started wiggling the wire and
	// schedule an event that will be executed when the transmission is complete.
	//
	NS_ASSERT_MSG(m_txMachineState == READY, "Must be READY to transmit");
	m_txMachineState = BUSY;
	m_currentPkt = p;
	m_phyTxBeginTrace(m_currentPkt);

	Time txTime = Seconds(m_bps.CalculateTxTime(p->GetSize()));
	Time txCompleteTime = txTime + m_tInterframeGap;

	NS_LOG_LOGIC("Schedule TransmitCompleteEvent in " << txCompleteTime.GetSeconds() << "sec");
	Simulator::Schedule(txCompleteTime,
			&PointToPointNetDevice::TransmitComplete, this);

	bool result = m_channel->TransmitStart(p, this, txTime);
	if (result == false) {
		m_phyTxDropTrace(p);
	}
	return result;
}

void PointToPointNetDevice::TransmitComplete(void) {
	NS_LOG_FUNCTION_NOARGS();

	//
	// This function is called to when we're all done transmitting a packet.
	// We try and pull another packet off of the transmit queue.  If the queue
	// is empty, we are done, otherwise we need to start transmitting the
	// next packet.
	//
	NS_ASSERT_MSG(m_txMachineState == BUSY, "Must be BUSY if transmitting");
	m_txMachineState = READY;

	NS_ASSERT_MSG(m_currentPkt != 0,
			"PointToPointNetDevice::TransmitComplete(): m_currentPkt zero");

	m_phyTxEndTrace(m_currentPkt);
	m_currentPkt = 0;

	Ptr<Packet> p = m_queue->Dequeue();
	if (p == 0) {
		//
		// No packet was on the queue, so we just exit.
		//
		return;
	}

	//
	// Got another packet off of the queue, so start the transmit process agin.
	//
	m_snifferTrace(p);
	m_promiscSnifferTrace(p);
	TransmitStart(p);
}

bool PointToPointNetDevice::Attach(Ptr<PointToPointChannel> ch) {
	NS_LOG_FUNCTION(this << &ch);

	m_channel = ch;

	m_channel->Attach(this);

	//
	// This device is up whenever it is attached to a channel.  A better plan
	// would be to have the link come up when both devices are attached, but this
	// is not done for now.
	//
	NotifyLinkUp();
	return true;
}

void PointToPointNetDevice::SetQueue(Ptr<Queue> q) {
	NS_LOG_FUNCTION(this << q);
	m_queue = q;
}

void PointToPointNetDevice::SetReceiveErrorModel(Ptr<ErrorModel> em) {
	NS_LOG_FUNCTION(this << em);
	m_receiveErrorModel = em;
}

void PointToPointNetDevice::Receive(Ptr<Packet> packet) {
	NS_LOG_FUNCTION(this << packet);
	uint16_t protocol = 0;

	if (m_receiveErrorModel && m_receiveErrorModel->IsCorrupt(packet)) {
		//
		// If we have an error model and it indicates that it is time to lose a
		// corrupted packet, don't forward this packet up, let it go.
		//
		m_phyRxDropTrace(packet);
	} else {
		//
		// Hit the trace hooks.  All of these hooks are in the same place in this
		// device becuase it is so simple, but this is not usually the case in
		// more complicated devices.
		//
		m_snifferTrace(packet);
		m_promiscSnifferTrace(packet);
		m_phyRxEndTrace(packet);

		//
		// Strip off the point-to-point protocol header and forward this packet
		// up the protocol stack.  Since this is a simple point-to-point link,
		// there is no difference in what the promisc callback sees and what the
		// normal receive callback sees.
		//
		ProcessHeader(packet, protocol);

		if (!m_promiscCallback.IsNull()) {
			m_macPromiscRxTrace(packet);
			m_promiscCallback(this, packet, protocol, GetRemote(), GetAddress(),
					NetDevice::PACKET_HOST);
		}

		m_macRxTrace(packet);

		/*----------------------------------Chunzhi------------------------------*/
//		std::cout << std::endl;
		Ptr<Node> node = this->GetNode();
		uint32_t nDevices = node->GetNDevices();

		Ipv4Header ipHeader;
		packet->PeekHeader(ipHeader);
		uint8_t upper_protocol = ipHeader.GetProtocol();
		uint8_t p_UDP = 17, p_TCP = 6; // TCP=6; UDP=17
		if (upper_protocol != p_UDP && upper_protocol != p_TCP) { // is not TCP(=6) or UDP(=17) packet
			m_rxCallback(this, packet, protocol, GetRemote());

			return;
		}

		bool isL3Forwarding = false;

		if (isL3Forwarding) { // use default layer 3 (IP) forwarding for TCP/UDP packets.
			m_rxCallback(this, packet, protocol, GetRemote());
			return;
		}

		NS_ASSERT(upper_protocol == p_UDP || upper_protocol == p_TCP);

		SrcIdTag srcIdTag;
		packet->PeekPacketTag(srcIdTag);
		DstIdTag dstIdTag;
		packet->PeekPacketTag(dstIdTag);
		TurningIdTag turningIdTag;
		packet->PeekPacketTag(turningIdTag);

		// Server node has two NetDevices, one for self-loop and the other for communication with switch.
		bool nodeIsServer = nDevices == 2;
		bool isDeliverUp = nodeIsServer
				&& dstIdTag.toString() == node->nodeId_FatTree.toString();
//		std::cout << "dst ip:" << ipHeader.GetDestination() << std::endl;
//		std::cout << "src node tag: ";
//		srcIdTag.Print(std::cout);
//		std::cout << "dst node tag: ";
//		dstIdTag.Print(std::cout);
//		std::cout << "turing switch node tag: ";
//		turningIdTag.Print(std::cout);
//		std::string nodeType = nodeIsServer ? "server" : "switch";
//		std::cout << "current " << nodeType << " tag:";
//		node->nodeId_FatTree.Print(std::cout);

		if (!isDeliverUp) { // forward to next hop

			//std::cout << "forward" << std::endl;
			if (nDevices == 2) {
				std::cout << "destination server, expected = "
						<< dstIdTag.toString() << ", got = "
						<< node->nodeId_FatTree.toString() << std::endl;
				assert(false);
			}

			uint32_t oifIndex = Forwarding_FatTree(packet, this->GetIfIndex());

//			ns3::NodeId nodeIdFatTree = this->GetNode()->nodeId_FatTree;
//			uint32_t normalOif = NormalForwarding_FatTree(nodeIdFatTree,
//					dstIdTag, srcIdTag, turningIdTag);
//			if (IsForwarding_FatTree(nodeIdFatTree, dstIdTag,
//					this->GetIfIndex())
//					&& IsNotFailure_FatTree(nodeIdFatTree, normalOif)) {
//				NS_ASSERT(node->GetDevice(oifIndex) != this);
//			}

			Address useless;
			PointToPointNetDevice* oif =
					dynamic_cast<PointToPointNetDevice*>(&(*node->GetDevice(
							oifIndex)));

			oif->Send(packet, useless, protocol);

			//			std::cout << "forward done" << std::endl;
		}/*-----------------------------------------------------------------------*/

		else { // deliver to local upper layer
//			std::cout << "deliver" << std::endl;

			// Comment by Chunzhi: PointToPointNetDevice::m_rxCallback is linked to Ipv4L3Protocol::Receive or Ipv6L3Protocol::Receive
			m_rxCallback(this, packet, protocol, GetRemote());
		}
	}
}

// add by Chunzhi
std::string i2s(uint i) {
	std::string s;
	std::stringstream out;
	out << i;
	s = out.str();
	return s;
}

/*-------------------------------By Zhiyong-----------------------------------------------------*/
uint32_t PointToPointNetDevice::NormalForwarding_FatTree(NodeId nodeId,
		IdTag dstId, IdTag turningId, uint iif) { // judge the upstream and downstream by dstId and srcId instead of iif.

	uint32_t oif = 0;  // output interface?  iif=input interface?

	/*---------------------------algorithm corrected by Chunzhi-------------------------------------*/
	/*-----------------------------Simplified by Zhang Xin------------------------------------*/
	if (nodeId.id_level == turningId.id_level) { // Start to perform the switching at the turning switch, which could be happened at any switch level
		switch (nodeId.id_level) {
		case 0: // core switch
			oif = dstId.id_pod + 1;
			break;
		case 1: // aggregate switch
			oif = dstId.id_switch + 1;
			break;
		case 2: // edge switch
			oif = dstId.id_level + 1; //  End system's id_level is the ID of the device rather than the level.
			break;
		default:
			std::cout
					<< "error in point-to point-net-device.h->PointToPointNetDevice::NormalForwarding_FatTree\n";
			break;
		}
		return oif;
	}

	assert(nodeId.id_level > turningId.id_level); //packet arrived at aggregate switches or edge switches

	if (iif >= 1 && iif <= Port_num / 2) { // upstream routing. iif <= Port_num / 2
		uint offset = 0;
		switch (nodeId.id_level) {
		case 2:
			offset = turningId.id_pod;
			break;
		case 1:
			offset = turningId.id_switch;
			break;
		default:
			std::cout
					<< "Algorithm wrong in point-to-point-net-device.cc NormalForwarding_FatTree \n";
		}
		oif = offset + 1 + Port_num / 2;
		return oif;
	}

	// downstram iif > Port_num / 2
	switch (nodeId.id_level) {
	case 1:
		oif = dstId.id_switch + 1;
		break;
	case 2:
		oif = dstId.id_level + 1;
		break;
	default:
		std::cout
				<< "error in point-to point-net-device.h->PointToPointNetDevice::NormalForwarding_FatTree\n";
		break;
	}

	assert(1 <= oif && oif <= Port_num);
	return oif;
}
/*-----------------------------------------------------------------------------------*/
//
//	if (nodeId.id_level == turningId.id_level) { // Start to perform downstream routing.
//		switch (nodeId.id_level) {
//		case 0:
//			oif = dstId.id_pod + 1;
//			break;
//		case 1:
//			oif = dstId.id_switch + 1;
//			break;
//		case 2:
//			oif = dstId.id_level + 1;
//			break;
//		default:
//			std::cout
//					<< "error in point-to point-net-device.h->PointToPointNetDevice::NormalForwarding_FatTree\n";
//			break;
//		}
//	} else {
//		assert(nodeId.id_level > turningId.id_level);
//		if (nodeId.id_pod == srcId.id_pod) { // upstream routing. iif <= Port_num / 2
//			switch (nodeId.id_level) {
//			case 1:
//				if (nodeId.id_pod < Port_num / 2) {
//					int temp = turningId.id_pod * (Port_num / 2)
//							+ turningId.id_switch
//							- nodeId.id_pod % (Port_num / 2);
//					if (temp >= 0)
//						oif = temp % (Port_num / 2) + Port_num / 2 + 1;
//					else
//						oif = temp % (Port_num / 2) + Port_num + 1;
//				} else {
//					int temp = turningId.id_pod
//							- nodeId.id_pod % (Port_num / 2);
//					if (temp >= 0) {
//						oif = temp + Port_num / 2 + 1;
//					} else {
//						oif = temp + Port_num + 1;
//					}
//				}
//				break;
//			case 2:
//				if (nodeId.id_pod < Port_num / 2) {
//					int temp = turningId.id_pod * (Port_num / 2)
//							+ turningId.id_switch - nodeId.id_pod;
//					if (temp >= 0)
//						oif = temp / (Port_num / 2) + Port_num / 2 + 1;
//					else
//						oif = Port_num;
//				} else {
//					int temp = turningId.id_pod
//							- nodeId.id_pod % (Port_num / 2);
//					if (temp >= 0) {
//						oif = turningId.id_switch + Port_num / 2 + 1;
//					} else {
//						if (turningId.id_switch == 0) {
//							oif = Port_num;
//						} else {
//							oif = turningId.id_switch + Port_num / 2;
//						}
//					}
//
//				}
//				break;
//			default:
//				std::cout
//						<< "error in point-to point-net-device.h->PointToPointNetDevice::NormalForwarding_FatTree\n";
//				break;
//			}
//		} else if (nodeId.id_pod == dstId.id_pod) { // downstram iif > Port_num / 2
//			switch (nodeId.id_level) {
//			case 1:
//				oif = dstId.id_switch + 1;
//				break;
//			case 2:
//				oif = dstId.id_level + 1;
//				break;
//			default:
//				std::cout
//						<< "error in point-to point-net-device.h->PointToPointNetDevice::NormalForwarding_FatTree\n";
//				break;
//			}
//		} else {
//			std::cout
//					<< " forwarding aglorithm error!!!  in the Point to Point Net Device"
//					<< std::endl;
//		}
//	}
bool PointToPointNetDevice::IsForwarding_FatTree(NodeId nodeId, IdTag dstId,
		uint32_t iif) {
	if (nodeId.id_level == 0) { // packet arriving at core switch
		if ((iif - 1) == dstId.id_pod) { // packet comes in from the pre-designed output port, it is a backtracking packet.
			return false;
		} else
			return true;
	}
	assert(nodeId.id_level == 1 || nodeId.id_level == 2);
	if ((iif > Port_num / 2) && (nodeId.id_pod != dstId.id_pod)) //packets comes from the upper link which arrived the wrong destination pod
		return false;
	else
		return true;
}

bool PointToPointNetDevice::IsNotFailure_FatTree(NodeId nodeId,
		uint32_t normalOif) {
	std::string failNodeKey = nodeId.toString();
	if (FailNode_oif_map.find(failNodeKey) != FailNode_oif_map.end()
			&& (FailNode_oif_map[failNodeKey] == normalOif)) {
		return false;
	}
	return true;
}

/**
 * Check if the switch can reach the given destination server
 */
bool PointToPointNetDevice::IsDestReachable(NodeId switchId, IdTag dstId) {
	if (switchId.id_level == 0) { // a core (level-0) switch can reach any destination server.
		return true;
	}

	if (switchId.id_level == 1) {
		if (switchId.id_pod == dstId.id_pod) {
			return true;
		} else {
			return false;
		}
	}

	if (switchId.id_level == 2) {
		if (switchId.id_pod == dstId.id_pod
				&& switchId.id_switch == dstId.id_switch) {
			return true;
		} else {
			return false;
		}
	}

	// invalid id_level.
	assert(false);
	return false;
}

/**
 * When the normal forwarding port fails, try to use another port to do the forwarding.
 */
uint32_t PointToPointNetDevice::FindRecoveryPort(uint32_t normal_oif) {
	uint oif = 0;

	assert(normal_oif >= 1 && normal_oif <= Port_num);

	if (normal_oif >= 1 && normal_oif <= Port_num / 2) { // downrouting port
		oif = normal_oif + 1;
		if (oif > Port_num / 2) {
			oif = 1;
		}
		assert(oif >= 1 && oif <= Port_num / 2);
	} else { // uprouting port
		oif = normal_oif + 1;
		if (oif > Port_num) {
			//oif = Port_num / 2;
			oif = Port_num / 2 + 1; //edit by Zhang Xin
		}
		assert(oif >= Port_num / 2 + 1 && oif <= Port_num);
	}

	assert(oif != normal_oif);
	return oif;
}

void PointToPointNetDevice::UpdateTurningSW(Ptr<Packet> packet, uint oif) {
	SrcIdTag srcId;
	packet->PeekPacketTag(srcId);
	DstIdTag dstId;
	packet->PeekPacketTag(dstId);
	TurningIdTag original_turningId;
	packet->RemovePacketTag(original_turningId);
	NodeId nodeId = this->GetNode()->nodeId_FatTree;

	TurningIdTag newTurningId(original_turningId.id_pod,
			original_turningId.id_switch, original_turningId.id_level);

	assert(nodeId.id_level > original_turningId.id_level);

	/*	if (isLRD) {
	 // use a local level-1 switch as the turning switch for re-routing
	 if (nodeId.id_level == 2) {
	 newTurningId.id_level = nodeId.id_level - 1;
	 newTurningId.id_pod = nodeId.id_pod;
	 newTurningId.id_switch = nodeId.id_switch;
	 }
	 packet->AddPacketTag(newTurningId);
	 return;
	 }*/

	if (original_turningId.id_level == 0) {
		if (nodeId.id_level == 1) {
			newTurningId.id_switch = oif - Port_num / 2 - 1;
		} else if (nodeId.id_level == 2) {
			newTurningId.id_pod = oif - Port_num / 2 - 1;
		}
		packet->AddPacketTag(newTurningId);
//		std::cout << "Chunzhi says original truning switch= "
//				<< original_turningId.toString() << ",  new turning switch="
//				<< newTurningId.toString() << std::endl;
		return;
	}

	assert(original_turningId.id_level == 1);
	newTurningId.id_switch = oif - Port_num / 2 - 1;

//	std::cout << "Chunzhi says original truning switch= "
//			<< original_turningId.toString() << ",  new turning switch="
//			<< newTurningId.toString() << std::endl;

	assert(newTurningId.id_level == original_turningId.id_level);
	assert(
			(newTurningId.id_pod != original_turningId.id_pod)
					|| (newTurningId.id_switch != original_turningId.id_switch));
	packet->AddPacketTag(newTurningId);
	return;
}

uint32_t PointToPointNetDevice::Forwarding_FatTree(Ptr<Packet> packet,
		uint32_t iif) {
	assert(iif >= 1 && iif <= Port_num);
	uint32_t oif = 0;
	SrcIdTag srcId;
	if (!packet->PeekPacketTag(srcId)) {
		std::cout << "packet lacks srcId tag";
		return 0;
	}
	DstIdTag dstId;
	if (!packet->PeekPacketTag(dstId)) {
		std::cout << "packet lacks dstId tag";
		return 0;
	}
	TurningIdTag turningId;
	if (!packet->PeekPacketTag(turningId)) {
		std::cout << "packet lacks trurningId tag";
		return 0;
	}
	NodeId nodeId = this->GetNode()->nodeId_FatTree;

	if (nodeId.id_level == turningId.id_level
			&& nodeId.toString() != turningId.toString()) { // Raise error for incorrectly routing.
		std::cout << "Wrong turning switch!!! Expected = "
				<< turningId.toString() << ", Got = " << nodeId.toString()
				<< ", for flow: " << srcId.toString() << "->"
				<< dstId.toString() << std::endl;
		assert(false);
		return 0;
	}

	/* *
	 * Down link is unique in a fat-tree topology.
	 * Therefore the turning switch with the destination can identify if this down link go through
	 * the failure link.
	 * */
	std::string reRoutingKey = dstId.toString() + turningId.toString();
	if (reRoutingKey.size() != 6) {
		std::cout << "reRoutingKey=" << reRoutingKey << ", turningId = "
				<< i2s(turningId.id_pod) << i2s(turningId.id_switch)
				<< i2s(turningId.id_level) << std::endl;
		std::cout << i2s(0xffffffff) << std::endl;
	}
	// Must use pointer to refer to the SAME map! Otherwise it's just a copy of the original map.
	std::map<std::string, uint32_t>* reRoutingMap =
			&this->GetNode()->reRoutingMap;

	/**************************** Start Edition by Zhang Xin   ***************************/
	bool isForwarding = IsForwarding_FatTree(nodeId, dstId, iif);

	if (isForwarding) { // Choose the oif according to either the re-routing pattern or routing pattern
		if (reRoutingMap->find(reRoutingKey) != reRoutingMap->end()) { //Following the existing reRouting pattern
			oif = (*reRoutingMap)[reRoutingKey];
			if (oif > Port_num / 2) { // add by Chunzhi: change TurningSwitch only for uprouting packets
				UpdateTurningSW(packet, oif);
			}
//			std::cout << "Reroute Map forwarding for rerouteKey= "
//					<< reRoutingKey << ", at switch=" << nodeId.toString()
//					<< ", port=" << oif << std::endl;
			return oif; // by Chunzhi
		} else { //Following the normal routing pattern
			/*-----------------------------------Chunzhi---------------------------------------------------------------------*/
			if (isLRD && nodeId.id_pod == dstId.id_pod && nodeId.id_level == 1
					&& iif >= 1 && iif <= Port_num / 2) { // Only for LRD algorithm. In the destination pod, the aggregation-level turning switch used to bypass failure.
				assert(IsDestReachable(nodeId, dstId));
				oif = dstId.id_switch + 1;
			}
			/*------------------------------------------------------------------------------------------------------------------*/
			else {
				oif = NormalForwarding_FatTree(nodeId, dstId, turningId, iif);

//				std::cout << "Chunzhi says original truning switch= "
//						<< turningId.toString() << std::endl;
			}
		}
	}

	bool isNormal = isForwarding && IsNotFailure_FatTree(nodeId, oif);

	/*---------Chunzhi----------------------*/
	if ((nodeId.id_level == 1 || nodeId.id_level == 2)
			&& nodeId.id_pod == dstId.id_pod) {
		if (iif > Port_num / 2) { //downrouting
			isNormal = isNormal && IsDestReachable(nodeId, dstId); // exclude backtracking/return packets
		}
	}
	/*--------------------------------------------*/

	// commented by Chunzhi
//	if (nodeId.id_pod == dstId.id_pod)
//		isNormal = isNormal && IsDestReachable(nodeId, dstId);
	if (isNormal) {
//		std::cout << "Normal forwarding: " << " switch=" << nodeId.toString()
//				<< ", port=" << oif << std::endl;
		return oif;
	} else {
		std::cout << "Chunzhi says abnormal forwarding: " << " switch="
				<< nodeId.toString() << ", port=" << oif << std::endl;
	}

	/*
	 * For link failures and redirected packets (v-turn or backtracking).
	 * */
	if (isLRD) { //Below is the LRD algorithm. We assume no failure link exists between core and aggregate
		/*
		 * In LRD all the packet are forwarding packets, so error can only occur while link failure
		 * happens or destination is not reachable.
		 * */
		if (1 == nodeId.id_level) { //V-turn to another access switch
			assert(!IsNotFailure_FatTree(nodeId, oif)); //error must incurred by link failure.
			oif = FindRecoveryPort(oif);
			(*reRoutingMap)[reRoutingKey] = oif;
			std::cout << "Local rerouted packet for LRD. Add reroute path: "
					<< reRoutingKey << "->" << oif << ", at level-"
					<< nodeId.id_level << " switch: " << nodeId.toString()
					<< std::endl;
			return oif;
		}
		if (2 == nodeId.id_level && iif <= Port_num / 2) { //Choose another upstream path
			assert(!IsNotFailure_FatTree(nodeId, oif)); //error must incurred by link failure.
			oif = FindRecoveryPort(oif);
			(*reRoutingMap)[reRoutingKey] = oif;
			UpdateTurningSW(packet, oif); // update the turning switch tag
			std::cout << "Local rerouted packet for LRD. Add reroute path: "
					<< reRoutingKey << "->" << oif << ", at level-"
					<< nodeId.id_level << " switch: " << nodeId.toString()
					<< std::endl;
			return oif;
		}
		if (2 == nodeId.id_level && iif > Port_num / 2) { //Send packets to another aggregate switch
			assert(!IsDestReachable(nodeId, dstId)); //packet can reach the destination due to the V-turn
			oif = FindRecoveryPort(iif);
			(*reRoutingMap)[reRoutingKey] = oif;

			/*---------------------Chunzhi---------------------*/
			if (srcId.id_pod == dstId.id_pod) { // 4-hop flow
				UpdateTurningSW(packet, oif); // update the turning switch tag
			}
			/*------------------------------------------------------*/

			std::cout << "Local rerouted packet for LRD. Add reroute path: "
					<< reRoutingKey << "->" << oif << ", at level-"
					<< nodeId.id_level << " switch: " << nodeId.toString()
					<< std::endl;
			return oif;
		}
	}

	if (!isLRD) { //Below is the LRB algorithm.
		/***********************************************************************
		 *****     For all the level 0 (core) switches          ***********
		 ***********************************************************************/
		if (0 == nodeId.id_level) {
			//packet->PeekPacketTag(isbackId1);
			//packet->RemovePacketTag(isbackId2);
			//packet->AddPacketTag(isbackId2);
			if (isForwarding) { //return this packet
				assert(!IsNotFailure_FatTree(nodeId, oif)); // error must incurred by link failure
				oif = iif;
				std::cout << "chunzhi says: U-turn from id_level = "
						<< nodeId.id_level << "; oif=" << oif << std::endl;
			} else { //keep backtracking
				oif = NormalForwarding_FatTree(nodeId, srcId, turningId, iif);
				std::cout << "chunzhi says: keep backtracking from id_level = "
						<< nodeId.id_level << "; oif=" << oif << std::endl;
			}

			return oif;
		}
		/***********************************************************************
		 *****     For all the level 1 (aggregate) switches          ***********
		 ***********************************************************************/
		/*		if (1==nodeId.id_level && ! isForwarding){ //keep backtracking

		 oif = NormalForwarding_FatTree(nodeId, srcId, turningId, iif);
		 return oif;
		 }
		 */
		if (1 == nodeId.id_level && iif > Port_num / 2) {
			if (!isForwarding) { //keep backtracking.
				/*
				 * We don't consider efficient Fat-Tree at this stage
				 * */
				oif = NormalForwarding_FatTree(nodeId, srcId, turningId, iif);
				std::cout << "chunzhi says: keep backtracking from node = "
						<< nodeId.toString() << "; oif=" << oif << std::endl;
				return oif;
			} else { // return this packet
				assert(!IsNotFailure_FatTree(nodeId, oif)); // error must be incurred by link failure
				oif = iif;
				std::cout << "chunzhi says: U-turn from node = "
						<< nodeId.toString() << "; oif=" << oif << std::endl;
				return oif;
			}
		}
		if (1 == nodeId.id_level && iif <= Port_num / 2) {
			assert(!IsNotFailure_FatTree(nodeId, oif)); // error must be incurred by link failure

			/*-----------------------------------------------Chunzhi------------------------------------------*/
			if (srcId.id_pod == dstId.id_pod) { // 4-hop flow, return this packet
				oif = iif;
				std::cout << "chunzhi says: U-turn from node = "
						<< nodeId.toString() << "; oif=" << oif << std::endl;
			}
			/*-----------------------------------------------------------------------------------------------------*/

			else { // 6-hop flow, choose another upstream path
				assert(srcId.id_pod != dstId.id_pod);
				oif = FindRecoveryPort(oif);
				UpdateTurningSW(packet, oif);
				(*reRoutingMap)[reRoutingKey] = oif; // add by Chunzhi
				std::cout << "chunzhi says: Recovery Path from node = "
						<< nodeId.toString() << "; oif=" << oif << std::endl;
			}
			return oif;
		}
		/***********************************************************************
		 *****     For all the level 2 (access) switches          ***********
		 ***********************************************************************/
		if (2 == nodeId.id_level && iif > Port_num / 2) {
			if (srcId.id_pod != dstId.id_pod) { // add by Chunzhi
				assert(!isForwarding); // We assume no failure exists in links connected to servers.
			}

			// Commented by Chunzhi: the reRoutingKey may not exist.
			/*if (iif != (*reRoutingMap)[reRoutingKey]) { //Following the existing re-routing pattern
			 oif = (*reRoutingMap)[reRoutingKey];
			 return oif;
			 }*/

			/*-------------------Chunzhi---------------------------*/
			if (reRoutingMap->find(reRoutingKey) != reRoutingMap->end()) { //Following the existing reRouting pattern
				oif = (*reRoutingMap).at(reRoutingKey);
				assert(iif != oif);
				UpdateTurningSW(packet, oif);
				std::cout << "chunzhi says: Recovery Path from node = "
						<< nodeId.toString() << "; oif=" << oif << std::endl;
				return oif;
			}
			/*----------------------------------------------------------*/

			// Create new re-routing pattern
			oif = FindRecoveryPort(iif);
			UpdateTurningSW(packet, oif);
			(*reRoutingMap)[reRoutingKey] = oif;
			std::cout << "chunzhi says: Recovery Path from node = "
					<< nodeId.toString() << "; oif=" << oif << std::endl;
			return oif;
		}
		if (2 == nodeId.id_level && iif <= Port_num / 2) { // Find another upstream path
			assert(!IsNotFailure_FatTree(nodeId, oif));
			oif = FindRecoveryPort(oif);
			UpdateTurningSW(packet, oif);
			(*reRoutingMap)[reRoutingKey] = oif;

			std::cout << "chunzhi says: Recovery Path from node = "
					<< nodeId.toString() << "; oif=" << oif << std::endl;
			return oif;
		}
	}
	return oif;

	/***************************************Edition by Zhang Xin End*******************************/
	/*if (reRoutingMap->find(reRoutingKey) != reRoutingMap->end())// Following the existing reRoutingMap to direct following packets
	 {
	 oif = (*reRoutingMap)[reRoutingKey];
	 UpdateTurningSW(packet, oif);
	 return oif;at
	 }

	 uint32_t normalOif=0;
	 bool isForwarding = IsForwarding_FatTree(nodeId, dstId, iif);
	 if (isForwarding){ //Following the normal routing path
	 normalOif = NormalForwarding_FatTree(nodeId, dstId, turningId,iif);// calculate the OIF without reRouting
	 if (!isLRD)
	 assert(iif != normalOif);
	 }


	 bool isNormal;
	 if (nodeId.id_pod == dstId.id_pod) { // in destination server pod
	 isNormal = isForwarding && IsNotFailure_FatTree(nodeId, normalOif) && IsDestReachable(nodeId, dstId);
	 } else {
	 isNormal = isForwarding && IsNotFailure_FatTree(nodeId, normalOif);
	 }

	 if (isNormal) {
	 assert(oif != iif);
	 return normalOif;
	 }

	 // backtracking or detect new port/link failure at current switch/node
	 //		nodeId.Print(std::cout);
	 IsBackTag isbackId1;
	 IsBackTag isbackId2 = IsBackTag(1);
	 IsBackTag isbackId3;
	 //	std::cout << "backward\n";

	 if (0==nodeId.id_level){
	 assert(!isLRD);//in LRD, we assume no failure in core switches.
	 packet->PeekPacketTag(isbackId1);
	 packet->RemovePacketTag(isbackId2);
	 packet->AddPacketTag(isbackId2);

	 if (isForwarding && !IsNotFailure_FatTree(nodeId,oif)){ //send back the packet from the input port
	 return iif;
	 }
	 else // keep backtracking it.
	 return NormalForwarding_FatTree(nodeId,srcId,turningId,iif);
	 }

	 if (1==nodeId.id_level){
	 //all the LRD packet is forwarding packet?
	 if (isLRD){
	 oif=FindRecoveryPort(oif);
	 (*reRoutingMap)[reRoutingKey]=oif;
	 std::cout << "LRD in downrouting path. Add reroute path: "
	 << reRoutingKey << "->" << oif << ", at level-"
	 << nodeId.id_level << " switch: "
	 << nodeId.toString() << std::endl;
	 }
	 }*/
	/*
	 switch (nodeId.id_level) {
	 case 0:
	 // In downrouting path, for LRD, we do not consider port/link failure at level-0 (core) switch.
	 assert(!isLRD);

	 if (isForwarding) { // just go back to the source.
	 oif = iif;
	 } else { // this is a backtracked packet, keep backtracking it.
	 oif = NormalForwarding_FatTree(nodeId, srcId, turningId, iif);
	 }
	 std::cout << "switch=" << nodeId.toString() << ", oif=" << oif
	 << ", normalOif=" << normalOif << ", iif=" << iif
	 << std::endl;

	 packet->PeekPacketTag(isbackId1);
	 //			std::cout<<"old isbackId = "<<isbackId1.isBack;
	 packet->RemovePacketTag(isbackId1);

	 packet->AddPacketTag(isbackId2);
	 //			std::cout<<" and the previous isbackId3 = "<<isbackId3.isBack;
	 packet->PeekPacketTag(isbackId3);
	 //			std::cout<<"; And the new isbackId = "<<isbackId3.isBack<<std::endl;;
	 break;

	 case 1:

	 if (nodeId.id_pod == dstId.id_pod) { // in destination pod.
	 // This must be a failure in downstream path.
	 assert(!IsNotFailure_FatTree(nodeId, normalOif));

	 if (isLRD) {
	 // use another downrouting port.
	 oif = FindRecoveryPort(normalOif);

	 // This is a V-turn switch.
	 // Record the reroute path for routing subsequent packets.
	 (*reRoutingMap)[reRoutingKey] = oif;
	 std::cout << "LRD in downrouting path. Add reroute path: "
	 << reRoutingKey << "->" << oif << ", at level-"
	 << nodeId.id_level << " switch: "
	 << nodeId.toString() << std::endl;
	 } else {
	 // start to backtrack, just go back
	 oif = iif;
	 //					oif = NormalForwarding_FatTree(nodeId, srcId, dstId,
	 //							turningId); // backtracking
	 std::cout << "backtracking at switch " << nodeId.toString()
	 << ", from port " << normalOif << " to " << oif
	 << std::endl;
	 }
	 } else { // in source pod
	 assert(nodeId.id_pod == srcId.id_pod);
	 if (iif <= Port_num / 2) { // failure in upstream path
	 assert(!IsNotFailure_FatTree(nodeId, normalOif));

	 // use another uprouting port.
	 oif = FindRecoveryPort(normalOif);

	 // This is a V-turn switch.
	 // Record the reroute path for routing subsequent packets.
	 UpdateTurningSW(packet, oif);
	 (*reRoutingMap)[reRoutingKey] = oif;
	 std::cout << "Add reroute path: " << reRoutingKey << "->"
	 << oif << ", at level-" << nodeId.id_level
	 << " switch: " << nodeId.toString() << std::endl;

	 } else { // backtracked packet
	 if (isEfficientFatTree) { // For efficient fat-tree, the V-turn switch is a level-1 switch.
	 if (iif != Port_num / 2 + 1) {
	 oif = Port_num / 2 + 1;
	 } else {
	 oif = Port_num;
	 }

	 // This is a V-turn switch.
	 // Record the reroute path for routing subsequent packets.
	 UpdateTurningSW(packet, oif);
	 (*reRoutingMap)[reRoutingKey] = oif;
	 std::cout << "Backtracked packet. Add reroute path: "
	 << reRoutingKey << "->" << oif << ", at level-"
	 << nodeId.id_level << " switch: "
	 << nodeId.toString() << std::endl;
	 }
	 // For regular fat-tree, keep backtracking the packet from this level-1 switch
	 // to the level-2 switch (the V-turn switch).
	 else {
	 // keep backtracking the packet.
	 oif = NormalForwarding_FatTree(nodeId, srcId, turningId,
	 iif); // backtracking;
	 //						oif = NormalForwarding_FatTree(nodeId, srcId, dstId,
	 //								turningId); // backtracking
	 }
	 }
	 }

	 break;
	 case 2:
	 if (nodeId.id_pod == dstId.id_pod) { // LRD V-turn switch
	 assert(isLRD);
	 assert(!IsDestReachable(nodeId, dstId));

	 // use another uprouting port (V-turn switch).
	 oif = FindRecoveryPort(iif);
	 assert(oif > Port_num / 2 && oif <= Port_num); // must be uprouting port.

	 (*reRoutingMap)[reRoutingKey] = oif;
	 std::cout << "Local rerouted packet for LRD. Add reroute path: "
	 << reRoutingKey << "->" << oif << ", at level-"
	 << nodeId.id_level << " switch: " << nodeId.toString()
	 << std::endl;
	 } else {
	 assert(nodeId.id_pod == srcId.id_pod);
	 if (iif <= Port_num / 2) { // failure in upstream path
	 std::cout << "nodeId=" << nodeId.toString() << ", port="
	 << normalOif << std::endl;
	 assert(!IsNotFailure_FatTree(nodeId, normalOif));

	 // use another uprouting port.
	 oif = FindRecoveryPort(normalOif);

	 // This is a V-turn switch.
	 // Record the reroute path for routing subsequent packets.
	 UpdateTurningSW(packet, oif);
	 (*reRoutingMap)[reRoutingKey] = oif;
	 std::cout << "Failure in source pod. Add reroute path: "
	 << reRoutingKey << "->" << oif << ", at level-"
	 << nodeId.id_level << " switch: "
	 << nodeId.toString() << std::endl;
	 } else { // backtracked packet
	 // use another uprouting port (V-turn switch).
	 oif = FindRecoveryPort(iif);
	 assert(oif > Port_num / 2 && oif <= Port_num);// must be uprouting port.

	 // This is a V-turn switch.
	 // Record the reroute path for routing subsequent packets.
	 UpdateTurningSW(packet, oif);
	 (*reRoutingMap)[reRoutingKey] = oif;
	 std::cout << "Backtracked packet. Add reroute path: "
	 << reRoutingKey << "->" << oif << ", at level-"
	 << nodeId.id_level << " switch: "
	 << nodeId.toString() << std::endl;
	 }
	 }

	 //				oif = NormalForwarding_FatTree(nodeId, dstId, srcId, turningId);
	 //				reRoutDistributor++;
	 //				if (reRoutDistributor == (Port_num / 2 - 1)) {
	 //					reRoutDistributor++;
	 //				}
	 //				oif = (oif + reRoutDistributor) % (Port_num / 2) + Port_num / 2
	 //						+ 1;
	 //				(*reRoutingMap)[reRoutingKey] = oif;
	 //				std::cout << "add to rerouting map in level 2 switch = "
	 //						<< std::endl;
	 //				for (std::map<std::string, uint32_t>::const_iterator it =
	 //						reRoutingMap->begin(); it != reRoutingMap->end();
	 //						++it) {
	 //					std::cout << "key=" << it->first << "; oif=" << it->second
	 //							<< "\n";
	 //				}
	 break;
	 default:
	 std::cout
	 << "error in point-to point-net-device.h->PointToPointNetDevice::Forwarding_FatTree\n";
	 break;
	 }

	 if (isForwarding) {
	 assert(oif != normalOif);
	 }
	 return oif;*/
}

/*-------------------------------------------------------------------------------------------*/

Ptr<Queue> PointToPointNetDevice::GetQueue(void) const {
	NS_LOG_FUNCTION_NOARGS();
	return m_queue;
}

void PointToPointNetDevice::NotifyLinkUp(void) {
	m_linkUp = true;
	m_linkChangeCallbacks();
}

void PointToPointNetDevice::SetIfIndex(const uint32_t index) {
	m_ifIndex = index;
}

uint32_t PointToPointNetDevice::GetIfIndex(void) const {
	return m_ifIndex;
}

Ptr<Channel> PointToPointNetDevice::GetChannel(void) const {
	return m_channel;
}

//
// This is a point-to-point device, so we really don't need any kind of address
// information.  However, the base class NetDevice wants us to define the
// methods to get and set the address.  Rather than be rude and assert, we let
// clients get and set the address, but simply ignore them.

void PointToPointNetDevice::SetAddress(Address address) {
	m_address = Mac48Address::ConvertFrom(address);
}

Address PointToPointNetDevice::GetAddress(void) const {
	return m_address;
}

bool PointToPointNetDevice::IsLinkUp(void) const {
	return m_linkUp;
}

void PointToPointNetDevice::AddLinkChangeCallback(Callback<void> callback) {
	m_linkChangeCallbacks.ConnectWithoutContext(callback);
}

//
// This is a point-to-point device, so every transmission is a broadcast to
// all of the devices on the network.
//

bool PointToPointNetDevice::IsBroadcast(void) const {
	return true;
}

//
// We don't really need any addressing information since this is a
// point-to-point device.  The base class NetDevice wants us to return a
// broadcast address, so we make up something reasonable.
//

Address PointToPointNetDevice::GetBroadcast(void) const {
	return Mac48Address("ff:ff:ff:ff:ff:ff");
}

bool PointToPointNetDevice::IsMulticast(void) const {
	return true;
}

Address PointToPointNetDevice::GetMulticast(Ipv4Address multicastGroup) const {
	return Mac48Address("01:00:5e:00:00:00");
}

Address PointToPointNetDevice::GetMulticast(Ipv6Address addr) const {
	NS_LOG_FUNCTION(this << addr);
	return Mac48Address("33:33:00:00:00:00");
}

bool PointToPointNetDevice::IsPointToPoint(void) const {
	return true;
}

bool PointToPointNetDevice::IsBridge(void) const {
	return false;
}

bool PointToPointNetDevice::Send(Ptr<Packet> packet, const Address &dest,
		uint16_t protocolNumber) {
	NS_LOG_FUNCTION_NOARGS();NS_LOG_LOGIC("p=" << packet << ", dest=" << &dest);NS_LOG_LOGIC("UID is " << packet->GetUid());

//
// If IsLinkUp() is false it means there is no channel to send any packet
// over so we just hit the drop trace on the packet and return an error.
//
	if (IsLinkUp() == false) {
		m_macTxDropTrace(packet);
		return false;
	}

//
// Stick a point to point protocol header on the packet in preparation for
// shoving it out the door.
//
	AddHeader(packet, protocolNumber);

	m_macTxTrace(packet);

//
// If there's a transmission in progress, we enque the packet for later
// transmission; otherwise we send it now.
//
	if (m_txMachineState == READY) {
		//
		// Even if the transmitter is immediately available, we still enqueue and
		// dequeue the packet to hit the tracing hooks.
		//
		if (m_queue->Enqueue(packet) == true) {
			packet = m_queue->Dequeue();
			m_snifferTrace(packet);
			m_promiscSnifferTrace(packet);
			return TransmitStart(packet);
		} else {
			// Enqueue may fail (overflow)
			m_macTxDropTrace(packet);
			return false;
		}
	} else {
		return m_queue->Enqueue(packet);
	}
}

bool PointToPointNetDevice::SendFrom(Ptr<Packet> packet, const Address &source,
		const Address &dest, uint16_t protocolNumber) {
	return false;
}

Ptr<Node> PointToPointNetDevice::GetNode(void) const {
	return m_node;
}

void PointToPointNetDevice::SetNode(Ptr<Node> node) {
	m_node = node;
}

bool PointToPointNetDevice::NeedsArp(void) const {
	return false;
}

void PointToPointNetDevice::SetReceiveCallback(NetDevice::ReceiveCallback cb) {
	m_rxCallback = cb;
}

void PointToPointNetDevice::SetPromiscReceiveCallback(
		NetDevice::PromiscReceiveCallback cb) {
	m_promiscCallback = cb;
}

bool PointToPointNetDevice::SupportsSendFrom(void) const {
	return false;
}

void PointToPointNetDevice::DoMpiReceive(Ptr<Packet> p) {
	Receive(p);
}

Address PointToPointNetDevice::GetRemote(void) const {
	NS_ASSERT(m_channel->GetNDevices() == 2);
	for (uint32_t i = 0; i < m_channel->GetNDevices(); ++i) {
		Ptr<NetDevice> tmp = m_channel->GetDevice(i);
		if (tmp != this) {
			return tmp->GetAddress();
		}
	}
	NS_ASSERT(false);
// quiet compiler.
	return Address();
}

bool PointToPointNetDevice::SetMtu(uint16_t mtu) {
	NS_LOG_FUNCTION(this << mtu);
	m_mtu = mtu;
	return true;
}

uint16_t PointToPointNetDevice::GetMtu(void) const {
	NS_LOG_FUNCTION_NOARGS();
	return m_mtu;
}

uint16_t PointToPointNetDevice::PppToEther(uint16_t proto) {
	switch (proto) {
	case 0x0021:
		return 0x0800; //IPv4
	case 0x0057:
		return 0x86DD; //IPv6
	default:
		NS_ASSERT_MSG(false, "PPP Protocol number not defined!");
	}
	return 0;
}

uint16_t PointToPointNetDevice::EtherToPpp(uint16_t proto) {
	switch (proto) {
	case 0x0800:
		return 0x0021; //IPv4
	case 0x86DD:
		return 0x0057; //IPv6
	default:
		NS_ASSERT_MSG(false, "PPP Protocol number not defined!");
	}
	return 0;
}

} // namespace ns3
