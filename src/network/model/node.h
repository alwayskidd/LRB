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
#ifndef NODE_H
#define NODE_H

#include <vector>

#include "ns3/object.h"
#include "ns3/callback.h"
#include "ns3/ptr.h"
#include "ns3/net-device.h"
#include <string>

#include "ns3/tag.h"
#include <iostream>
#include "ns3/ipv4-address.h"
#include <map>

using namespace ns3;

namespace ns3 {

class Application;
class Packet;
class Address;

/*------------------------------------------Chunzhi------------------------------*/

class NodeId {
public:
	uint32_t id_pod, id_switch, id_level;
	NodeId();
	NodeId(uint32_t id_pod, uint32_t id_switch, uint32_t id_level);
	void Print(std::ostream &os);
	bool operator ==(const NodeId) const;

	/**
	 * return format: "xxx", e.g. "000", "112"
	 */
	std::string toString();
};

class IdTag: public Tag, public NodeId {
public:
	static TypeId GetTypeId(void);
	virtual TypeId GetInstanceTypeId(void) const;
	virtual uint32_t GetSerializedSize(void) const;
	virtual void Serialize(TagBuffer i) const;
	virtual void Deserialize(TagBuffer i);
	virtual void Print(std::ostream &os) const;
	IdTag();
	IdTag(uint32_t id_pod, uint32_t id_switch, uint32_t id_level);
};

class SrcIdTag: public IdTag {
public:
	static TypeId GetTypeId(void);
	virtual TypeId GetInstanceTypeId(void) const;
	SrcIdTag();
	SrcIdTag(uint32_t id_pod, uint32_t id_switch, uint32_t id_level);
};

class DstIdTag: public IdTag {
public:
	static TypeId GetTypeId(void);
	virtual TypeId GetInstanceTypeId(void) const;
	DstIdTag();
	DstIdTag(uint32_t id_pod, uint32_t id_switch, uint32_t id_level);
};

class TurningIdTag: public IdTag {
public:
	static TypeId GetTypeId(void);
	virtual TypeId GetInstanceTypeId(void) const;
	TurningIdTag();
	TurningIdTag(uint32_t id_pod, uint32_t id_switch, uint32_t id_level);
};
/*--------------------------------------------------------------------------------------------*/
/*--------------------------------------------------By Zhiyong--------------------------------*/
class TimeStampTag: public Tag {
public:
	static TypeId GetTypeId(void);
	virtual TypeId GetInstanceTypeId(void) const;
	virtual uint32_t GetSerializedSize(void) const;
	virtual void Serialize(TagBuffer i) const;
	virtual void Deserialize(TagBuffer i);
	virtual void Print(std::ostream &os) const;
	TimeStampTag();
	TimeStampTag(double time_);
	double time;
};
class PacketSumTag: public Tag {
public:
	static TypeId GetTypeId(void);
	virtual TypeId GetInstanceTypeId(void) const;
	virtual uint32_t GetSerializedSize(void) const;
	virtual void Serialize(TagBuffer i) const;
	virtual void Deserialize(TagBuffer i);
	virtual void Print(std::ostream &os) const;
	PacketSumTag();
	PacketSumTag(uint32_t nPkt_, double delaySum_);
	double delaySum;
	uint32_t nPkt;
};
class IsBackTag: public Tag { // this tag is used when this is a backward packet
public:
	static TypeId GetTypeId(void);
	virtual TypeId GetInstanceTypeId(void) const;
	virtual uint32_t GetSerializedSize(void) const;
	virtual void Serialize(TagBuffer i) const;
	virtual void Deserialize(TagBuffer i);
	virtual void Print(std::ostream &os) const;
	IsBackTag();
	IsBackTag(uint32_t isBack_);
	uint32_t isBack;
};
/*--------------------------------------------------------------------------------------------*/
/**
 * \ingroup network
 *
 * \brief A network Node.
 *
 * This class holds together:
 *   - a list of NetDevice objects which represent the network interfaces
 *     of this node which are connected to other Node instances through
 *     Channel instances.
 *   - a list of Application objects which represent the userspace
 *     traffic generation applications which interact with the Node
 *     through the Socket API.
 *   - a node Id: a unique per-node identifier.
 *   - a system Id: a unique Id used for parallel simulations.
 *
 * Every Node created is added to the NodeList automatically.
 */
class Node: public Object {
public:
	static TypeId GetTypeId(void);

	Node();
	/**
	 * \param systemId a unique integer used for parallel simulations.
	 */
	Node(uint32_t systemId);

	virtual ~Node();

	/**
	 * Add by Chunzhi
	 * Add this function such that Node is hashable for std::map.
	 */
	bool operator <(const Node) const;

	/**
	 * \returns the unique id of this node.
	 *
	 * This unique id happens to be also the index of the Node into
	 * the NodeList.
	 */
	uint32_t GetId(void) const;

	/**
	 * \returns the system id for parallel simulations associated
	 *          to this node.
	 */
	uint32_t GetSystemId(void) const;

	/**
	 * \param device NetDevice to associate to this node.
	 * \returns the index of the NetDevice into the Node's list of
	 *          NetDevice.
	 *
	 * Associate this device to this node.
	 */
	uint32_t AddDevice(Ptr<NetDevice> device);
	/**
	 * \param index the index of the requested NetDevice
	 * \returns the requested NetDevice associated to this Node.
	 *
	 * The indexes used by the GetDevice method start at one and
	 * end at GetNDevices ()
	 */
	Ptr<NetDevice> GetDevice(uint32_t index) const;
	/**
	 * \returns the number of NetDevice instances associated
	 *          to this Node.
	 */
	uint32_t GetNDevices(void) const;

	/**
	 * \param application Application to associate to this node.
	 * \returns the index of the Application within the Node's list
	 *          of Application.
	 *
	 * Associated this Application to this Node.
	 */
	uint32_t AddApplication(Ptr<Application> application);
	/**
	 * \param index
	 * \returns the application associated to this requested index
	 *          within this Node.
	 */
	Ptr<Application> GetApplication(uint32_t index) const;

	/**
	 * \returns the number of applications associated to this Node.
	 */
	uint32_t GetNApplications(void) const;

	/**
	 * A protocol handler
	 *
	 * \param device a pointer to the net device which received the packet
	 * \param packet the packet received
	 * \param protocol the 16 bit protocol number associated with this packet.
	 *        This protocol number is expected to be the same protocol number
	 *        given to the Send method by the user on the sender side.
	 * \param sender the address of the sender
	 * \param receiver the address of the receiver; Note: this value is
	 *                 only valid for promiscuous mode protocol
	 *                 handlers.  Note:  If the L2 protocol does not use L2
	 *                 addresses, the address reported here is the value of
	 *                 device->GetAddress().
	 * \param packetType type of packet received
	 *                   (broadcast/multicast/unicast/otherhost); Note:
	 *                   this value is only valid for promiscuous mode
	 *                   protocol handlers.
	 */
	typedef Callback<void, Ptr<NetDevice>, Ptr<const Packet>, uint16_t,
			const Address &, const Address &, NetDevice::PacketType> ProtocolHandler;
	/**
	 * \param handler the handler to register
	 * \param protocolType the type of protocol this handler is
	 *        interested in. This protocol type is a so-called
	 *        EtherType, as registered here:
	 *        http://standards.ieee.org/regauth/ethertype/eth.txt
	 *        the value zero is interpreted as matching all
	 *        protocols.
	 * \param device the device attached to this handler. If the
	 *        value is zero, the handler is attached to all
	 *        devices on this node.
	 * \param promiscuous whether to register a promiscuous mode handler
	 */
	void RegisterProtocolHandler(ProtocolHandler handler, uint16_t protocolType,
			Ptr<NetDevice> device, bool promiscuous = false);
	/**
	 * \param handler the handler to unregister
	 *
	 * After this call returns, the input handler will never
	 * be invoked anymore.
	 */
	void UnregisterProtocolHandler(ProtocolHandler handler);

	/**
	 * A callback invoked whenever a device is added to a node.
	 */
	typedef Callback<void, Ptr<NetDevice> > DeviceAdditionListener;
	/**
	 * \param listener the listener to add
	 *
	 * Add a new listener to the list of listeners for the device-added
	 * event. When a new listener is added, it is notified of the existance
	 * of all already-added devices to make discovery of devices easier.
	 */
	void RegisterDeviceAdditionListener(DeviceAdditionListener listener);
	/**
	 * \param listener the listener to remove
	 *
	 * Remove an existing listener from the list of listeners for the
	 * device-added event.
	 */
	void UnregisterDeviceAdditionListener(DeviceAdditionListener listener);

	/**
	 * \returns true if checksums are enabled, false otherwise.
	 */
	static bool ChecksumEnabled(void);
	/*
	 * this function used in self-routing
	 * used to set the ID of every node
	 */
	void SetId_FatTree(uint32_t id0, uint32_t id1, uint32_t idlevel);

	/*-----------------------------------------Chunzhi------------------------------*/
	NodeId nodeId_FatTree; // Chunzhi
	std::map<std::string, uint32_t> reRoutingMap;
	/*------------------------------------------------------------------------------*/

	/*
	 * get the number of the specific id
	 * para id, the possible value could be id0, id1, idlevel
	 */
	uint32_t GetId0_FatTree(void);
	uint32_t GetId1_FatTree(void);
	uint32_t GetIdlevel_FatTree(void);

protected:
	/**
	 * The dispose method. Subclasses must override this method
	 * and must chain up to it by calling Node::DoDispose at the
	 * end of their own DoDispose method.
	 */
	virtual void DoDispose(void);
	virtual void DoInitialize(void);
private:
	void NotifyDeviceAdded(Ptr<NetDevice> device);
//	uint32_t Id0; //used to do selfrouting
//	uint32_t Id1; //used to do selfrouting
//	uint32_t Idlevel; //used to do selfrouting
	bool NonPromiscReceiveFromDevice(Ptr<NetDevice> device, Ptr<const Packet>,
			uint16_t protocol, const Address &from);
	bool PromiscReceiveFromDevice(Ptr<NetDevice> device, Ptr<const Packet>,
			uint16_t protocol, const Address &from, const Address &to,
			NetDevice::PacketType packetType);
	bool ReceiveFromDevice(Ptr<NetDevice> device, Ptr<const Packet>,
			uint16_t protocol, const Address &from, const Address &to,
			NetDevice::PacketType packetType, bool promisc);

	void Construct(void);

	struct ProtocolHandlerEntry {
		ProtocolHandler handler;
		Ptr<NetDevice> device;
		uint16_t protocol;
		bool promiscuous;
	};
	typedef std::vector<struct Node::ProtocolHandlerEntry> ProtocolHandlerList;
	typedef std::vector<DeviceAdditionListener> DeviceAdditionListenerList;

	uint32_t m_id;         // Node id for this node
	uint32_t m_sid;        // System id for this node
	std::vector<Ptr<NetDevice> > m_devices;
	std::vector<Ptr<Application> > m_applications;
	ProtocolHandlerList m_handlers;
	DeviceAdditionListenerList m_deviceAdditionListeners;
};

} // namespace ns3

//extern std::map<Ipv4Address, NodeId> IpServerMap;
extern std::map<Ipv4Address, Ptr<Node> > IpServerMap;
extern std::map<Ptr<Node>, Ipv4Address> ServerIpMap;

#endif /* NODE_H */
