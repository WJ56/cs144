#include "network_interface.hh"

#include "arp_message.hh"
#include "ethernet_frame.hh"

#include <iostream>

// Dummy implementation of a network interface
// Translates from {IP datagram, next hop address} to link-layer frame, and from link-layer frame to IP datagram

// For Lab 5, please replace with a real implementation that passes the
// automated checks run by `make check_lab5`.

// You will need to add private members to the class declaration in `network_interface.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] ethernet_address Ethernet (what ARP calls "hardware") address of the interface
//! \param[in] ip_address IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface(const EthernetAddress &ethernet_address, const Address &ip_address)
    : _ethernet_address(ethernet_address), _ip_address(ip_address) {
    cerr << "DEBUG: Network interface has Ethernet address " << to_string(_ethernet_address) << " and IP address "
         << ip_address.ip() << "\n";
}

//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but may also be another host if directly connected to the same network as the destination)
//! (Note: the Address type can be converted to a uint32_t (raw 32-bit IP address) with the Address::ipv4_numeric() method.)
void NetworkInterface::send_datagram(const InternetDatagram &dgram, const Address &next_hop) {
    // std::iterator it = _arp_table.find(next_hop);
    //convert IP address of next hop to raw 32-bit representation (used in ARP header)
    const uint32_t next_hop_ip = next_hop.ipv4_numeric();
    const uint32_t my_ip = _ip_address.ipv4_numeric();
    // DUMMY_CODE(dgram, next_hop, next_hop_ip);
    if(_arp_table.find(next_hop_ip) != _arp_table.end()){
        // 发送IPv4报文（封装成帧）
        EthernetFrame ethernet_frame_ipv4;
        ethernet_frame_ipv4.header() = {_arp_table[next_hop_ip]._ethernet_add, _ethernet_address, EthernetHeader::TYPE_IPv4};
        ethernet_frame_ipv4.payload() = dgram.serialize();
        _frames_out.push(ethernet_frame_ipv4);
        // std::cerr << "notice1:" << ethernet_frame_ipv4.header().to_string() << endl;
        return;
    }else{
        // 发送arp request（封装成帧）
        if(_arp_request.find(next_hop_ip) == _arp_request.end() || _time_now - _arp_request[next_hop_ip]._t >= 5 * 1000){
            // 发送arp request
            EthernetFrame ethernet_frame_arp_request;
            ethernet_frame_arp_request.header() = {ETHERNET_BROADCAST, _ethernet_address, EthernetHeader::TYPE_ARP};
            ARPMessage arp_request;
            arp_request.opcode = ARPMessage::OPCODE_REQUEST;
            arp_request.sender_ethernet_address = _ethernet_address;
            arp_request.sender_ip_address = my_ip;
            arp_request.target_ip_address = next_hop_ip;
            ethernet_frame_arp_request.payload() = arp_request.serialize();
            // std::cerr << "notice2:" << ethernet_frame_arp_request.header().to_string() << endl;
            // std::cerr << "notice3:" << arp_request.to_string() << endl;
            _frames_out.push(ethernet_frame_arp_request);        
            // 只有重发了，才添加或更新
            _arp_request[next_hop_ip]._t = _time_now;
        }
        _arp_request[next_hop_ip]._unsend_dgram.push(dgram);
    }
}

//! \param[in] frame the incoming Ethernet frame
optional<InternetDatagram> NetworkInterface::recv_frame(const EthernetFrame &frame) {
    DUMMY_CODE(frame);
    // ipv4
    if(frame.header().type == EthernetHeader::TYPE_IPv4 && frame.header().dst == _ethernet_address){
        InternetDatagram temp;
        if(temp.parse(frame.payload()) == ParseResult::NoError){
            return temp;
        }else return {};
    }
    if(frame.header().type == EthernetHeader::TYPE_ARP){
        ARPMessage arp_message;
        if(arp_message.parse(frame.payload()) != ParseResult::NoError) return{};
        // 记住
        _arp_table[arp_message.sender_ip_address]._ethernet_add = arp_message.sender_ethernet_address;
        _arp_table[arp_message.sender_ip_address]._ttl = _time_now;
        // 如果接收到的是arp request
        if(arp_message.opcode == ARPMessage::OPCODE_REQUEST && arp_message.target_ip_address == _ip_address.ipv4_numeric()){
            // 发送arp reply
            EthernetFrame ethernet_frame_arp_reply;
            ethernet_frame_arp_reply.header() = {arp_message.sender_ethernet_address, _ethernet_address, EthernetHeader::TYPE_ARP};
            ARPMessage arp_reply;
            arp_reply.opcode = ARPMessage::OPCODE_REPLY;
            arp_reply.sender_ethernet_address = _ethernet_address;
            arp_reply.sender_ip_address = _ip_address.ipv4_numeric();
            arp_reply.target_ip_address = arp_message.sender_ip_address;
            arp_reply.target_ethernet_address = arp_message.sender_ethernet_address;
            ethernet_frame_arp_reply.payload() = arp_reply.serialize();
            _frames_out.push(ethernet_frame_arp_reply);
            // std::cerr << "notice4:" << ethernet_frame_arp_reply.header().to_string() << endl;
            return {};
        }
        //如果接收到的是arp reply
        if(arp_message.opcode == ARPMessage::OPCODE_REPLY && _arp_request.find(arp_message.sender_ip_address) != _arp_request.end()){
            while(!_arp_request[arp_message.sender_ip_address]._unsend_dgram.empty()){
                send_datagram(_arp_request[arp_message.sender_ip_address]._unsend_dgram.front(), Address::from_ipv4_numeric(arp_message.sender_ip_address));
                _arp_request[arp_message.sender_ip_address]._unsend_dgram.pop();
            }
            _arp_request.erase(_arp_request.find(arp_message.sender_ip_address));
            return {};
        }
    }
    return {};
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick(const size_t ms_since_last_tick) { 
    DUMMY_CODE(ms_since_last_tick); 
    _time_now += ms_since_last_tick;
    for(std::map<uint32_t, ip_to_ethernet_info>::iterator it = _arp_table.begin(); it != _arp_table.end();){
        if(_time_now - it->second._ttl > 30 * 1000){
            it = _arp_table.erase(it);
        }else ++it;
    }
}

