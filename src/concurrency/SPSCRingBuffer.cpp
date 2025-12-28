/*
#include "SPSCRingBuffer.h"

template<typename T,size_t size>
bool SPSCQueue<T,size>::push(const T& item){

    size_t current_head= head.load(std::memory_order_relaxed);
    size_t next_head= (current_head+1) & (size-1);

    if(next_head==tail.load(std::memory_order_acquire)){
        return false;
    }

    buffer[current_head]=item;

    head.store(next_head,std::memory_order_release);
    return true;
}

template<typename T,size_t size>
bool SPSCQueue<T,size>::pop(T& item){

    size_t current_tail = tail.load(std::memory_order_relaxed);

    if(current_tail==head.load(std::memory_order_acquire)){
        return false;
    }

    item=buffer[current_tail];
    tail.store((current_tail+1)&(size-1),std::memory_order_release);
    return true;
}

template<typename T,size_t size>
size_t SPSCQueue<T,size>::Size(){
    size_t current_head=head.load(std::memory_order_acquire);
    size_t current_tail=tail.load(std::memory_order_acquire);

    if(current_head>=current_tail){
        return current_head-current_tail;
    }else{
        return size + current_head - current_tail;
    }
}

template<typename T,size_t size>
bool SPSCQueue<T,size>::empty(){
    size_t current_head=head.load(std::memory_order_acquire);
    size_t current_tail=tail.load(std::memory_order_acquire);

    if(current_head==current_tail) return true;

    return false;
}

template class SPSCQueue<MarketDataMessage,65536>;
*/
#include "SPSCRingBuffer.h"

template<typename T, std::size_t Size>
inline bool SPSCQueue<T, Size>::push(const T& item) {
    std::size_t current_tail = tail_.load(std::memory_order_relaxed);
    std::size_t next_tail = (current_tail + 1) & (Size - 1);

    if (next_tail == head_.load(std::memory_order_acquire)) {
        return false;
    }

    buffer_[current_tail] = item;

    tail_.store(next_tail, std::memory_order_release);
    return true;
}

template<typename T, std::size_t Size>
inline bool SPSCQueue<T, Size>::pop(T& item) {
    std::size_t current_head = head_.load(std::memory_order_relaxed);

    if (current_head == tail_.load(std::memory_order_acquire)) {
        return false;
    }

    item = buffer_[current_head];

    head_.store((current_head + 1) & (Size - 1),
                std::memory_order_release);
    return true;
}

template<typename T, std::size_t Size>
inline std::size_t SPSCQueue<T, Size>::size() const {
    std::size_t tail = tail_.load(std::memory_order_acquire);
    std::size_t head = head_.load(std::memory_order_acquire);

    if (tail >= head) {
        return tail - head;
    } else {
        return Size + tail - head;
    }
}

template<typename T, std::size_t Size>
inline bool SPSCQueue<T, Size>::empty() const {
    return head_.load(std::memory_order_acquire) ==
           tail_.load(std::memory_order_acquire);
}

template class SPSCQueue<OrderEvent, 65536>;

//SPSCQueue<MarketDataMessage, 65536> g_market_data_queue;