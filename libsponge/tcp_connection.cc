#include "tcp_connection.hh"

#include <iostream>

// Dummy implementation of a TCP connection

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

size_t TCPConnection::remaining_outbound_capacity() const { return _sender.stream_in().remaining_capacity(); }

size_t TCPConnection::bytes_in_flight() const { return _sender.bytes_in_flight(); }

size_t TCPConnection::unassembled_bytes() const { return _receiver.unassembled_bytes(); }

size_t TCPConnection::time_since_last_segment_received() const { return _linger; }

void TCPConnection::segment_received(const TCPSegment &seg) { 
    DUMMY_CODE(seg); 
    if(!_active) return;
    // if(_sender.fin_send() && seg.header().ack && _receiver.stream_out() )
    _linger = 0;
    // 1. 如果RST（reset）标志位为真，将发送端stream和接受端stream设置成error state并终止连接。
    if(seg.header().rst){
        _sender.stream_in().set_error();
        _receiver.stream_out().set_error();
        // 终止连接???
        _active = false;
        return;
    }
    // 2. 把segment传递给TCPReceiver，这样的话，TCPReceiver就能从segment取出它所关心的字段进行处理了：seqno，SYN，payload，FIN。
    _receiver.segment_received(seg);
    // 3. 如果ACK标志位为真，通知TCPSender有segment被确认，TCPSender关心的字段有ackno和window_size。
    if(seg.header().ack){
        _sender.ack_received(seg.header().ackno, seg.header().win);
    }
    // 4. 如果收到的segment不为空（是正确的，因为syn或fin各占了一个序列号），TCPConnection必须确保至少给这个segment回复一个ACK，以便远端的发送方更新ackno和window_size。
    if(seg.length_in_sequence_space()){
        if(seg.header().syn && !_sender.syn_send()){
            _sender.fill_window();
        }
        if(!fill_window()){
            _sender.send_empty_segment();
            fill_window();
        }
    }
    // 5. 给“keep-alive” segment回复消息。对面的终端可能会发送一个不合法的seqno，来探测你的TCP连接是否仍然alive
    if (_receiver.ackno().has_value() && (seg.length_in_sequence_space() == 0) && seg.header().seqno == _receiver.ackno().value() - 1) {
        _sender.send_empty_segment();
        // 在发送segment之前，
        // TCPConnection会读取TCPReceiver中关于ackno和window_size相关的信息。
        // 如果当前TCPReceiver里有一个合法的ackno，
        // TCPConnection会更改TCPSegment里的ACK标志位，
        // 将其设置为真。
        fill_window();
    }
    // 如果接受完毕，则后续是进行的被动关闭，否则为主动关闭
    if(_receiver.stream_out().input_ended() && !_sender.stream_in().eof()){
        _linger_after_streams_finish = false;
    }
}

bool TCPConnection::active() const { return _active; }

size_t TCPConnection::write(const string &data) {
    DUMMY_CODE(data);
    size_t len =  _sender.stream_in().write(data);
    _sender.fill_window();
    fill_window();
    return len;
}

void TCPConnection::tick(const size_t ms_since_last_tick) { 
    DUMMY_CODE(ms_since_last_tick); 
    if(!_active) return;
    _linger += ms_since_last_tick;
    // 1. 告诉TCPSender时间正在流逝
    _sender.tick(ms_since_last_tick);
    // 2. 如果同一个segment连续重传的次数超过TCPConfig::MAX_RETX_ATTEMPTS，终止连接，并且给对方发送一个reset segment（一个RST为真的空segment）。
    if(_sender.consecutive_retransmissions() > TCPConfig::MAX_RETX_ATTEMPTS){
        send_rst_segment();
        return;
    }
    // 主动关闭意思是，我发送方已经完成了任务，对方可能要么也恰好发送完成要么还没发送完。
    if(_receiver.stream_out().eof() && _sender.stream_in().eof() && !_sender.bytes_in_flight()){
        if(!_linger_after_streams_finish || _linger >= 10 * _cfg.rt_timeout){
            _active = false;
            return;
        }
    }
    // 为了让_sender中重发的报文传送出去
    fill_window();
}

void TCPConnection::end_input_stream() {
    _sender.stream_in().end_input();
    _sender.fill_window();
    fill_window();
}

void TCPConnection::connect() {
    if(!_active) return;
    _sender.fill_window();
    fill_window();
}

TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";

            // Your code here: need to send a RST segment to the peer
            send_rst_segment();
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}

bool TCPConnection::fill_window(){
    if(_sender.segments_out().empty()) return false;
    while(!_sender.segments_out().empty()){
        TCPSegment tcp_segment = _sender.segments_out().front();
        _sender.segments_out().pop();
        if(_receiver.ackno().has_value()){
            tcp_segment.header().ack = 1;
            tcp_segment.header().ackno = _receiver.ackno().value();
        }
        tcp_segment.header().win = UINT16_MAX < _receiver.window_size() ? UINT16_MAX : _receiver.window_size();
        _segments_out.push(tcp_segment);
    }
    return true;
}

void TCPConnection::send_rst_segment(){
    while(!_segments_out.empty()){
        _segments_out.pop();
    }
    while(!_sender.segments_out().empty()){
        _sender.segments_out().pop();
    }
    _sender.send_empty_segment();
    TCPSegment tcp_segment = _sender.segments_out().front();
    tcp_segment.header().rst = true;
    if(_receiver.ackno().has_value()){
        tcp_segment.header().ack = 1;
        tcp_segment.header().ackno = _receiver.ackno().value();
    }
    _segments_out.push(tcp_segment);
    _sender.stream_in().set_error();
    _receiver.stream_out().set_error();
    _active = false;
}
