//
//  Copyright (c) 2009-2011 Artyom Beilis (Tonkikh)
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_LOCALE_WITH_ICU
#include <iostream>
int main()
{
        std::cout << "ICU is not build... Skipping" << std::endl;
}
#else

//#define BOOST_LOCALE_ERROR_LIMIT 1000000
//#define BOOST_LOCALE_ERROR_LIMIT 0

#include <boost/locale/boundary.hpp>
#include <boost/locale/generator.hpp>
#include "test_locale.hpp"
#include "test_locale_tools.hpp"
#include <list>

// Debugging code

template<typename Char>
void print_str(std::basic_string<Char> const &/*s*/)
{
}

template<>
void print_str<char>(std::basic_string<char> const &s)
{
    std::cout << "[" << s <<"]" << std::endl;
}


namespace lb = boost::locale::boundary;

template<typename Char,typename Iterator>
void test_word_container(Iterator begin,Iterator end,
    std::vector<int> const &ipos,
    std::vector<int> const &imasks,
    std::vector<std::basic_string<Char> > const &ichunks,
    std::locale l,
    lb::boundary_type bt=lb::word
    )
{
    for(int sm=(bt == lb::word ? 31 : 3 ) ;sm>=0;sm--) {
        unsigned mask = 
              ((sm & 1 ) != 0) * 0xF
            + ((sm & 2 ) != 0) * 0xF0
            + ((sm & 4 ) != 0) * 0xF00
            + ((sm & 8 ) != 0) * 0xF000
            + ((sm & 16) != 0) * 0xF0000;

        std::vector<int> masks,pos;
        std::vector<unsigned> bmasks;
        std::basic_string<Char> empty_chunk;

        std::vector<std::basic_string<Char> > chunks;
        std::vector<std::basic_string<Char> > fchunks;
        std::vector<Iterator> iters;
        iters.push_back(begin);
        bmasks.push_back(0);

        for(unsigned i=0;i<imasks.size();i++) {
            if(imasks[i] & mask) {
                masks.push_back(imasks[i]);
                chunks.push_back(ichunks[i]);
                fchunks.push_back(empty_chunk + ichunks[i]);
                empty_chunk.clear();
                pos.push_back(ipos[i]);
            }
            else {
                empty_chunk+=ichunks[i];
            }

            if((imasks[i] & mask) || i==imasks.size()-1){
                Iterator ptr=begin;
                std::advance(ptr,ipos[i]);
                iters.push_back(ptr);
                bmasks.push_back(imasks[i]);
            }
        }

        //
        // segment iterator tests
        //
        {
            lb::segment_index<Iterator> map(bt,begin,end,l);
            typedef typename lb::segment_index<Iterator>::iterator iter_type;

            map.rule(mask);

            {        
                unsigned i=0;
                iter_type p;
                map.full_select(false);
                for(p=map.begin();p!=map.end();++p,i++) {
                    TEST(p->str()==chunks[i]);
                    TEST(p->rule() == unsigned(masks[i]));
                }
                
                TEST(chunks.size() == i);
                for(;;) {
                    if(p==map.begin()) {
                        TEST(i==0);
                        break;
                    }
                    else {
                        --p;
                        TEST(p->str()==chunks[--i]);
                        TEST(p->rule() == unsigned(masks[i]));
                    }
                }
                for(i=0,p=map.end();i<chunks.size();i++){
                    --p;
                    unsigned index = chunks.size() - i - 1;
                    TEST(p->str()==chunks[index]);
                    TEST(p->rule() == unsigned(masks[index]));
                }
                TEST(p==map.begin());
            }

            {
                unsigned i=0;
                iter_type p;
                map.full_select(true);
                for(p=map.begin();p!=map.end();++p,i++) {
                    TEST(p->str()==fchunks[i]);
                    TEST(p->rule() == unsigned(masks[i]));
                }

                TEST(chunks.size() == i);
                
                for(;;) {
                    if(p==map.begin()) {
                        TEST(i==0);
                        break;
                    }
                    else {
                        --p;
                        if(p->str()!=fchunks[i-1]) {
                            print_str(p->str());
                            print_str(fchunks[i-1]);
                        }
                        TEST(p->str()==fchunks[--i]);
                        TEST(p->rule() == unsigned(masks[i]));
                    }
                }
                
                for(i=0,p=map.end();i<chunks.size();i++){
                    --p;
                    unsigned index = chunks.size() - i - 1;
                    TEST(p->str()==fchunks[index]);
                    TEST(p->rule() == unsigned(masks[index]));
                }
                TEST(p==map.begin());
            }
            
            {            
                iter_type p;
                unsigned chunk_ptr=0;
                unsigned i=0;
                map.full_select(false);
                for(Iterator optr=begin;optr!=end;optr++,i++) {
                    p=map.find(optr);
                    if(chunk_ptr < pos.size() && i>=unsigned(pos[chunk_ptr])){
                        chunk_ptr++;
                    }
                    if(chunk_ptr>=pos.size()) {
                        TEST(p==map.end());
                    }
                    else {
                        TEST(p->str()==chunks[chunk_ptr]);
                        TEST(p->rule()==unsigned(masks[chunk_ptr]));
                    }
                }
            }
            {            
                iter_type p;
                unsigned chunk_ptr=0;
                unsigned i=0;
                map.full_select(true);
                for(Iterator optr=begin;optr!=end;optr++,i++) {
                    p=map.find(optr);
                    if(chunk_ptr < pos.size() && i>=unsigned(pos[chunk_ptr])){
                        chunk_ptr++;
                    }
                    if(chunk_ptr>=pos.size()) {
                        TEST(p==map.end());
                    }
                    else {
                        TEST(p->str()==fchunks[chunk_ptr]);
                        TEST(p->rule()==unsigned(masks[chunk_ptr]));
                    }
                }
            }

        } // segment iterator tests

        { // break iterator tests
            lb::boundary_point_index<Iterator> map(bt,begin,end,l);
            typedef typename lb::boundary_point_index<Iterator>::iterator iter_type;

            map.rule(mask);
        
            unsigned i=0;
            iter_type p;
            for(p=map.begin();p!=map.end();++p,i++) {
                TEST(p->iterator()==iters[i]);
                TEST(p->rule()==bmasks[i]);
            }

            TEST(iters.size() == i);

            do {
                --p;
                --i;
                TEST(p->iterator()==iters.at(i));
            } while(p!=map.begin());
            TEST(i==0);

            unsigned iters_ptr=0;
            for(Iterator optr=begin;optr!=end;optr++) {
                p=map.find(optr);
                TEST(p->iterator()==iters[iters_ptr]);
                if(iters.at(iters_ptr)==optr)
                    iters_ptr++;
            }
        
        } // break iterator tests

        { // copy test
            typedef lb::segment_index<Iterator> ti_type;
            typedef lb::boundary_point_index<Iterator> bi_type;
            {   // segment to bound
                ti_type ti(bt,begin,end,l);
                ti.rule(mask);
                {
                    bi_type bi(ti);
                    bi.rule(mask);
                    unsigned i=0;
                    typename bi_type::iterator p;
                    for(p=bi.begin();p!=bi.end();++p,i++) {
                        TEST(p->iterator()==iters[i]);
                        TEST(p->rule()==bmasks[i]);
                    }
                }
                {
                    bi_type bi;
                    bi.rule(mask);
                    bi = ti;
                    unsigned i=0;
                    typename bi_type::iterator p;
                    for(p=bi.begin();p!=bi.end();++p,i++) {
                        TEST(p->iterator()==iters[i]);
                        TEST(p->rule()==bmasks[i]);
                    }
                }
                // boundary_point to bound
                bi_type bi_2(bt,begin,end,l);
                bi_2.rule(mask);
                {
                    bi_type bi(bi_2);
                    unsigned i=0;
                    typename bi_type::iterator p;
                    for(p=bi.begin();p!=bi.end();++p,i++) {
                        TEST(p->iterator()==iters[i]);
                        TEST(p->rule()==bmasks[i]);
                    }
                }
                {
                    bi_type bi;
                    bi = bi_2;
                    unsigned i=0;
                    typename bi_type::iterator p;
                    for(p=bi.begin();p!=bi.end();++p,i++) {
                        TEST(p->iterator()==iters[i]);
                        TEST(p->rule()==bmasks[i]);
                    }
                }
            }
            {   // boundary_point to segment
                bi_type bi(bt,begin,end,l);
                {
                    ti_type ti(bi);
                    ti.rule(mask);
                    unsigned i=0;
                    typename ti_type::iterator p;
                    for(p=ti.begin();p!=ti.end();++p,i++) {
                        TEST(p->str()==chunks[i]);
                        TEST(p->rule()==unsigned(masks[i]));
                    }
                }
                {
                    ti_type ti;
                    ti.rule(mask);
                    ti = (bi);
                    unsigned i=0;
                    typename ti_type::iterator p;
                    for(p=ti.begin();p!=ti.end();++p,i++) {
                        TEST(p->str()==chunks[i]);
                        TEST(p->rule()==unsigned(masks[i]));
                    }
                }
                ti_type ti_2(bt,begin,end,l);
                ti_2.rule(mask);
                {
                    ti_type ti(ti_2);
                    unsigned i=0;
                    typename ti_type::iterator p;
                    for(p=ti.begin();p!=ti.end();++p,i++) {
                        TEST(p->str()==chunks[i]);
                        TEST(p->rule()==unsigned(masks[i]));
                    }
                }
                {
                    ti_type ti;
                    ti = (ti_2);
                    unsigned i=0;
                    typename ti_type::iterator p;
                    for(p=ti.begin();p!=ti.end();++p,i++) {
                        TEST(p->str()==chunks[i]);
                        TEST(p->rule()==unsigned(masks[i]));
                    }
                }
            }
        }
    } // for mask

}

template<typename Char>
void run_word(std::string *original,int *none,int *num,int *word,int *kana,int *ideo,std::locale l,lb::boundary_type b=lb::word)
{
    std::vector<int> pos;
    std::vector<std::basic_string<Char> > chunks;
    std::vector<int> masks;
    std::basic_string<Char> test_string;
    for(int i=0;!original[i].empty();i++) {
        chunks.push_back(to_correct_string<Char>(original[i],l));
        test_string+=chunks.back();
        pos.push_back(test_string.size());
        masks.push_back(
              ( none ? none[i]*15 : 0)
            | ( num  ? ((num[i]*15)  << 4) : 0) 
            | ( word ? ((word[i]*15) << 8) : 0) 
            | ( kana ? ((kana[i]*15) << 12) : 0)
            | ( ideo ? ((ideo[i]*15) << 16) : 0)
        );
    }

    std::list<Char> lst(test_string.begin(),test_string.end());
    test_word_container<Char>(lst.begin(),lst.end(),pos,masks,chunks,l,b);
    test_word_container<Char>(test_string.begin(),test_string.end(),pos,masks,chunks,l,b);
}

std::string character[]={"שָ","ל","וֹ","ם","!",""};
int         nones[]={1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};

std::string sentence1[]={"To be\n","or not\n","to be?\n"," That is the question. ","Or maybe not",""};
int         sentence1a[]={      0,          0,        1,                         1,             0, 0};
int         sentence1b[]={      1,          1,        0,                         0,             1, 0};

std::string line1[]={"To ","be\n","or ","not\n","to ","be",""};
int         line1a[]={ 1,   0,     1 ,  0,       1,   1 , 0 };
int         line1b[]={ 0,   1,     0 ,  1,       0,   0 , 0 };


void test_boundaries(std::string *all,int *first,int *second,lb::boundary_type t)
{
    boost::locale::generator g;
    std::cout << " char UTF-8" << std::endl;
    run_word<char>(all,first,second,0,0,0,g("he_IL.UTF-8"),t);
    std::cout << " char CP1255" << std::endl;
    run_word<char>(all,first,second,0,0,0,g("he_IL.cp1255"),t);
    std::cout << " wchar_t"<<std::endl;
    run_word<wchar_t>(all,first,second,0,0,0,g("he_IL.UTF-8"),t);
    #ifdef BOOST_HAS_CHAR16_T
    std::cout << " char16_t"<<std::endl;
    run_word<char16_t>(all,first,second,0,0,0,g("he_IL.UTF-8"),t);
    #endif
    #ifdef BOOST_HAS_CHAR32_T
    std::cout << " char32_t"<<std::endl;
    run_word<char32_t>(all,first,second,0,0,0,g("he_IL.UTF-8"),t);
    #endif

}

void word_boundary()
{
    boost::locale::generator g;
    
    std::string all1[]={"10"," ","Hello"," ","Windows7"," ","平仮名","ひらがな","ヒラガナ",""};
    int        none1[]={ 0,   1,      0,  1,         0,   1,      0,         0,          0};
    int         num1[]={ 1,   0,      0,  0,         1,   0,      0 ,        0 ,         0};
    int        word1[]={ 0,   0,      1,  0,         1,   0,      0 ,        0 ,         0};
    int        kana1[]={ 0,   0,      0,  0,         0,   0,      0,         1 ,         1}; 
    int        ideo1[]={ 0,   0,      0,  0,         0,   0,      1,         0 ,         0}; 


    int zero[25]={0};
    std::string all2[]={""};

    std::string all3[]={" "," ","Hello",",","World","!"," ",""};
    int        none3[]={ 1,  1,      0,  1,      0,   1,  1, 0};
    int        word3[]={ 0,  0,      1,  0,      1,   0,  0, 0};

    std::cout << " char UTF-8" << std::endl;
    run_word<char>(all1,none1,num1,word1,kana1,ideo1,g("ja_JP.UTF-8"));
    run_word<char>(all2,zero,zero,zero,zero,zero,g("en_US.UTF-8"));
    run_word<char>(all3,none3,zero,word3,zero,zero,g("en_US.UTF-8"));

    std::cout << " char Shift-JIS" << std::endl;
    run_word<char>(all1,none1,num1,word1,kana1,ideo1,g("ja_JP.Shift-JIS"));
    run_word<char>(all2,zero,zero,zero,zero,zero,g("ja_JP.Shift-JIS"));
    run_word<char>(all3,none3,zero,word3,zero,zero,g("ja_JP.Shift-JIS"));

    std::cout << " wchar_t"<<std::endl;
    run_word<wchar_t>(all1,none1,num1,word1,kana1,ideo1,g("ja_JP.UTF-8"));
    run_word<wchar_t>(all2,zero,zero,zero,zero,zero,g("en_US.UTF-8"));
    run_word<wchar_t>(all3,none3,zero,word3,zero,zero,g("en_US.UTF-8"));

    #ifdef BOOST_HAS_CHAR16_T
    std::cout << " char16_t"<<std::endl;
    run_word<char16_t>(all1,none1,num1,word1,kana1,ideo1,g("ja_JP.UTF-8"));
    run_word<char16_t>(all2,zero,zero,zero,zero,zero,g("en_US.UTF-8"));
    run_word<char16_t>(all3,none3,zero,word3,zero,zero,g("en_US.UTF-8"));
    #endif 

    #ifdef BOOST_HAS_CHAR32_T
    std::cout << " char32_t"<<std::endl;
    run_word<char32_t>(all1,none1,num1,word1,kana1,ideo1,g("ja_JP.UTF-8"));
    run_word<char32_t>(all2,zero,zero,zero,zero,zero,g("en_US.UTF-8"));
    run_word<char32_t>(all3,none3,zero,word3,zero,zero,g("en_US.UTF-8"));
    #endif 
}
void test_op_one_side(std::string const &sl,std::string const &sr,int val)
{
    boost::locale::boundary::ssegment l(sl.begin(),sl.end(),0),r(sr.begin(),sr.end(),0);

    // segment
    TEST( (l==r) == (val==0));
    TEST( (l!=r) == (val!=0));
    TEST( (l<=r) == (val<=0));
    TEST( (l< r) == (val<0));
    TEST( (l>=r) == (val>=0));
    TEST( (l> r) == (val>0));

    // C string
    TEST( (l==sr.c_str()) == (val==0));
    TEST( (l!=sr.c_str()) == (val!=0));
    TEST( (l<=sr.c_str()) == (val<=0));
    TEST( (l< sr.c_str()) == (val<0));
    TEST( (l>=sr.c_str()) == (val>=0));
    TEST( (l> sr.c_str()) == (val>0));
    
    TEST( (sl.c_str()==r) == (val==0));
    TEST( (sl.c_str()!=r) == (val!=0));
    TEST( (sl.c_str()<=r) == (val<=0));
    TEST( (sl.c_str()< r) == (val<0));
    TEST( (sl.c_str()>=r) == (val>=0));
    TEST( (sl.c_str()> r) == (val>0));


    // C++ string
    TEST( (l==sr) == (val==0));
    TEST( (l!=sr) == (val!=0));
    TEST( (l<=sr) == (val<=0));
    TEST( (l< sr) == (val<0));
    TEST( (l>=sr) == (val>=0));
    TEST( (l> sr) == (val>0));
    
    TEST( (sl==r) == (val==0));
    TEST( (sl!=r) == (val!=0));
    TEST( (sl<=r) == (val<=0));
    TEST( (sl< r) == (val<0));
    TEST( (sl>=r) == (val>=0));
    TEST( (sl> r) == (val>0));
    // self check
    TEST( (sl==sr) == (val==0));
    TEST( (sl!=sr) == (val!=0));
    TEST( (sl<=sr) == (val<=0));
    TEST( (sl< sr) == (val<0));
    TEST( (sl>=sr) == (val>=0));
    TEST( (sl> sr) == (val>0));

}

void test_op(std::string const &sl,std::string const &sr,int val)
{
    test_op_one_side(sl,sr,val);
    test_op_one_side(sr,sl,-val);
}
void segment_operator()
{
    test_op("","a",-1);
    test_op("","",0);
    test_op("aa","aaa",-1);
    test_op("aa","ab",-1);
}

int main()
{
    try {
        std::cout << "Testing segment operators" << std::endl;
        segment_operator();
        std::cout << "Testing word boundary" << std::endl;
        word_boundary();
        std::cout << "Testing character boundary" << std::endl;
        test_boundaries(character,nones,0,lb::character);
        std::cout << "Testing sentence boundary" << std::endl;
        test_boundaries(sentence1,sentence1a,sentence1b,lb::sentence);
        std::cout << "Testing line boundary" << std::endl;
        test_boundaries(line1,line1a,line1b,lb::line);
    }
    catch(std::exception const &e) {
        std::cerr << "Failed " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    FINALIZE();
}

#endif // NOICU
// vim: tabstop=4 expandtab shiftwidth=4 softtabstop=4

// boostinspect:noascii 
