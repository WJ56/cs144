#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    DUMMY_CODE(seg);
    TCPHeader header_tcpsegment = seg.header();
    Buffer payload_tcpsegment = seg.payload();
    size_t length_tcpsegment = seg.length_in_sequence_space();
    WrappingInt32 seqno_tcpsegment = header_tcpsegment.seqno;
    bool syn_tcpsegment = header_tcpsegment.syn;
    bool fin_tcpsegment = header_tcpsegment.fin;

}

optional<WrappingInt32> TCPReceiver::ackno() const { 

    return {}; 
}

size_t TCPReceiver::window_size() const { 

    return {}; 
}
