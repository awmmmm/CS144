#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    const TCPHeader head = seg.header();
    Buffer payload = seg.payload();
    WrappingInt32 seqno = head.seqno;
    uint64_t checkpoint = _reassembler.stream_out().bytes_written();
    // checkpoint = checkpoint > 0 ? checkpoint - 1 : 0;
    if (head.syn) {
        _isn = true;
        isn = seqno;
    }
    // no_isn need write?
    if (_isn) {
        uint64_t abs_seqno = unwrap(seqno, isn, checkpoint);
        size_t index = 0;
        if (!head.syn)
            index = abs_seqno - 1;
        if (head.fin)
            _reassembler.push_substring(move(payload.copy()), index, true);
        else
            _reassembler.push_substring(move(payload.copy()), index, false);
    }
}

optional<WrappingInt32> TCPReceiver::ackno() const {
    WrappingInt32 res{0};

    if (TCPReceiver::_isn) {
        uint64_t ackno_64 = TCPReceiver::_reassembler.stream_out().bytes_written() + 1;
        if (TCPReceiver::_reassembler.stream_out().input_ended())
            ackno_64 += 1;
        res = wrap(ackno_64, TCPReceiver::isn);
    } else
        return nullopt;
    optional<WrappingInt32> ans = res;
    return ans;

    // return {}; }
}
size_t TCPReceiver::window_size() const { return {_reassembler.stream_out().remaining_capacity()}; }
