#include "reassembler.hh"
#include "byte_stream.hh"
#include <algorithm>
#include <cstdint>
#include <string>
#include <utility>
#include <stdexcept>

using namespace std;

uint64_t Reassembler::first_unpopped_idx()
{
  return reader().bytes_popped();
}

uint64_t Reassembler::first_unassembled_idx()
{
  return writer().bytes_pushed();
}

uint64_t Reassembler::first_unaccepted_idx()
{
  return first_unassembled_idx() + writer().available_capacity();
}

// 将 <first_idx, data> 分割到指定区域内
void Reassembler::split(uint64_t &first_idx, string &data)
{
  uint64_t last_idx = first_idx + data.size() - 1;
  uint64_t l_idx = first_unassembled_idx();
  uint64_t r_idx = first_unaccepted_idx() - 1;
  if (!data.size() || (first_idx >= l_idx && last_idx <= r_idx)) return;
  if (first_idx > r_idx || last_idx < l_idx || l_idx > r_idx) {
    data.clear();
    return;
  }

  uint64_t new_first_idx = std::max(l_idx, first_idx);
  uint64_t new_last_idx = std::min(r_idx, last_idx);
  data = data.substr(new_first_idx - first_idx, new_last_idx - new_first_idx + 1);
  first_idx = new_first_idx;
}

// 检查两个区间是否重叠
bool is_overlapped(uint64_t a_start, uint64_t a_len, uint64_t b_start, uint64_t b_len)
{
  if (!a_len || !b_len) return false;
  uint64_t a_end = a_start + a_len - 1;
  uint64_t b_end = b_start + b_len - 1;
  return !(a_end < b_start || a_start > b_end);
}

// 将 <b_idx, b_data> 合并到 <a_idx, a_data> 中
void Reassembler::merge(uint64_t &a_idx, string &a_data, uint64_t b_idx, string b_data)
{
  if (a_idx <= b_idx && ((a_idx + a_data.size()) >= (b_idx + b_data.size()))) return;
  if (a_idx >= b_idx && ((a_idx + a_data.size()) <= (b_idx + b_data.size()))) {
    a_data = b_data;
    a_idx = b_idx;
    return;
  }
  if (a_idx < b_idx) {
    a_data = a_data.substr(0, b_idx - a_idx) + b_data;
  } else {
    a_data = b_data.substr(0, a_idx - b_idx) + a_data;
    a_idx = b_idx;
  }
}

// 向 cache 中合并地插入 <first_idx, data>
// 维护 cache, num_bytes_stored, end_idx_valid
void Reassembler::insert_into_cache(uint64_t first_idx, std::string data, bool is_last_substring)
{
  // 先 split 到合适范围内
  uint64_t unsplited_first_idx = first_idx;
  string unsplited_data = data;
  split(first_idx, data);

  // 如果传来的最后一个字符串没有被截断
  if (first_idx + data.size() == unsplited_first_idx + unsplited_data.size() && is_last_substring) {
    end_idx_valid = true;
  }
  if (!data.size()) return; // 空串不需要放入 cache 内，判断完 is_last_substring 逻辑后即可返回

  // 删除被 merge 的条目
  for (auto it = cache.begin(); it != cache.end();) {
    if (is_overlapped(first_idx, data.size(), it->first, it->second.size())) {
	    merge(first_idx, data, it->first, it->second);
	    num_bytes_stored -= it->second.size();
	    it = cache.erase(it);
    } else {
	    it++;
    }
  }

  // 插入 merge 后的条目
  cache.insert(make_pair(first_idx, data));
  num_bytes_stored += data.size();
}

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
  if (writer().is_closed()) return;

  insert_into_cache(first_index, data, is_last_substring);

  // 将符合条件的写入 bytestream
  while (!cache.empty() && first_unassembled_idx() == cache.begin()->first) {
    output_.writer().push(cache.begin()->second);
    num_bytes_stored -= cache.begin()->second.size();
    cache.erase(cache.begin());
  }

  // 合适情况下关闭 bytestream
  if (cache.empty() && end_idx_valid) {
    if (num_bytes_stored != 0) throw std::logic_error("num_bytes_stored should be 0.");
    output_.writer().close();
  }
}

uint64_t Reassembler::bytes_pending() const
{
  // Your code here.
  return num_bytes_stored;
}