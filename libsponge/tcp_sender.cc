#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity) {}

uint64_t TCPSender::bytes_in_flight() const { return _next_seqno - _abs_ackno; }

void TCPSender::fill_window() {
    if(_fin_f) return;
    if(!_syn_f){
        TCPSegment tcp_segment;
        tcp_segment.header().syn = 1;
        _syn_f = true;
        send_tcp_segment(tcp_segment);
        return;
    }
    if(_stream.eof() && bytes_in_flight() < _window_size){
        TCPSegment tcp_segment;
        tcp_segment.header().fin = 1;
        _fin_f = true;
        send_tcp_segment(tcp_segment);
        return;
    }
    while (!_stream.buffer_empty() && bytes_in_flight() < _window_size)
    {
        TCPSegment tcp_segment;
        size_t len = min(_window_size - bytes_in_flight(), TCPConfig::MAX_PAYLOAD_SIZE);
        tcp_segment.payload() = Buffer(_stream.read(len));
        if(_stream.eof() && bytes_in_flight() + tcp_segment.length_in_sequence_space() < _window_size){
            tcp_segment.header().fin = 1;
            _fin_f = true;
        }
        send_tcp_segment(tcp_segment);
    }
    
    // while(!_fin_f && bytes_in_flight() < _window_size){
    //     TCPSegment tcp_segment;
    //     if(!_syn_f){
    //         tcp_segment.header().syn = 1;
    //         _syn_f = true;
    //     }else{
    //         size_t len = min(_window_size - bytes_in_flight(), TCPConfig::MAX_PAYLOAD_SIZE);
    //         tcp_segment.payload() = Buffer(move(_stream.read(len)));
    //         if(_stream.eof() && bytes_in_flight() + tcp_segment.length_in_sequence_space() < _window_size){
    //             tcp_segment.header().fin = 1;
    //             _fin_f = true;
    //         }
    //     }
    //     tcp_segment.header().seqno = wrap(_next_seqno, _isn);
    //     _segments_out.push(tcp_segment);
    //     _unreceive_segments.push(tcp_segment);
    //     _next_seqno += tcp_segment.length_in_sequence_space();
    //     if(!_retransmission_timer){
    //         _retransmission_timer = true;
    //         _retransmission_timer_times = 0;
    //     }
    // }
}

void TCPSender::send_tcp_segment(TCPSegment &tcp_segment){
    tcp_segment.header().seqno = wrap(_next_seqno, _isn);
    _segments_out.push(tcp_segment);
    _unreceive_segments.push(tcp_segment);
    _next_seqno += tcp_segment.length_in_sequence_space();
    if(!_retransmission_timer){
        _retransmission_timer = true;
        _retransmission_timer_times = 0;
    }
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) { 
    uint64_t abs_ackno = unwrap(ackno, _isn, _abs_ackno);
    if(abs_ackno < _abs_ackno || abs_ackno > _next_seqno) return;
    _abs_ackno = abs_ackno;
    if(window_size == 0){
        _window_size = 1;
        _window_size_0 = true;
    }else{
        _window_size = window_size;
        _window_size_0 = false;
    }
    // while(!_unreceive_segments.empty()){
    //     TCPSegment tcp_segment = _unreceive_segments.front();
    //     if(unwrap(tcp_segment.header().seqno, _isn, _abs_ackno) + tcp_segment.length_in_sequence_space() > _abs_ackno) return;
    //     _unreceive_segments.pop();
    //     _rto = _initial_retransmission_timeout;
    //     _consecutive_retransmissions = 0;
    //     _retransmission_timer_times = 0;
    // }
    // _retransmission_timer = !_unreceive_segments.empty();
    // fill_window();
    bool f = false;
    while(!_unreceive_segments.empty()){
        TCPSegment tcp_segment = _unreceive_segments.front();
        if(unwrap(tcp_segment.header().seqno, _isn, _abs_ackno) + tcp_segment.length_in_sequence_space() <= _abs_ackno){
            _unreceive_segments.pop();
            f = true;
        }else break;
    }
    if(f){
        _rto = _initial_retransmission_timeout;
        _consecutive_retransmissions = 0;
        _retransmission_timer_times = 0;
        _retransmission_timer = !_unreceive_segments.empty();
        fill_window();
    }
 }

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) { 
    if(!_retransmission_timer) return;
    _retransmission_timer_times += ms_since_last_tick;
    if(_retransmission_timer_times >= _rto && !_unreceive_segments.empty()){
        TCPSegment tcp_segment = _unreceive_segments.front();
        _segments_out.push(tcp_segment);
        _retransmission_timer_times = 0;
        if(!_window_size_0 || tcp_segment.header().syn){
            _consecutive_retransmissions++;
            _rto *= 2;
        }
    }
 }

unsigned int TCPSender::consecutive_retransmissions() const { return _consecutive_retransmissions; }

void TCPSender::send_empty_segment() {
    TCPSegment tcp_segment;
   tcp_segment.header().seqno = wrap(_next_seqno, _isn);
   _unreceive_segments.push(tcp_segment);
}
