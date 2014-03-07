/* Boost.Flyweight example of use of key-value flyweights.
 *
 * Copyright 2006-2008 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/flyweight for library home page.
 */

#include <boost/array.hpp>
#include <boost/flyweight.hpp>
#include <boost/flyweight/key_value.hpp>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

using namespace boost::flyweights;

/* A class simulating a texture resource loaded from file */

class texture
{
public:
  texture(const std::string& filename):filename(filename)
  {
    std::cout<<"loaded "<<filename<<" file"<<std::endl;
  }

  texture(const texture& x):filename(x.filename)
  {
    std::cout<<"texture["<<filename<<"] copied"<<std::endl;
  }

  ~texture()
  {
    std::cout<<"unloaded "<<filename<<std::endl;
  }

  const std::string& get_filename()const{return filename;}

  // rest of the interface

private:
  std::string filename;
};

/* key extractor of filename strings from textures */

struct texture_filename_extractor
{
  const std::string& operator()(const texture& x)const
  {
    return x.get_filename();
  }
};

/* texture flyweight */

typedef flyweight<
  key_value<std::string,texture,texture_filename_extractor>
> texture_flyweight;

int main()
{
  /* texture filenames */

  const char* texture_filenames[]={
    "grass.texture","sand.texture","water.texture","wood.texture",
    "granite.texture","cotton.texture","concrete.texture","carpet.texture"
  };
  const int num_texture_filenames=
    sizeof(texture_filenames)/sizeof(texture_filenames[0]);

  /* create a massive vector of textures */

  std::cout<<"creating flyweights...\n"<<std::endl;

  std::vector<texture_flyweight> textures;
  for(int i=0;i<50000;++i){
    textures.push_back(
      texture_flyweight(texture_filenames[std::rand()%num_texture_filenames]));
  }

  /* Just for the sake of making use of the key extractor,
   * assign some flyweights with texture objects rather than strings.
   */

  for(int j=0;j<50000;++j){
    textures.push_back(
      texture_flyweight(
        textures[std::rand()%textures.size()].get()));
  }

  std::cout<<"\n"<<textures.size()<<" flyweights created\n"<<std::endl;

  return 0;
}
