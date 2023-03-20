#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) : _output(capacity), _capacity(capacity), begin(0), number_map(0), _eof(-1) {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    if(_output.input_ended()) return;
    if(eof) _eof = index + data.size();
    if(index > begin){
        if(unreassembler.find(index) != unreassembler.end()){
            if(unreassembler[index].size() >= data.size()) return;
            else{
                number_map = number_map + data.size() - unreassembler[index].size();
                unreassembler.find(index)->second = data;
            }
        }else{
            unreassembler[index] = data;
            number_map += data.size();
        }

        auto it =  unreassembler.find(index);
        if(it != unreassembler.begin()){
            auto former = --it;
            ++it;
            if(former->first + former->second.size() >= it->first + it->second.size()){
                number_map -= it->second.size();
                unreassembler.erase(it->first);
                it = former;
            }else if(it->first <= former->first + former->second.size()){
                number_map = number_map + it->second.size() - (former->first + former->second.size() - it->first);
                former->second = former->second + it->second.substr(former->first + former->second.size() - it->first, it->second.size() - (former->first + former->second.size() - it->first));
                number_map -= it->second.size();
                unreassembler.erase(it->first);
                it = former;
            }
        }
        if(it != --unreassembler.end()){
            auto latter = ++it;
            --it;
            if(latter->first + latter->second.size() <= it->first + it->second.size()){
                number_map -= latter->second.size();
                unreassembler.erase(latter->first);
            }else if(latter->first <= it->first + it->second.size()){
                number_map = number_map + latter->second.size() - (it->first + it->second.size() - latter->first);
                it->second += latter->second.substr(it->first + it->second.size() - latter->first, latter->second.size() - (it->first + it->second.size() - latter->first));
                number_map -= latter->second.size();
                unreassembler.erase(latter->first);
            }
        }
        

        // if(_capacity - _output.buffer_size() - number_map >= 0) return;
        // else{
        //     size_t remain = -(_capacity - _output.buffer_size() - number_map);
        //     for(auto it = unreassembler.rbegin(); it != unreassembler.rend(); it++){
        //         if(it->second.size() <= remain){
        //             unreassembler.erase(it->first);
        //             remain -= it->second.size();
        //             if(remain == 0) return;
        //         }else{
        //             it->second.erase(it->second.size() - remain, remain);
        //             return;
        //         }
        //     }
        // }
    }else{
        if(index + data.size() < begin) return;
        size_t length = index + data.size() - begin;
        size_t m = min(length, _capacity - _output.buffer_size());
        _output.write(data.substr(begin - index, m));
        begin += m;
    }
    while(!unreassembler.empty() && unreassembler.begin()->first <= begin){
        if(_capacity - _output.buffer_size() > 0){
            size_t last = unreassembler.begin()->first + unreassembler.begin()->second.size();
            if(last - 1 >= begin){
                size_t m = min(last, begin + _capacity - _output.buffer_size());
                _output.write(unreassembler.begin()->second.substr(begin - unreassembler.begin()->first, m));
                begin = m;
            }
            number_map -= unreassembler.begin()->second.size();
            unreassembler.erase(unreassembler.begin()->first);
        }
    }
    if(_eof == begin){
        _output.end_input();
    }
}

// void StreamReassembler::cut_map(size_t s){
//     size_t i = s;
//     for(auto it = unreassembler.rbegin(); it != unreassembler.rend(); it++){
//         if(it->second.size() <= i){ 
//             unreassembler.erase(it->first);
//             i -= it->second.size();
//             if(i == 0) return;
//         }
//         else{
//             it->second.erase(it->second.size() - i, i);
//             return;
//         }
//     }
//     number_map -= s;
// }


size_t StreamReassembler::unassembled_bytes() const { 
    return number_map;
 }

bool StreamReassembler::empty() const { 
    return number_map == 0;
 }
