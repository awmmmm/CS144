#include "tcp_connection.hh"

#include <iostream>

// Dummy implementation of a TCP connection

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

void TCPConnection::tcpconnection_send(bool RST) {
    while (!_sender.segments_out().empty()) {
        TCPSegment seg = _sender.segments_out().front();
        _sender.segments_out().pop();
        if (RST)
            seg.header().rst = true;
        if (_receiver.window_size() > numeric_limits<uint16_t>::max())
            seg.header().win = numeric_limits<uint16_t>::max();
        else
            seg.header().win = _receiver.window_size();
        if (_receiver.ackno().has_value()) {
            seg.header().ackno = _receiver.ackno().value();
            seg.header().ack = true;
        }
        _segments_out.push(seg);
    }
}

size_t TCPConnection::remaining_outbound_capacity() const { return {_sender.stream_in().remaining_capacity()}; }

size_t TCPConnection::bytes_in_flight() const { return {_sender.bytes_in_flight()}; }

size_t TCPConnection::unassembled_bytes() const { return {_receiver.unassembled_bytes()}; }

size_t TCPConnection::time_since_last_segment_received() const { return {_time_since_last_segment_received}; }

void TCPConnection::segment_received(const TCPSegment &seg) {
    // _time_since_last_segment_received = _current_time - _last_time;
    // _last_time = _current_time;
    if (seg.header().rst) {
        // if the rst (reset) flag is set, sets both the inbound and outbound streams to the error
        // state and kills the connection permanently. Otherwise it. . .
        _sender.stream_in().set_error();
        inbound_stream().set_error();
        _linger_after_streams_finish = false;
        // _receiver.stream_out().set_error();
        //     If the inbound stream ends before the TCPConnection
        // has reached EOF on its outbound stream, this variable needs to be set to false.
        // if(!_sender.stream_in().eof())

        // kill it
        //-------------------------------------
        // not fininshed
        //   ~TCPConnection();
    } else {
        // gives the segment to the TCPReceiver so it can inspect the fields it cares about on
        // incoming segments: seqno, syn , payload, and fin .
        _receiver.segment_received(seg);
        // if the ack flag is set, tells the TCPSender about the fields it cares about on incoming
        // segments: ackno and window size.
        if (seg.header().ack)
            _sender.ack_received(seg.header().ackno, seg.header().win);

        //         if the incoming segment occupied any sequence numbers, the TCPConnection makes
        // sure that at least one segment is sent in reply, to reflect an update in the ackno and
        // window size.
        // if (_receiver.ackno().has_value() && seg.length_in_sequence_space() != 0){
        if (seg.length_in_sequence_space() != 0) {
            _sender.fill_window();
            if (_sender.segments_out().empty()) {
                _sender.send_empty_segment();
            }
        }

        //         if (_receiver.ackno().has_value() and (seg.length_in_sequence_space() == 0)
        // and seg.header().seqno == _receiver.ackno().value() - 1) {
        // _sender.send_empty_segment();
        if (_receiver.ackno().has_value() && (seg.length_in_sequence_space() == 0) &&
            (seg.header().seqno == _receiver.ackno().value() - 1))
            //  if (_receiver.ackno().has_value() && (seg.length_in_sequence_space() == 0) &&
            //     (seg.header().seqno == _receiver.ackno().value()))
            _sender.send_empty_segment();

        // if just fin not send
        if (_sender.stream_in().eof() && _sender.next_seqno_absolute() < (_sender.stream_in().bytes_written() + 2))
            _sender.fill_window();

        tcpconnection_send(false);
        // received the last seg
        //         stream_in().eof()
        // and next_seqno_absolute() == stream_in().bytes_written() + 2
        // and sender.bytes_in_flight() > 0
        if (inbound_stream().input_ended() && _sender.stream_in().eof()) {
            _last_time = _current_time;
            _time_since_last_segment_received = 0;
            _last_received = true;
        }
        if (inbound_stream().input_ended() && !_sender.stream_in().eof())
            _linger_after_streams_finish = false;

        // }
    }
}

bool TCPConnection::active() const {
    //! \brief Is the connection still alive in any way?
    //! \returns `true` if either stream is still running or if the TCPConnection is lingering
    //! after both streams have finished (e.g. to ACK retransmissions from the peer)
    // if ((!_sender.stream_in().error() && !_receiver.stream_out().error())&&((!inbound_stream().eof())
    // ||(!_sender.stream_in().eof()))
    //  || (inbound_stream().eof()&&_sender.stream_in().eof()&&_linger_after_streams_finish))
    //     return true;
    if (!_sender.stream_in().error() && !_receiver.stream_out().error()) {
        if ((!_receiver.stream_out().input_ended()) ||
            !(_sender.stream_in().eof() && _sender.next_seqno_absolute() == (_sender.stream_in().bytes_written() + 2) &&
              _sender.bytes_in_flight() == 0))
            return true;
        if (_receiver.stream_out().eof() && _sender.stream_in().eof() && _linger_after_streams_finish)
            return true;
    }
    return false;
}

size_t TCPConnection::write(const string &data) {
    // DUMMY_CODE(data);
    size_t res = _sender.stream_in().write(data);
    _sender.fill_window();
    tcpconnection_send(false);
    return res;
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) {
    _current_time += ms_since_last_tick;
    // _sender.fill_window();
    _sender.tick(ms_since_last_tick);

    if (_sender.consecutive_retransmissions() > TCPConfig::MAX_RETX_ATTEMPTS) {
        _sender.send_empty_segment();  // how to with RST
        _sender.stream_in().set_error();
        inbound_stream().set_error();
        _linger_after_streams_finish = false;
        TCPSegment seg = _sender.segments_out().back();
        seg.header().rst = true;
        _segments_out.push(seg);
        // tcpconnection_send(true);
        return;
    }
    //     stream_in().eof()
    // and next_seqno_absolute() < stream_in().bytes_written() + 2
    //  if(active()&&((!_sender.stream_in().buffer_empty())||(_sender.stream_in().eof()&&_sender.next_seqno_absolute()<(_sender.stream_in().bytes_written()+2))))
    if (active() && ((!_sender.stream_in().buffer_empty())))
        _sender.fill_window();
    tcpconnection_send(false);
    if (_last_received) {
        _time_since_last_segment_received = _current_time - _last_time;
    }
    if (_last_received && _time_since_last_segment_received >= 10 * _cfg.rt_timeout)
        _linger_after_streams_finish = false;
    // end the connection cleanly if necessary
    // ---------------------------------------------
}

void TCPConnection::end_input_stream() {
    _sender.stream_in().end_input();
    if (_sender.stream_in().eof()) {  // if buffer already empty
        _sender.fill_window();
        tcpconnection_send(false);
    }
}

void TCPConnection::connect() {
    _sender.fill_window();
    //     The exceptions
    // are just at the very beginning of the connection, before the receiver has anything to
    // acknowledge.
    tcpconnection_send(false);
}

TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";

            // Your code here: need to send a RST segment to the peer
            _sender.send_empty_segment();
            tcpconnection_send(true);
            _sender.stream_in().set_error();
            inbound_stream().set_error();
            _linger_after_streams_finish = false;
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}
