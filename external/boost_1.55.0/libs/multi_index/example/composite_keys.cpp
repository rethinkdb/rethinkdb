/* Boost.MultiIndex example of composite keys.
 *
 * Copyright 2003-2008 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/multi_index for library home page.
 */

#if !defined(NDEBUG)
#define BOOST_MULTI_INDEX_ENABLE_INVARIANT_CHECKING
#define BOOST_MULTI_INDEX_ENABLE_SAFE_MODE
#endif

#include <boost/call_traits.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/next_prior.hpp>
#include <boost/tokenizer.hpp>
#include <functional>
#include <iostream>
#include <iterator>
#include <map>
#include <string>

using namespace boost::multi_index;

/* A file record maintains some info on name and size as well
 * as a pointer to the directory it belongs (null meaning the root
 * directory.)
 */

struct file_entry
{
  file_entry(
    std::string name_,unsigned size_,bool is_dir_,const file_entry* dir_):
    name(name_),size(size_),is_dir(is_dir_),dir(dir_)
  {}

  std::string       name;
  unsigned          size;
  bool              is_dir;
  const file_entry* dir;

  friend std::ostream& operator<<(std::ostream& os,const file_entry& f)
  {
    os<<f.name<<"\t"<<f.size;
    if(f.is_dir)os<<"\t <dir>";
    return os;
  }
};

/* A file system is just a multi_index_container of entries with indices on
 * file and size. These indices are firstly ordered by directory, as commands
 * work on a current directory basis. Composite keys are just fine to model
 * this.
 * NB: The use of derivation here instead of simple typedef is explained in
 * Compiler specifics: type hiding.
 */

struct name_key:composite_key<
  file_entry,
  BOOST_MULTI_INDEX_MEMBER(file_entry,const file_entry*,dir),
  BOOST_MULTI_INDEX_MEMBER(file_entry,std::string,name)
>{};

struct size_key:composite_key<
  file_entry,
  BOOST_MULTI_INDEX_MEMBER(file_entry,const file_entry* const,dir),
  BOOST_MULTI_INDEX_MEMBER(file_entry,unsigned,size)
>{};

/* see Compiler specifics: composite_key in compilers without partial
 * template specialization, for info on composite_key_result_less
 */

typedef multi_index_container<
  file_entry,
  indexed_by<
    /* primary index sorted by name (inside the same directory) */
    ordered_unique<
      name_key
#if defined(BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION)
      ,composite_key_result_less<name_key::result_type>
#endif
    >,
    /* secondary index sorted by size (inside the same directory) */
    ordered_non_unique<
      size_key
#if defined(BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION)
      ,composite_key_result_less<size_key::result_type>
#endif
    >
  >
> file_system;

/* typedef's of the two indices of file_system */

typedef nth_index<file_system,0>::type file_system_by_name;
typedef nth_index<file_system,1>::type file_system_by_size;

/* We build a rudimentary file system simulation out of some global
 * info and a map of commands provided to the user.
 */

static file_system fs;                 /* the one and only file system */
static file_system_by_name& fs_by_name=fs;         /* name index to fs */
static file_system_by_size& fs_by_size=get<1>(fs); /* size index to fs */
static const file_entry* current_dir=0;            /* root directory   */

/* command framework */

/* A command provides an execute memfun fed with the corresponding params
 * (first param is stripped off as it serves to identify the command
 * currently being used.)
 */

typedef boost::tokenizer<boost::char_separator<char> > command_tokenizer;

class command
{
public:
  virtual ~command(){}
  virtual void execute(
    command_tokenizer::iterator tok1,command_tokenizer::iterator tok2)=0;
};

/* available commands */

/* cd: syntax cd [.|..|<directory>] */

class command_cd:public command
{
public:
  virtual void execute(
    command_tokenizer::iterator tok1,command_tokenizer::iterator tok2)
  {
    if(tok1==tok2)return;
    std::string dir=*tok1++;

    if(dir==".")return;
    if(dir==".."){
      if(current_dir)current_dir=current_dir->dir;
      return;
    }

    file_system_by_name::iterator it=fs.find(
      boost::make_tuple(current_dir,dir));
    if(it==fs.end()){
      std::cout<<"non-existent directory"<<std::endl;
      return;
    }
    if(!it->is_dir){
      std::cout<<dir<<" is not a directory"<<std::endl;
      return;
    }
    current_dir=&*it;
  }
};
static command_cd cd;

/* ls: syntax ls [-s] */

class command_ls:public command
{
public:
  virtual void execute(
    command_tokenizer::iterator tok1,command_tokenizer::iterator tok2)
  {
    std::string option;
    if(tok1!=tok2)option=*tok1++;

    if(!option.empty()){
      if(option!="-s"){
        std::cout<<"incorrect parameter"<<std::endl;
        return;
      }

      /* list by size */

      file_system_by_size::iterator it0,it1;
      boost::tie(it0,it1)=fs_by_size.equal_range(
        boost::make_tuple(current_dir));
      std::copy(it0,it1,std::ostream_iterator<file_entry>(std::cout,"\n"));

      return;
    }

    /* list by name */

    file_system_by_name::iterator it0,it1;
    boost::tie(it0,it1)=fs.equal_range(boost::make_tuple(current_dir));
    std::copy(it0,it1,std::ostream_iterator<file_entry>(std::cout,"\n"));
  }
};
static command_ls ls;

/* mkdir: syntax mkdir <directory> */

class command_mkdir:public command
{
public:
  virtual void execute(
    command_tokenizer::iterator tok1,command_tokenizer::iterator tok2)
  {
    std::string dir;
    if(tok1!=tok2)dir=*tok1++;

    if(dir.empty()){
      std::cout<<"missing parameter"<<std::endl;
      return;
    }

    if(dir=="."||dir==".."){
      std::cout<<"incorrect parameter"<<std::endl;
      return;
    }

    if(!fs.insert(file_entry(dir,0,true,current_dir)).second){
      std::cout<<"directory already exists"<<std::endl;
      return;
    }
  }
};
static command_mkdir mkdir;

/* table of commands, a map from command names to class command pointers */

typedef std::map<std::string,command*> command_table;
static command_table cmt;

int main()
{
  /* fill the file system with some data */

  file_system::iterator it0,it1;
  
  fs.insert(file_entry("usr.cfg",240,false,0));
  fs.insert(file_entry("memo.txt",2430,false,0));
  it0=fs.insert(file_entry("dev",0,true,0)).first;
    fs.insert(file_entry("tty0",128,false,&*it0));
    fs.insert(file_entry("tty1",128,false,&*it0));
  it0=fs.insert(file_entry("usr",0,true,0)).first;
    it1=fs.insert(file_entry("bin",0,true,&*it0)).first;
      fs.insert(file_entry("bjam",172032,false,&*it1));
  it0=fs.insert(file_entry("home",0,true,0)).first;
    it1=fs.insert(file_entry("andy",0,true,&*it0)).first;
      fs.insert(file_entry("logo.jpg",5345,false,&*it1)).first;
      fs.insert(file_entry("foo.cpp",890,false,&*it1)).first;
      fs.insert(file_entry("foo.hpp",93,false,&*it1)).first;
      fs.insert(file_entry("foo.html",750,false,&*it1)).first;
      fs.insert(file_entry("a.obj",12302,false,&*it1)).first;
      fs.insert(file_entry(".bash_history",8780,false,&*it1)).first;
    it1=fs.insert(file_entry("rachel",0,true,&*it0)).first;
      fs.insert(file_entry("test.py",650,false,&*it1)).first;
      fs.insert(file_entry("todo.txt",241,false,&*it1)).first;
      fs.insert(file_entry(".bash_history",9510,false,&*it1)).first;

  /* fill the command table */

  cmt["cd"]   =&cd;
  cmt["ls"]   =&ls;
  cmt["mkdir"]=&mkdir;

  /* main looop */

  for(;;){
    /* print out the current directory and the prompt symbol */

    if(current_dir)std::cout<<current_dir->name;
    std::cout<<">";

    /* get an input line from the user: if empty, exit the program */

    std::string com;
    std::getline(std::cin,com);
    command_tokenizer tok(com,boost::char_separator<char>(" \t\n"));
    if(tok.begin()==tok.end())break; /* null command, exit */

    /* select the corresponding command and execute it */

    command_table::iterator it=cmt.find(*tok.begin());
    if(it==cmt.end()){
      std::cout<<"invalid command"<<std::endl;
      continue;
    }

    it->second->execute(boost::next(tok.begin()),tok.end());
  }
  
  return 0;
}
