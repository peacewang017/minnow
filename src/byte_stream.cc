#include "byte_stream.hh"

using namespace std;

ByteStream::ByteStream(uint64_t capacity)
    : buffer(), capacity_(capacity)
{
}

bool Writer::is_closed() const
{
    // Your code here.
    return (!is_open);
}

void Writer::push(string data)
{
    // Your code here.
    if (!is_open) return;
    for (auto ch : data) {
        if (buffer.size() >= capacity_) return;
        buffer.push(ch);
        nums_bytes_pushed++;
    }
    return;
}

void Writer::close()
{
    // Your code here.
    is_open = false;
}

uint64_t Writer::available_capacity() const
{
    // Your code here.
    return (capacity_ - (uint64_t)buffer.size());
}

uint64_t Writer::bytes_pushed() const
{
    // Your code here.
    return nums_bytes_pushed;
}

bool Reader::is_finished() const
{
    // Your code here.
    return (bool)(!is_open && buffer.empty());
}

uint64_t Reader::bytes_popped() const
{
    // Your code here.
    return nums_bytes_poped;
}

string_view Reader::peek() const
{
    // Your code here.
    if (!buffer.empty()) {
        return string_view(&buffer.front(), sizeof(char));
    }
    return std::string_view();
}

void Reader::pop(uint64_t len)
{
    for (uint64_t i = 0; i < len; i++) {
        if (buffer.empty()) return;
        buffer.pop();
        nums_bytes_poped++;
    }
}

uint64_t Reader::bytes_buffered() const
{
    // Your code here.
    return (uint64_t)buffer.size();
}