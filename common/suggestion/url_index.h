// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_SUGGESTION_URL_INDEX_H_
#define COMMON_SUGGESTION_URL_INDEX_H_

#include <map>
#include <set>
#include <vector>

#include "base/containers/hash_tables.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/string16.h"

class GURL;

namespace opera {

namespace query_parser {
class QueryNode;
}

/**
 * Index of URL entries by mentioned terms. Entries are conceptually a title,
 * an URL, and a numerical ID that is stored instead of actual entry. This
 * enables searching for those IDs based on supplied query terms. Note that
 * this only ever stores numerical IDs and terms, rather than actual URLs and
 * titles, to minimise memory usage.
 *
 * @note This class is not thread-safe.
 */
class URLIndex {
 public:
  typedef int64_t ID;
  typedef std::set<ID> URLIDSet;

  URLIndex();
  ~URLIndex();

  void IndexEntry(ID id, const base::string16& title, const GURL& url);
  void RemoveEntry(ID id);
  void Clear();
  bool IsEmpty() const;

  URLIDSet Search(const std::vector<query_parser::QueryNode*>& query_nodes) const;

 private:
  // Type for term identifiers. Each term encountered in entries is assigned a
  // numerical identifier in order to store fewer copies of the term string.
  // These identifiers are transient in nature, and are not exposed to index
  // clients. They are also invalidated in some situations, like clearing
  // index, or removing last reference to the corresponding term.
  typedef size_t TermID;

  // Terms are stored as wide strings, because that's what chromium's
  // QueryParser uses.
  typedef base::string16 Term;

  typedef base::hash_map<TermID, Term> TermIDMap;
  typedef base::hash_map<Term, TermID> TermMap;
  typedef base::hash_set<TermID> TermIDSet;

  // Note how the map from terms to entry IDs uses std::map rather than
  // hash_map, in order to enable us to do easy prefix matching. Only ordered
  // map can do that in conjunction with lower_bound() method.
  typedef std::map<Term, URLIDSet> TermURLIDMap;
  typedef base::hash_map<ID, TermIDSet> URLIDTermIDMap;

  enum {
    // Invalid term ID used to indicate previously unknown term.
    UNKNOWN_TERM_ID = 0
  };

  /**
   * Fetches ID of a @a term. If the term is not known, newly assigned ID
   * will be returned.
   */
  TermID GetTermID(const Term& term);

  /**
   * Removes assignment between term and ID. Also clears list of entry IDs
   * assigned to that term. For performance reasons it is assumed that mapping
   * from IDs to terms already does not contain this term.
   */
  void RemoveTermID(TermID id);

  /**
   * Gets term IDs for terms in @a text, and adds them to the set @a ids.
   * Previously unknown terms will be assigned new IDs.
   */
  void AddTermIDsForText(const base::string16& text, TermIDSet* ids);

  /**
   * Gathers all the terms based on given entry. This also encapsulates any
   * relevant preprocessing of textual content that is supposed to be used for
   * indexing this entry.
   */
  void AddTermIDsForEntry(
      ID id, const base::string16& title, const GURL& url, TermIDSet* ids);

  void RegisterID(ID id, const TermIDSet& term_ids);
  void UnregisterID(ID id);

  URLIDSet SearchForTerm(const Term& word) const;

  /** All the known terms indexed by their IDs. */
  TermIDMap terms_;

  /**
   * Term identifiers used to save memory. The idea is to use IDs in value
   * position of mapping from entry IDs to terms.
   */
  TermMap term_ids_;

  /** Used for assigning indices to new terms. */
  TermID next_id_;

  /**
   * The actual index: maps terms to set of entry IDs corresponding to the
   * terms. This is used for running actual searches.
   *
   * Note how it uses ordered map in order to enable prefix matching to a
   * certain extent.
   */
  TermURLIDMap index_;

  /**
   * Reversed index for quick processing of entry removal. This allows us to
   * avoid repeating costly query processing, and remove the entry based only
   * on its ID rather than full information used to register it.
   */
  URLIDTermIDMap rev_index_;

  DISALLOW_COPY_AND_ASSIGN(URLIndex);
};

}  // namespace opera

#endif  // COMMON_SUGGESTION_URL_INDEX_H_
