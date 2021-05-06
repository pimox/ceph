// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*-
// vim: ts=8 sw=2 smarttab
/*
 * Ceph - scalable distributed file system
 *
 * Copyright (C) 2004-2006 Sage Weil <sage@newdream.net>
 *
 * This is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1, as published by the Free Software
 * Foundation.  See file COPYING.
 *
 */


#ifndef OS_LFNINDEX_H
#define OS_LFNINDEX_H

#include <string>
#include <map>
#include <set>
#include <vector>
#include <exception>

#include "osd/osd_types.h"
#include "include/object.h"
#include "common/ceph_crypto.h"

#include "CollectionIndex.h"

/**
 * LFNIndex also encapsulates logic for manipulating
 * subdirectories of a collection as well as the long filename
 * logic.
 *
 * The protected methods provide machinery for derived classes to
 * manipulate subdirectories and objects.
 *
 * The virtual methods are to be overridden to provide the actual
 * hashed layout.
 *
 * User must call created when an object is created.
 *
 * Synchronization: Calling code must ensure that there are no object
 * creations or deletions during the lifetime of a Path object (except
 * of an object at that path).
 *
 * Unless otherwise noted, methods which return an int return 0 on success
 * and a negative error code on failure.
 */
#define WRAP_RETRY(x) {				\
  bool failed = false;				\
  int r = 0;					\
  init_inject_failure();			\
  while (1) {					\
    try {					\
      if (failed) {				\
	r = cleanup();				\
	ceph_assert(r == 0);				\
      }						\
      { x }					\
      out:					\
      complete_inject_failure();		\
      return r;					\
    } catch (RetryException&) {			\
      failed = true;				\
    } catch (...) {				\
      ceph_abort();				\
    }						\
  }						\
  return -1;					\
  }						\



class LFNIndex : public CollectionIndex {
  /// Hash digest output size.
  static const int FILENAME_LFN_DIGEST_SIZE = CEPH_CRYPTO_SHA1_DIGESTSIZE;
  /// Length of filename hash.
  static const int FILENAME_HASH_LEN = FILENAME_LFN_DIGEST_SIZE;
  /// Max filename size.
  static const int FILENAME_MAX_LEN = 4096;
  /// Length of hashed filename.
  static const int FILENAME_SHORT_LEN = 255;
  /// Length of hashed filename prefix.
  static const int FILENAME_PREFIX_LEN;
  /// Length of hashed filename cookie.
  static const int FILENAME_EXTRA = 4;
  /// Lfn cookie value.
  static const std::string FILENAME_COOKIE;
  /// Name of LFN attribute for storing full name.
  static const std::string LFN_ATTR;
  /// Prefix for subdir index attributes.
  static const std::string PHASH_ATTR_PREFIX;
  /// Prefix for index subdirectories.
  static const std::string SUBDIR_PREFIX;

  /// Path to Index base.
  const std::string base_path;

protected:
  const uint32_t index_version;

  /// true if retry injection is enabled
  struct RetryException : public std::exception {};
  bool error_injection_enabled;
  bool error_injection_on;
  double error_injection_probability;
  uint64_t last_failure;
  uint64_t current_failure;
  void init_inject_failure() {
    if (error_injection_on) {
      error_injection_enabled = true;
      last_failure = current_failure = 0;
    }
  }
  void maybe_inject_failure();
  void complete_inject_failure() {
    error_injection_enabled = false;
  }

private:
  std::string lfn_attribute, lfn_alt_attribute;
  coll_t collection;

public:
  /// Constructor
  LFNIndex(
    CephContext* cct,
    coll_t collection,
    const char *base_path, ///< [in] path to Index root
    uint32_t index_version,
    double _error_injection_probability=0)
    : CollectionIndex(cct, collection),
      base_path(base_path),
      index_version(index_version),
      error_injection_enabled(false),
      error_injection_on(_error_injection_probability != 0),
      error_injection_probability(_error_injection_probability),
      last_failure(0), current_failure(0),
      collection(collection) {
    if (index_version == HASH_INDEX_TAG) {
      lfn_attribute = LFN_ATTR;
    } else {
      char buf[100];
      snprintf(buf, sizeof(buf), "%d", index_version);
      lfn_attribute = LFN_ATTR + std::string(buf);
      lfn_alt_attribute = LFN_ATTR + std::string(buf) + "-alt";
   }
  }

  coll_t coll() const override { return collection; }

  /// Virtual destructor
  ~LFNIndex() override {}

  /// @see CollectionIndex
  int init() override;

  /// @see CollectionIndex
  int cleanup() override = 0;

  /// @see CollectionIndex
  int created(
    const ghobject_t &oid,
    const char *path
    ) override;

  /// @see CollectionIndex
  int unlink(
    const ghobject_t &oid
    ) override;

  /// @see CollectionIndex
  int lookup(
    const ghobject_t &oid,
    IndexedPath *path,
    int *hardlink
    ) override;

  /// @see CollectionIndex;
  int pre_hash_collection(
      uint32_t pg_num,
      uint64_t expected_num_objs
      ) override;

  /// @see CollectionIndex
  int collection_list_partial(
    const ghobject_t &start,
    const ghobject_t &end,
    int max_count,
    std::vector<ghobject_t> *ls,
    ghobject_t *next
    ) override;

  virtual int _split(
    uint32_t match,                             //< [in] value to match
    uint32_t bits,                              //< [in] bits to check
    CollectionIndex* dest                       //< [in] destination index
    ) = 0;
  virtual int _merge(
    uint32_t bits,                              //< [in] bits for target
    CollectionIndex* dest                       //< [in] destination index
    ) = 0;

  /// @see CollectionIndex
  int split(
    uint32_t match,
    uint32_t bits,
    CollectionIndex* dest
    ) override {
    WRAP_RETRY(
      r = _split(match, bits, dest);
      goto out;
      );
  }

  /// @see CollectionIndex
  int merge(
    uint32_t bits,
    CollectionIndex* dest
    ) override {
    WRAP_RETRY(
      r = _merge(bits, dest);
      goto out;
      );
  }

  /**
   * Returns the length of the longest escaped name which could result
   * from any clone, shard, or rollback object of this object
   */
  static uint64_t get_max_escaped_name_len(const hobject_t &obj);

protected:
  virtual int _init() = 0;

  /// Will be called upon object creation
  virtual int _created(
    const std::vector<std::string> &path, ///< [in] Path to subdir.
    const ghobject_t &oid,      ///< [in] Object created.
    const std::string &mangled_name  ///< [in] Mangled filename.
    ) = 0;

  /// Will be called to remove an object
  virtual int _remove(
    const std::vector<std::string> &path,     ///< [in] Path to subdir.
    const ghobject_t &oid,          ///< [in] Object to remove.
    const std::string &mangled_name	    ///< [in] Mangled filename.
    ) = 0;

  /// Return the path and mangled_name for oid.
  virtual int _lookup(
    const ghobject_t &oid,///< [in] Object for lookup.
    std::vector<std::string> *path, ///< [out] Path to the object.
    std::string *mangled_name, ///< [out] Mangled filename.
    int *exists		  ///< [out] True if the object exists.
    ) = 0;

  /// Pre-hash the collection with the given pg number and
  /// expected number of objects in the collection.
  virtual int _pre_hash_collection(
      uint32_t pg_num,
      uint64_t expected_num_objs
      ) = 0;

  /// @see CollectionIndex
  virtual int _collection_list_partial(
    const ghobject_t &start,
    const ghobject_t &end,
    int max_count,
    std::vector<ghobject_t> *ls,
    ghobject_t *next
    ) = 0;

protected:

  /* Non-virtual utility methods */

  /// Sync a subdirectory
  int fsync_dir(
    const std::vector<std::string> &path ///< [in] Path to sync
    ); ///< @return Error Code, 0 on success

  /// Link an object from from into to
  int link_object(
    const std::vector<std::string> &from,   ///< [in] Source subdirectory.
    const std::vector<std::string> &to,     ///< [in] Dest subdirectory.
    const ghobject_t &oid,        ///< [in] Object to move.
    const std::string &from_short_name ///< [in] Mangled filename of oid.
    ); ///< @return Error Code, 0 on success

  /**
   * Efficiently remove objects from a subdirectory
   *
   * remove_object invalidates mangled names in the directory requiring
   * the mangled name of each additional object to be looked up a second
   * time.  remove_objects removes the need for additional lookups
   *
   * @param [in] dir Directory from which to remove.
   * @param [in] map of objects to remove to mangle names
   * @param [in,out] map of filenames to objects
   * @return Error Code, 0 on success.
   */
  int remove_objects(
    const std::vector<std::string> &dir,
    const std::map<std::string, ghobject_t> &to_remove,
    std::map<std::string, ghobject_t> *remaining
    );


  /**
   * Moves contents of from into to.
   *
   * Invalidates mangled names in to.  If interrupted, all objects will be
   * present in to before objects are removed from from.  Ignores EEXIST
   * while linking into to.
   * @return Error Code, 0 on success
   */
  int move_objects(
    const std::vector<std::string> &from, ///< [in] Source subdirectory.
    const std::vector<std::string> &to    ///< [in] Dest subdirectory.
    );

  /**
   * Remove an object from from.
   *
   * Invalidates mangled names in from.
   * @return Error Code, 0 on success
   */
  int remove_object(
    const std::vector<std::string> &from,  ///< [in] Directory from which to remove.
    const ghobject_t &to_remove   ///< [in] Object to remove.
    );

  /**
   * Gets the filename corresponding to oid in from.
   *
   * The filename may differ between subdirectories.  Furthermore,
   * file creations ore removals in from may invalidate the name.
   * @return Error code on failure, 0 on success
   */
  int get_mangled_name(
    const std::vector<std::string> &from, ///< [in] Subdirectory
    const ghobject_t &oid,	///< [in] Object
    std::string *mangled_name,	///< [out] Filename
    int *hardlink		///< [out] hardlink for this file, hardlink=0 mean no-exist
    );

  /// do move subdir from from to dest
  static int move_subdir(
    LFNIndex &from,             ///< [in] from index
    LFNIndex &dest,             ///< [in] to index
    const std::vector<std::string> &path, ///< [in] path containing dir
    std::string dir                  ///< [in] dir to move
    );

  /// do move object from from to dest
  static int move_object(
    LFNIndex &from,             ///< [in] from index
    LFNIndex &dest,             ///< [in] to index
    const std::vector<std::string> &path, ///< [in] path to split
    const std::pair<std::string, ghobject_t> &obj ///< [in] obj to move
    );

  /**
   * Lists objects in to_list.
   *
   * @param [in] to_list Directory to list.
   * @param [in] max_objects Max number to list.
   * @param [in,out] handle Cookie for continuing the listing.
   * Initialize to zero to start at the beginning of the directory.
   * @param [out] out Mapping of listed object filenames to objects.
   * @return Error code on failure, 0 on success
   */
  int list_objects(
    const std::vector<std::string> &to_list,
    int max_objects,
    long *handle,
    std::map<std::string, ghobject_t> *out
    );

  /// Lists subdirectories.
  int list_subdirs(
    const std::vector<std::string> &to_list, ///< [in] Directory to list.
    std::vector<std::string> *out		   ///< [out] Subdirectories listed.
    );

  /// Create subdirectory.
  int create_path(
    const std::vector<std::string> &to_create ///< [in] Subdirectory to create.
    );

  /// Remove subdirectory.
  int remove_path(
    const std::vector<std::string> &to_remove ///< [in] Subdirectory to remove.
    );

  /// Check whether to_check exists.
  int path_exists(
    const std::vector<std::string> &to_check, ///< [in] Subdirectory to check.
    int *exists			    ///< [out] 1 if it exists, 0 else
    );

  /// Save attr_value to attr_name attribute on path.
  int add_attr_path(
    const std::vector<std::string> &path, ///< [in] Path to modify.
    const std::string &attr_name, 	///< [in] Name of attribute.
    ceph::buffer::list &attr_value	///< [in] Value to save.
    );

  /// Read into attr_value attribute attr_name on path.
  int get_attr_path(
    const std::vector<std::string> &path, ///< [in] Path to read.
    const std::string &attr_name, 	///< [in] Attribute to read.
    ceph::buffer::list &attr_value	///< [out] Attribute value read.
    );

  /// Remove attr from path
  int remove_attr_path(
    const std::vector<std::string> &path, ///< [in] path from which to remove attr
    const std::string &attr_name	///< [in] attr to remove
    ); ///< @return Error code, 0 on success

private:
  /* lfn translation functions */

  /**
   * Gets the version specific lfn attribute tag
   */
  const std::string &get_lfn_attr() const {
    return lfn_attribute;
  }
  const std::string &get_alt_lfn_attr() const {
    return lfn_alt_attribute;
  }

  /**
   * Gets the filename corresponding to oid in path.
   *
   * @param [in] path Path in which to get filename for oid.
   * @param [in] oid Object for which to get filename.
   * @param [out] mangled_name Filename for oid, pass NULL if not needed.
   * @param [out] full_path Fullpath for oid, pass NULL if not needed.
   * @param [out] hardlink of this file, 0 mean no-exist, pass NULL if
   * not needed
   * @return Error Code, 0 on success.
   */
  int lfn_get_name(
    const std::vector<std::string> &path,
    const ghobject_t &oid,
    std::string *mangled_name,
    std::string *full_path,
    int *hardlink
    );

  /// Adjusts path contents when oid is created at name mangled_name.
  int lfn_created(
    const std::vector<std::string> &path, ///< [in] Path to adjust.
    const ghobject_t &oid,	///< [in] Object created.
    const std::string &mangled_name  ///< [in] Filename of created object.
    );

  /// Removes oid from path while adjusting path contents
  int lfn_unlink(
    const std::vector<std::string> &path, ///< [in] Path containing oid.
    const ghobject_t &oid,	///< [in] Object to remove.
    const std::string &mangled_name	///< [in] Filename of object to remove.
    );

  ///Transate a file into and ghobject_t.
  int lfn_translate(
    const std::vector<std::string> &path, ///< [in] Path containing the file.
    const std::string &short_name,	///< [in] Filename to translate.
    ghobject_t *out		///< [out] Object found.
    ); ///< @return Negative error code on error, 0 if not an object, 1 else

  /* manglers/demanglers */
  /// Filters object filenames
  bool lfn_is_object(
    const std::string &short_name ///< [in] Filename to check
    ); ///< True if short_name is an object, false otherwise

  /// Filters subdir filenames
  bool lfn_is_subdir(
    const std::string &short_name, ///< [in] Filename to check.
    std::string *demangled_name    ///< [out] Demangled subdir name.
    ); ///< @return True if short_name is a subdir, false otherwise

  /// Generate object name
  std::string lfn_generate_object_name_keyless(
    const ghobject_t &oid ///< [in] Object for which to generate.
    ); ///< @return Generated object name.

  /// Generate object name
  std::string lfn_generate_object_name_poolless(
    const ghobject_t &oid ///< [in] Object for which to generate.
    ); ///< @return Generated object name.

  /// Generate object name
  static std::string lfn_generate_object_name_current(
    const ghobject_t &oid ///< [in] Object for which to generate.
    ); ///< @return Generated object name.

  /// Generate object name
  std::string lfn_generate_object_name(
    const ghobject_t &oid ///< [in] Object for which to generate.
    ) {
    if (index_version == HASH_INDEX_TAG)
      return lfn_generate_object_name_keyless(oid);
    if (index_version == HASH_INDEX_TAG_2)
      return lfn_generate_object_name_poolless(oid);
    else
      return lfn_generate_object_name_current(oid);
  } ///< @return Generated object name.

  /// Parse object name
  int lfn_parse_object_name_keyless(
    const std::string &long_name, ///< [in] Name to parse
    ghobject_t *out	     ///< [out] Resulting Object
    ); ///< @return True if successful, False otherwise.

  /// Parse object name
  int lfn_parse_object_name_poolless(
    const std::string &long_name, ///< [in] Name to parse
    ghobject_t *out	     ///< [out] Resulting Object
    ); ///< @return True if successful, False otherwise.

  /// Parse object name
  int lfn_parse_object_name(
    const std::string &long_name, ///< [in] Name to parse
    ghobject_t *out	     ///< [out] Resulting Object
    ); ///< @return True if successful, False otherwise.

  /// Checks whether short_name is a hashed filename.
  bool lfn_is_hashed_filename(
    const std::string &short_name ///< [in] Name to check.
    ); ///< @return True if short_name is hashed, False otherwise.

  /// Checks whether long_name must be hashed.
  bool lfn_must_hash(
    const std::string &long_name ///< [in] Name to check.
    ); ///< @return True if long_name must be hashed, False otherwise.

  /// Generate hashed name.
  std::string lfn_get_short_name(
    const ghobject_t &oid, ///< [in] Object for which to generate.
    int i		   ///< [in] Index of hashed name to generate.
    ); ///< @return Hashed filename.

  /* other common methods */
  /// Gets the base path
  const std::string &get_base_path(); ///< @return Index base_path

  /// Get full path the subdir
  std::string get_full_path_subdir(
    const std::vector<std::string> &rel ///< [in] The subdir.
    ); ///< @return Full path to rel.

  /// Get full path to object
  std::string get_full_path(
    const std::vector<std::string> &rel, ///< [in] Path to object.
    const std::string &name	       ///< [in] Filename of object.
    ); ///< @return Fullpath to object at name in rel.

  /// Get mangled path component
  std::string mangle_path_component(
    const std::string &component ///< [in] Component to mangle
    ); /// @return Mangled component

  /// Demangle component
  std::string demangle_path_component(
    const std::string &component ///< [in] Subdir name to demangle
    ); ///< @return Demangled path component.

  /// Decompose full path into object name and filename.
  int decompose_full_path(
    const char *in,      ///< [in] Full path to object.
    std::vector<std::string> *out, ///< [out] Path to object at in.
    ghobject_t *oid,	 ///< [out] Object at in.
    std::string *shortname	 ///< [out] Filename of object at in.
    ); ///< @return Error Code, 0 on success.

  /// Mangle attribute name
  std::string mangle_attr_name(
    const std::string &attr ///< [in] Attribute to mangle.
    ); ///< @return Mangled attribute name.

  /// checks whether long_name could hash to short_name
  bool short_name_matches(
    const char *short_name,    ///< [in] name to check against
    const char *cand_long_name ///< [in] candidate long name
    );

  /// Builds hashed filename
  void build_filename(
    const char *old_filename, ///< [in] Filename to convert.
    int i,		      ///< [in] Index of hash.
    char *filename,	      ///< [out] Resulting filename.
    int len		      ///< [in] Size of buffer for filename
    ); ///< @return Error Code, 0 on success

  /// Get hash of filename
  int hash_filename(
    const char *filename, ///< [in] Filename to hash.
    char *hash,		  ///< [out] Hash of filename.
    int len		  ///< [in] Size of hash buffer.
    ); ///< @return Error Code, 0 on success.

  friend class TestWrapLFNIndex;
};
typedef LFNIndex::IndexedPath IndexedPath;

#endif
