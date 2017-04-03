/* -----------------------------------------------------------------------------
 * std_vector.i
 * ----------------------------------------------------------------------------- */

%include <std_common.i>

%{
#include <vector>
// TODO(marejde): Add a preprocessor define for building without exceptions that
// is set only when building Mobile Android? (Do not forget to update all get()
// and set() methods below if one is added.
// #include <stdexcept>
%}

namespace std {
    
    template<class T> class vector {
      public:
        typedef size_t size_type;
        typedef T value_type;
        typedef const value_type& const_reference;
        vector();
        vector(size_type n);
        size_type size() const;
        size_type capacity() const;
        void reserve(size_type n);
        %rename(isEmpty) empty;
        bool empty() const;
        void clear();
        %rename(add) push_back;
        void push_back(const value_type& x);
        %extend {
            const_reference get(int i) /* throw (std::out_of_range) */ {
                /* int size = int(self->size()); */
                /* if (i>=0 && i<size) */
                /*     return (*self)[i]; */
                /* else */
                /*     throw std::out_of_range("vector index out of range"); */
                return (*self)[i];
            }
            void set(int i, const value_type& val) /*throw (std::out_of_range)*/ {
                /* int size = int(self->size()); */
                /* if (i>=0 && i<size) */
                /*    (*self)[i] = val; */
                /* else */
                /*     throw std::out_of_range("vector index out of range"); */
                (*self)[i] = val;
            }
        }
    };

    // bool specialization
    template<> class vector<bool> {
      public:
        typedef size_t size_type;
        typedef bool value_type;
        typedef bool const_reference;
        vector();
        vector(size_type n);
        size_type size() const;
        size_type capacity() const;
        void reserve(size_type n);
        %rename(isEmpty) empty;
        bool empty() const;
        void clear();
        %rename(add) push_back;
        void push_back(const value_type& x);
        %extend {
          const_reference get(int i) /*throw (std::out_of_range)*/ {
                /* int size = int(self->size()); */
                /* if (i>=0 && i<size) */
                /*     return (*self)[i]; */
                /* else */
                /*     throw std::out_of_range("vector index out of range"); */
                return (*self)[i];
            }
            void set(int i, const value_type& val) /*throw (std::out_of_range)*/ {
                /* int size = int(self->size()); */
                /* if (i>=0 && i<size) */
                /*     (*self)[i] = val; */
                /* else */
                /*     throw std::out_of_range("vector index out of range"); */
                (*self)[i] = val;
            }
        }
    };
}

%define specialize_std_vector(T)
#warning "specialize_std_vector - specialization for type T no longer needed"
%enddef

