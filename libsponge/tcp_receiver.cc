#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    DUMMY_CODE(seg);
    // 如果已经全部接收
    if(_reassembler.stream_out().input_ended()) return;
    // 如果isn已经确定，则不需要接收syn
    if(isn_flag && seg.header().syn) return;
    // 如果isn还没有确定，则必须要求seg的syn为true
    if(!isn_flag && !seg.header().syn) return;
    // 如果啥也没有
    if(seg.length_in_sequence_space() == 0) return;

    TCPHeader header_tcpsegment = seg.header();
    Buffer payload_tcpsegment = seg.payload();
    // size_t length_tcpsegment = seg.length_in_sequence_space();
    WrappingInt32 seqno_tcpsegment = header_tcpsegment.seqno;
    bool syn_tcpsegment = header_tcpsegment.syn;
    bool fin_tcpsegment = header_tcpsegment.fin;

    string data = payload_tcpsegment.copy();
    uint64_t checkpoint = _reassembler.stream_out().bytes_written();
    if(syn_tcpsegment && !isn_flag){
        isn_flag = true;
        isn = seqno_tcpsegment;
        // 已经确定是开头
        _reassembler.push_substring(data, 0, fin_tcpsegment);
        return;
    }
    uint64_t index = unwrap(seqno_tcpsegment, isn, checkpoint);
    if(index == 0) return;
    _reassembler.push_substring(data, index - 1, fin_tcpsegment);           // note: index is stream's index, it is beginning at zero.
}

optional<WrappingInt32> TCPReceiver::ackno() const { 
    if(!isn_flag) return {};
    size_t num = _reassembler.stream_out().bytes_written() + 1;
    if(_reassembler.stream_out().input_ended()) num++;      // expected new data
    return wrap(num, isn);
}

size_t TCPReceiver::window_size() const { 
    return _capacity -  _reassembler.stream_out().buffer_size(); 
}
