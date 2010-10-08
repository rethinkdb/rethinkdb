/*
 * Summary: C++ interface for memcached server
 *
 * Copy: See Copyright for the status of this software.
 *
 * Authors: Padraig O'Sullivan <osullivan.padraig@gmail.com>
 *          Patrick Galbraith <patg@patg.net>
 */

/**
 * @file memcached.hpp
 * @brief Libmemcached C++ interface
 */

#ifndef LIBMEMCACHEDPP_H
#define LIBMEMCACHEDPP_H

#include <libmemcached/memcached.h>
#include <libmemcached/exception.hpp>

#include <string.h>

#include <sstream>
#include <string>
#include <vector>
#include <map>

namespace memcache
{

/**
 * This is the core memcached library (if later, other objects
 * are needed, they will be created from this class).
 */
class Memcache
{
public:

  Memcache()
    :
      servers_list(),
      memc(),
      result()
  {
    memcached_create(&memc);
  }

  Memcache(const std::string &in_servers_list)
    :
      servers_list(in_servers_list),
      memc(),
      result()
  {
    memcached_create(&memc);
    init();
  }

  Memcache(const std::string &hostname,
           in_port_t port)
    :
      servers_list(),
      memc(),
      result()
  {
    memcached_create(&memc);

    servers_list.append(hostname);
    servers_list.append(":");
    std::ostringstream strsmt;
    strsmt << port;
    servers_list.append(strsmt.str());

    init();
  }

  Memcache(memcached_st *clone)
    :
      servers_list(),
      memc(),
      result()
  {
    memcached_clone(&memc, clone);
  }

  Memcache(const Memcache &rhs)
    :
      servers_list(rhs.servers_list),
      memc(),
      result()
  {
    memcached_clone(&memc, const_cast<memcached_st *>(&rhs.getImpl()));
    init();
  }

  Memcache &operator=(const Memcache &rhs)
  {
    if (this != &rhs)
    {
      memcached_clone(&memc, const_cast<memcached_st *>(&rhs.getImpl()));
      init();
    }

    return *this;
  }

  ~Memcache()
  {
    memcached_free(&memc);
  }

  void init()
  {
    memcached_server_st *servers;
    servers= memcached_servers_parse(servers_list.c_str());
    memcached_server_push(&memc, servers);
    memcached_server_free(servers);
  }

  /**
   * Get the internal memcached_st *
   */
  memcached_st &getImpl()
  {
    return memc;
  }

  /**
   * Get the internal memcached_st *
   */
  const memcached_st &getImpl() const
  {
    return memc;
  }

  /**
   * Return an error string for the given return structure.
   *
   * @param[in] rc a memcached_return_t structure
   * @return error string corresponding to given return code in the library.
   */
  const std::string getError(memcached_return_t rc) const
  {
    /* first parameter to strerror is unused */
    return memcached_strerror(NULL, rc);
  }


  bool setBehavior(memcached_behavior_t flag, uint64_t data)
  {
    memcached_return_t rc;
    rc= memcached_behavior_set(&memc, flag, data);
    return (rc == MEMCACHED_SUCCESS);
  }

  uint64_t getBehavior(memcached_behavior_t flag) {
    return memcached_behavior_get(&memc, flag);
  }

  /**
   * Return the string which contains the list of memcached servers being
   * used.
   *
   * @return a std::string containing the list of memcached servers
   */
  const std::string getServersList() const
  {
    return servers_list;
  }

  /**
   * Set the list of memcached servers to use.
   *
   * @param[in] in_servers_list list of servers
   * @return true on success; false otherwise
   */
  bool setServers(const std::string &in_servers_list)
  {
    servers_list.assign(in_servers_list);
    init();

    return (memcached_server_count(&memc));
  }

  /**
   * Add a server to the list of memcached servers to use.
   *
   * @param[in] server_name name of the server to add
   * @param[in] port port number of server to add
   * @return true on success; false otherwise
   */
  bool addServer(const std::string &server_name, in_port_t port)
  {
    memcached_return_t rc;

    rc= memcached_server_add(&memc, server_name.c_str(), port);

    return (rc == MEMCACHED_SUCCESS);
  }

  /**
   * Remove a server from the list of memcached servers to use.
   *
   * @param[in] server_name name of the server to remove
   * @param[in] port port number of server to remove
   * @return true on success; false otherwise
   */
  bool removeServer(const std::string &server_name, in_port_t port)
  {
    std::string tmp_str;
    std::ostringstream strstm;
    tmp_str.append(",");
    tmp_str.append(server_name);
    tmp_str.append(":");
    strstm << port;
    tmp_str.append(strstm.str());

    //memcached_return_t rc= memcached_server_remove(server);
    
    return false;
  }

  /**
   * Fetches an individual value from the server. mget() must always
   * be called before using this method.
   *
   * @param[in] key key of object to fetch
   * @param[out] ret_val store returned object in this vector
   * @return a memcached return structure
   */
  memcached_return_t fetch(std::string &key,
                         std::vector<char> &ret_val)
  {
    char ret_key[MEMCACHED_MAX_KEY];
    size_t value_length= 0;
    size_t key_length= 0;
    memcached_return_t rc;
    uint32_t flags= 0;
    char *value= memcached_fetch(&memc, ret_key, &key_length,
                                 &value_length, &flags, &rc);
    if (value && ret_val.empty())
    {
      ret_val.reserve(value_length);
      ret_val.assign(value, value + value_length);
      key.assign(ret_key);
      free(value);
    }
    else if (value)
    {
      free(value);
    }

    return rc;
  }

  /**
   * Fetches an individual value from the server.
   *
   * @param[in] key key of object whose value to get
   * @param[out] ret_val object that is retrieved is stored in
   *                     this vector
   * @return true on success; false otherwise
   */
  bool get(const std::string &key,
           std::vector<char> &ret_val) throw (Error)
  {
    uint32_t flags= 0;
    memcached_return_t rc;
    size_t value_length= 0;

    if (key.empty())
    {
      throw(Error("the key supplied is empty!", false));
    }
    char *value= memcached_get(&memc, key.c_str(), key.length(),
                               &value_length, &flags, &rc);
    if (value != NULL && ret_val.empty())
    {
      ret_val.reserve(value_length);
      ret_val.assign(value, value + value_length);
      free(value);
      return true;
    }
    return false;
  }

  /**
   * Fetches an individual from a server which is specified by
   * the master_key parameter that is used for determining which
   * server an object was stored in if key partitioning was
   * used for storage.
   *
   * @param[in] master_key key that specifies server object is stored on
   * @param[in] key key of object whose value to get
   * @param[out] ret_val object that is retrieved is stored in
   *                     this vector
   * @return true on success; false otherwise
   */
  bool getByKey(const std::string &master_key,
                const std::string &key,
                std::vector<char> &ret_val) throw(Error)
  {
    uint32_t flags= 0;
    memcached_return_t rc;
    size_t value_length= 0;

    if (master_key.empty() || key.empty())
    {
      throw(Error("the master key or key supplied is empty!", false));
    }
    char *value= memcached_get_by_key(&memc,
                                      master_key.c_str(), master_key.length(),
                                      key.c_str(), key.length(),
                                      &value_length, &flags, &rc);
    if (value)
    {
      ret_val.reserve(value_length);
      ret_val.assign(value, value + value_length);
      free(value);
      return true;
    }
    return false;
  }

  /**
   * Selects multiple keys at once. This method always
   * works asynchronously.
   *
   * @param[in] keys vector of keys to select
   * @return true if all keys are found
   */
  bool mget(std::vector<std::string> &keys)
  {
    std::vector<const char *> real_keys;
    std::vector<size_t> key_len;
    /*
     * Construct an array which will contain the length
     * of each of the strings in the input vector. Also, to
     * interface with the memcached C API, we need to convert
     * the vector of std::string's to a vector of char *.
     */
    real_keys.reserve(keys.size());
    key_len.reserve(keys.size());

    std::vector<std::string>::iterator it= keys.begin();

    while (it != keys.end())
    {
      real_keys.push_back(const_cast<char *>((*it).c_str()));
      key_len.push_back((*it).length());
      ++it;
    }

    /*
     * If the std::vector of keys is empty then we cannot
     * call memcached_mget as we will get undefined behavior.
     */
    if (! real_keys.empty())
    {
      memcached_return_t rc= memcached_mget(&memc, &real_keys[0], &key_len[0],
                                          real_keys.size());
      return (rc == MEMCACHED_SUCCESS);
    }

    return false;
  }

  /**
   * Writes an object to the server. If the object already exists, it will
   * overwrite the existing object. This method always returns true
   * when using non-blocking mode unless a network error occurs.
   *
   * @param[in] key key of object to write to server
   * @param[in] value value of object to write to server
   * @param[in] expiration time to keep the object stored in the server for
   * @param[in] flags flags to store with the object
   * @return true on succcess; false otherwise
   */
  bool set(const std::string &key,
           const std::vector<char> &value,
           time_t expiration,
           uint32_t flags) throw(Error)
  {
    if (key.empty() || value.empty())
    {
      throw(Error("the key or value supplied is empty!", false));
    }
    memcached_return_t rc= memcached_set(&memc,
                                       key.c_str(), key.length(),
                                       &value[0], value.size(),
                                       expiration, flags);
    return (rc == MEMCACHED_SUCCESS || rc == MEMCACHED_BUFFERED);
  }

  /**
   * Writes an object to a server specified by the master_key parameter.
   * If the object already exists, it will overwrite the existing object.
   *
   * @param[in] master_key key that specifies server to write to
   * @param[in] key key of object to write to server
   * @param[in] value value of object to write to server
   * @param[in] expiration time to keep the object stored in the server for
   * @param[in] flags flags to store with the object
   * @return true on succcess; false otherwise
   */
  bool setByKey(const std::string &master_key,
                const std::string &key,
                const std::vector<char> &value,
                time_t expiration,
                uint32_t flags) throw(Error)
  {
    if (master_key.empty() ||
        key.empty() ||
        value.empty())
    {
      throw(Error("the key or value supplied is empty!", false));
    }
    memcached_return_t rc= memcached_set_by_key(&memc, master_key.c_str(),
                                              master_key.length(),
                                              key.c_str(), key.length(),
                                              &value[0], value.size(),
                                              expiration,
                                              flags);
    return (rc == MEMCACHED_SUCCESS);
  }

  /**
   * Writes a list of objects to the server. Objects are specified by
   * 2 vectors - 1 vector of keys and 1 vector of values.
   *
   * @param[in] keys vector of keys of objects to write to server
   * @param[in] values vector of values of objects to write to server
   * @param[in] expiration time to keep the objects stored in server for
   * @param[in] flags flags to store with the objects
   * @return true on success; false otherwise
   */
  bool setAll(std::vector<std::string> &keys,
              std::vector< std::vector<char> *> &values,
              time_t expiration,
              uint32_t flags) throw(Error)
  {
    if (keys.size() != values.size())
    {
      throw(Error("The number of keys and values do not match!", false));
    }
    bool retval= true;
    std::vector<std::string>::iterator key_it= keys.begin();
    std::vector< std::vector<char> *>::iterator val_it= values.begin();
    while (key_it != keys.end())
    {
      retval= set((*key_it), *(*val_it), expiration, flags);
      if (retval == false)
      {
        return retval;
      }
      ++key_it;
      ++val_it;
    }
    return retval;
  }

  /**
   * Writes a list of objects to the server. Objects are specified by
   * a map of keys to values.
   *
   * @param[in] key_value_map map of keys and values to store in server
   * @param[in] expiration time to keep the objects stored in server for
   * @param[in] flags flags to store with the objects
   * @return true on success; false otherwise
   */
  bool setAll(std::map<const std::string, std::vector<char> > &key_value_map,
              time_t expiration,
              uint32_t flags) throw(Error)
  {
    if (key_value_map.empty())
    {
      throw(Error("The key/values are not properly set!", false));
    }
    bool retval= true;
    std::map<const std::string, std::vector<char> >::iterator it=
      key_value_map.begin();
    while (it != key_value_map.end())
    {
      retval= set(it->first, it->second, expiration, flags);
      if (retval == false)
      {
        std::string err_buff("There was an error setting the key ");
        err_buff.append(it->first);
        throw(Error(err_buff, false));
      }
      ++it;
    }
    return true;
  }

  /**
   * Increment the value of the object associated with the specified
   * key by the offset given. The resulting value is saved in the value
   * parameter.
   *
   * @param[in] key key of object in server whose value to increment
   * @param[in] offset amount to increment object's value by
   * @param[out] value store the result of the increment here
   * @return true on success; false otherwise
   */
  bool increment(const std::string &key, uint32_t offset, uint64_t *value) throw(Error)
  {
    if (key.empty())
    {
      throw(Error("the key supplied is empty!", false));
    }
    memcached_return_t rc= memcached_increment(&memc, key.c_str(), key.length(),
                                             offset, value);
    return (rc == MEMCACHED_SUCCESS);
  }

  /**
   * Decrement the value of the object associated with the specified
   * key by the offset given. The resulting value is saved in the value
   * parameter.
   *
   * @param[in] key key of object in server whose value to decrement
   * @param[in] offset amount to increment object's value by
   * @param[out] value store the result of the decrement here
   * @return true on success; false otherwise
   */
  bool decrement(const std::string &key, uint32_t offset, uint64_t *value)
    throw(Error)
  {
    if (key.empty())
    {
      throw(Error("the key supplied is empty!", false));
    }
    memcached_return_t rc= memcached_decrement(&memc, key.c_str(),
                                             key.length(),
                                             offset, value);
    return (rc == MEMCACHED_SUCCESS);
  }


  /**
   * Add an object with the specified key and value to the server. This
   * function returns false if the object already exists on the server.
   *
   * @param[in] key key of object to add
   * @param[in] value of object to add
   * @return true on success; false otherwise
   */
  bool add(const std::string &key, const std::vector<char> &value)
    throw(Error)
  {
    if (key.empty() || value.empty())
    {
      throw(Error("the key or value supplied is empty!", false));
    }
    memcached_return_t rc= memcached_add(&memc, key.c_str(), key.length(),
                                       &value[0], value.size(), 0, 0);
    return (rc == MEMCACHED_SUCCESS);
  }

  /**
   * Add an object with the specified key and value to the server. This
   * function returns false if the object already exists on the server. The
   * server to add the object to is specified by the master_key parameter.
   *
   * @param[in[ master_key key of server to add object to
   * @param[in] key key of object to add
   * @param[in] value of object to add
   * @return true on success; false otherwise
   */
  bool addByKey(const std::string &master_key,
                const std::string &key,
                const std::vector<char> &value) throw(Error)
  {
    if (master_key.empty() ||
        key.empty() ||
        value.empty())
    {
      throw(Error("the master key or key supplied is empty!", false));
    }
    memcached_return_t rc= memcached_add_by_key(&memc,
                                              master_key.c_str(),
                                              master_key.length(),
                                              key.c_str(),
                                              key.length(),
                                              &value[0],
                                              value.size(),
                                              0, 0);
    return (rc == MEMCACHED_SUCCESS);
  }

  /**
   * Replaces an object on the server. This method only succeeds
   * if the object is already present on the server.
   *
   * @param[in] key key of object to replace
   * @param[in[ value value to replace object with
   * @return true on success; false otherwise
   */
  bool replace(const std::string &key, const std::vector<char> &value) throw(Error)
  {
    if (key.empty() ||
        value.empty())
    {
      throw(Error("the key or value supplied is empty!", false));
    }
    memcached_return_t rc= memcached_replace(&memc, key.c_str(), key.length(),
                                           &value[0], value.size(),
                                           0, 0);
    return (rc == MEMCACHED_SUCCESS);
  }

  /**
   * Replaces an object on the server. This method only succeeds
   * if the object is already present on the server. The server
   * to replace the object on is specified by the master_key param.
   *
   * @param[in] master_key key of server to replace object on
   * @param[in] key key of object to replace
   * @param[in[ value value to replace object with
   * @return true on success; false otherwise
   */
  bool replaceByKey(const std::string &master_key,
                    const std::string &key,
                    const std::vector<char> &value)
  {
    if (master_key.empty() ||
        key.empty() ||
        value.empty())
    {
      throw(Error("the master key or key supplied is empty!", false));
    }
    memcached_return_t rc= memcached_replace_by_key(&memc,
                                                  master_key.c_str(),
                                                  master_key.length(),
                                                  key.c_str(),
                                                  key.length(),
                                                  &value[0],
                                                  value.size(),
                                                  0, 0);
    return (rc == MEMCACHED_SUCCESS);
  }

  /**
   * Places a segment of data before the last piece of data stored.
   *
   * @param[in] key key of object whose value we will prepend data to
   * @param[in] value data to prepend to object's value
   * @return true on success; false otherwise
   */
  bool prepend(const std::string &key, const std::vector<char> &value)
    throw(Error)
  {
    if (key.empty() || value.empty())
    {
      throw(Error("the key or value supplied is empty!", false));
    }
    memcached_return_t rc= memcached_prepend(&memc, key.c_str(), key.length(),
                                           &value[0], value.size(), 0, 0);
    return (rc == MEMCACHED_SUCCESS);
  }

  /**
   * Places a segment of data before the last piece of data stored. The
   * server on which the object where we will be prepending data is stored
   * on is specified by the master_key parameter.
   *
   * @param[in] master_key key of server where object is stored
   * @param[in] key key of object whose value we will prepend data to
   * @param[in] value data to prepend to object's value
   * @return true on success; false otherwise
   */
  bool prependByKey(const std::string &master_key,
                    const std::string &key,
                    const std::vector<char> &value)
      throw(Error)
  {
    if (master_key.empty() ||
        key.empty() ||
        value.empty())
    {
      throw(Error("the master key or key supplied is empty!", false));
    }
    memcached_return_t rc= memcached_prepend_by_key(&memc,
                                                  master_key.c_str(),
                                                  master_key.length(),
                                                  key.c_str(),
                                                  key.length(),
                                                  &value[0],
                                                  value.size(),
                                                  0,
                                                  0);
    return (rc == MEMCACHED_SUCCESS);
  }

  /**
   * Places a segment of data at the end of the last piece of data stored.
   *
   * @param[in] key key of object whose value we will append data to
   * @param[in] value data to append to object's value
   * @return true on success; false otherwise
   */
  bool append(const std::string &key, const std::vector<char> &value)
    throw(Error)
  {
    if (key.empty() || value.empty())
    {
      throw(Error("the key or value supplied is empty!", false));
    }
    memcached_return_t rc= memcached_append(&memc,
                                          key.c_str(),
                                          key.length(),
                                          &value[0],
                                          value.size(),
                                          0, 0);
    return (rc == MEMCACHED_SUCCESS);
  }

  /**
   * Places a segment of data at the end of the last piece of data stored. The
   * server on which the object where we will be appending data is stored
   * on is specified by the master_key parameter.
   *
   * @param[in] master_key key of server where object is stored
   * @param[in] key key of object whose value we will append data to
   * @param[in] value data to append to object's value
   * @return true on success; false otherwise
   */
  bool appendByKey(const std::string &master_key,
                   const std::string &key,
                   const std::vector<char> &value)
    throw(Error)
  {
    if (master_key.empty() ||
        key.empty() ||
        value.empty())
    {
      throw(Error("the master key or key supplied is empty!", false));
    }
    memcached_return_t rc= memcached_append_by_key(&memc,
                                                 master_key.c_str(),
                                                 master_key.length(),
                                                 key.c_str(),
                                                 key.length(),
                                                 &value[0],
                                                 value.size(),
                                                 0, 0);
    return (rc == MEMCACHED_SUCCESS);
  }

  /**
   * Overwrite data in the server as long as the cas_arg value
   * is still the same in the server.
   *
   * @param[in] key key of object in server
   * @param[in] value value to store for object in server
   * @param[in] cas_arg "cas" value
   */
  bool cas(const std::string &key,
           const std::vector<char> &value,
           uint64_t cas_arg) throw(Error)
  {
    if (key.empty() || value.empty())
    {
      throw(Error("the key or value supplied is empty!", false));
    }
    memcached_return_t rc= memcached_cas(&memc, key.c_str(), key.length(),
                                       &value[0], value.size(),
                                       0, 0, cas_arg);
    return (rc == MEMCACHED_SUCCESS);
  }

  /**
   * Overwrite data in the server as long as the cas_arg value
   * is still the same in the server. The server to use is
   * specified by the master_key parameter.
   *
   * @param[in] master_key specifies server to operate on
   * @param[in] key key of object in server
   * @param[in] value value to store for object in server
   * @param[in] cas_arg "cas" value
   */
  bool casByKey(const std::string &master_key,
                const std::string &key,
                const std::vector<char> &value,
                uint64_t cas_arg) throw(Error)
  {
    if (master_key.empty() ||
        key.empty() ||
        value.empty())
    {
      throw(Error("the master key, key or value supplied is empty!", false));
    }
    memcached_return_t rc= memcached_cas_by_key(&memc,
                                              master_key.c_str(),
                                              master_key.length(),
                                              key.c_str(),
                                              key.length(),
                                              &value[0],
                                              value.size(),
                                              0, 0, cas_arg);
    return (rc == MEMCACHED_SUCCESS);
  }

  /**
   * Delete an object from the server specified by the key given.
   *
   * @param[in] key key of object to delete
   * @return true on success; false otherwise
   */
  bool remove(const std::string &key) throw(Error)
  {
    if (key.empty())
    {
      throw(Error("the key supplied is empty!", false));
    }
    memcached_return_t rc= memcached_delete(&memc, key.c_str(), key.length(), 0);
    return (rc == MEMCACHED_SUCCESS);
  }

  /**
   * Delete an object from the server specified by the key given.
   *
   * @param[in] key key of object to delete
   * @param[in] expiration time to delete the object after
   * @return true on success; false otherwise
   */
  bool remove(const std::string &key,
              time_t expiration) throw(Error)
  {
    if (key.empty())
    {
      throw(Error("the key supplied is empty!", false));
    }
    memcached_return_t rc= memcached_delete(&memc,
                                          key.c_str(),
                                          key.length(),
                                          expiration);
    return (rc == MEMCACHED_SUCCESS);
  }

  /**
   * Delete an object from the server specified by the key given.
   *
   * @param[in] master_key specifies server to remove object from
   * @param[in] key key of object to delete
   * @return true on success; false otherwise
   */
  bool removeByKey(const std::string &master_key,
                   const std::string &key) throw(Error)
  {
    if (master_key.empty() || key.empty())
    {
      throw(Error("the master key or key supplied is empty!", false));
    }
    memcached_return_t rc= memcached_delete_by_key(&memc,
                                                 master_key.c_str(),
                                                 master_key.length(),
                                                 key.c_str(),
                                                 key.length(),
                                                 0);
    return (rc == MEMCACHED_SUCCESS);
  }

  /**
   * Delete an object from the server specified by the key given.
   *
   * @param[in] master_key specifies server to remove object from
   * @param[in] key key of object to delete
   * @param[in] expiration time to delete the object after
   * @return true on success; false otherwise
   */
  bool removeByKey(const std::string &master_key,
                   const std::string &key,
                   time_t expiration) throw(Error)
  {
    if (master_key.empty() || key.empty())
    {
      throw(Error("the master key or key supplied is empty!", false));
    }
    memcached_return_t rc= memcached_delete_by_key(&memc,
                                                 master_key.c_str(),
                                                 master_key.length(),
                                                 key.c_str(),
                                                 key.length(),
                                                 expiration);
    return (rc == MEMCACHED_SUCCESS);
  }

  /**
   * Wipe the contents of memcached servers.
   *
   * @param[in] expiration time to wait until wiping contents of
   *                       memcached servers
   * @return true on success; false otherwise
   */
  bool flush(time_t expiration)
  {
    memcached_return_t rc= memcached_flush(&memc, expiration);
    return (rc == MEMCACHED_SUCCESS);
  }

  /**
   * Callback function for result sets. It passes the result
   * sets to the list of functions provided.
   *
   * @param[in] callback list of callback functions
   * @param[in] context pointer to memory reference that is
   *                    supplied to the calling function
   * @param[in] num_of_callbacks number of callback functions
   * @return true on success; false otherwise
   */
  bool fetchExecute(memcached_execute_fn *callback,
                    void *context,
                    uint32_t num_of_callbacks)
  {
    memcached_return_t rc= memcached_fetch_execute(&memc,
                                                 callback,
                                                 context,
                                                 num_of_callbacks);
    return (rc == MEMCACHED_SUCCESS);
  }

  /**
   * Get the library version string.
   * @return std::string containing a copy of the library version string.
   */
  const std::string libVersion() const
  {
    const char *ver= memcached_lib_version();
    const std::string version(ver);
    return version;
  }

  /**
   * Retrieve memcached statistics. Populate a std::map with the retrieved
   * stats. Each server will map to another std::map of the key:value stats.
   *
   * @param[out] stats_map a std::map to be populated with the memcached
   *                       stats
   * @return true on success; false otherwise
   */
  bool getStats(std::map< std::string, std::map<std::string, std::string> >
                &stats_map)
  {
    memcached_return_t rc;
    memcached_stat_st *stats= memcached_stat(&memc, NULL, &rc);

    if (rc != MEMCACHED_SUCCESS &&
        rc != MEMCACHED_SOME_ERRORS)
    {
      return false;
    }

    uint32_t server_count= memcached_server_count(&memc);

    /*
     * For each memcached server, construct a std::map for its stats and add
     * it to the std::map of overall stats.
     */
    for (uint32_t x= 0; x < server_count; x++)
    {
      memcached_server_instance_st instance=
        memcached_server_instance_by_position(&memc, x);
      std::ostringstream strstm;
      std::string server_name(memcached_server_name(instance));
      server_name.append(":");
      strstm << memcached_server_port(instance);
      server_name.append(strstm.str());

      std::map<std::string, std::string> server_stats;
      char **list= NULL;
      char **ptr= NULL;

      list= memcached_stat_get_keys(&memc, &stats[x], &rc);
      for (ptr= list; *ptr; ptr++)
      {
        char *value= memcached_stat_get_value(&memc, &stats[x], *ptr, &rc);
        server_stats[*ptr]= value;
        free(value);
      }
     
      stats_map[server_name]= server_stats;
      free(list);
    }

    memcached_stat_free(&memc, stats);
    return true;
  }

private:

  std::string servers_list;
  memcached_st memc;
  memcached_result_st result;
};

}

#endif /* LIBMEMCACHEDPP_H */
