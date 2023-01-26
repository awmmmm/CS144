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
    , _RTO(retx_timeout)
    , _stream(capacity) {}

inline void TCPSender::timer_start() {
    if (!_run) {
        _run = true;
        // _start_timer = true;
        timeexpire = _total_ms + _RTO;
    }

    // timestart = _total_ms;
}

inline void TCPSender::timer_alarm() {
    if (timeexpire != 0 && timeexpire <= _total_ms)
        _expire = true;
}
inline void TCPSender::timer_end() {
    _run = false;
    _expire = false;
    _start_timer = false;
    timestart = 0;
    timeexpire = 0;
}
uint64_t TCPSender::bytes_in_flight() const { return _bytes_in_flight; }

void TCPSender::fill_window() {
    // if(_window_size == 0 && _window_size_receiver!=0) return;
    if (_window_size == 0)
        return;
    if (_stream.input_ended() && _stream.bytes_read() == _stream.bytes_written() &&
        _next_seqno == _stream.bytes_written() + 2)
        return;  // means end
    uint16_t cosume_size = 0;
    if (_next_seqno == 0) {  // means start??
        TCPSegment seg;
        seg.header().syn = true;
        seg.header().seqno = wrap(0, _isn);
        seg.payload() = _stream.read(0);
        // temp_size --;
        _segments_out.push(seg);
        _next_seqno += seg.length_in_sequence_space();
        cosume_size += seg.length_in_sequence_space();
        // uint64_t absseqo = unwrap(seg.header().seqno + seg.length_in_sequence_space() - 1, _isn,
        // _stream.bytes_read()); ckpt = _stream.bytes_read();
        // outstanding_segments_out.push_back(outstanding_segment{seg, _next_seqno});
        outstanding_segments_out.push(seg);
        _bytes_in_flight += seg.length_in_sequence_space();
        timer_start();
        if (_window_size > cosume_size)
            _window_size -= cosume_size;
        else
            _window_size = 0;
        return;
    } else if (_stream.input_ended() && _stream.bytes_read() == _stream.bytes_written() &&
               _next_seqno == _stream.bytes_written() + 1) {
        // fin
        TCPSegment seg;
        seg.header().fin = true;
        seg.header().seqno = wrap(_next_seqno, _isn);
        seg.payload() = _stream.read(0);
        // temp_size --;
        _segments_out.push(seg);
        _next_seqno += seg.length_in_sequence_space();
        cosume_size += seg.length_in_sequence_space();
        // uint64_t absseqo = unwrap(seg.header().seqno + seg.length_in_sequence_space() - 1, _isn,
        // _stream.bytes_read()); ckpt = _stream.bytes_read();
        // outstanding_segments_out.push_back(outstanding_segment{seg, _next_seqno});
        outstanding_segments_out.push(seg);
        _bytes_in_flight += seg.length_in_sequence_space();
        timer_start();
        if (_window_size > cosume_size)
            _window_size -= cosume_size;
        else
            _window_size = 0;
        return;
    } else if (_stream.buffer_empty()) {
        return;
    }

    uint16_t remaining_size = _window_size;

    // if (_window_size_receiver == 0)
    //     remaining_size = 1;

    while ((!_stream.buffer_empty()) && remaining_size != 0) {
        uint16_t temp_size =
            (remaining_size > TCPConfig::MAX_PAYLOAD_SIZE) ? TCPConfig::MAX_PAYLOAD_SIZE : remaining_size;
        TCPSegment seg;

        seg.header().seqno = wrap(_next_seqno, _isn);
        seg.payload() = _stream.read(temp_size);
        if (seg.length_in_sequence_space() < remaining_size) {  // fin&&MAX_PAYLOAD_SIZE
            if (_stream.input_ended() && _stream.bytes_read() == _stream.bytes_written()) {
                seg.header().fin = true;
                _segments_out.push(seg);
                _next_seqno += seg.length_in_sequence_space();
                cosume_size += seg.length_in_sequence_space();
                outstanding_segments_out.push(seg);
                _bytes_in_flight += seg.length_in_sequence_space();

                timer_start();
                break;
            }
        }
        remaining_size -= seg.length_in_sequence_space();
        // _window_size -= seg.length_in_sequence_space();
        _segments_out.push(seg);
        _next_seqno += seg.length_in_sequence_space();
        cosume_size += seg.length_in_sequence_space();
        outstanding_segments_out.push(seg);
        _bytes_in_flight += seg.length_in_sequence_space();
        timer_start();
    }
    if (_window_size > cosume_size)
        _window_size -= cosume_size;
    else
        _window_size = 0;
    // timer start?
    return;
    // no bigger than the
    // value given by TCPConfig::MAX PAYLOAD SIZE (1452 bytes).
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    uint64_t absackno = unwrap(ackno, _isn, _next_seqno);
    if (absackno > _next_seqno)
        return;
    _window_size_receiver = window_size;
    uint16_t __window_size = 0;
    if (window_size == 0) {
        __window_size = 1;
    } else
        __window_size = window_size;
    if ((absackno + __window_size) >= _next_seqno) {
        _window_size = (absackno + __window_size - _next_seqno);
        // _window_size_receiver = (absackno + window_size - _next_seqno);

    }

    else {
        // _window_size_receiver = 0;
        _window_size = 0;
    }

    bool restart = false;
    // When all outstanding data has been acknowledged, stop the retransmission timer.
    if (!outstanding_segments_out.empty()) {
        // move some thing out of the outstanding_segments_out;
        // binary search and a class which contain segement and it's last ack
        // just loop if not enough then use binary search

        if ((outstanding_segments_out.front().length_in_sequence_space() +
             unwrap(outstanding_segments_out.front().header().seqno, _isn, _next_seqno)) <= absackno) {
            restart = true;
        }
        while ((!outstanding_segments_out.empty()) &&
               (outstanding_segments_out.front().length_in_sequence_space() +
                unwrap(outstanding_segments_out.front().header().seqno, _isn, _next_seqno)) <= absackno) {
            _bytes_in_flight -= outstanding_segments_out.front().length_in_sequence_space();
            outstanding_segments_out.pop();
        }

    
    }
    // When all outstanding data has been acknowledged, stop the retransmission timer.
    if (outstanding_segments_out.empty())
        timer_end();

    if (restart) {
        _RTO = _initial_retransmission_timeout;
        if (!outstanding_segments_out.empty()) {
            // restart timer
            timer_end();
            timer_start();
        }
        _consecutive_retransmissions = 0;
    }

    // fill_window();
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    _total_ms += ms_since_last_tick;
    // maybe timer's operate should be init;
    // if (_start_timer) {
    //     timestart = _total_ms;
    //     _start_timer = false;
    //     timeexpire = timestart + _RTO;
    // }
    if (_run)
        timer_alarm();
    if (_expire) {
        // retransmission
        _segments_out.push(outstanding_segments_out.front());
        if (_window_size_receiver != 0) {
            _consecutive_retransmissions++;
            _RTO = 2 * _RTO;
        }
        timer_end();
        _run = true;
        // timestart = _total_ms;
        // _start_timer = false;
        timeexpire = _total_ms + _RTO;
    }
}

unsigned int TCPSender::consecutive_retransmissions() const { return _consecutive_retransmissions; }

void TCPSender::send_empty_segment() {
    TCPSegment seg;

    seg.header().seqno = wrap(_next_seqno, _isn);

    _segments_out.push(seg);
}
