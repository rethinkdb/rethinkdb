// Copyright Nikolay Mladenov 2007.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// boost::python::make_tuple below are for gcc 4.4 -std=c++0x compatibility
// (Intel C++ 10 and 11 with -std=c++0x don't need the full qualification).

#include <boost/python/converter/registrations.hpp>
#include <boost/python/object/function_doc_signature.hpp>
#include <boost/python/errors.hpp>
#include <boost/python/str.hpp>
#include <boost/python/args.hpp>
#include <boost/python/tuple.hpp>

#include <boost/python/detail/signature.hpp>

#include <vector>

namespace boost { namespace python { namespace objects {

    bool function_doc_signature_generator::arity_cmp( function const *f1, function const *f2 )
    {
        return f1->m_fn.max_arity() < f2->m_fn.max_arity();
    }

    bool function_doc_signature_generator::are_seq_overloads( function const *f1, function const *f2 , bool check_docs)
    {
        py_function const & impl1 = f1->m_fn;
        py_function const & impl2  = f2->m_fn;

        //the number of parameters differs by 1
        if (impl2.max_arity()-impl1.max_arity() != 1)
            return false;

        // if check docs then f1 shold not have docstring or have the same docstring as f2
        if (check_docs && f2->doc() != f1->doc() && f1->doc())
            return false;

        python::detail::signature_element const* s1 = impl1.signature();
        python::detail::signature_element const* s2 = impl2.signature();

        unsigned size = impl1.max_arity()+1;

        for (unsigned i = 0; i != size; ++i)
        {
            //check if the argument types are the same
            if (s1[i].basename != s2[i].basename)
                return false;

            //return type
            if (!i) continue;

            //check if the argument default values are the same
            bool f1_has_names = bool(f1->m_arg_names);
            bool f2_has_names = bool(f2->m_arg_names);
            if ( (f1_has_names && f2_has_names && f2->m_arg_names[i-1]!=f1->m_arg_names[i-1])
                 || (f1_has_names && !f2_has_names)
                 || (!f1_has_names && f2_has_names && f2->m_arg_names[i-1]!=python::object())
                )
                return false;
        }
        return true;
    }

    std::vector<function const*> function_doc_signature_generator::flatten(function const *f)
    {
        object name = f->name();

        std::vector<function const*> res;

        while (f) {

            //this if takes out the not_implemented_function
            if (f->name() == name)
                res.push_back(f);

            f=f->m_overloads.get();
        }

        //std::sort(res.begin(),res.end(), &arity_cmp);

        return res;
    }
    std::vector<function const*> function_doc_signature_generator::split_seq_overloads( const std::vector<function const *> &funcs, bool split_on_doc_change)
    {
        std::vector<function const*> res;

        std::vector<function const*>::const_iterator fi = funcs.begin();

        function const * last = *fi;

        while (++fi != funcs.end()){

            //check if fi starts a new chain of overloads
            if (!are_seq_overloads( last, *fi, split_on_doc_change ))
                res.push_back(last);

            last = *fi;
        }

        if (last)
            res.push_back(last);

        return res;
    }

    str function_doc_signature_generator::raw_function_pretty_signature(function const *f, size_t n_overloads,  bool cpp_types )
    {
        str res("object");

        res = str("%s %s(%s)" % make_tuple( res, f->m_name, str("tuple args, dict kwds")) );

        return res;
    }

    const  char * function_doc_signature_generator::py_type_str(const python::detail::signature_element &s)
    {
        if (s.basename==std::string("void")){
            static const char * none = "None";
            return none;
        }

        PyTypeObject const * py_type = s.pytype_f?s.pytype_f():0;
        if ( py_type )
            return  py_type->tp_name;
        else{
            static const char * object = "object";
            return object;
        }
    }

    str function_doc_signature_generator::parameter_string(py_function const &f, size_t n, object arg_names, bool cpp_types)
    {
        str param;

        python::detail::signature_element  const * s = f.signature();
        if (cpp_types)
        {
            if(!n)
                s = &f.get_return_type();
            if (s[n].basename == 0)
            {
                return str("...");
            }

            param = str(s[n].basename);

            if (s[n].lvalue)
                 param += " {lvalue}";

        }
        else
        {
            if (n) //we are processing an argument and trying to come up with a name for it
            {
                object kv;
                if ( arg_names && (kv = arg_names[n-1]) )
                    param = str( " (%s)%s" % make_tuple(py_type_str(s[n]),kv[0]) );
                else
                    param = str(" (%s)%s%d" % make_tuple(py_type_str(s[n]),"arg", n) );
            }
            else //we are processing the return type
                param = py_type_str(f.get_return_type());
        }

        //an argument - check for default value and append it
        if(n && arg_names)
        {
            object kv(arg_names[n-1]);
            if (kv && len(kv) == 2)
            {
                param = str("%s=%r" % make_tuple(param, kv[1]));
            }
        }
        return param;
    }

    str function_doc_signature_generator::pretty_signature(function const *f, size_t n_overloads,  bool cpp_types )
    {
        py_function
            const& impl = f->m_fn;
            ;


        unsigned arity = impl.max_arity();

        if(arity == unsigned(-1))// is this the proper raw function test?
        {
            return raw_function_pretty_signature(f,n_overloads,cpp_types);
        }

        list formal_params;

        size_t n_extra_default_args=0;

        for (unsigned n = 0; n <= arity; ++n)
        {
            str param;

            formal_params.append(
                parameter_string(impl, n, f->m_arg_names, cpp_types)
                );

            // find all the arguments with default values preceeding the arity-n_overloads
            if (n && f->m_arg_names)
            {
                object kv(f->m_arg_names[n-1]);

                if (kv && len(kv) == 2)
                {
                    //default argument preceeding the arity-n_overloads
                    if( n <= arity-n_overloads)
                        ++n_extra_default_args;
                }
                else
                    //argument without default, preceeding the arity-n_overloads
                    if( n <= arity-n_overloads)
                        n_extra_default_args = 0;
            }
        }

        n_overloads+=n_extra_default_args;

        if (!arity && cpp_types)
            formal_params.append("void");

        str ret_type (formal_params.pop(0));
        if (cpp_types )
        {
            return str(
                "%s %s(%s%s%s%s)"
                % boost::python::make_tuple // workaround, see top
                ( ret_type
                    , f->m_name
                    , str(",").join(formal_params.slice(0,arity-n_overloads))
                    , n_overloads ? (n_overloads!=arity?str(" [,"):str("[ ")) : str()
                    , str(" [,").join(formal_params.slice(arity-n_overloads,arity))
                    , std::string(n_overloads,']')
                    ));
        }else{
            return str(
                "%s(%s%s%s%s) -> %s"
                % boost::python::make_tuple // workaround, see top
                ( f->m_name
                    , str(",").join(formal_params.slice(0,arity-n_overloads))
                    , n_overloads ? (n_overloads!=arity?str(" [,"):str("[ ")) : str()
                    , str(" [,").join(formal_params.slice(arity-n_overloads,arity))
                    , std::string(n_overloads,']')
                    , ret_type
                    ));
        }

        return str(
            "%s %s(%s%s%s%s) %s"
            % boost::python::make_tuple // workaround, see top
            ( cpp_types?ret_type:str("")
                , f->m_name
                , str(",").join(formal_params.slice(0,arity-n_overloads))
                , n_overloads ? (n_overloads!=arity?str(" [,"):str("[ ")) : str()
                , str(" [,").join(formal_params.slice(arity-n_overloads,arity))
                , std::string(n_overloads,']')
                , cpp_types?str(""):ret_type
                ));

    }

    namespace detail {    
        char py_signature_tag[] = "PY signature :";
        char cpp_signature_tag[] = "C++ signature :";
    }

    list function_doc_signature_generator::function_doc_signatures( function const * f)
    {
        list signatures;
        std::vector<function const*> funcs = flatten( f);
        std::vector<function const*> split_funcs = split_seq_overloads( funcs, true);
        std::vector<function const*>::const_iterator sfi=split_funcs.begin(), fi;
        size_t n_overloads=0;
        for (fi=funcs.begin(); fi!=funcs.end(); ++fi)
        {
            if(*sfi == *fi){
                if((*fi)->doc())
                {
                    str func_doc = str((*fi)->doc());
                    
                    int doc_len = len(func_doc);

                    bool show_py_signature = doc_len >= int(sizeof(detail::py_signature_tag)/sizeof(char)-1)
                                            && str(detail::py_signature_tag) == func_doc.slice(0, int(sizeof(detail::py_signature_tag)/sizeof(char))-1);
                    if(show_py_signature)
                    {
                        func_doc = str(func_doc.slice(int(sizeof(detail::py_signature_tag)/sizeof(char))-1, _));
                        doc_len = len(func_doc);
                    }
                    
                    bool show_cpp_signature = doc_len >= int(sizeof(detail::cpp_signature_tag)/sizeof(char)-1)
                                            && str(detail::cpp_signature_tag) == func_doc.slice( 1-int(sizeof(detail::cpp_signature_tag)/sizeof(char)), _);
                    
                    if(show_cpp_signature)
                    {
                        func_doc = str(func_doc.slice(_, 1-int(sizeof(detail::cpp_signature_tag)/sizeof(char))));
                        doc_len = len(func_doc);
                    }
                    
                    str res="\n";
                    str pad = "\n";

                    if(show_py_signature)
                    { 
                        str sig = pretty_signature(*fi, n_overloads,false);
                        res+=sig;
                        if(doc_len || show_cpp_signature )res+=" :";
                        pad+= str("    ");
                    }
                    
                    if(doc_len)
                    {
                        if(show_py_signature)
                            res+=pad;
                         res+= pad.join(func_doc.split("\n"));
                    }

                    if( show_cpp_signature)
                    {
                        if(len(res)>1)
                            res+="\n"+pad;
                        res+=detail::cpp_signature_tag+pad+"    "+pretty_signature(*fi, n_overloads,true);
                    }
                    
                    signatures.append(res);
                }
                ++sfi;
                n_overloads = 0;
            }else
                ++n_overloads ;
        }

        return signatures;
    }


}}}

