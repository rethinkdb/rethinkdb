/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// demo_exception.cpp

// (C) Copyright 2002-4 Robert Ramey - http://www.rrsd.com . 
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Example of safe exception handling for pointer de-serialization
// 
// This example was prepared by Robert Ramey to demonstrate and test 
// safe exception handling during the de-serialization of pointers in
// a non-trivial example.
//
// Hopefully, this addresses exception issues raised by
// Vahan Margaryan who spent considerable time and effort
// in the analysis and testing of issues of exception safety 
// of the serialization library.

#include <algorithm>
#include <iostream>
#include <cstddef> // NULL
#include <fstream>
#include <string>

#include <cstdio> // remove
#include <boost/config.hpp>
#if defined(BOOST_NO_STDC_NAMESPACE)
namespace std{ 
    using ::remove;
}
#endif

#include <boost/archive/tmpdir.hpp>

#ifndef BOOST_NO_EXCEPTIONS
#include <exception>
#endif

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

#include <boost/serialization/list.hpp>
#include <boost/serialization/split_member.hpp>

template<class TPTR>
struct deleter
{
    void operator()(TPTR t){
        delete t;
    }
};

class Course;
class Student;

class Student
{
public:
    static int count;
    Student(){
        count++;
    }
    ~Student(){
        some_courses.clear();
        count--;
    }
    std::list<Course *> some_courses;
private:
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int /* file_version */){
        ar & some_courses;
    }
};

int Student::count = 0;

class Course
{
public:
    static int count;
    Course(){
        count++;
    }
    ~Course(){
        // doesnt delete pointers in list
        // since it doesn't "own" them
        some_students.clear();
        count--;
    }
    std::list<Student *> some_students;
private:
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int /* file_version */){
        ar & some_students;
    }
};

int Course::count = 0;

class School
{
public:
    ~School(){
        // must delete all the students because
        // it "owns" them
        std::for_each(all_students.begin(), all_students.end(), deleter<Student *>());
        all_students.clear();
        // as well as courses
        std::for_each(all_courses.begin(), all_courses.end(), deleter<Course *>());
        all_courses.clear();
    }
    std::list<Student *> all_students;
    std::list<Course *> all_courses;
private:
    friend class boost::serialization::access;
    BOOST_SERIALIZATION_SPLIT_MEMBER()
    template<class Archive>
    void save(Archive & ar, const unsigned int file_version) const;
    template<class Archive>
    void load(Archive & ar, const unsigned int file_version);
};

#if 0
// case 1:
template<class Archive>
void School::serialize(Archive & ar, const unsigned int /* file_version */){
    // if an exeception occurs while loading courses
    // the structure courses may have some courses each
    // with students
    ar & all_courses;
    // while all_students will have no members.
    ar & all_students; // create students that have no courses
    // so ~School() will delete all members of courses
    // but this will NOT delete any students - see above
    // a memory leak will be the result.
}

// switching the order of serialization doesn't help in this case
// case 2:
template<class Archive>
void School::serialize(Archive & ar, const unsigned int /* file_version */){
    ar & all_students;
    ar >> all_courses;  // create any courses that have no students
}
#endif

template<class Archive>
void School::save(Archive & ar, const unsigned int /* file_version */) const {
    ar << all_students;
    ar << all_courses;
}

template<class Archive>
void School::load(Archive & ar, const unsigned int /* file_version */){
    // if an exeception occurs while loading courses
    // the structure courses may have some courses each
    // with students
    try{
        // deserialization of a Course * will in general provoke the
        // deserialization of Student * which are added to the list of
        // students for a class.  That is, this process will result
        // in the copying of a pointer.
        ar >> all_courses;
        ar >> all_students; // create students that have no courses
    }
    catch(std::exception){
        // elminate any dangling references
        all_courses.clear();
        all_students.clear();
        throw;
    }
}

void init(School *school){
    Student *bob = new Student();
    Student *ted = new Student();
    Student *carol = new Student();
    Student *alice = new Student();

    school->all_students.push_back(bob);
    school->all_students.push_back(ted);
    school->all_students.push_back(carol);
    school->all_students.push_back(alice);

    Course *math = new Course();
    Course *history = new Course();
    Course *literature = new Course();
    Course *gym = new Course();

    school->all_courses.push_back(math);
    school->all_courses.push_back(history);
    school->all_courses.push_back(literature);
    school->all_courses.push_back(gym);

    bob->some_courses.push_back(math);
    math->some_students.push_back(bob);
    bob->some_courses.push_back(literature);
    literature->some_students.push_back(bob);

    ted->some_courses.push_back(math);
    math->some_students.push_back(ted);
    ted->some_courses.push_back(history);
    history->some_students.push_back(ted);

    alice->some_courses.push_back(literature);
    literature->some_students.push_back(alice);
    alice->some_courses.push_back(history);
    history->some_students.push_back(alice);

    // no students signed up for gym
    // carol has no courses
}

void save(const School * const school, const char *filename){
    std::ofstream ofile(filename);
    boost::archive::text_oarchive ar(ofile);
    ar << school;
}

void load(School * & school, const char *filename){
    std::ifstream ifile(filename);
    boost::archive::text_iarchive ar(ifile);
    try{
        ar >> school;
    }
    catch(std::exception){
        // eliminate dangling reference
        school = NULL;
    }
}

int main(int argc, char *argv[]){
    std::string filename(boost::archive::tmpdir());
    filename += "/demofile.txt";

    School *school = new School();
    std::cout << "1. student count = " << Student::count << std::endl;
    std::cout << "2. class count = " << Course::count << std::endl;
    init(school);
    std::cout << "3. student count = " << Student::count << std::endl;
    std::cout << "4. class count = " << Course::count << std::endl;
    save(school, filename.c_str());
    delete school;
    school = NULL;
    std::cout << "5. student count = " << Student::count << std::endl;
    std::cout << "6. class count = " << Course::count << std::endl;
    load(school, filename.c_str());
    std::cout << "7. student count = " << Student::count << std::endl;
    std::cout << "8. class count = " << Course::count << std::endl;
    delete school;
    std::cout << "9. student count = " << Student::count << std::endl;
    std::cout << "10. class count = " << Course::count << std::endl;
    std::remove(filename.c_str());
    return Student::count + Course::count;
}
