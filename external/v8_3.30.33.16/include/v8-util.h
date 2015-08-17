// Copyright 2014 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_UTIL_H_
#define V8_UTIL_H_

#include "v8.h"
#include <map>
#include <vector>

/**
 * Support for Persistent containers.
 *
 * C++11 embedders can use STL containers with UniquePersistent values,
 * but pre-C++11 does not support the required move semantic and hence
 * may want these container classes.
 */
namespace v8 {

typedef uintptr_t PersistentContainerValue;
static const uintptr_t kPersistentContainerNotFound = 0;
enum PersistentContainerCallbackType {
  kNotWeak,
  kWeak
};


/**
 * A default trait implemenation for PersistentValueMap which uses std::map
 * as a backing map.
 *
 * Users will have to implement their own weak callbacks & dispose traits.
 */
template<typename K, typename V>
class StdMapTraits {
 public:
  // STL map & related:
  typedef std::map<K, PersistentContainerValue> Impl;
  typedef typename Impl::iterator Iterator;

  static bool Empty(Impl* impl) { return impl->empty(); }
  static size_t Size(Impl* impl) { return impl->size(); }
  static void Swap(Impl& a, Impl& b) { std::swap(a, b); }  // NOLINT
  static Iterator Begin(Impl* impl) { return impl->begin(); }
  static Iterator End(Impl* impl) { return impl->end(); }
  static K Key(Iterator it) { return it->first; }
  static PersistentContainerValue Value(Iterator it) { return it->second; }
  static PersistentContainerValue Set(Impl* impl, K key,
      PersistentContainerValue value) {
    std::pair<Iterator, bool> res = impl->insert(std::make_pair(key, value));
    PersistentContainerValue old_value = kPersistentContainerNotFound;
    if (!res.second) {
      old_value = res.first->second;
      res.first->second = value;
    }
    return old_value;
  }
  static PersistentContainerValue Get(Impl* impl, K key) {
    Iterator it = impl->find(key);
    if (it == impl->end()) return kPersistentContainerNotFound;
    return it->second;
  }
  static PersistentContainerValue Remove(Impl* impl, K key) {
    Iterator it = impl->find(key);
    if (it == impl->end()) return kPersistentContainerNotFound;
    PersistentContainerValue value = it->second;
    impl->erase(it);
    return value;
  }
};


/**
 * A default trait implementation for PersistentValueMap, which inherits
 * a std:map backing map from StdMapTraits and holds non-weak persistent
 * objects and has no special Dispose handling.
 *
 * You should not derive from this class, since MapType depends on the
 * surrounding class, and hence a subclass cannot simply inherit the methods.
 */
template<typename K, typename V>
class DefaultPersistentValueMapTraits : public StdMapTraits<K, V> {
 public:
  // Weak callback & friends:
  static const PersistentContainerCallbackType kCallbackType = kNotWeak;
  typedef PersistentValueMap<K, V, DefaultPersistentValueMapTraits<K, V> >
      MapType;
  typedef void WeakCallbackDataType;

  static WeakCallbackDataType* WeakCallbackParameter(
      MapType* map, const K& key, Local<V> value) {
    return NULL;
  }
  static MapType* MapFromWeakCallbackData(
          const WeakCallbackData<V, WeakCallbackDataType>& data) {
    return NULL;
  }
  static K KeyFromWeakCallbackData(
      const WeakCallbackData<V, WeakCallbackDataType>& data) {
    return K();
  }
  static void DisposeCallbackData(WeakCallbackDataType* data) { }
  static void Dispose(Isolate* isolate, UniquePersistent<V> value, K key) { }
};


/**
 * A map wrapper that allows using UniquePersistent as a mapped value.
 * C++11 embedders don't need this class, as they can use UniquePersistent
 * directly in std containers.
 *
 * The map relies on a backing map, whose type and accessors are described
 * by the Traits class. The backing map will handle values of type
 * PersistentContainerValue, with all conversion into and out of V8
 * handles being transparently handled by this class.
 */
template<typename K, typename V, typename Traits>
class PersistentValueMap {
 public:
  explicit PersistentValueMap(Isolate* isolate) : isolate_(isolate) {}

  ~PersistentValueMap() { Clear(); }

  Isolate* GetIsolate() { return isolate_; }

  /**
   * Return size of the map.
   */
  size_t Size() { return Traits::Size(&impl_); }

  /**
   * Return whether the map holds weak persistents.
   */
  bool IsWeak() { return Traits::kCallbackType != kNotWeak; }

  /**
   * Get value stored in map.
   */
  Local<V> Get(const K& key) {
    return Local<V>::New(isolate_, FromVal(Traits::Get(&impl_, key)));
  }

  /**
   * Check whether a value is contained in the map.
   */
  bool Contains(const K& key) {
    return Traits::Get(&impl_, key) != kPersistentContainerNotFound;
  }

  /**
   * Get value stored in map and set it in returnValue.
   * Return true if a value was found.
   */
  bool SetReturnValue(const K& key,
      ReturnValue<Value> returnValue) {
    return SetReturnValueFromVal(&returnValue, Traits::Get(&impl_, key));
  }

  /**
   * Call Isolate::SetReference with the given parent and the map value.
   */
  void SetReference(const K& key,
      const Persistent<Object>& parent) {
    GetIsolate()->SetReference(
      reinterpret_cast<internal::Object**>(parent.val_),
      reinterpret_cast<internal::Object**>(FromVal(Traits::Get(&impl_, key))));
  }

  /**
   * Put value into map. Depending on Traits::kIsWeak, the value will be held
   * by the map strongly or weakly.
   * Returns old value as UniquePersistent.
   */
  UniquePersistent<V> Set(const K& key, Local<V> value) {
    UniquePersistent<V> persistent(isolate_, value);
    return SetUnique(key, &persistent);
  }

  /**
   * Put value into map, like Set(const K&, Local<V>).
   */
  UniquePersistent<V> Set(const K& key, UniquePersistent<V> value) {
    return SetUnique(key, &value);
  }

  /**
   * Return value for key and remove it from the map.
   */
  UniquePersistent<V> Remove(const K& key) {
    return Release(Traits::Remove(&impl_, key)).Pass();
  }

  /**
  * Traverses the map repeatedly,
  * in case side effects of disposal cause insertions.
  **/
  void Clear() {
    typedef typename Traits::Iterator It;
    HandleScope handle_scope(isolate_);
    // TODO(dcarney): figure out if this swap and loop is necessary.
    while (!Traits::Empty(&impl_)) {
      typename Traits::Impl impl;
      Traits::Swap(impl_, impl);
      for (It i = Traits::Begin(&impl); i != Traits::End(&impl); ++i) {
        Traits::Dispose(isolate_, Release(Traits::Value(i)).Pass(),
                        Traits::Key(i));
      }
    }
  }

  /**
   * Helper class for GetReference/SetWithReference. Do not use outside
   * that context.
   */
  class PersistentValueReference {
   public:
    PersistentValueReference() : value_(kPersistentContainerNotFound) { }
    PersistentValueReference(const PersistentValueReference& other)
        : value_(other.value_) { }

    Local<V> NewLocal(Isolate* isolate) const {
      return Local<V>::New(isolate, FromVal(value_));
    }
    bool IsEmpty() const {
      return value_ == kPersistentContainerNotFound;
    }
    template<typename T>
    bool SetReturnValue(ReturnValue<T> returnValue) {
      return SetReturnValueFromVal(&returnValue, value_);
    }
    void Reset() {
      value_ = kPersistentContainerNotFound;
    }
    void operator=(const PersistentValueReference& other) {
      value_ = other.value_;
    }

   private:
    friend class PersistentValueMap;

    explicit PersistentValueReference(PersistentContainerValue value)
        : value_(value) { }

    void operator=(PersistentContainerValue value) {
      value_ = value;
    }

    PersistentContainerValue value_;
  };

  /**
   * Get a reference to a map value. This enables fast, repeated access
   * to a value stored in the map while the map remains unchanged.
   *
   * Careful: This is potentially unsafe, so please use with care.
   * The value will become invalid if the value for this key changes
   * in the underlying map, as a result of Set or Remove for the same
   * key; as a result of the weak callback for the same key; or as a
   * result of calling Clear() or destruction of the map.
   */
  PersistentValueReference GetReference(const K& key) {
    return PersistentValueReference(Traits::Get(&impl_, key));
  }

  /**
   * Put a value into the map and update the reference.
   * Restrictions of GetReference apply here as well.
   */
  UniquePersistent<V> Set(const K& key, UniquePersistent<V> value,
                          PersistentValueReference* reference) {
    *reference = Leak(&value);
    return SetUnique(key, &value);
  }

 private:
  PersistentValueMap(PersistentValueMap&);
  void operator=(PersistentValueMap&);

  /**
   * Put the value into the map, and set the 'weak' callback when demanded
   * by the Traits class.
   */
  UniquePersistent<V> SetUnique(const K& key, UniquePersistent<V>* persistent) {
    if (Traits::kCallbackType != kNotWeak) {
      Local<V> value(Local<V>::New(isolate_, *persistent));
      persistent->template SetWeak<typename Traits::WeakCallbackDataType>(
        Traits::WeakCallbackParameter(this, key, value), WeakCallback);
    }
    PersistentContainerValue old_value =
        Traits::Set(&impl_, key, ClearAndLeak(persistent));
    return Release(old_value).Pass();
  }

  static void WeakCallback(
      const WeakCallbackData<V, typename Traits::WeakCallbackDataType>& data) {
    if (Traits::kCallbackType != kNotWeak) {
      PersistentValueMap<K, V, Traits>* persistentValueMap =
          Traits::MapFromWeakCallbackData(data);
      K key = Traits::KeyFromWeakCallbackData(data);
      Traits::Dispose(data.GetIsolate(),
                      persistentValueMap->Remove(key).Pass(), key);
      Traits::DisposeCallbackData(data.GetParameter());
    }
  }

  static V* FromVal(PersistentContainerValue v) {
    return reinterpret_cast<V*>(v);
  }

  static bool SetReturnValueFromVal(
      ReturnValue<Value>* returnValue, PersistentContainerValue value) {
    bool hasValue = value != kPersistentContainerNotFound;
    if (hasValue) {
      returnValue->SetInternal(
          *reinterpret_cast<internal::Object**>(FromVal(value)));
    }
    return hasValue;
  }

  static PersistentContainerValue ClearAndLeak(
      UniquePersistent<V>* persistent) {
    V* v = persistent->val_;
    persistent->val_ = 0;
    return reinterpret_cast<PersistentContainerValue>(v);
  }

  static PersistentContainerValue Leak(
      UniquePersistent<V>* persistent) {
    return reinterpret_cast<PersistentContainerValue>(persistent->val_);
  }

  /**
   * Return a container value as UniquePersistent and make sure the weak
   * callback is properly disposed of. All remove functionality should go
   * through this.
   */
  static UniquePersistent<V> Release(PersistentContainerValue v) {
    UniquePersistent<V> p;
    p.val_ = FromVal(v);
    if (Traits::kCallbackType != kNotWeak && p.IsWeak()) {
      Traits::DisposeCallbackData(
          p.template ClearWeak<typename Traits::WeakCallbackDataType>());
    }
    return p.Pass();
  }

  Isolate* isolate_;
  typename Traits::Impl impl_;
};


/**
 * A map that uses UniquePersistent as value and std::map as the backing
 * implementation. Persistents are held non-weak.
 *
 * C++11 embedders don't need this class, as they can use
 * UniquePersistent directly in std containers.
 */
template<typename K, typename V,
    typename Traits = DefaultPersistentValueMapTraits<K, V> >
class StdPersistentValueMap : public PersistentValueMap<K, V, Traits> {
 public:
  explicit StdPersistentValueMap(Isolate* isolate)
      : PersistentValueMap<K, V, Traits>(isolate) {}
};


class DefaultPersistentValueVectorTraits {
 public:
  typedef std::vector<PersistentContainerValue> Impl;

  static void Append(Impl* impl, PersistentContainerValue value) {
    impl->push_back(value);
  }
  static bool IsEmpty(const Impl* impl) {
    return impl->empty();
  }
  static size_t Size(const Impl* impl) {
    return impl->size();
  }
  static PersistentContainerValue Get(const Impl* impl, size_t i) {
    return (i < impl->size()) ? impl->at(i) : kPersistentContainerNotFound;
  }
  static void ReserveCapacity(Impl* impl, size_t capacity) {
    impl->reserve(capacity);
  }
  static void Clear(Impl* impl) {
    impl->clear();
  }
};


/**
 * A vector wrapper that safely stores UniquePersistent values.
 * C++11 embedders don't need this class, as they can use UniquePersistent
 * directly in std containers.
 *
 * This class relies on a backing vector implementation, whose type and methods
 * are described by the Traits class. The backing map will handle values of type
 * PersistentContainerValue, with all conversion into and out of V8
 * handles being transparently handled by this class.
 */
template<typename V, typename Traits = DefaultPersistentValueVectorTraits>
class PersistentValueVector {
 public:
  explicit PersistentValueVector(Isolate* isolate) : isolate_(isolate) { }

  ~PersistentValueVector() {
    Clear();
  }

  /**
   * Append a value to the vector.
   */
  void Append(Local<V> value) {
    UniquePersistent<V> persistent(isolate_, value);
    Traits::Append(&impl_, ClearAndLeak(&persistent));
  }

  /**
   * Append a persistent's value to the vector.
   */
  void Append(UniquePersistent<V> persistent) {
    Traits::Append(&impl_, ClearAndLeak(&persistent));
  }

  /**
   * Are there any values in the vector?
   */
  bool IsEmpty() const {
    return Traits::IsEmpty(&impl_);
  }

  /**
   * How many elements are in the vector?
   */
  size_t Size() const {
    return Traits::Size(&impl_);
  }

  /**
   * Retrieve the i-th value in the vector.
   */
  Local<V> Get(size_t index) const {
    return Local<V>::New(isolate_, FromVal(Traits::Get(&impl_, index)));
  }

  /**
   * Remove all elements from the vector.
   */
  void Clear() {
    size_t length = Traits::Size(&impl_);
    for (size_t i = 0; i < length; i++) {
      UniquePersistent<V> p;
      p.val_ = FromVal(Traits::Get(&impl_, i));
    }
    Traits::Clear(&impl_);
  }

  /**
   * Reserve capacity in the vector.
   * (Efficiency gains depend on the backing implementation.)
   */
  void ReserveCapacity(size_t capacity) {
    Traits::ReserveCapacity(&impl_, capacity);
  }

 private:
  static PersistentContainerValue ClearAndLeak(
      UniquePersistent<V>* persistent) {
    V* v = persistent->val_;
    persistent->val_ = 0;
    return reinterpret_cast<PersistentContainerValue>(v);
  }

  static V* FromVal(PersistentContainerValue v) {
    return reinterpret_cast<V*>(v);
  }

  Isolate* isolate_;
  typename Traits::Impl impl_;
};

}  // namespace v8

#endif  // V8_UTIL_H_
