/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2006 Georgia Tech Research Corporation, INRIA
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
 *
 * Authors: George F. Riley<riley@ece.gatech.edu>
 *          Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#include "node.h"
#include "node-list.h"
#include "net-device.h"
#include "application.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/object-vector.h"
#include "ns3/uinteger.h"
#include "ns3/log.h"
#include "ns3/assert.h"
#include "ns3/global-value.h"
#include "ns3/boolean.h"
#include "ns3/simulator.h"
#include <string>

NS_LOG_COMPONENT_DEFINE("Node");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED(Node);

/*--------------------------------------------Chunzhi-----------------------------------*/

// add by Chunzhi
std::string i2s(uint i) {
	std::string s;
	std::stringstream out;
	out << i;
	s = out.str();
	return s;
}

NodeId::NodeId() {
	id_pod = 0xffffffff;
	id_switch = 0xffffffff;
	id_level = 0xffffffff;
}

NodeId::NodeId(uint32_t id_pod, uint32_t id_switch, uint32_t id_level) {
	this->id_pod = id_pod;
	this->id_switch = id_switch;
	this->id_level = id_level;
}

bool NodeId::operator ==(const NodeId other) const {
	return id_pod == other.id_pod && id_switch == other.id_switch
			&& id_level == other.id_level;
}

void NodeId::Print(std::ostream &os) {
	os << "node tag id = " << id_pod << "," << id_switch << "," << id_level
			<< std::endl;
}

std::string NodeId::toString() {
	return i2s(id_pod) + i2s(id_switch) + i2s(id_level);
}

IdTag::IdTag() :
		NodeId() {
}

IdTag::IdTag(uint32_t id_pod, uint32_t id_switch, uint32_t id_level) :
		NodeId(id_pod, id_switch, id_level) {
}

TypeId IdTag::GetTypeId(void) {
	static TypeId tid = TypeId("ns3::IdTag").SetParent<Tag>().AddConstructor<
			IdTag>();
	return tid;
}
TypeId IdTag::GetInstanceTypeId(void) const {
	return GetTypeId();
}
uint32_t IdTag::GetSerializedSize(void) const {
	return sizeof(uint32_t) * 3;
}
void IdTag::Serialize(TagBuffer i) const {
	i.WriteU32(id_pod);
	i.WriteU32(id_switch);
	i.WriteU32(id_level);
}
void IdTag::Deserialize(TagBuffer i) {
	id_pod = i.ReadU32();
	id_switch = i.ReadU32();
	id_level = i.ReadU32();
}
void IdTag::Print(std::ostream &os) const {
	os << "node tag id = " << id_pod << "," << id_switch << "," << id_level
			<< std::endl;
}

TypeId SrcIdTag::GetTypeId(void) {
	static TypeId tid =
			TypeId("ns3::SrcIdTag").SetParent<IdTag>().AddConstructor<SrcIdTag>();
	return tid;
}

TypeId SrcIdTag::GetInstanceTypeId(void) const {
	return GetTypeId();
}

SrcIdTag::SrcIdTag() :
		IdTag() {
}

SrcIdTag::SrcIdTag(uint32_t id_pod, uint32_t id_switch, uint32_t id_level) :
		IdTag(id_pod, id_switch, id_level) {

}

DstIdTag::DstIdTag() :
		IdTag() {

}
DstIdTag::DstIdTag(uint32_t id_pod, uint32_t id_switch, uint32_t id_level) :
		IdTag(id_pod, id_switch, id_level) {
}

TypeId DstIdTag::GetTypeId(void) {
	static TypeId tid =
			TypeId("ns3::DstIdTag").SetParent<IdTag>().AddConstructor<DstIdTag>();
	return tid;
}

TypeId DstIdTag::GetInstanceTypeId(void) const {
	return GetTypeId();
}

TurningIdTag::TurningIdTag() :
		IdTag() {
}
TurningIdTag::TurningIdTag(uint32_t id_pod, uint32_t id_switch,
		uint32_t id_level) :
		IdTag(id_pod, id_switch, id_level) {
}

TypeId TurningIdTag::GetTypeId(void) {
	static TypeId tid =
			TypeId("ns3::TurningIdTag").SetParent<IdTag>().AddConstructor<
					TurningIdTag>();
	return tid;
}
TypeId TurningIdTag::GetInstanceTypeId(void) const {
	return GetTypeId();
}
/*----------------------------------------------------------------------------------------*/
/*------------------------------By Zhiyong------------------------------------------------*/
TimeStampTag::TimeStampTag() :
		Tag() {
	time = 1;
}
TimeStampTag::TimeStampTag(double time_) {
	this->time = time_;
}
TypeId TimeStampTag::GetTypeId(void) {
	static TypeId tid =
			TypeId("ns3::TimeStampTag").SetParent<Tag>().AddConstructor<
					TimeStampTag>();
	return tid;
}
TypeId TimeStampTag::GetInstanceTypeId(void) const {
	return GetTypeId();
}
uint32_t TimeStampTag::GetSerializedSize(void) const {
	return sizeof(double);
}
void TimeStampTag::Serialize(TagBuffer i) const {
	i.WriteDouble(time);
}
void TimeStampTag::Deserialize(TagBuffer i) {
	time = i.ReadDouble();
}
void TimeStampTag::Print(std::ostream &os) const {
	std::cout << "Start time is: " << time << std::endl;
}

PacketSumTag::PacketSumTag() :
		Tag() {
	delaySum = 1;
	nPkt = 0;
}
PacketSumTag::PacketSumTag(uint32_t nPkt_, double delaySum_) {
	this->delaySum = delaySum_;
	this->nPkt = nPkt_;
}
TypeId PacketSumTag::GetTypeId(void) {
	static TypeId tid =
			TypeId("ns3::PacketSumTag").SetParent<Tag>().AddConstructor<
					PacketSumTag>();
	return tid;
}
TypeId PacketSumTag::GetInstanceTypeId(void) const {
	return GetTypeId();
}
uint32_t PacketSumTag::GetSerializedSize(void) const {
	return sizeof(double) + sizeof(uint32_t);
}
void PacketSumTag::Serialize(TagBuffer i) const {
	i.WriteU32(nPkt);
	i.WriteDouble(delaySum);
}
void PacketSumTag::Deserialize(TagBuffer i) {
	nPkt = i.ReadU32();
	delaySum = i.ReadDouble();
}
void PacketSumTag::Print(std::ostream &os) const {
	std::cout << "DelaySum and number of packet are: " << delaySum << " , "
			<< nPkt << std::endl;
}

IsBackTag::IsBackTag() :
		Tag() {
	isBack = 0;
}
IsBackTag::IsBackTag(uint32_t isBack_) {
	this->isBack = isBack_;
}
TypeId IsBackTag::GetTypeId(void) {
	static TypeId tid =
			TypeId("ns3::IsBackTag").SetParent<Tag>().AddConstructor<IsBackTag>();
	return tid;
}
TypeId IsBackTag::GetInstanceTypeId(void) const {
	return GetTypeId();
}
uint32_t IsBackTag::GetSerializedSize(void) const {
	return sizeof(uint32_t);
}
void IsBackTag::Serialize(TagBuffer i) const {
	i.WriteU32(isBack);
}
void IsBackTag::Deserialize(TagBuffer i) {
	isBack = i.ReadU32();
}
void IsBackTag::Print(std::ostream &os) const {
	if (isBack == 1) {
		std::cout << "This is a Backward packet " << std::endl;
	} else if (isBack == 0) {
		std::cout << "This is a normal Forward packet " << std::endl;
	} else {
		std::cout << "something error happen in IsBackTag " << std::endl;
	}
}
/*----------------------------------------------------------------------------------------*/

/*
 *uint32_t Id0;//used to do selfrouting
 *uint32_t Id1;//used to do selfrouting
 *uint32_t Idlevel;//used to do selfrouting
 */
GlobalValue g_checksumEnabled = GlobalValue("ChecksumEnabled",
		"A global switch to enable all checksums for all protocols",
		BooleanValue(false), MakeBooleanChecker());

TypeId Node::GetTypeId(void) {
	static TypeId tid =
			TypeId("ns3::Node").SetParent<Object>().AddConstructor<Node>().AddAttribute(
					"DeviceList",
					"The list of devices associated to this Node.",
					ObjectVectorValue(),
					MakeObjectVectorAccessor(&Node::m_devices),
					MakeObjectVectorChecker<NetDevice>()).AddAttribute(
					"ApplicationList",
					"The list of applications associated to this Node.",
					ObjectVectorValue(),
					MakeObjectVectorAccessor(&Node::m_applications),
					MakeObjectVectorChecker<Application>()).AddAttribute("Id",
					"The id (unique integer) of this Node.",
					TypeId::ATTR_GET, // allow only getting it.
					UintegerValue(0), MakeUintegerAccessor(&Node::m_id),
					MakeUintegerChecker<uint32_t>()).AddAttribute("SystemId",
					"The systemId of this node: a unique integer used for parallel simulations.",
					TypeId::ATTR_GET | TypeId::ATTR_SET, UintegerValue(0),
					MakeUintegerAccessor(&Node::m_sid),
					MakeUintegerChecker<uint32_t>());
	return tid;
}
void Node::SetId_FatTree(uint32_t id0, uint32_t id1, uint32_t idlevel) {
//	Id0 = id0;
//	Id1 = id1;
//	Idlevel = idlevel;
	nodeId_FatTree = NodeId(id0, id1, idlevel);
}
//uint32_t Node::GetId0_FatTree(void) {
//	return Id0;
//}
//uint32_t Node::GetId1_FatTree(void) {
//	return Id1;
//}
//uint32_t Node::GetIdlevel_FatTree(void) {
//	return Idlevel;
//}
Node::Node() :
		m_id(0), m_sid(0) {
	NS_LOG_FUNCTION (this);
	Construct();
}

Node::Node(uint32_t sid) :
		m_id(0), m_sid(sid) {
	NS_LOG_FUNCTION (this << sid);
	Construct();
}

void Node::Construct(void) {
	NS_LOG_FUNCTION (this);
	m_id = NodeList::Add(this);
}

Node::~Node() {
	NS_LOG_FUNCTION (this);
}

bool Node::operator <(const Node other) const {
	return GetId() < other.GetId();
}

uint32_t Node::GetId(void) const {
	NS_LOG_FUNCTION (this);
	return m_id;
}

uint32_t Node::GetSystemId(void) const {
	NS_LOG_FUNCTION (this);
	return m_sid;
}

uint32_t Node::AddDevice(Ptr<NetDevice> device) {
	NS_LOG_FUNCTION (this << device);
	uint32_t index = m_devices.size();
	m_devices.push_back(device);
	device->SetNode(this);
	device->SetIfIndex(index);
	device->SetReceiveCallback(
			MakeCallback(&Node::NonPromiscReceiveFromDevice, this));
	Simulator::ScheduleWithContext(GetId(), Seconds(0.0),
			&NetDevice::Initialize, device);
	NotifyDeviceAdded(device);
	return index;
}
Ptr<NetDevice> Node::GetDevice(uint32_t index) const {
	NS_LOG_FUNCTION (this << index);
	NS_ASSERT_MSG(index < m_devices.size(),
			"Device index " << index << " is out of range (only have " << m_devices.size () << " devices).");
	return m_devices[index];
}
uint32_t Node::GetNDevices(void) const {
	NS_LOG_FUNCTION (this);
	return m_devices.size();
}

uint32_t Node::AddApplication(Ptr<Application> application) {
	NS_LOG_FUNCTION (this << application);
	uint32_t index = m_applications.size();
	m_applications.push_back(application);
	application->SetNode(this);
	Simulator::ScheduleWithContext(GetId(), Seconds(0.0),
			&Application::Initialize, application);
	return index;
}
Ptr<Application> Node::GetApplication(uint32_t index) const {
	NS_LOG_FUNCTION (this << index);
	NS_ASSERT_MSG(index < m_applications.size(),
			"Application index " << index << " is out of range (only have " << m_applications.size () << " applications).");
	return m_applications[index];
}
uint32_t Node::GetNApplications(void) const {
	NS_LOG_FUNCTION (this);
	return m_applications.size();
}

void Node::DoDispose() {
	NS_LOG_FUNCTION (this);
	m_deviceAdditionListeners.clear();
	m_handlers.clear();
	for (std::vector<Ptr<NetDevice> >::iterator i = m_devices.begin();
			i != m_devices.end(); i++) {
		Ptr<NetDevice> device = *i;
		device->Dispose();
		*i = 0;
	}
	m_devices.clear();
	for (std::vector<Ptr<Application> >::iterator i = m_applications.begin();
			i != m_applications.end(); i++) {
		Ptr<Application> application = *i;
		application->Dispose();
		*i = 0;
	}
	m_applications.clear();
	Object::DoDispose();
}
void Node::DoInitialize(void) {
	NS_LOG_FUNCTION (this);
	for (std::vector<Ptr<NetDevice> >::iterator i = m_devices.begin();
			i != m_devices.end(); i++) {
		Ptr<NetDevice> device = *i;
		device->Initialize();
	}
	for (std::vector<Ptr<Application> >::iterator i = m_applications.begin();
			i != m_applications.end(); i++) {
		Ptr<Application> application = *i;
		application->Initialize();
	}

	Object::DoInitialize();
}

void Node::RegisterProtocolHandler(ProtocolHandler handler,
		uint16_t protocolType, Ptr<NetDevice> device, bool promiscuous) {
	NS_LOG_FUNCTION (this << &handler << protocolType << device << promiscuous);
	struct Node::ProtocolHandlerEntry entry;
	entry.handler = handler;
	entry.protocol = protocolType;
	entry.device = device;
	entry.promiscuous = promiscuous;

	// On demand enable promiscuous mode in netdevices
	if (promiscuous) {
		if (device == 0) {
			for (std::vector<Ptr<NetDevice> >::iterator i = m_devices.begin();
					i != m_devices.end(); i++) {
				Ptr<NetDevice> dev = *i;
				dev->SetPromiscReceiveCallback(
						MakeCallback(&Node::PromiscReceiveFromDevice, this));
			}
		} else {
			device->SetPromiscReceiveCallback(
					MakeCallback(&Node::PromiscReceiveFromDevice, this));
		}
	}

	m_handlers.push_back(entry);
}

void Node::UnregisterProtocolHandler(ProtocolHandler handler) {
	NS_LOG_FUNCTION (this << &handler);
	for (ProtocolHandlerList::iterator i = m_handlers.begin();
			i != m_handlers.end(); i++) {
		if (i->handler.IsEqual(handler)) {
			m_handlers.erase(i);
			break;
		}
	}
}

bool Node::ChecksumEnabled(void) {
	NS_LOG_FUNCTION_NOARGS ();
	BooleanValue val;
	g_checksumEnabled.GetValue(val);
	return val.Get();
}

bool Node::PromiscReceiveFromDevice(Ptr<NetDevice> device,
		Ptr<const Packet> packet, uint16_t protocol, const Address &from,
		const Address &to, NetDevice::PacketType packetType) {
	NS_LOG_FUNCTION (this << device << packet << protocol << &from << &to << packetType);
	return ReceiveFromDevice(device, packet, protocol, from, to, packetType,
			true);
}

bool Node::NonPromiscReceiveFromDevice(Ptr<NetDevice> device,
		Ptr<const Packet> packet, uint16_t protocol, const Address &from) {
	NS_LOG_FUNCTION (this << device << packet << protocol << &from);
	return ReceiveFromDevice(device, packet, protocol, from,
			device->GetAddress(), NetDevice::PacketType(0), false);
}

bool Node::ReceiveFromDevice(Ptr<NetDevice> device, Ptr<const Packet> packet,
		uint16_t protocol, const Address &from, const Address &to,
		NetDevice::PacketType packetType, bool promiscuous) {
	NS_LOG_FUNCTION (this << device << packet << protocol << &from << &to << packetType << promiscuous);
	NS_ASSERT_MSG(Simulator::GetContext() == GetId(),
			"Received packet with erroneous context ; " << "make sure the channels in use are correctly updating events context " << "when transfering events from one node to another.");NS_LOG_DEBUG ("Node " << GetId () << " ReceiveFromDevice:  dev "
			<< device->GetIfIndex () << " (type=" << device->GetInstanceTypeId ().GetName ()
			<< ") Packet UID " << packet->GetUid ());
	bool found = false;

	for (ProtocolHandlerList::iterator i = m_handlers.begin();
			i != m_handlers.end(); i++) {
		if (i->device == 0 || (i->device != 0 && i->device == device)) {
			if (i->protocol == 0 || i->protocol == protocol) {
				if (promiscuous == i->promiscuous) {
					i->handler(device, packet, protocol, from, to, packetType);
					found = true;
				}
			}
		}
	}
	return found;
}
void Node::RegisterDeviceAdditionListener(DeviceAdditionListener listener) {
	NS_LOG_FUNCTION (this << &listener);
	m_deviceAdditionListeners.push_back(listener);
	// and, then, notify the new listener about all existing devices.
	for (std::vector<Ptr<NetDevice> >::const_iterator i = m_devices.begin();
			i != m_devices.end(); ++i) {
		listener(*i);
	}
}
void Node::UnregisterDeviceAdditionListener(DeviceAdditionListener listener) {
	NS_LOG_FUNCTION (this << &listener);
	for (DeviceAdditionListenerList::iterator i =
			m_deviceAdditionListeners.begin();
			i != m_deviceAdditionListeners.end(); i++) {
		if ((*i).IsEqual(listener)) {
			m_deviceAdditionListeners.erase(i);
			break;
		}
	}
}

void Node::NotifyDeviceAdded(Ptr<NetDevice> device) {
	NS_LOG_FUNCTION (this << device);
	for (DeviceAdditionListenerList::iterator i =
			m_deviceAdditionListeners.begin();
			i != m_deviceAdditionListeners.end(); i++) {
		(*i)(device);
	}
}

} // namespace ns3
