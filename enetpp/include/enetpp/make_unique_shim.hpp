#pragma once

#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>

namespace std {
    template<class T> struct _Unique_if_shim {
        typedef unique_ptr<T> _Single_object;
    };

    template<class T> struct _Unique_if_shim<T[]> {
        typedef unique_ptr<T[]> _Unknown_bound;
    };

    template<class T, size_t N> struct _Unique_if_shim<T[N]> {
        typedef void _Known_bound;
    };

    template<class T, class... Args>
        typename _Unique_if_shim<T>::_Single_object
        make_unique_shim(Args&&... args) {
            return unique_ptr<T>(new T(std::forward<Args>(args)...));
        }

    template<class T>
        typename _Unique_if_shim<T>::_Unknown_bound
        make_unique_shim(size_t n) {
            typedef typename remove_extent<T>::type U;
            return unique_ptr<T>(new U[n]());
        }

    template<class T, class... Args>
        typename _Unique_if_shim<T>::_Known_bound
        make_unique_shim(Args&&...) = delete;
}
