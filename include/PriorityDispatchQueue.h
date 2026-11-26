#ifndef ERLHUB_PRIORITYDISPATCHQUEUE_H
#define ERLHUB_PRIORITYDISPATCHQUEUE_H

#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <vector>

#include "Records.h"

// UNIT IV: reusable generic template container + priority functor.
namespace ReliefOps {
namespace Logistics {

// Custom priority-based sorting functor for urgent medical deliveries.
struct DispatchPriorityComparator {
    bool operator()(const DispatchRequest& a, const DispatchRequest& b) const {
        const int ap = static_cast<int>(a.priority);
        const int bp = static_cast<int>(b.priority);
        if (ap != bp) {
            return ap > bp;  // higher enum value = more urgent
        }
        if (a.triageApproved != b.triageApproved) {
            return a.triageApproved;  // commander-approved triage wins ties
        }
        return a.sequence < b.sequence;  // FIFO inside a priority tier
    }
};

template <typename T>
class PriorityDispatchQueue {
public:
    using Compare = DispatchPriorityComparator;

    PriorityDispatchQueue() = default;

    void push(const T& item) { items_.push_back(item); }

    bool empty() const noexcept { return items_.empty(); }
    std::size_t size() const noexcept { return items_.size(); }
    void clear() noexcept { items_.clear(); }

    // Return by reference: callers may inspect (and, for T with friend access,
    // update) the live highest-priority request without copying it.
    T& peek() {
        if (items_.empty()) {
            throw std::runtime_error("Priority dispatch queue is empty");
        }
        return *std::max_element(items_.begin(), items_.end(), Compare{});
    }

    const T& peek() const {
        if (items_.empty()) {
            throw std::runtime_error("Priority dispatch queue is empty");
        }
        return *std::max_element(items_.begin(), items_.end(), Compare{});
    }

    T pop() {
        T top = peek();
        auto it = std::max_element(items_.begin(), items_.end(), Compare{});
        items_.erase(it);
        return top;
    }

    const std::vector<T>& rawRef() const { return items_; }

    // std::sort + custom functor produce a display-ordered copy.
    template <typename Compare>
    std::vector<T> sortedSnapshot(Compare comp) const {
        std::vector<T> copy = items_;
        std::sort(copy.begin(), copy.end(), comp);
        return copy;
    }

    std::vector<T> priorityOrdered() const {
        return sortedSnapshot(DispatchPriorityComparator{});
    }

private:
    std::vector<T> items_;
};

}  // namespace Logistics
}  // namespace ReliefOps

#endif  // ERLHUB_PRIORITYDISPATCHQUEUE_H
