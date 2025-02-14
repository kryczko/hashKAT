// Copyright (c) 2010, Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// ---

#ifndef UTIL_GTL_HASHTABLE_COMMON_H_
#define UTIL_GTL_HASHTABLE_COMMON_H_

#include <google/sparsehash/sparseconfig.h>
#include <assert.h>
#include <stdexcept>                 // For length_error

// Settings contains parameters for growing and shrinking the table.
// It also packages zero-size functor (ie. hasher).
//
// It does some munging of the hash value in cases where we think
// (fear) the original hash function might not be very good.  In
// particular, the default hash of pointers is the identity hash,
// so probably all the low bits are 0.  We identify when we think
// we're hashing a pointer, and chop off the low bits.  Note this
// isn't perfect: even when the key is a pointer, we can't tell
// for sure that the hash is the identity hash.  If it's not, this
// is needless work (and possibly, though not likely, harmful).

template<typename Key, typename HashFunc,
         typename SizeType, int HT_MIN_BUCKETS>
class sh_hashtable_settings : public HashFunc {
 public:
  typedef Key key_type;
  typedef HashFunc hasher;
  typedef SizeType size_type;

 public:
  sh_hashtable_settings(const hasher& hf,
                        const float ht_occupancy_flt,
                        const float ht_empty_flt)
      : hasher(hf),
//        enlarge_threshold_(0),
//        shrink_threshold_(0),
//        consider_shrink_(false),
//        use_empty_(false),
//        use_deleted_(false),
        num_deleted(0)
 {
    set_enlarge_factor(ht_occupancy_flt);
    set_shrink_factor(ht_empty_flt);
  }

  size_type hash(const key_type& v) const {
    // We munge the hash value when we don't trust hasher::operator().
    return hash_munger<Key>::MungedHash(hasher::operator()(v));
  }

  float enlarge_factor() const {
    return 0.80f;
  }
  void set_enlarge_factor(float f) {
//    enlarge_factor_ = f;
  }
  float shrink_factor() const {
    return 0.32f;
  }
  void set_shrink_factor(float f) {
//    shrink_factor_ = f;
  }
  void set_shrink_threshold(size_type t) {
//    shrink_threshold_ = t;
  }

  size_type enlarge_size(size_type x) const {
    return x * 8u / 10u;
  }
  size_type shrink_size(size_type x) const {
    return x * 32u / 100u;
  }

  bool consider_shrink() const {
    return false;
  }
  void set_consider_shrink(bool t) {
//    consider_shrink_ = t;
  }

  bool use_empty() const {
    return false;//use_empty_;
  }
  void set_use_empty(bool t) {
//    use_empty_ = t;
  }

  bool use_deleted() const {
    return true;//use_deleted_;
  }
  void set_use_deleted(bool t) {
//    use_deleted_ = t;
  }

  size_type num_ht_copies() const {
      return 0;
//    return static_cast<size_type>(num_ht_copies_);
  }
  void inc_num_ht_copies() {
//    ++num_ht_copies_;
  }

  // Reset the enlarge and shrink thresholds
  void reset_thresholds(size_type num_buckets) {
//    set_enlarge_threshold(enlarge_size(num_buckets));
//    set_shrink_threshold(shrink_size(num_buckets));
    // whatever caused us to reset already considered
//    set_consider_shrink(false);
  }

  // Caller is resposible for calling reset_threshold right after
  // set_resizing_parameters.
  void set_resizing_parameters(float shrink, float grow) {
    assert(shrink >= 0.0);
    assert(grow <= 1.0);
    if (shrink > grow/2.0f)
      shrink = grow / 2.0f;     // otherwise we thrash hashtable size
    set_shrink_factor(shrink);
    set_enlarge_factor(grow);
  }

  // This is the smallest size a hashtable can be without being too crowded
  // If you like, you can give a min #buckets as well as a min #elts
  size_type min_buckets(size_type num_elts, size_type min_buckets_wanted) {
    float enlarge = enlarge_factor();
    size_type sz = HT_MIN_BUCKETS;             // min buckets allowed
    while ( sz < min_buckets_wanted ||
            num_elts >= static_cast<size_type>(sz * enlarge) ) {
      // This just prevents overflowing size_type, since sz can exceed
      // max_size() here.
      if (static_cast<size_type>(sz * 2) < sz) {
        throw std::length_error("resize overflow");  // protect against overflow
      }
      sz *= 2;
    }
    return sz;
  }

 public:
  template<class HashKey> class hash_munger {
   public:
    static size_t MungedHash(size_t hash) {
      return hash;
    }
  };
  // This matches when the hashtable key is a pointer.
  template<class HashKey> class hash_munger<HashKey*> {
   public:
    static size_t MungedHash(size_t hash) {
      // TODO(csilvers): consider rotating instead:
      //    static const int shift = (sizeof(void *) == 4) ? 2 : 3;
      //    return (hash << (sizeof(hash) * 8) - shift)) | (hash >> shift);
      // This matters if we ever change sparse/dense_hash_* to compare
      // hashes before comparing actual values.  It's speedy on x86.
      return hash / sizeof(void*);   // get rid of known-0 bits
    }
  };
  // AD: This class has been hacked to bits to save on memory
  // Fields were here, currently this class has none of the members it did have.
  // This was done to save on memory.
  unsigned int num_deleted;   // how many occupied buckets are marked deleted
};

#endif  // UTIL_GTL_HASHTABLE_COMMON_H_
