#ifndef COMMON_STL_UTIL_H_
#define COMMON_STL_UTIL_H_

// These used to be in Chromium but have since been removed. We still
// have code that depend on them though. At some point, we should
// clean-up our code and remove them.

// For a range within a container of pairs, calls delete (non-array version) on
// BOTH items in the pairs.
// NOTE: Like STLDeleteContainerPointers, it is important that this deletes
// behind the iterator because if both the key and value are deleted, the
// container may call the hash function on the iterator when it is advanced,
// which could result in the hash function trying to dereference a stale
// pointer.
template <class ForwardIterator>
void STLDeleteContainerPairPointers(ForwardIterator begin,
                                    ForwardIterator end) {
  while (begin != end) {
    ForwardIterator temp = begin;
    ++begin;
    delete temp->first;
    delete temp->second;
  }
}

// Given a pointer to an STL container this class will delete all the value
// pointers when it goes out of scope.
template <class T>
class STLValueDeleter {
 public:
  STLValueDeleter<T>(T* container) : container_(container) {}
  ~STLValueDeleter<T>() { STLDeleteValues(container_); }

 private:
  T* container_;
};

#endif  // COMMON_STL_UTIL_H_
