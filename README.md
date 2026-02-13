# Localized Allocator

A custom STL-style allocator created to study allocation and deallocation behavior of STL containers and experiment with cache-local memory layouts.

This allocator manages memory in contiguous chunks and maintains a free list of reusable blocks.

## Design

- Memory is managed in units of T
- Free blocks are stored in a sorted singly linked list
- Each free block begins with a header
- Adjacent free blocks are merged during deallocation
- Memory is primarily allocated in chunks of CONSEC_SIZE objects

### Allocation Behavior

The constructor allocates one chunk of CONSEC_SIZE objects.

On allocation request:

	- If a suitable free block exists → it is split or fully used.
	- If no suitable block exists:
	- If request ≤ CONSEC_SIZE → a new chunk is allocated and added to the pool.
	- If request > CONSEC_SIZE → memory is allocated directly using new[].
### Deallocation Behavior

	- Freed blocks are inserted into the free list in sorted order.

	- Adjacent free blocks are merged to reduce fragmentation.

	- Very large blocks may be removed from the internal pool.

## Assumptions

- sizeof(T) >= sizeof(header)
- Not thread safe.
- Intended for experimentation and learning, not production use.

## Purpose

- Learning custom allocator design
- Observing STL container allocation patterns
- Experimenting with memory locality