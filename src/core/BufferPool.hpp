#pragma once
#include <vector>
#include <cstddef>
#include <cstdint>
#include <span>
#include <memory>

namespace core {

class BufferPool {
public:
    static constexpr size_t BLOCK_SIZE = 4096;

    explicit BufferPool(size_t block_count);
    ~BufferPool() = default;

    BufferPool(const BufferPool&) = delete;
    BufferPool& operator=(const BufferPool&) = delete;

    void* allocate();
    void deallocate(void* ptr);

    size_t capacity() const { return m_blocks.size(); }
    size_t free_count() const { return m_free_indices.size(); }

private:
    std::vector<std::byte> m_memory; 
    std::vector<size_t> m_free_indices;
    std::vector<void*> m_blocks; // Pointers to start of each block
};

}
