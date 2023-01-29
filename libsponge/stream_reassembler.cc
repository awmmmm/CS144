#include "stream_reassembler.hh"

#include <algorithm>
// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) : _output(capacity), _capacity(capacity) {
    // set the order of set
}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    size_t data_index = index + data.size();

    if (string_buffer.size() != _output.remaining_capacity()) {  // update end and string_buf
        first_unaccepctable = first_unassembled + _output.remaining_capacity();
        // size_t expand_size = _output.remaining_capacity() - string_buffer.size();
        string_buffer.resize(_output.remaining_capacity());
    }

    if (eof && data_index <= first_unaccepctable) {
        end_index = data_index;
        _eof = true;
        if (_output.bytes_written() == end_index) {
            string_buffer.clear();
            _output.end_input();  //?
            return;
        }
    }
    if (index >= first_unaccepctable)
        return;
    data_index = (data_index > first_unaccepctable ? first_unaccepctable : data_index);

    if (data_index <= first_unassembled)
        return;
    // 1/need write
    if (index <= first_unassembled && data_index > first_unassembled) {
        // write
        if (!sections.empty() && need_merge(index, data_index, sections.begin()->first, sections.begin()->second)) {
            pair<size_t, size_t> res = set_merge(index, data_index);
            size_t pop_size = res.second - first_unassembled;

            if (res.second > data_index) {
                _output.write(data.substr(first_unassembled - index));
                _output.write(string(string_buffer.begin() + (data_index - first_unassembled),
                                     string_buffer.begin() + (res.second - first_unassembled)));

                first_unassembled = res.second;
                first_unaccepctable = first_unassembled + _output.remaining_capacity();
            } else {
                _output.write(data.substr(first_unassembled - index));
                first_unassembled = data_index;
                first_unaccepctable = first_unassembled + _output.remaining_capacity();
            }

            //   for(size_t i=0;i<pop_size;i++)string_buffer.pop_front();
            string_buffer.erase(string_buffer.begin(), string_buffer.begin() + pop_size);
        }

        else {
            size_t pop_size = data_index - first_unassembled;
            _output.write(data.substr(first_unassembled - index, data_index - first_unassembled));
            first_unassembled = data_index;
            first_unaccepctable = first_unassembled + _output.remaining_capacity();
            string_buffer.erase(string_buffer.begin(), string_buffer.begin() + pop_size);
        }

        if (_eof && _output.bytes_written() == end_index) {
            string_buffer.clear();
            _output.end_input();  //?
            return;
        }
    } else if (index > first_unassembled && index < first_unaccepctable) {
        push_into_buf(index, data_index, data, first_unassembled);
    }
}

size_t StreamReassembler::unassembled_bytes() const {
    size_t res = 0;
    for (auto i : sections)
        res += (i.second - i.first);

    return res;  // wrong it may have duplication
}

bool StreamReassembler::empty() const { return sections.empty(); }

void StreamReassembler::push_into_buf(const size_t left,
                                      const size_t right,
                                      const string &data,
                                      const size_t _first_unassembled) {
    copy(data.begin(), data.begin() + (right - left), string_buffer.begin() + (left - _first_unassembled));
    if (!sections.empty()) {
        auto iter = sections.begin();
        size_t tmp_left = left;
        size_t tmp_right = right;
        pair<size_t, size_t> res = {tmp_left, tmp_right};
        // bool flag = false;
        while (!sections.empty() && iter != sections.end()) {
            if (need_merge(tmp_left, tmp_right, iter->first, iter->second)) {
                // flag = true;
                res = merge(tmp_left, tmp_right, iter->first, iter->second);
                tmp_left = res.first;
                tmp_right = res.second;
                auto temp_iter = iter;
                iter++;
                sections.erase(temp_iter);
            } else {
                if (tmp_right < (iter->first))
                    break;
                iter++;
            }
        }
        sections.insert({tmp_left, tmp_right});
    } else
        // pair<size_t, size_t> res = set_merge(left, right);
        //  size_t new_left = set_merge(left,right).first;
        //  size_t new_right = set_merge(left,right).second;
        sections.insert({left, right});
}

bool StreamReassembler::need_merge(const size_t o_left,
                                   const size_t o_right,
                                   const size_t n_left,
                                   const size_t n_right) {
    if ((o_right >= n_left && o_right <= n_right) || (o_left >= n_left && o_left <= n_right) ||
        (o_left <= n_left && o_right >= n_right))
        return true;
    return false;
}

pair<size_t, size_t> StreamReassembler::merge(const size_t o_left,
                                              const size_t o_right,
                                              const size_t n_left,
                                              const size_t n_right) {
    return {min({o_left, o_right, n_left, n_right}), max({o_left, o_right, n_left, n_right})};
}
pair<size_t, size_t> StreamReassembler::set_merge(const size_t left, const size_t right) {
    auto iter = sections.begin();
    size_t tmp_left = left;
    size_t tmp_right = right;
    pair<size_t, size_t> res = {tmp_left, tmp_right};
    while (!sections.empty() && iter != sections.end()) {
        if (need_merge(tmp_left, tmp_right, iter->first, iter->second)) {
            res = merge(tmp_left, tmp_right, iter->first, iter->second);
            tmp_left = res.first;
            tmp_right = res.second;
            auto temp_iter = iter;
            iter++;
            sections.erase(temp_iter);
        } else
            break;
    }
    return {tmp_left, tmp_right};
}