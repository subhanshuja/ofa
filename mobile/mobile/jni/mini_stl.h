/* -*- Mode: c++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
 *
 * Copyright (C) 2014 Software ASA.  All rights reserved.
 *
 * This file is part of the Opera web browser.  It may not be distributed
 * under any circumstances.
 */

namespace mstd {

class string {
 public:
  string()
      : size_(0), fill_(0), data_(NULL), ptr_("") {
  }

  string(const string& str)
      : size_(0), fill_(0), data_(NULL), ptr_("") {
    assign(str);
  }

  string(const char* str)
      : size_(0), fill_(0), data_(NULL), ptr_("") {
    assign(str);
  }

  ~string() {
    free(data_);
  }

  string& operator=(const string& str) {
    assign(str);
    return *this;
  }

  string& operator=(const char* str) {
    assign(str);
    return *this;
  }

  bool operator==(const string& str) const {
    return str.fill_ == fill_ &&
        (fill_ == 0 || memcmp(str.data_, data_, fill_) == 0);
  }

  bool operator!=(const string& str) const {
    return str.fill_ != fill_ ||
        (fill_ > 0 && memcmp(str.data_, data_, fill_) != 0);
  }

  void clear() {
    free(data_);
    data_ = NULL;
    ptr_ = "";
    size_ = fill_ = 0;
  }

  void assign(const string& str) {
    assign(str.data_, str.fill_);
  }

  void assign(const char* str) {
    assign(str, strlen(str));
  }

  void assign(const char* data, size_t size) {
    if (size == 0) {
      clear();
    } else {
      need(size);
      memcpy(data_, data, size);
      fill_ = size;
      data_[fill_] = '\0';
    }
  }

  void append(char c) {
    append(&c, 1);
  }

  void append(const char* str) {
    append(str, strlen(str));
  }

  void append(const string& str) {
    append(str.data_, str.fill_);
  }

  void append(const char* data, size_t size) {
    if (size > 0) {
      need(fill_ + size);
      memcpy(data_ + fill_, data, size);
      fill_ += size;
      data_[fill_] = '\0';
    }
  }

  void erase(size_t start, size_t end) {
    if (start >= end) return;
    if (start > fill_) return;
    if (end > fill_) end = fill_;
    if (start == 0 && end == fill_) {
      clear();
      return;
    }
    fill_ -= end - start;
    memmove(data_ + start, data_ + end, (fill_ - start) + 1);
  }

  char at(size_t i) const {
    MOP_ASSERT(i < fill_);
    return ptr_[i];
  }

  const char* c_str() const {
    return ptr_;
  }
  const char* data() const {
    return ptr_;
  }
  size_t size() const {
    return fill_;
  }
  bool empty() const {
    return fill_ == 0;
  }

 private:
  void need(size_t bytes) {
    bytes++; // Always allow for zero terminator
    if (bytes <= size_) return;
    size_ *= 2;
    if (size_ < bytes) size_ = bytes;
    if (size_ < 32) size_ = 32;
    ptr_ = data_ = static_cast<char*>(realloc(data_, size_));
  }

  size_t size_, fill_;
  char* data_;
  const char* ptr_;
};

template<typename T>
class vector {
 public:
  class const_iterator {
   public:
    const T& operator*() const {
      MOP_ASSERT(offset_ < vector_->fill_);
      return vector_->data_[offset_];
    }

    const T* operator->() const {
      MOP_ASSERT(offset_ < vector_->fill_);
      return vector_->data_ + offset_;
    }

    bool operator==(const const_iterator& i) const {
      return i.vector_ == vector_ && i.offset_ == offset_;
    }

    bool operator!=(const const_iterator& i) const {
      return i.vector_ != vector_ || i.offset_ != offset_;
    }

    const_iterator& operator+=(int val) {
      offset_ += val;
      return *this;
    }

    const_iterator& operator++() {
      offset_++;
      return *this;
    }

    const_iterator operator++(int) {
      return const_iterator(vector_, offset_++);
    }

   private:
    friend class vector;
    const_iterator(const vector* vector, size_t offset)
        : vector_(vector), offset_(offset) {
      MOP_ASSERT(vector);
      MOP_ASSERT(offset_ <= vector->fill_);
    }

    const vector* vector_;
    size_t offset_;
  };

  class iterator {
   public:
    T& operator*() {
      MOP_ASSERT(offset_ < vector_->fill_);
      return vector_->data_[offset_];
    }

    T* operator->() {
      MOP_ASSERT(offset_ < vector_->fill_);
      return vector_->data_ + offset_;
    }

    bool operator==(const iterator& i) const {
      return i.vector_ == vector_ && i.offset_ == offset_;
    }

    bool operator!=(const iterator& i) const {
      return i.vector_ != vector_ || i.offset_ != offset_;
    }

    iterator& operator+=(int val) {
      offset_ += val;
      return *this;
    }

    iterator& operator++() {
      offset_++;
      return *this;
    }

    iterator operator++(int) {
      return iterator(vector_, offset_++);
    }

   private:
    friend class vector;
    iterator(vector* vector, size_t offset)
        : vector_(vector), offset_(offset) {
      MOP_ASSERT(vector);
      MOP_ASSERT(offset_ <= vector->fill_);
    }

    vector* vector_;
    size_t offset_;
  };

  vector()
      : size_(0), fill_(0), data_(0) {
  }

  vector(const vector& vector)
      : size_(0), fill_(0), data_(0) {
    reserve(vector.size());
    insert(end(), vector.begin(), vector.end());
  }

  ~vector() {
    delete[] data_;
  }

  vector& operator=(const vector& vector) {
    clear();
    reserve(vector.size());
    insert(end(), vector.begin(), vector.end());
    return *this;
  }

  template <class InputIterator>
  void insert(iterator position, InputIterator first, InputIterator last) {
    MOP_ASSERT(position.vector_ == this);
    while (first != last) {
      insert(position, *first);
      ++position;
      ++first;
    }
  }

  void insert(iterator position, const T&value) {
    MOP_ASSERT(position.vector_ == this);
    MOP_ASSERT(position.offset_ <= fill_);
    reserve(fill_ + 1);
    for (size_t r = fill_ - 1, w = fill_; w > position.offset_; r--, w--) {
      data_[w] = data_[r];
    }
    data_[position.offset_] = value;
    fill_++;
  }

  void reserve(size_t count) {
    if (count <= size_) return;
    size_ *= 2;
    if (size_ < count) size_ = count;
    if (size_ < 8) size_ = 8;
    T* tmp = new T[size_];
    for (size_t i = 0; i < fill_; i++) {
      tmp[i] = data_[i];
    }
    delete[] data_;
    data_ = tmp;
  }

  void clear() {
    delete[] data_;
    data_ = NULL;
    fill_ = size_ = 0;
  }

  size_t size() const {
    return fill_;
  }

  bool empty() const {
    return fill_ == 0;
  }

  void push_back(const T& element) {
    reserve(fill_ + 1);
    data_[fill_++] = element;
  }

  iterator begin() {
    return iterator(this, 0);
  }

  iterator end() {
    return iterator(this, fill_);
  }

  const_iterator begin() const {
    return const_iterator(this, 0);
  }

  const_iterator end() const {
    return const_iterator(this, fill_);
  }

  iterator remove(iterator position) {
    MOP_ASSERT(position.vector_ == this);
    MOP_ASSERT(position.offset_ < fill_);
    for (size_t r = position.offset_ + 1, w = position.offset_;
         r < fill_;
         r++, w++) {
      data_[w] = data_[r];
    }
    fill_--;
    return iterator(this, position.offset_);
  }

  iterator remove(const_iterator position) {
    MOP_ASSERT(position.vector_ == this);
    return remove(iterator(this, position.offset_));
  }

  T *data() { return data_; }
  const T *data() const { return data_; }

 private:
  size_t size_, fill_;
  T* data_;
};

}  // namespace mstd
