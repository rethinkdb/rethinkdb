// ----------------------------------------------------------------------------
// Copyright (C) 2002-2006 Marcin Kalicinski
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see www.boost.org
// ----------------------------------------------------------------------------

#include "test_utils.hpp"
#include <boost/property_tree/json_parser.hpp>

///////////////////////////////////////////////////////////////////////////////
// Test data

const char *ok_data_1 = 
    "{}";

const char *ok_data_2 = 
    "  \t {\n"
    "  \t \"name 0\" : \"value\", \t // comment \n"
    "  \t \"name 1\" : \"\", // comment \n"
    "  \t \"name 2\" : true, // comment \n"
    "  \t \"name 3\" : false, // comment \n"
    "  \t \"name 4\" : null, // comment \n"
    "  \t \"name 5\" : 0, // comment \n"
    "  \t \"name 6\" : -5, // comment \n"
    "  \t \"name 7\" : 1.1, // comment \n"
    "  \t \"name 8\" : -956.45e+4, // comment \n"
    "  \t \"name 8\" : 5956.45E-11, // comment \n"
    "  \t \"name 9\" : [1,2,3,4], // comment \n"
    "  \t \"name 10\" : {\"a\":\"b\"} // comment \n"
    "  \t } // comment \n";

const char *ok_data_3 = 
    "{\"a\":{\"b\":\"c\"}}";

const char *ok_data_4 = 
    "{\"a\":[{\"b\":\"c\"},{\"d\":\"e\"},1,2,3,4],\"f\":null}";

const char *ok_data_5 = 
    "{/* \n\n//{}//{}{\n{//\n}//{}{\n */}";

const char *ok_data_6 = 
    "{\"menu\": {\n"
    "    \"header\": \"SVG Viewer\",\n"
    "    \"items\": [\n"
    "        {\"id\": \"Open\"},\n"
    "        {\"id\": \"OpenNew\", \"label\": \"Open New\"},\n"
    "        null,\n"
    "        {\"id\": \"ZoomIn\", \"label\": \"Zoom In\"},\n"
    "        {\"id\": \"ZoomOut\", \"label\": \"Zoom Out\"},\n"
    "        {\"id\": \"OriginalView\", \"label\": \"Original View\"},\n"
    "        null,\n"
    "        {\"id\": \"Quality\"},\n"
    "        {\"id\": \"Pause\"},\n"
    "        {\"id\": \"Mute\"},\n"
    "        null,\n"
    "        {\"id\": \"Find\", \"label\": \"Find...\"},\n"
    "        {\"id\": \"FindAgain\", \"label\": \"Find Again\"},\n"
    "        {\"id\": \"Copy\"},\n"
    "        {\"id\": \"CopyAgain\", \"label\": \"Copy Again\"},\n"
    "        {\"id\": \"CopySVG\", \"label\": \"Copy SVG\"},\n"
    "        {\"id\": \"ViewSVG\", \"label\": \"View SVG\"},\n"
    "        {\"id\": \"ViewSource\", \"label\": \"View Source\"},\n"
    "        {\"id\": \"SaveAs\", \"label\": \"Save As\"},\n"
    "        null,\n"
    "        {\"id\": \"Help\"},\n"
    "        {\"id\": \"About\", \"label\": \"About Adobe CVG Viewer...\"}\n"
    "    ]\n"
    "}}\n";

const char *ok_data_7 = 
    "{\"web-app\": {\n"
    "  \"servlet\": [    // Defines the CDSServlet\n"
    "    {\n"
    "      \"servlet-name\": \"cofaxCDS\",\n"
    "      \"servlet-class\": \"org.cofax.cds.CDSServlet\",\n"
    "      \"init-param\": {\n"
    "        \"configGlossary:installationAt\": \"Philadelphia, PA\",\n"
    "        \"configGlossary:adminEmail\": \"ksm@pobox.com\",\n"
    "        \"configGlossary:poweredBy\": \"Cofax\",\n"
    "        \"configGlossary:poweredByIcon\": \"/images/cofax.gif\",\n"
    "        \"configGlossary:staticPath\": \"/content/static\",\n"
    "        \"templateProcessorClass\": \"org.cofax.WysiwygTemplate\",\n"
    "        \"templateLoaderClass\": \"org.cofax.FilesTemplateLoader\",\n"
    "        \"templatePath\": \"templates\",\n"
    "        \"templateOverridePath\": \"\",\n"
    "        \"defaultListTemplate\": \"listTemplate.htm\",\n"
    "        \"defaultFileTemplate\": \"articleTemplate.htm\",\n"
    "        \"useJSP\": false,\n"
    "        \"jspListTemplate\": \"listTemplate.jsp\",\n"
    "        \"jspFileTemplate\": \"articleTemplate.jsp\",\n"
    "        \"cachePackageTagsTrack\": 200,\n"
    "        \"cachePackageTagsStore\": 200,\n"
    "        \"cachePackageTagsRefresh\": 60,\n"
    "        \"cacheTemplatesTrack\": 100,\n"
    "        \"cacheTemplatesStore\": 50,\n"
    "        \"cacheTemplatesRefresh\": 15,\n"
    "        \"cachePagesTrack\": 200,\n"
    "        \"cachePagesStore\": 100,\n"
    "        \"cachePagesRefresh\": 10,\n"
    "        \"cachePagesDirtyRead\": 10,\n"
    "        \"searchEngineListTemplate\": \"forSearchEnginesList.htm\",\n"
    "        \"searchEngineFileTemplate\": \"forSearchEngines.htm\",\n"
    "        \"searchEngineRobotsDb\": \"WEB-INF/robots.db\",\n"
    "        \"useDataStore\": true,\n"
    "        \"dataStoreClass\": \"org.cofax.SqlDataStore\",\n"
    "        \"redirectionClass\": \"org.cofax.SqlRedirection\",\n"
    "        \"dataStoreName\": \"cofax\",\n"
    "        \"dataStoreDriver\": \"com.microsoft.jdbc.sqlserver.SQLServerDriver\",\n"
    "        \"dataStoreUrl\": \"jdbc:microsoft:sqlserver://LOCALHOST:1433;DatabaseName=goon\",\n"
    "        \"dataStoreUser\": \"sa\",\n"
    "        \"dataStorePassword\": \"dataStoreTestQuery\",\n"
    "        \"dataStoreTestQuery\": \"SET NOCOUNT ON;select test='test';\",\n"
    "        \"dataStoreLogFile\": \"/usr/local/tomcat/logs/datastore.log\",\n"
    "        \"dataStoreInitConns\": 10,\n"
    "        \"dataStoreMaxConns\": 100,\n"
    "        \"dataStoreConnUsageLimit\": 100,\n"
    "        \"dataStoreLogLevel\": \"debug\",\n"
    "        \"maxUrlLength\": 500}},\n"
    "    {\n"
    "      \"servlet-name\": \"cofaxEmail\",\n"
    "\n      \"servlet-class\": \"org.cofax.cds.EmailServlet\",\n"
    "      \"init-param\": {\n"
    "      \"mailHost\": \"mail1\",\n"
    "      \"mailHostOverride\": \"mail2\"}},\n"
    "    {\n"
    "      \"servlet-name\": \"cofaxAdmin\",\n"
    "      \"servlet-class\": \"org.cofax.cds.AdminServlet\"},\n"
    " \n"
    "    {\n"
    "      \"servlet-name\": \"fileServlet\",\n"
    "      \"servlet-class\": \"org.cofax.cds.FileServlet\"},\n"
    "    {\n"
    "      \"servlet-name\": \"cofaxTools\",\n"
    "      \"servlet-class\": \"org.cofax.cms.CofaxToolsServlet\",\n"
    "      \"init-param\": {\n"
    "        \"templatePath\": \"toolstemplates/\",\n"
    "        \"log\": 1,\n"
    "        \"logLocation\": \"/usr/local/tomcat/logs/CofaxTools.log\",\n"
    "        \"logMaxSize\": \"\",\n"
    "        \"dataLog\": 1,\n"
    "        \"dataLogLocation\": \"/usr/local/tomcat/logs/dataLog.log\",\n"
    "        \"dataLogMaxSize\": \"\",\n"
    "        \"removePageCache\": \"/content/admin/remove?cache=pages&id=\",\n"
    "        \"removeTemplateCache\": \"/content/admin/remove?cache=templates&id=\",\n"
    "        \"fileTransferFolder\": \"/usr/local/tomcat/webapps/content/fileTransferFolder\",\n"
    "        \"lookInContext\": 1,\n"
    "        \"adminGroupID\": 4,\n"
    "        \"betaServer\": true}}],\n"
    "  \"servlet-mapping\": {\n"
    "    \"cofaxCDS\": \"/\",\n"
    "    \"cofaxEmail\": \"/cofaxutil/aemail/*\",\n"
    "    \"cofaxAdmin\": \"/admin/*\",\n"
    "    \"fileServlet\": \"/static/*\",\n"
    "    \"cofaxTools\": \"/tools/*\"},\n"
    " \n"
    "  \"taglib\": {\n"
    "    \"taglib-uri\": \"cofax.tld\",\n"
    "    \"taglib-location\": \"/WEB-INF/tlds/cofax.tld\"}}}\n";

const char *ok_data_8 = 
    "{\"widget\": {\n"
    "    \"debug\": \"on\",\n"
    "    \"window\": {\n"
    "        \"title\": \"Sample Konfabulator Widget\",        \"name\": \"main_window\",        \"width\": 500,        \"height\": 500\n"
    "    },    \"image\": { \n"
    "        \"src\": \"Images/Sun.png\",\n"
    "        \"name\": \"sun1\",        \"hOffset\": 250,        \"vOffset\": 250,        \"alignment\": \"center\"\n"
    "    },    \"text\": {\n"
    "        \"data\": \"Click Here\",\n"
    "        \"size\": 36,\n"
    "        \"style\": \"bold\",        \"name\": \"text1\",        \"hOffset\": 250,        \"vOffset\": 100,        \"alignment\": \"center\",\n"
    "        \"onMouseUp\": \"sun1.opacity = (sun1.opacity / 100) * 90;\"\n"
    "    }\n"
    "}} \n";

const char *ok_data_9 = 
    "{\"menu\": {\n"
    "  \"id\": \"file\",\n"
    "  \"value\": \"File:\",\n"
    "  \"popup\": {\n"
    "    \"menuitem\": [\n"
    "      {\"value\": \"New\", \"onclick\": \"CreateNewDoc()\"},\n"
    "      {\"value\": \"Open\", \"onclick\": \"OpenDoc()\"},\n"
    "\n      {\"value\": \"Close\", \"onclick\": \"CloseDoc()\"}\n"
    "    ]\n"
    "  }\n"
    "}}\n";

const char *ok_data_10 = 
    "{\n"
    "   \"glossary\": {\n"
    "       \"title\": \"example glossary\",\n"
    "       \"GlossDiv\": {\n"
    "           \"title\": \"S\",\n"
    "           \"GlossList\": [{\n"
    "                \"ID\": \"SGML\",\n"
    "                \"SortAs\": \"SGML\",\n"
    "                \"GlossTerm\": \"Standard Generalized Markup Language\",\n"
    "                \"Acronym\": \"SGML\",\n"
    "                \"Abbrev\": \"ISO 8879:1986\",\n"
    "                \"GlossDef\": \n"
    "\"A meta-markup language, used to create markup languages such as DocBook.\",\n"
    "                \"GlossSeeAlso\": [\"GML\", \"XML\", \"markup\"]\n"
    "            }]\n"
    "        }\n"
    "    }\n"
    "}\n";

const char *ok_data_11 = 
    "{\n"
    "    \"data\" : [\n"
    "        {\n"
    "            \"First Name\" : \"Bob\",\n"
    "            \"Last Name\" : \"Smith\",\n"
    "            \"Email\" : \"bsmith@someurl.com\",\n"
    "            \"Phone\" : \"(555) 555-1212\"\n"
    "        },\n"
    "        {\n"
    "            \"First Name\" : \"Jan\",\n"
    "            \"Last Name\" : \"Smith\",\n"
    "            \"Email\" : \"jsmith@someurl.com\",\n"
    "            \"Phone\" : \"(555) 555-3434\"\n"
    "        },\n"
    "        {\n"
    "            \"First Name\" : \"Sue\",\n"
    "            \"Last Name\" : \"Smith\",\n"
    "            \"Email\" : \"ssmith@someurl.com\",\n"
    "            \"Phone\" : \"(555) 555-5656\"\n"
    "        }\n"
    "    ]\n"
    "}\n";

const char *ok_data_12 = 
    "{\"  \\\" \\\\ \\b \\f \\n \\r \\t  \" : \"multi\" \"-\" \"string\"}";

const char *error_data_1 = 
    "";   // No root object

const char *error_data_2 = 
    "{\n\"a\":1\n";      // Unclosed object brace

const char *error_data_3 = 
    "{\n\"a\":\n[1,2,3,4\n}";      // Unclosed array brace

const char *error_data_4 = 
    "{\n\"a\"\n}";      // No object

const char *bug_data_pr4387 =
    "[1, 2, 3]"; // Root array

struct ReadFunc
{
    template<class Ptree>
    void operator()(const std::string &filename, Ptree &pt) const
    {
        boost::property_tree::read_json(filename, pt);
    }
};

struct WriteFunc
{
    template<class Ptree>
    void operator()(const std::string &filename, const Ptree &pt) const
    {
        boost::property_tree::write_json(filename, pt);
    }
};

template<class Ptree>
void test_json_parser()
{

    using namespace boost::property_tree;

    generic_parser_test_ok<Ptree, ReadFunc, WriteFunc>
    (
        ReadFunc(), WriteFunc(), ok_data_1, NULL, 
        "testok1.json", NULL, "testok1out.json", 1, 0, 0
    );
    
    generic_parser_test_ok<Ptree, ReadFunc, WriteFunc>
    (
        ReadFunc(), WriteFunc(), ok_data_2, NULL, 
        "testok2.json", NULL, "testok2out.json", 18, 50, 74
    );

    generic_parser_test_ok<Ptree, ReadFunc, WriteFunc>
    (
        ReadFunc(), WriteFunc(), ok_data_3, NULL, 
        "testok3.json", NULL, "testok3out.json", 3, 1, 2
    );

    generic_parser_test_ok<Ptree, ReadFunc, WriteFunc>
    (
        ReadFunc(), WriteFunc(), ok_data_4, NULL, 
        "testok4.json", NULL, "testok4out.json", 11, 10, 4
    );

    generic_parser_test_ok<Ptree, ReadFunc, WriteFunc>
    (
        ReadFunc(), WriteFunc(), ok_data_5, NULL, 
        "testok5.json", NULL, "testok5out.json", 1, 0, 0
    );

    generic_parser_test_ok<Ptree, ReadFunc, WriteFunc>
    (
        ReadFunc(), WriteFunc(), ok_data_6, NULL, 
        "testok6.json", NULL, "testok6out.json", 56, 265, 111
    );

    generic_parser_test_ok<Ptree, ReadFunc, WriteFunc>
    (
        ReadFunc(), WriteFunc(), ok_data_7, NULL, 
        "testok7.json", NULL, "testok7out.json", 87, 1046, 1216
    );

    generic_parser_test_ok<Ptree, ReadFunc, WriteFunc>
    (
        ReadFunc(), WriteFunc(), ok_data_8, NULL, 
        "testok8.json", NULL, "testok8out.json", 23, 149, 125
    );

    generic_parser_test_ok<Ptree, ReadFunc, WriteFunc>
    (
        ReadFunc(), WriteFunc(), ok_data_9, NULL, 
        "testok9.json", NULL, "testok9out.json", 15, 54, 60
    );

    generic_parser_test_ok<Ptree, ReadFunc, WriteFunc>
    (
        ReadFunc(), WriteFunc(), ok_data_10, NULL, 
        "testok10.json", NULL, "testok10out.json", 17, 162, 85
    );

    generic_parser_test_ok<Ptree, ReadFunc, WriteFunc>
    (
        ReadFunc(), WriteFunc(), ok_data_11, NULL, 
        "testok11.json", NULL, "testok11out.json", 17, 120, 91
    );

    generic_parser_test_ok<Ptree, ReadFunc, WriteFunc>
    (
        ReadFunc(), WriteFunc(), ok_data_12, NULL, 
        "testok12.json", NULL, "testok12out.json", 2, 12, 17
    );

    generic_parser_test_error<ptree, ReadFunc, WriteFunc, json_parser_error>
    (
        ReadFunc(), WriteFunc(), error_data_1, NULL,
        "testerr1.json", NULL, "testerr1out.json", 1
    );

    generic_parser_test_error<ptree, ReadFunc, WriteFunc, json_parser_error>
    (
        ReadFunc(), WriteFunc(), error_data_2, NULL,
        "testerr2.json", NULL, "testerr2out.json", 3
    );

    generic_parser_test_error<ptree, ReadFunc, WriteFunc, json_parser_error>
    (
        ReadFunc(), WriteFunc(), error_data_3, NULL,
        "testerr3.json", NULL, "testerr3out.json", 4
    );

    generic_parser_test_error<ptree, ReadFunc, WriteFunc, json_parser_error>
    (
        ReadFunc(), WriteFunc(), error_data_4, NULL,
        "testerr4.json", NULL, "testerr4out.json", 3
    );

    generic_parser_test_ok<Ptree, ReadFunc, WriteFunc>
    (
        ReadFunc(), WriteFunc(), bug_data_pr4387, NULL, 
        "testpr4387.json", NULL, "testpr4387out.json", 4, 3, 0
    );

}

int test_main(int argc, char *argv[])
{
    using namespace boost::property_tree;
    test_json_parser<ptree>();
    test_json_parser<iptree>();
#ifndef BOOST_NO_CWCHAR
    test_json_parser<wptree>();
    test_json_parser<wiptree>();
#endif
    return 0;
}
