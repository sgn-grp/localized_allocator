#ifndef ALLOCATOR_HPP
#define ALLOCATOR_HPP

#include <cstddef>

#define CONSEC_SIZE 1024

struct header {
    size_t free_size;           // size in terms of T objects       so comparison for merging must be done as of header was a T*
    struct header *next_ptr;
};

// currently assuming the size of T to be greater than size of header
template<typename T>
class localized_allocator {
    header *block_list_head, *block_list_tail;

    header* mergeblocks(header *block1, header *block2){
        block1->free_size += block2->free_size;
        block1->next_ptr = block2->next_ptr;
        return block1;
    }

    header *mergeinsert(header *previous, header *new_node) {
        if (block_list_head == nullptr) {
                block_list_head = block_list_tail = new_node;
                return new_node;
        };

        header* nextnode;
        if (previous == nullptr) {
            new_node->next_ptr = block_list_head;
            block_list_head = new_node;
        } else if (previous->next_ptr == nullptr) {
            previous->next_ptr = new_node;
            block_list_tail = new_node;
            new_node->next_ptr = nullptr;
        } else {
            new_node->next_ptr = previous->next_ptr;
            previous->next_ptr = new_node;
        }

        nextnode = new_node->next_ptr;

        if (previous != nullptr && (T*)previous + previous->free_size == (T*)new_node) {
            mergeblocks(previous, new_node);
            new_node = previous;
        }

        if ((T*)new_node + new_node->free_size == (T*)nextnode) {
            mergeblocks(new_node, nextnode);
        }

        return new_node;
    }

    header* sorted_insert(header *newnode) {
        if (block_list_head == nullptr) {
            newnode->next_ptr = nullptr;
            block_list_head = block_list_tail = newnode;
            return newnode;
        }
        if (newnode < block_list_head) {
            return mergeinsert(nullptr, newnode);
        }

        header *temp = block_list_head, *previous = nullptr;
        while (temp->next_ptr != nullptr && newnode > temp->next_ptr) {
            temp = temp->next_ptr;
        }
        return mergeinsert(temp, newnode);
    }

    header* removenode(header *node, header *prev = nullptr) {
        if (block_list_head == block_list_tail && block_list_head == node) {        // if last block is being removed
            block_list_head = block_list_tail = (header*)new T[CONSEC_SIZE];
            block_list_head->free_size = CONSEC_SIZE;
            block_list_head->next_ptr = nullptr;
        }
        if (node == block_list_head) {              // removing from head
            block_list_head = node->next_ptr;
            node->next_ptr = nullptr;
        }
        else {
            if (prev == nullptr) {                  // find previous if not given
                prev = block_list_head;
                while (prev->next_ptr != node) prev = prev->next_ptr;
            }
            prev->next_ptr = node->next_ptr;
            node->next_ptr = nullptr;
            if (node == block_list_tail) block_list_tail = prev;
        }
        return node;
    }

    header* stripnblock(size_t n) {
        header *temp = block_list_head, *prev = nullptr;
        while (temp != nullptr && temp->free_size < n) {        // finding a block of suitable size
            prev = temp;
            temp = temp->next_ptr;
        }

        if (temp == nullptr) {              // if no suitable size block found
            if (n <= CONSEC_SIZE) {         // if request is lower than our contiguous allocation buffer size
                temp = (header*)new T[CONSEC_SIZE];
                temp->free_size = CONSEC_SIZE;
                temp->next_ptr = nullptr;
                sorted_insert(temp);
                prev = nullptr;
            }
            else {
                return nullptr;
            }
        }

        if (temp->free_size == n) {         // if the request occupies a complete block
            removenode(temp, prev);
            return temp;
        }

        // normal removal
        ((header*) ((T*)temp + n) )->next_ptr = temp->next_ptr;
        ((header*) ((T*)temp + n) )->free_size = temp->free_size - n;
        if (prev != nullptr)
            prev->next_ptr = (header*)( (T*)prev->next_ptr + n);
        else {
            if (block_list_head != block_list_tail) block_list_head = (header*)( (T*)block_list_head + n);
            else block_list_head = block_list_tail = (header*)( (T*)block_list_head + n);
        }
        return temp;
    }

public:
    using value_type = T;
    using pointer = T *;
    using const_pointer = const T *;

    template<typename U>
    struct rebind {
        using other = localized_allocator<U>;
    };

    using difference_type = std::ptrdiff_t;

    localized_allocator() : block_list_head((header*)new T[CONSEC_SIZE]) {
        block_list_tail = block_list_head;
        block_list_head[0].next_ptr = nullptr;
        block_list_head[0].free_size = CONSEC_SIZE;
    }

    T *allocate(unsigned long);

    void deallocate(T *, std::size_t);

    ~localized_allocator() {
        header *temp = block_list_head;
        while (temp != nullptr) {
            temp = temp->next_ptr;
            delete[] block_list_head;
            block_list_head = temp;
        }
    }
};

template<class T>
T *localized_allocator<T>::allocate(unsigned long n) {
    auto blk = (T*) stripnblock(n);
    if (blk == nullptr) return new T[n];
    else return blk;
}

template<class T>
void localized_allocator<T>::deallocate(T *tofree, std::size_t n) {
    auto freed = (header*) tofree;
    freed->free_size = n;
    freed->next_ptr = nullptr;

    freed = sorted_insert(freed);
    if (freed->free_size > CONSEC_SIZE) removenode(freed);
}

#endif
