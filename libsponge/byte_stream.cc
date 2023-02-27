#include "byte_stream.hh"

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;
using std::deque;

ByteStream::ByteStream(const size_t capacity) {
    _capacity = capacity;
}

size_t ByteStream::write(const string &data) {
    DUMMY_CODE(data);
    size_t len = data.size();
    if(len > _capacity - _que.size()){
        len = _capacity - _que.size();
    }
    _bytes_written += len;
    for(size_t i = 0; i < len; i++){
        _que.push_back(data[i]);
    }
    return len;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    DUMMY_CODE(len);
    size_t length = len;
    if(length > _que.size()){
        length = _que.size();
    }
    return string().assign(_que.begin(), _que.begin() + length);
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) { 
    DUMMY_CODE(len); 
    for(size_t i = 0; i < len && !eof(); i++){
        _que.pop_front();
        _bytes_read ++;
    }
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    DUMMY_CODE(len);
    string str;
    for(size_t i = 0; i < len && !eof(); i++){
        str += _que.front();
        _que.pop_front();
    }
    _bytes_read += str.size();
    return str;
}

void ByteStream::end_input() { _input_ended = 1;}

bool ByteStream::input_ended() const { return _input_ended; }

size_t ByteStream::buffer_size() const { return _que.size(); }

bool ByteStream::buffer_empty() const { return _que.empty(); }

bool ByteStream::eof() const { return _que.empty() && _input_ended; }

size_t ByteStream::bytes_written() const { return _bytes_written; }

size_t ByteStream::bytes_read() const { return _bytes_read; }

size_t ByteStream::remaining_capacity() const { return _capacity - _que.size(); }
