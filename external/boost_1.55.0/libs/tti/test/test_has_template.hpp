
//  (C) Copyright Edward Diener 2011
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#if !defined(TEST_HAS_TEMPLATE_HPP)
#define TEST_HAS_TEMPLATE_HPP

#include "test_structs.hpp"
#include <boost/tti/has_template.hpp>

#if BOOST_PP_VARIADICS

BOOST_TTI_HAS_TEMPLATE(ATPMemberTemplate)
BOOST_TTI_TRAIT_HAS_TEMPLATE(HaveCL,CLMemberTemplate,BOOST_PP_NIL)
BOOST_TTI_HAS_TEMPLATE(AMemberTemplate)
BOOST_TTI_TRAIT_HAS_TEMPLATE(HaveAnotherMT,AnotherMemberTemplate,BOOST_PP_NIL)
BOOST_TTI_HAS_TEMPLATE(SomeMemberTemplate,BOOST_PP_NIL)
BOOST_TTI_TRAIT_HAS_TEMPLATE(ATemplateWithParms,ManyParameters)
BOOST_TTI_HAS_TEMPLATE(SimpleTMP,BOOST_PP_NIL)
BOOST_TTI_HAS_TEMPLATE(TemplateNotExist)

#else // !BOOST_PP_VARIADICS

BOOST_TTI_HAS_TEMPLATE(ATPMemberTemplate,BOOST_PP_NIL)
BOOST_TTI_TRAIT_HAS_TEMPLATE(HaveCL,CLMemberTemplate,BOOST_PP_NIL)
BOOST_TTI_HAS_TEMPLATE(AMemberTemplate,BOOST_PP_NIL)
BOOST_TTI_TRAIT_HAS_TEMPLATE(HaveAnotherMT,AnotherMemberTemplate,BOOST_PP_NIL)
BOOST_TTI_HAS_TEMPLATE(SomeMemberTemplate,BOOST_PP_NIL)
BOOST_TTI_TRAIT_HAS_TEMPLATE(ATemplateWithParms,ManyParameters,BOOST_PP_NIL)
BOOST_TTI_HAS_TEMPLATE(SimpleTMP,BOOST_PP_NIL)
BOOST_TTI_HAS_TEMPLATE(TemplateNotExist,BOOST_PP_NIL)

#endif // BOOST_PP_VARIADICS

#endif // TEST_HAS_TEMPLATE_HPP
