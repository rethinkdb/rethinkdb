//
//  Copyright (c) 2009-2011 Artyom Beilis (Tonkikh)
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/locale/generator.hpp>
#include <boost/locale/localization_backend.hpp>
#include <boost/locale/message.hpp>
#include <boost/locale/gnu_gettext.hpp>
#include <boost/locale/encoding.hpp>
#include "test_locale.hpp"
#include "test_locale_tools.hpp"
#include <fstream>

namespace bl = boost::locale;

std::string backend;

bool file_loader_is_actually_called = false;

struct file_loader {
    std::vector<char> operator()(std::string const &name,std::string const &/*encoding*/) const
    {
        std::vector<char> buffer;
        std::ifstream f(name.c_str(),std::ifstream::binary);
        if(!f)
            return buffer;
        f.seekg(0,std::ifstream::end);
        size_t len = f.tellg();
        if(len == 0)
            return buffer;
        f.seekg(0);
        buffer.resize(len,'\0');
        f.read(&buffer[0],len);
        file_loader_is_actually_called = true;
        return buffer;
    }
};


std::string same_s(std::string s)
{
    return s;
}

std::wstring same_w(std::wstring s)
{
    return s;
}

#ifdef BOOST_HAS_CHAR16_T
std::u16string same_u16(std::u16string s)
{
    return s;
}
#endif

#ifdef BOOST_HAS_CHAR32_T
std::u32string same_u32(std::u32string s)
{
    return s;
}
#endif

template<typename Char>
void strings_equal(std::string n_c,std::string n_s,std::string n_p,int n,std::string iexpected,std::locale const &l,std::string domain)
{
    typedef std::basic_string<Char> string_type;
    string_type expected=to_correct_string<Char>(iexpected,l);

    string_type c = to<Char>(n_c.c_str());
    string_type s = to<Char>(n_s.c_str());
    string_type p = to<Char>(n_p.c_str());

    if(domain=="default") {
        TEST(bl::translate(c,s,p,n).str(l)==expected);
        Char const *c_c_str = c.c_str(),*s_c_str=s.c_str(), *p_c_str=p.c_str(); // workaround gcc-3.4 bug
        TEST(bl::translate(c_c_str,s_c_str,p_c_str,n).str(l)==expected);
        std::locale tmp_locale=std::locale();
        std::locale::global(l);
        string_type tmp=bl::translate(c,s,p,n);
        TEST(tmp==expected);
        tmp=bl::translate(c,s,p,n).str();
        TEST(tmp==expected);
        std::locale::global(tmp_locale);

        std::basic_ostringstream<Char> ss;
        ss.imbue(l);
        ss << bl::translate(c,s,p,n);
        TEST(ss.str()==expected);
    }
    TEST( bl::translate(c,s,p,n).str(l,domain)==expected );
    std::locale tmp_locale=std::locale();
    std::locale::global(l);
    TEST(bl::translate(c,s,p,n).str(domain)==expected);
    std::locale::global(tmp_locale);
    {
        std::basic_ostringstream<Char> ss;
        ss.imbue(l);
        ss << bl::as::domain(domain) << bl::translate(c,s,p,n);
        TEST(ss.str()==expected);
    }
    {
        std::basic_ostringstream<Char> ss;
        ss.imbue(l);
        ss << bl::as::domain(domain) << bl::translate(c.c_str(),s.c_str(),p.c_str(),n);
        TEST(ss.str()==expected);
    }
}




template<typename Char>
void strings_equal(std::string n_s,std::string n_p,int n,std::string iexpected,std::locale const &l,std::string domain)
{
    typedef std::basic_string<Char> string_type;
    string_type expected=to_correct_string<Char>(iexpected,l);
    string_type s = to<Char>(n_s.c_str());
    string_type p = to<Char>(n_p.c_str());
    if(domain=="default") {
        TEST(bl::translate(s,p,n).str(l)==expected);
        Char const *s_c_str=s.c_str(), *p_c_str=p.c_str(); // workaround gcc-3.4 bug
        TEST(bl::translate(s_c_str,p_c_str,n).str(l)==expected);
        std::locale tmp_locale=std::locale();
        std::locale::global(l);
        string_type tmp=bl::translate(s,p,n);
        TEST(tmp==expected);
        tmp=bl::translate(s,p,n).str();
        TEST(tmp==expected);
        std::locale::global(tmp_locale);

        std::basic_ostringstream<Char> ss;
        ss.imbue(l);
        ss << bl::translate(s,p,n);
        TEST(ss.str()==expected);
    }
    TEST(bl::translate(s,p,n).str(l,domain)==expected);
    std::locale tmp_locale=std::locale();
    std::locale::global(l);
    TEST(bl::translate(s,p,n).str(domain)==expected);
    std::locale::global(tmp_locale);
    {
        std::basic_ostringstream<Char> ss;
        ss.imbue(l);
        ss << bl::as::domain(domain) << bl::translate(s,p,n);
        TEST(ss.str()==expected);
    }
    {
        std::basic_ostringstream<Char> ss;
        ss.imbue(l);
        ss << bl::as::domain(domain) << bl::translate(s.c_str(),p.c_str(),n);
        TEST(ss.str()==expected);
    }
}


template<typename Char>
void strings_equal(std::string n_c,std::string n_original,std::string iexpected,std::locale const &l,std::string domain)
{
    typedef std::basic_string<Char> string_type;
    string_type expected=to_correct_string<Char>(iexpected,l);
    string_type original = to<Char>(n_original.c_str());
    string_type c = to<Char>(n_c.c_str());
    if(domain=="default") {
        TEST(bl::translate(c,original).str(l)==expected);
        Char const *original_c_str=original.c_str(); // workaround gcc-3.4 bug
        Char const *context_c_str = c.c_str();
        TEST(bl::translate(context_c_str,original_c_str).str(l)==expected);
        std::locale tmp_locale=std::locale();
        std::locale::global(l);
        string_type tmp=bl::translate(c,original);
        TEST(tmp==expected);
        tmp=bl::translate(c,original).str();
        TEST(tmp==expected);
        std::locale::global(tmp_locale);

        std::basic_ostringstream<Char> ss;
        ss.imbue(l);
        ss << bl::translate(c,original);
        TEST(ss.str()==expected);
    }
    TEST(bl::translate(c,original).str(l,domain)==expected);
    std::locale tmp_locale=std::locale();
    std::locale::global(l);
    TEST(bl::translate(c,original).str(domain)==expected);
    std::locale::global(tmp_locale);
    {
        std::basic_ostringstream<Char> ss;
        ss.imbue(l);
        ss << bl::as::domain(domain) << bl::translate(c,original);
        TEST(ss.str()==expected);
    }
    {
        std::basic_ostringstream<Char> ss;
        ss.imbue(l);
        ss << bl::as::domain(domain) << bl::translate(c.c_str(),original.c_str());
        TEST(ss.str()==expected);
    }
}




template<typename Char>
void strings_equal(std::string n_original,std::string iexpected,std::locale const &l,std::string domain)
{
    typedef std::basic_string<Char> string_type;
    string_type expected=to_correct_string<Char>(iexpected,l);
    string_type original = to<Char>(n_original.c_str());
    if(domain=="default") {
        TEST(bl::translate(original).str(l)==expected);
        Char const *original_c_str=original.c_str(); // workaround gcc-3.4 bug
        TEST(bl::translate(original_c_str).str(l)==expected);
        std::locale tmp_locale=std::locale();
        std::locale::global(l);
        string_type tmp=bl::translate(original);
        TEST(tmp==expected);
        tmp=bl::translate(original).str();
        TEST(tmp==expected);
        std::locale::global(tmp_locale);

        std::basic_ostringstream<Char> ss;
        ss.imbue(l);
        ss << bl::translate(original);
        TEST(ss.str()==expected);
    }
    TEST(bl::translate(original).str(l,domain)==expected);
    std::locale tmp_locale=std::locale();
    std::locale::global(l);
    TEST(bl::translate(original).str(domain)==expected);
    std::locale::global(tmp_locale);
    {
        std::basic_ostringstream<Char> ss;
        ss.imbue(l);
        ss << bl::as::domain(domain) << bl::translate(original);
        TEST(ss.str()==expected);
    }
    {
        std::basic_ostringstream<Char> ss;
        ss.imbue(l);
        ss << bl::as::domain(domain) << bl::translate(original.c_str());
        TEST(ss.str()==expected);
    }
}

void test_cntranslate(std::string c,std::string s,std::string p,int n,std::string expected,std::locale const &l,std::string domain)
{
    strings_equal<char>(c,s,p,n,expected,l,domain);
    strings_equal<wchar_t>(c,s,p,n,expected,l,domain);
    #ifdef BOOST_HAS_CHAR16_T
    if(backend=="icu" || backend=="std")
        strings_equal<char16_t>(c,s,p,n,expected,l,domain);
    #endif
    #ifdef BOOST_HAS_CHAR32_T
    if(backend=="icu" || backend=="std")
        strings_equal<char32_t>(c,s,p,n,expected,l,domain);
    #endif
}


void test_ntranslate(std::string s,std::string p,int n,std::string expected,std::locale const &l,std::string domain)
{
    strings_equal<char>(s,p,n,expected,l,domain);
    strings_equal<wchar_t>(s,p,n,expected,l,domain);
    #ifdef BOOST_HAS_CHAR16_T
    if(backend=="icu" || backend=="std")
        strings_equal<char16_t>(s,p,n,expected,l,domain);
    #endif
    #ifdef BOOST_HAS_CHAR32_T
    if(backend=="icu" || backend=="std")
        strings_equal<char32_t>(s,p,n,expected,l,domain);
    #endif
}

void test_ctranslate(std::string c,std::string original,std::string expected,std::locale const &l,std::string domain)
{
    strings_equal<char>(c,original,expected,l,domain);
    strings_equal<wchar_t>(c,original,expected,l,domain);
    #ifdef BOOST_HAS_CHAR16_T
    if(backend=="icu" || backend=="std")
        strings_equal<char16_t>(c,original,expected,l,domain);
    #endif
    #ifdef BOOST_HAS_CHAR32_T
    if(backend=="icu" || backend=="std")
        strings_equal<char32_t>(c,original,expected,l,domain);
    #endif
}



void test_translate(std::string original,std::string expected,std::locale const &l,std::string domain)
{
    strings_equal<char>(original,expected,l,domain);
    strings_equal<wchar_t>(original,expected,l,domain);
    #ifdef BOOST_HAS_CHAR16_T
    if(backend=="icu" || backend=="std")
        strings_equal<char16_t>(original,expected,l,domain);
    #endif
    #ifdef BOOST_HAS_CHAR32_T
    if(backend=="icu" || backend=="std")
        strings_equal<char32_t>(original,expected,l,domain);
    #endif
}

bool iso_8859_8_not_supported = false;


int main(int argc,char **argv)
{
    try {
        std::string def[] = {
        #ifdef BOOST_LOCALE_WITH_ICU
            "icu" , 
        #endif
        #ifndef BOOST_LOCALE_NO_STD_BACKEND
            "std" ,
        #endif
        #ifndef BOOST_LOCALE_NO_POSIX_BACKEND
            "posix",
        #endif
        #ifndef BOOST_LOCALE_NO_WINAPI_BACKEND
            "winapi",
        #endif
        };
        for(int type = 0 ; type < int(sizeof(def)/sizeof(def[0])) ; type ++ ) {
            boost::locale::localization_backend_manager tmp_backend = boost::locale::localization_backend_manager::global();
            tmp_backend.select(def[type]);
            boost::locale::localization_backend_manager::global(tmp_backend);
            
            backend = def[type];
            
            std::cout << "Testing for backend --------- " << def[type] << std::endl;

            boost::locale::generator g;
            g.add_messages_domain("default");
            g.add_messages_domain("simple");
            g.add_messages_domain("full");
            g.add_messages_domain("fall");
            if(argc==2)
                g.add_messages_path(argv[1]);
            else
                g.add_messages_path("./");

            
            std::string locales[] = { "he_IL.UTF-8", "he_IL.ISO8859-8" };

            for(unsigned i=0;i<sizeof(locales)/sizeof(locales[0]);i++){
                std::locale l;
                
                if(i==1) {
                    try {
                        l = g(locales[i]);
                    }
                    catch(boost::locale::conv::invalid_charset_error const &e) {
                        std::cout << "Looks like ISO-8859-8 is not supported! skipping" << std::endl;
                        iso_8859_8_not_supported = true;
                        continue;
                    }
                }
                else {
                        l = g(locales[i]);
                }
                
                std::cout << "  Testing "<<locales[i]<<std::endl;
                std::cout << "    single forms" << std::endl;

                test_translate("hello","שלום",l,"default");
                test_translate("hello","היי",l,"simple");
                test_translate("hello","hello",l,"undefined");
                test_translate("untranslated","untranslated",l,"default");
                // Check removal of old "context" information
                test_translate("#untranslated","#untranslated",l,"default");
                test_translate("##untranslated","##untranslated",l,"default");
                test_ctranslate("context","hello","שלום בהקשר אחר",l,"default");
                test_translate("#hello","#שלום",l,"default");

                std::cout << "    plural forms" << std::endl;

                {
                    test_ntranslate("x day","x days",0,"x ימים",l,"default");
                    test_ntranslate("x day","x days",1,"יום x",l,"default");
                    test_ntranslate("x day","x days",2,"יומיים",l,"default");
                    test_ntranslate("x day","x days",3,"x ימים",l,"default");
                    test_ntranslate("x day","x days",20,"x יום",l,"default");
                    
                    test_ntranslate("x day","x days",0,"x days",l,"undefined");
                    test_ntranslate("x day","x days",1,"x day",l,"undefined");
                    test_ntranslate("x day","x days",2,"x days",l,"undefined");
                    test_ntranslate("x day","x days",20,"x days",l,"undefined");
                }
                std::cout << "    plural forms with context" << std::endl;
                {
                    std::string inp = "context"; 
                    std::string out = "בהקשר "; 

                    test_cntranslate(inp,"x day","x days",0,out+"x ימים",l,"default");
                    test_cntranslate(inp,"x day","x days",1,out+"יום x",l,"default");
                    test_cntranslate(inp,"x day","x days",2,out+"יומיים",l,"default");
                    test_cntranslate(inp,"x day","x days",3,out+"x ימים",l,"default");
                    test_cntranslate(inp,"x day","x days",20,out+"x יום",l,"default");
                    
                    test_cntranslate(inp,"x day","x days",0,"x days",l,"undefined");
                    test_cntranslate(inp,"x day","x days",1,"x day",l,"undefined");
                    test_cntranslate(inp,"x day","x days",2,"x days",l,"undefined");
                    test_cntranslate(inp,"x day","x days",20,"x days",l,"undefined");
                }
            }
            std::cout << "  Testing fallbacks" <<std::endl;
            test_translate("test","he_IL",g("he_IL.UTF-8"),"full");
            test_translate("test","he",g("he_IL.UTF-8"),"fall");
            
            std::cout << "  Testing automatic conversions " << std::endl;
            std::locale::global(g("he_IL.UTF-8"));


            TEST(same_s(bl::translate("hello"))=="שלום");
            TEST(same_w(bl::translate(to<wchar_t>("hello")))==to<wchar_t>("שלום"));
            
            #ifdef BOOST_HAS_CHAR16_T
            if(backend=="icu" || backend=="std")
                TEST(same_u16(bl::translate(to<char16_t>("hello")))==to<char16_t>("שלום"));
            #endif
            
            #ifdef BOOST_HAS_CHAR32_T
            if(backend=="icu" || backend=="std")
                TEST(same_u32(bl::translate(to<char32_t>("hello")))==to<char32_t>("שלום"));
            #endif

        }
        
        std::cout << "Testing custom file system support" << std::endl;
        {
            boost::locale::gnu_gettext::messages_info info;
            info.language = "he";
            info.country = "IL";
            info.encoding="UTF-8";
            if(argc==2)
                info.paths.push_back(argv[1]);
            else
                info.paths.push_back("./");

            info.domains.push_back(bl::gnu_gettext::messages_info::domain("default"));
            info.callback = file_loader();

            std::locale l(std::locale::classic(),boost::locale::gnu_gettext::create_messages_facet<char>(info));
            TEST(file_loader_is_actually_called);
            TEST(bl::translate("hello").str(l)=="שלום");
        }
        if(iso_8859_8_not_supported)
        {
            std::cout << "ISO 8859-8 not supported so skipping non-US-ASCII keys" << std::endl; 
        }
        else 
        {
            std::cout << "Testing non-US-ASCII keys" << std::endl; 
            std::cout << "  UTF-8 keys" << std::endl; 
            {
                boost::locale::generator g;
                g.add_messages_domain("default");
                if(argc==2)
                    g.add_messages_path(argv[1]);
                else
                    g.add_messages_path("./");
                
                std::locale l = g("he_IL.UTF-8");

                // narrow
                TEST(bl::gettext("בדיקה",l)=="test");
                TEST(bl::gettext("לא קיים",l)=="לא קיים");
                
                // wide
                std::wstring wtest = bl::conv::to_utf<wchar_t>("בדיקה","UTF-8");
                std::wstring wmiss = bl::conv::to_utf<wchar_t>("לא קיים","UTF-8");
                TEST(bl::gettext(wtest.c_str(),l)==L"test");
                TEST(bl::gettext(wmiss.c_str(),l)==wmiss);

                l=g("he_IL.ISO-8859-8");

                // conversion with substitution
                TEST(bl::gettext("test-あにま-בדיקה",l)==bl::conv::from_utf("test--בדיקה","ISO-8859-8"));
            }
            
            std::cout << "  `ANSI' keys" << std::endl; 

            {
                boost::locale::generator g;
                g.add_messages_domain("default/ISO-8859-8");
                if(argc==2)
                    g.add_messages_path(argv[1]);
                else
                    g.add_messages_path("./");
                
                std::locale l = g("he_IL.UTF-8");

                // narrow non-UTF-8 keys
                // match
                TEST(bl::gettext(bl::conv::from_utf("בדיקה","ISO-8859-8").c_str(),l)=="test");
                // conversion
                TEST(bl::gettext(bl::conv::from_utf("לא קיים","ISO-8859-8").c_str(),l)=="לא קיים");
            }
        }
        // Test compiles
        {
            bl::gettext("");
            bl::gettext(L"");
            bl::dgettext("","");
            bl::dgettext("",L"");
            bl::pgettext("","");
            bl::pgettext(L"",L"");
            bl::dpgettext("","","");
            bl::dpgettext("",L"",L"");
            bl::ngettext("","",1);
            bl::ngettext(L"",L"",1);
            bl::dngettext("","","",1);
            bl::dngettext("",L"",L"",1);
            bl::npgettext("","","",1);
            bl::npgettext(L"",L"",L"",1);
            bl::dnpgettext("","","","",1);
            bl::dnpgettext("",L"",L"",L"",1);
        }
        
    }
    catch(std::exception const &e) {
        std::cerr << "Failed " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    FINALIZE();
}

// vim: tabstop=4 expandtab shiftwidth=4 softtabstop=4
// boostinspect:noascii 
