#ifndef HOLLOW_STACK_H_
#define HOLLOW_STACK_H_

#include <exception>

class EmptyStackException : public std::exception {};

class IgnoredElementException : public std::exception {};

/**
 * HollowStack
 *
 * Acts as a regular stack with the methods:
 *    `push`, `top` and `pop`
 * but also adds :
 *    - Random access to elements for a given height.
 *    - Hollowness: The stack has a fixed constant capacity,
 *      thus providing fast and constant operations.
 *      When the capacity is reached, elements at the middle of
 *      the stack are forgotten.
 *      For example with a capacity of 100, if more than 100 elements
 *      are inserted, the stack will keep the 50 bottom-most elements
 *      and the 50 top-most elements.
 */
template <unsigned int CAPACITY, typename T>
class HollowStack {

public:

    HollowStack():
            bottom_(),
            top_(),
            height_(-1),
            middleSize_(0),
            topHeight(-1),
            topFirst_(0),
            topSize_(0) {}

    /**
     * Check if the stack is empty
     */
    inline bool is_empty() {
        return height_ < 0;
    }

    /**
     * Returns the height of this stack.
     * Also the height of the top element.
     */
    inline int height() {
        if (is_empty()) throw new EmptyStackException;
        return height_;
    }

    /**
     * Tells if the top element has been forgotten
     */
    inline bool is_top_forgotten() {
        return is_forgotten(height_);
    }

    /**
     * Tells if the element at the given `height` has
     * been forgotten
     */
    bool is_forgotten(int height) {
        if (is_empty()) throw new EmptyStackException;
        return height >= BOTTOM_CAPACITY && height < topHeight;
    }

    /**
     * Pushes the given `element` on top of this stack.
     * This may lead to some elements pushed earlier
     * being forgotten if this stack's capacity has
     * been reached
     */
    void push(T element) {
        height_++;
        if (height_ < BOTTOM_CAPACITY) {
            bottom_[height_] = element;
        }
        else if (topSize_ < TOP_CAPACITY) {
            top_[topSize_] = element;
            topSize_++;
        }
        else {
            top_[topFirst_] = element;
            middleSize_++;
            topHeight = BOTTOM_CAPACITY + middleSize_;
            topFirst_ = modulo(topFirst_ + 1, TOP_CAPACITY);
        }
    }

    /**
     * Returns the element on top of this stack
     */
    inline T top() {
        return peek(height_);
    }

    /**
     * Returns the element at the given `height`
     * in this stack
     */
    T peek(int height) {
        if (is_empty()) throw new EmptyStackException;

        if (height < BOTTOM_CAPACITY) {
            return bottom_[height];
        }
        else if (height < topHeight) {
            throw new IgnoredElementException;
        }
        else {
            int topIndex = height - topHeight;
            return top_[modulo(topFirst_ + topIndex, TOP_CAPACITY)];
        }
    }

    /**
     * Discards the top element of this stack
     */
    void pop() {
        if (is_empty()) throw new EmptyStackException;

        if (height_ < BOTTOM_CAPACITY) {
            height_--;
        }
        else if (height_ < BOTTOM_CAPACITY + middleSize_) {
            height_--;
            if (middleSize_ > 0) {
                middleSize_--;
            }
        }
        else {
            height_--;
            topSize_--;
        }
    }

private:

    static const int BOTTOM_CAPACITY = (CAPACITY >> 1) + (CAPACITY & 1);
    static const int TOP_CAPACITY = (CAPACITY >> 1);

    T bottom_[BOTTOM_CAPACITY];
    T top_[TOP_CAPACITY];

    int height_;
    int middleSize_;
    int topHeight;
    int topFirst_;
    int topSize_;

    static inline int modulo(int dividend, int divisor) {
        int modulo = dividend % divisor;
        return modulo < 0
               ? modulo + divisor
               : modulo;
    }
};

#endif