#ifndef SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH
#define SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH

#include "byte_stream.hh"

#include <cstdint>
#include <deque>
#include <set>
#include <string>

//! \brief A class that assembles a series of excerpts from a byte stream (possibly out of order,
//! possibly overlapping) into an in-order byte stream.
class StreamReassembler {
  private:
    // Your code here -- add private members as necessary.

    struct Comp {
        bool operator()(const std::pair<size_t, size_t> &lhs, const std::pair<size_t, size_t> &rhs) const {
            return lhs.first < rhs.first;
        }
    };

    ByteStream _output;  //!< The reassembled in-order byte stream
    size_t _capacity;    //!< The maximum number of bytes

    // size_t _capacity_strbuf;

    size_t first_unassembled = 0;
    size_t first_unaccepctable = 0;  // means the buffer left bound
    std::deque<char> string_buffer = {};
    std::set<std::pair<size_t, size_t>, Comp> sections = {};

    std::pair<size_t, size_t> merge(
        const size_t o_left,
        const size_t o_right,
        const size_t n_left,
        const size_t n_right);  // operate std::vector<char> &str_buf and the set maybe need merge more than once
    // need more consider the args and maybe return pair<left,right> after merge;insert once is better

    bool _eof = false;
    size_t end_index = 0;  // if end_index == byte_written real end;

    void push_into_buf(const size_t left, const size_t right, const std::string &data, const size_t _first_unassembled);

    bool need_merge(const size_t o_left, const size_t o_right, const size_t n_left, const size_t n_right);

    std::pair<size_t, size_t> set_merge(const size_t left, const size_t right);

    // size_t end = 0;        // buffer's end
    // size_t temp_head = 0;  // temp's head
    // size_t temp_end = 0;   // temp's end
    // size_t _index = 0;     // the next index of the last index of current stream byte;also means buffer's head
    // // these point are all index based on the byte within stream
    // // all end like iter in a vector,which means it's point to the next of last byte
    // std::string string_buffer = "";  // used to keep substring until early index is done
    //                                  // or may be deque<char> is better
    //                                  // deque<char> string_buffer = {};
  public:
    //! \brief Construct a `StreamReassembler` that will store up to `capacity` bytes.
    //! \note This capacity limits both the bytes that have been reassembled,
    //! and those that have not yet been reassembled.
    StreamReassembler(const size_t capacity);

    //! \brief Receive a substring and write any newly contiguous bytes into the stream.
    //!
    //! The StreamReassembler will stay within the memory limits of the `capacity`.
    //! Bytes that would exceed the capacity are silently discarded.
    //!
    //! \param data the substring
    //! \param index indicates the index (place in sequence) of the first byte in `data`
    //! \param eof the last byte of `data` will be the last byte in the entire stream
    void push_substring(const std::string &data, const uint64_t index, const bool eof);

    //! \name Access the reassembled byte stream
    //!@{
    const ByteStream &stream_out() const { return _output; }
    ByteStream &stream_out() { return _output; }
    //!@}

    //! The number of bytes in the substrings stored but not yet reassembled
    //!
    //! \note If the byte at a particular index has been pushed more than once, it
    //! should only be counted once for the purpose of this function.
    size_t unassembled_bytes() const;

    //! \brief Is the internal state empty (other than the output stream)?
    //! \returns `true` if no substrings are waiting to be assembled
    bool empty() const;
};

#endif  // SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH
