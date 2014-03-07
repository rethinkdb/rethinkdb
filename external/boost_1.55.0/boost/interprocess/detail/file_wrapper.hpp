//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2006-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_DETAIL_FILE_WRAPPER_HPP
#define BOOST_INTERPROCESS_DETAIL_FILE_WRAPPER_HPP

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>
#include <boost/interprocess/detail/os_file_functions.hpp>
#include <boost/interprocess/creation_tags.hpp>
#include <boost/move/move.hpp>
#include <boost/interprocess/creation_tags.hpp>

namespace boost {
namespace interprocess {
namespace ipcdetail{

class file_wrapper
{
   /// @cond
   BOOST_MOVABLE_BUT_NOT_COPYABLE(file_wrapper)
   /// @endcond
   public:

   //!Default constructor.
   //!Represents an empty file_wrapper.
   file_wrapper();

   //!Creates a file object with name "name" and mode "mode", with the access mode "mode"
   //!If the file previously exists, throws an error.
   file_wrapper(create_only_t, const char *name, mode_t mode, const permissions &perm = permissions())
   {  this->priv_open_or_create(ipcdetail::DoCreate, name, mode, perm);  }

   //!Tries to create a file with name "name" and mode "mode", with the
   //!access mode "mode". If the file previously exists, it tries to open it with mode "mode".
   //!Otherwise throws an error.
   file_wrapper(open_or_create_t, const char *name, mode_t mode, const permissions &perm  = permissions())
   {  this->priv_open_or_create(ipcdetail::DoOpenOrCreate, name, mode, perm);  }

   //!Tries to open a file with name "name", with the access mode "mode".
   //!If the file does not previously exist, it throws an error.
   file_wrapper(open_only_t, const char *name, mode_t mode)
   {  this->priv_open_or_create(ipcdetail::DoOpen, name, mode, permissions());  }

   //!Moves the ownership of "moved"'s file to *this.
   //!After the call, "moved" does not represent any file.
   //!Does not throw
   file_wrapper(BOOST_RV_REF(file_wrapper) moved)
      :  m_handle(file_handle_t(ipcdetail::invalid_file()))
   {  this->swap(moved);   }

   //!Moves the ownership of "moved"'s file to *this.
   //!After the call, "moved" does not represent any file.
   //!Does not throw
   file_wrapper &operator=(BOOST_RV_REF(file_wrapper) moved)
   {
      file_wrapper tmp(boost::move(moved));
      this->swap(tmp);
      return *this;
   }

   //!Swaps to file_wrappers.
   //!Does not throw
   void swap(file_wrapper &other);

   //!Erases a file from the system.
   //!Returns false on error. Never throws
   static bool remove(const char *name);

   //!Sets the size of the file
   void truncate(offset_t length);

   //!Closes the
   //!file
   ~file_wrapper();

   //!Returns the name of the file
   //!used in the constructor
   const char *get_name() const;

   //!Returns the name of the file
   //!used in the constructor
   bool get_size(offset_t &size) const;

   //!Returns access mode
   //!used in the constructor
   mode_t get_mode() const;

   //!Get mapping handle
   //!to use with mapped_region
   mapping_handle_t get_mapping_handle() const;

   private:
   //!Closes a previously opened file mapping. Never throws.
   void priv_close();
   //!Closes a previously opened file mapping. Never throws.
   bool priv_open_or_create(ipcdetail::create_enum_t type, const char *filename, mode_t mode, const permissions &perm);

   file_handle_t  m_handle;
   mode_t      m_mode;
   std::string       m_filename;
};

inline file_wrapper::file_wrapper()
   :  m_handle(file_handle_t(ipcdetail::invalid_file()))
{}

inline file_wrapper::~file_wrapper()
{  this->priv_close(); }

inline const char *file_wrapper::get_name() const
{  return m_filename.c_str(); }

inline bool file_wrapper::get_size(offset_t &size) const
{  return get_file_size((file_handle_t)m_handle, size);  }

inline void file_wrapper::swap(file_wrapper &other)
{
   std::swap(m_handle,  other.m_handle);
   std::swap(m_mode,    other.m_mode);
   m_filename.swap(other.m_filename);
}

inline mapping_handle_t file_wrapper::get_mapping_handle() const
{  return mapping_handle_from_file_handle(m_handle);  }

inline mode_t file_wrapper::get_mode() const
{  return m_mode; }

inline bool file_wrapper::priv_open_or_create
   (ipcdetail::create_enum_t type,
    const char *filename,
    mode_t mode,
    const permissions &perm = permissions())
{
   m_filename = filename;

   if(mode != read_only && mode != read_write){
      error_info err(mode_error);
      throw interprocess_exception(err);
   }

   //Open file existing native API to obtain the handle
   switch(type){
      case ipcdetail::DoOpen:
         m_handle = open_existing_file(filename, mode);
      break;
      case ipcdetail::DoCreate:
         m_handle = create_new_file(filename, mode, perm);
      break;
      case ipcdetail::DoOpenOrCreate:
         m_handle = create_or_open_file(filename, mode, perm);
      break;
      default:
         {
            error_info err = other_error;
            throw interprocess_exception(err);
         }
   }

   //Check for error
   if(m_handle == invalid_file()){
      throw interprocess_exception(error_info(system_error_code()));
   }

   m_mode = mode;
   return true;
}

inline bool file_wrapper::remove(const char *filename)
{  return delete_file(filename); }

inline void file_wrapper::truncate(offset_t length)
{
   if(!truncate_file(m_handle, length)){
      error_info err(system_error_code());
      throw interprocess_exception(err);
   }
}

inline void file_wrapper::priv_close()
{
   if(m_handle != invalid_file()){
      close_file(m_handle);
      m_handle = invalid_file();
   }
}

}  //namespace ipcdetail{
}  //namespace interprocess {
}  //namespace boost {

#include <boost/interprocess/detail/config_end.hpp>

#endif   //BOOST_INTERPROCESS_DETAIL_FILE_WRAPPER_HPP
