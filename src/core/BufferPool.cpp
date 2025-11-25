#include "BufferPool.hpp"
#include <stdexcept>

namespace core {

BufferPool::BufferPool(size_t block_count) {
    m_memory.resize(block_count * BLOCK_SIZE);
    m_free_indices.reserve(block_count);
    m_blocks.reserve(block_count);

    for (size_t i = 0; i < block_count; ++i) {
        m_blocks.push_back(m_memory.data() + (i * BLOCK_SIZE));
        m_free_indices.push_back(i);
    }
}

void* BufferPool::allocate() {
    if (m_free_indices.empty()) {
        return nullptr;
    }
    size_t idx = m_free_indices.back();
    m_free_indices.pop_back();
    return m_blocks[idx];
}

void BufferPool::deallocate(void* ptr) {
    // O(N) search to find index? No, we can calculate it.
    // Assuming ptr is valid.
    std::byte* p = static_cast<std::byte*>(ptr);
    std::byte* start = m_memory.data();
    
    if (p < start || p >= start + m_memory.size()) {
        return; // Invalid pointer
    }
    
    size_t offset = p - start;
    if (offset % BLOCK_SIZE != 0) {
        return; // Misaligned
    }
    
    size_t idx = offset / BLOCK_SIZE;
    m_free_indices.push_back(idx);
}

}
