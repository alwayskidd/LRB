/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2007 University of Washington
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
#include "ns3/enum.h"
#include "ns3/uinteger.h"
#include "drop-tail-queue.h"
#include "ns3/node.h"
#include "ns3/simulator.h"
#include "ns3/boolean.h"

NS_LOG_COMPONENT_DEFINE("DropTailQueue");
double startTimeFatTree;
double stopTimeFatTree;
uint32_t selectedNode;
namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED(DropTailQueue);

TypeId DropTailQueue::GetTypeId(void) {
	static TypeId tid =
			TypeId("ns3::DropTailQueue").SetParent<Queue>().AddConstructor<
					DropTailQueue>().AddAttribute("Mode",
					"Whether to use bytes (see MaxBytes) or packets (see MaxPackets) as the maximum queue size metric.",
					EnumValue(QUEUE_MODE_PACKETS),
					MakeEnumAccessor(&DropTailQueue::SetMode),
					MakeEnumChecker(QUEUE_MODE_BYTES, "QUEUE_MODE_BYTES",
							QUEUE_MODE_PACKETS, "QUEUE_MODE_PACKETS")).AddAttribute(
					"MaxPackets",
					"The maximum number of packets accepted by this DropTailQueue.",
					UintegerValue(100),
					MakeUintegerAccessor(&DropTailQueue::m_maxPackets),
					MakeUintegerChecker<uint32_t>()).AddAttribute("MaxBytes",
					"The maximum number of bytes accepted by this DropTailQueue.",
					UintegerValue(100 * 65535),
					MakeUintegerAccessor(&DropTailQueue::m_maxBytes),
					MakeUintegerChecker<uint32_t>()).AddAttribute("TraceDrop",
					"Trace dropped packets (by Chunzhi).", UintegerValue(0),
					MakeUintegerAccessor(&DropTailQueue::m_enableDropTrace),
					MakeUintegerChecker<uint32_t>());

	return tid;
}

DropTailQueue::DropTailQueue() :
		Queue(), m_packets(), m_bytesInQueue(0) {
	NS_LOG_FUNCTION (this);
	/*---------------------By Zhiyong---------------------------------*/

	dropFor_18 = 0;
	dropFor_35 = 0;
	dropFor_20 = 0;
	dropFor_40 = 0;
	dropFor_21 = 0;
	inQueue_18 = 0;
	inQueue_35 = 0;
	inQueue_20 = 0;
	inQueue_40 = 0;
	inQueue_21 = 0;
	dropFor_18_129 = 0;
	dropFor_35_129 = 0;
	/*--------------------------------------------------------------*/
}

DropTailQueue::~DropTailQueue() {
	NS_LOG_FUNCTION (this);
}

void DropTailQueue::SetMode(DropTailQueue::QueueMode mode) {
	NS_LOG_FUNCTION (this << mode);
	m_mode = mode;
}

DropTailQueue::QueueMode DropTailQueue::GetMode(void) {
	NS_LOG_FUNCTION (this);
	return m_mode;
}
void DropTailQueue::PrintDropCount(void) {
//	std::cout << "called time: " << Simulator::Now().GetSeconds()
//			<< " DropCount print called" << std::endl;
	if (selectedNode == 167 || selectedNode == 171) {
		std::cout << "--------------Dropped-----1-----------------"
				<< std::endl;
		std::cout << "Dropped Packet for Flow 2 (18->5): " << dropFor_18
				<< std::endl;
		std::cout << "Dropped Packet for backGround Flow 2 (20->113): "
				<< dropFor_20 << std::endl;
		std::cout << "Dropped Packet for backGround2 Flow 2 (21->113): "
				<< dropFor_21 << std::endl;
		std::cout << "--------------Dropped-----2-----------------"
				<< std::endl;
		std::cout << "Dropped Packet for Flow 1 (35->5): " << dropFor_35
				<< std::endl;
		std::cout << "Dropped Packet for backGround Flow 1 (40->100): "
				<< dropFor_40 << std::endl;
		std::cout << "--------------In queued----1------------------"
				<< std::endl;
		std::cout << "In queued Packet for Flow 2 (18->5):  " << inQueue_18
				<< std::endl;
		std::cout << "In queued Packet for backGround Flow 2 (20->113): "
				<< inQueue_20 << std::endl;
		std::cout << "In queued Packet for backGround2 Flow 2 (21->113): "
				<< inQueue_21 << std::endl;
		std::cout << "--------------In queued-----2-----------------"
				<< std::endl;
		std::cout << "In queued Packet for Flow 1 (35->5): " << inQueue_35
				<< std::endl;
		std::cout << "In queued Packet for backGround Flow 1 (40->100): "
				<< inQueue_40 << std::endl;
	} else if (selectedNode == 129) {
		std::cout << "--------------Dropped----------------------"
				<< std::endl;
		std::cout << "Dropped Packet for Flow 2 (18->5): " << dropFor_18_129
				<< std::endl;
		std::cout << "Dropped Packet for Flow 1 (35->5): "
				<< dropFor_35_129 << std::endl;
	}
}
bool DropTailQueue::DoEnqueue(Ptr<Packet> p) {
	NS_LOG_FUNCTION (this << p);
	/*---------------------by Zhiyong--------------------*/
	if (m_enableDropTrace == 1 && !isScheduled) {
		std::cout << m_enableDropTrace << std::endl;
		std::cout << "called time: " << Simulator::Now().GetSeconds()
				<< " Schedule print called" << std::endl;
		Simulator::Schedule(Seconds(stopTimeFatTree),
				&DropTailQueue::PrintDropCount, this);
		isScheduled = true;
	}
	/*-------------------------------------------------*/
	if (m_mode == QUEUE_MODE_PACKETS && (m_packets.size() >= m_maxPackets)) {
		NS_LOG_LOGIC ("Queue full (at max packets) -- droppping pkt");
		Drop(p);
		/*-------------By Zhiyong----------------------------*/
		if (m_enableDropTrace == 1) {
			SrcIdTag node_src;
			p->PeekPacketTag(node_src);
			IsBackTag isback;
			p->PeekPacketTag(isback);
			if (node_src == NodeId(1, 0, 2)) {
				dropFor_18++;
			} else if (node_src == NodeId(1, 1, 0)) {
				dropFor_20++;
			} else if (node_src == NodeId(1, 1, 1)) {
				dropFor_21++;
			} else if (node_src == NodeId(2, 0, 3)) {
				dropFor_35++;
			} else if (node_src == NodeId(2, 2, 0)) {
				dropFor_40++;
			}
			switch (selectedNode) {
			case 167:
				if (node_src == NodeId(1, 0, 2)) {
					if (isback.isBack == 1) {
						std::cout << "Time:" << Simulator::Now().GetSeconds()
								<< " Flow2:(18->5)Revise" << " Drop"
								<< " QueueSize:" << m_bytesInQueue / 578
								<< std::endl;
					} else {
						std::cout << "Time:" << Simulator::Now().GetSeconds()
								<< " Flow2:(18->5)" << " Drop" << " QueueSize:"
								<< m_bytesInQueue << std::endl;
					}

				} else if (node_src == NodeId(1, 1, 0)) {
					std::cout << "Time:" << Simulator::Now().GetSeconds()
							<< " BackgroundFlow2:(20->113)" << " Drop"
							<< " QueueSize:" << m_bytesInQueue << std::endl;

				} else if (node_src == NodeId(1, 1, 1)) {
					std::cout << "Time:" << Simulator::Now().GetSeconds()
							<< " Background2Flow2:(21->113)" << " Drop"
							<< " QueueSize:" << m_bytesInQueue << std::endl;

				}
				break;
			case 171:
				if (node_src == NodeId(2, 0, 3)) {
					if (isback.isBack == 1) {
						std::cout << "Time:" << Simulator::Now().GetSeconds()
								<< " Flow1:(35->5)Revise" << " Drop"
								<< " QueueSize:" << m_bytesInQueue << std::endl;
					} else {
						std::cout << "Time:" << Simulator::Now().GetSeconds()
								<< " Flow1:(35->5)" << " Drop" << " QueueSize:"
								<< m_bytesInQueue << std::endl;
					}

				} else if (node_src == NodeId(2, 2, 0)) {
					std::cout << "Time:" << Simulator::Now().GetSeconds()
							<< " BackgroundFlow1:(40->100)" << " Drop"
							<< " QueueSize:" << m_bytesInQueue << std::endl;

				}
				break;
			case 129:
				if (node_src == NodeId(2, 0, 3)) {
					std::cout << "Time:" << Simulator::Now().GetSeconds()
							<< " Flow1:(35->5)" << " Drop" << " QueueSize:"
							<< m_bytesInQueue << std::endl;
					dropFor_35_129++;
				} else {
					std::cout << "Time:" << Simulator::Now().GetSeconds()
							<< " Flow2:(18->5)" << " Drop" << " QueueSize:"
							<< m_bytesInQueue << std::endl;
					dropFor_18_129++;
				}
				break;
			}
		}
		/*--------------------------------------------------*/
		return false;
	}

	if (m_mode == QUEUE_MODE_BYTES
			&& (m_bytesInQueue + p->GetSize() >= m_maxBytes)) {
		NS_LOG_LOGIC ("Queue full (packet would exceed max bytes) -- droppping pkt");
		Drop(p);
		/*-------------By Zhiyong----------------------------*/
		if (m_enableDropTrace == 1) {
			SrcIdTag node_src;
			p->PeekPacketTag(node_src);
			IsBackTag isback;
			p->PeekPacketTag(isback);
			if (node_src == NodeId(1, 0, 2)) {
				dropFor_18++;
			} else if (node_src == NodeId(1, 1, 0)) {
				dropFor_20++;
			} else if (node_src == NodeId(1, 1, 1)) {
				dropFor_21++;
			} else if (node_src == NodeId(2, 0, 3)) {
				dropFor_35++;
			} else if (node_src == NodeId(2, 2, 0)) {
				dropFor_40++;
			}
			switch (selectedNode) {
			case 167:
				if (node_src == NodeId(1, 0, 2)) {
					if (isback.isBack == 1) {
						std::cout << "Time:" << Simulator::Now().GetSeconds()
								<< " Flow2:(18->5)Revise" << " Drop"
								<< " QueueSize:" << m_bytesInQueue << std::endl;
					} else {
						std::cout << "Time:" << Simulator::Now().GetSeconds()
								<< " Flow2:(18->5)" << " Drop" << " QueueSize:"
								<< m_bytesInQueue << std::endl;
					}

				} else if (node_src == NodeId(1, 1, 0)) {
					std::cout << "Time:" << Simulator::Now().GetSeconds()
							<< " BackgroundFlow2:(20->113)" << " Drop"
							<< " QueueSize:" << m_bytesInQueue << std::endl;

				} else if (node_src == NodeId(1, 1, 1)) {
					std::cout << "Time:" << Simulator::Now().GetSeconds()
							<< " Background2Flow2:(21->113)" << " Drop"
							<< " QueueSize:" << m_bytesInQueue << std::endl;

				}
				break;
			case 171:
				if (node_src == NodeId(2, 0, 3)) {
					if (isback.isBack == 1) {
						std::cout << "Time:" << Simulator::Now().GetSeconds()
								<< " Flow1:(35->5)Revise" << " Drop"
								<< " QueueSize:" << m_bytesInQueue << std::endl;
					} else {
						std::cout << "Time:" << Simulator::Now().GetSeconds()
								<< " Flow1:(35->5)" << " Drop" << " QueueSize:"
								<< m_bytesInQueue << std::endl;
					}

				} else if (node_src == NodeId(2, 2, 0)) {
					std::cout << "Time:" << Simulator::Now().GetSeconds()
							<< " BackgroundFlow1:(40->100)" << " Drop"
							<< " QueueSize:" << m_bytesInQueue << std::endl;

				}
				break;
			case 129:
				if (node_src == NodeId(2, 0, 3)) {
					std::cout << "Time:" << Simulator::Now().GetSeconds()
							<< " Flow1:(35->5)" << " Drop" << " QueueSize:"
							<< m_bytesInQueue << std::endl;
					dropFor_35_129++;
				} else {
					std::cout << "Time:" << Simulator::Now().GetSeconds()
							<< " Flow2:(18->5)" << " Drop" << " QueueSize:"
							<< m_bytesInQueue << std::endl;
					dropFor_18_129++;
				}
				break;
			}
		}
		/*--------------------------------------------------*/
		return false;
	}

	m_bytesInQueue += p->GetSize();
	m_packets.push(p);
	/*-------------By Zhiyong----------------------------*/
	if (m_enableDropTrace == 1) {
		SrcIdTag node_src;
		p->PeekPacketTag(node_src);
		IsBackTag isback;
		p->PeekPacketTag(isback);
		if (node_src == NodeId(1, 0, 2)) {
			inQueue_18++;
		} else if (node_src == NodeId(1, 1, 0)) {
			inQueue_20++;
		} else if (node_src == NodeId(1, 1, 1)) {
			inQueue_21++;
		} else if (node_src == NodeId(2, 0, 3)) {
			inQueue_35++;
		} else if (node_src == NodeId(2, 2, 0)) {
			inQueue_40++;
		}
		switch (selectedNode) {
		case 167:
			if (node_src == NodeId(1, 0, 2)) {
				if (isback.isBack == 1) {
					std::cout << "Time:" << Simulator::Now().GetSeconds()
							<< " Flow2:(18->5)Revise" << " Enqueue"
							<< " QueueSize:" << m_bytesInQueue << std::endl;
				} else {
					std::cout << "Time:" << Simulator::Now().GetSeconds()
							<< " Flow2:(18->5)" << " Enqueue" << " QueueSize:"
							<< m_bytesInQueue << std::endl;
				}

			} else if (node_src == NodeId(1, 1, 0)) {
				std::cout << "Time:" << Simulator::Now().GetSeconds()
						<< " BackgroundFlow2:(20->113)" << " Enqueue"
						<< " QueueSize:" << m_bytesInQueue << std::endl;

			} else if (node_src == NodeId(1, 1, 1)) {
				std::cout << "Time:" << Simulator::Now().GetSeconds()
						<< " Background2Flow2:(21->113)" << " Enqueue"
						<< " QueueSize:" << m_bytesInQueue << std::endl;

			}
			break;
		case 171:
			if (node_src == NodeId(2, 0, 3)) {
				if (isback.isBack == 1) {
					std::cout << "Time:" << Simulator::Now().GetSeconds()
							<< " Flow1:(35->5)Revise" << " Enqueue"
							<< " QueueSize:" << m_bytesInQueue << std::endl;
				} else {
					std::cout << "Time:" << Simulator::Now().GetSeconds()
							<< " Flow1:(35->5)" << " Enqueue" << " QueueSize:"
							<< m_bytesInQueue << std::endl;
				}

			} else if (node_src == NodeId(2, 2, 0)) {
				std::cout << "Time:" << Simulator::Now().GetSeconds()
						<< " BackgroundFlow1:(40->100)" << " Enqueue"
						<< " QueueSize:" << m_bytesInQueue << std::endl;

			}
			break;
		case 129:
			if (node_src == NodeId(2, 0, 3)) {
				std::cout << "Time:" << Simulator::Now().GetSeconds()
						<< " Flow1:(35->5)" << " Enqueue" << " QueueSize:"
						<< m_bytesInQueue << std::endl;
			} else {
				std::cout << "Time:" << Simulator::Now().GetSeconds()
						<< " Flow2:(18->5)" << " Enqueue" << " QueueSize:"
						<< m_bytesInQueue << std::endl;
			}
			break;
		}
	}
	/*--------------------------------------------------*/
	NS_LOG_LOGIC ("Number packets " << m_packets.size ());NS_LOG_LOGIC ("Number bytes " << m_bytesInQueue);

	return true;
}

Ptr<Packet> DropTailQueue::DoDequeue(void) {
	NS_LOG_FUNCTION (this);

	if (m_packets.empty()) {
		NS_LOG_LOGIC ("Queue empty");
		return 0;
	}

	Ptr<Packet> p = m_packets.front();
	m_packets.pop();
	m_bytesInQueue -= p->GetSize();

	/*-------------By Zhiyong----------------------------*/
	if (m_enableDropTrace == 1) {
		SrcIdTag node_src;
		p->PeekPacketTag(node_src);
		switch (selectedNode) {
		case 167:
			if (node_src == NodeId(1, 0, 2)) {
				std::cout << "Time:" << Simulator::Now().GetSeconds()
						<< " Flow2:(18->5)" << " Dequeue" << " QueueSize:"
						<< m_bytesInQueue << std::endl;
			} else if (node_src == NodeId(1, 1, 0)) {
				std::cout << "Time:" << Simulator::Now().GetSeconds()
						<< " BackgroundFlow2:(20->113)" << " Dequeue"
						<< " QueueSize:" << m_bytesInQueue << std::endl;
			} else if (node_src == NodeId(1, 1, 1)) {
				std::cout << "Time:" << Simulator::Now().GetSeconds()
						<< " Background2Flow2:(21->113)" << " Dequeue"
						<< " QueueSize:" << m_bytesInQueue << std::endl;
			}
			break;
		case 171:
			if (node_src == NodeId(2, 0, 3)) {
				std::cout << "Time:" << Simulator::Now().GetSeconds()
						<< " Flow1:(35->5)" << " Dequeue" << " QueueSize:"
						<< m_bytesInQueue << std::endl;
			} else if (node_src == NodeId(2, 2, 0)) {
				std::cout << "Time:" << Simulator::Now().GetSeconds()
						<< " BackgroundFlow1:(40->100)" << " Dequeue"
						<< " QueueSize:" << m_bytesInQueue << std::endl;
			}
			break;
		case 129:
			if (node_src == NodeId(2, 0, 3)) {
				std::cout << "Time:" << Simulator::Now().GetSeconds()
						<< " Flow1:(35->5)" << " Dequeue" << " QueueSize:"
						<< m_bytesInQueue << std::endl;
			} else {
				std::cout << "Time:" << Simulator::Now().GetSeconds()
						<< " Flow2:(18->5)" << " Dequeue" << " QueueSize:"
						<< m_bytesInQueue << std::endl;
			}
			break;
		}
	}
	/*--------------------------------------------------*/

	NS_LOG_LOGIC ("Popped " << p);

	NS_LOG_LOGIC ("Number packets " << m_packets.size ());NS_LOG_LOGIC ("Number bytes " << m_bytesInQueue);

	return p;
}

Ptr<const Packet> DropTailQueue::DoPeek(void) const {
	NS_LOG_FUNCTION (this);

	if (m_packets.empty()) {
		NS_LOG_LOGIC ("Queue empty");
		return 0;
	}

	Ptr<Packet> p = m_packets.front();

	NS_LOG_LOGIC ("Number packets " << m_packets.size ());NS_LOG_LOGIC ("Number bytes " << m_bytesInQueue);

	return p;
}

} // namespace ns3

