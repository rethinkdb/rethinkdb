import xml.sax.saxutils
import time

def make_test_name( library_idx, test_idx ):
    return "test_%02d_%02d" % ( library_idx, test_idx )

def make_library_name( library_idx ):
    if library_idx % 4 in ( 0, 1 ):
        return "library_%02d/%02d" % ( int( library_idx / 4 ) * 4, library_idx % 4 )
    else:
        return "library_%02d" % library_idx

def make_toolset_name( toolset_idx ):
    return "toolset_%02d" % toolset_idx

def make_library_target_directory( library_idx, toolset_idx, variant = None ):
    base = "lib/%s/%s" % ( make_library_name( library_idx )
                           , make_toolset_name( toolset_idx ) )
    if variant is not None:
        return "%s/%s" % ( base, variant )
    else:
        return base

def make_test_target_directory( library_idx, toolset_idx, test_name, variant ):
    base = "%s/%s/%s" % ( make_library_name( library_idx )
                          , make_toolset_name( toolset_idx )
                          , test_name )
    if variant is not None:
        return "%s/%s" % ( base, variant )
    else:
        return base

def format_timestamp( timestamp ):
    return time.strftime( "%Y-%m-%dT%H:%M:%SZ", timestamp )

def make_test_log( xml_generator
                   , library_idx
                   , toolset_idx
                   , test_name
                   , test_type
                   , test_result
                   , show_run_output
                   , variant ):
    library = make_library_name( library_idx )
    toolset_name = make_toolset_name( toolset_idx )
    
    target_directory = ""
    if test_type != "lib":
        target_directory = make_test_target_directory( library_idx, toolset_idx, test_name, variant )
    else:
        target_directory = make_library_target_directory( library_idx, toolset_idx, variant )
        
    xml_generator.startElement( "test-log", { "library": library
                                  , "test-name":  test_name
                                  , "toolset": toolset_name
                                  , "test-type": test_type
                                  , "test-program": "some_program"
                                  , "target-directory": target_directory
                                  , "show-run-output": show_run_output
                                  } )

    if test_type != "lib":

        if test_result == "success" and ( toolset_idx + 1 ) % 4:
            xml_generator.startElement( "compile", { "result": "success" } );
            xml_generator.characters( "Compiling in %s" % target_directory )
            xml_generator.endElement( "compile" )

        if test_type.find( "link" ) == 0 or test_type.find( "run" ) == 0 and toolset_idx % 4:
            xml_generator.startElement( "lib", { "result": test_result } );
            xml_generator.characters( make_library_target_directory( library_idx, toolset_idx ) )
            xml_generator.endElement( "lib" )

            xml_generator.startElement( "link", { "result": "success" } );
            xml_generator.characters( "Linking in %s" % target_directory )
            xml_generator.endElement( "link" )

        if test_type.find( "run" ) == 0 and ( toolset_idx + 2 ) % 4:
            xml_generator.startElement( "run", { "result": test_result } );
            xml_generator.characters( "Running in %s" % target_directory )
            xml_generator.endElement( "run" )

    else:
        xml_generator.startElement( "compile", { "result": test_result } );
        xml_generator.characters( "Compiling in %s" % make_library_target_directory( library_idx, toolset_idx ) )
        xml_generator.endElement( "compile" )



    xml_generator.endElement( "test-log" )


def make_expicit_failure_markup( num_of_libs, num_of_toolsets, num_of_tests ):
    g = xml.sax.saxutils.XMLGenerator( open( "explicit-failures-markup.xml", "w" ), "utf-8" )
    g.startDocument()
    g.startElement( "explicit-failures-markup", {} );

    # required toolsets
    for i_toolset in range( 0, num_of_toolsets ):
        if i_toolset < 2:
            g.startElement( "mark-toolset", { "name": "toolset_%02d" % i_toolset, "status":"required"} )
            g.endElement( "mark-toolset" )

    for i_library in range( 0, num_of_libs ):
        g.startElement( "library", { "name": make_library_name( i_library ) } )
        if i_library % 4 == 0:
            g.startElement( "mark-unusable", {} )
            for i_toolset in range( 0, num_of_toolsets ):
                if i_toolset % 2 == 1:
                    g.startElement( "toolset", { "name": make_toolset_name( i_toolset ) } )
                    g.endElement( "toolset" )
            g.startElement( "note", { "author": u"T. T\xe8st" } )
            g.characters( "Test note" )
            g.endElement( "note" )
            g.endElement( "mark-unusable" )

        for i_test in range( 0, num_of_tests ):

            category = 0
            explicitly_marked_failure = 0
            unresearched = 0

            if i_test % 2 == 0:
                category = i_test % 3
            
            if i_test % 3 == 0:
                explicitly_marked_failure = 1
                if i_test % 2 == 0:
                    unresearched = 1

            if category or explicitly_marked_failure:
                test_attrs = { "name": make_test_name( i_library, i_test ) }
                if category:
                    test_attrs[ "category" ] = "Category %s" % category
                g.startElement( "test", test_attrs )
                if explicitly_marked_failure:
                    failure_attrs = {}
                    if unresearched: failure_attrs[ "reason" ] = "not-researched"
                    
                    g.startElement( "mark-failure", failure_attrs )
                    
                    g.startElement( "toolset", { "name": make_toolset_name( 1 ) } )
                    g.endElement( "toolset" )
                    g.startElement( "toolset", { "name": make_toolset_name( 0 ) } )
                    g.endElement( "toolset" )
                    g.startElement( "toolset", { "name": make_toolset_name( 2 ) } )
                    g.endElement( "toolset" )

                    g.startElement( "note", {  "author": u"V. Ann\xf3tated" } )
                    g.characters( "Some thoughtful note" )
                    g.endElement( "note" )
                    
                    g.endElement( "mark-failure" )
                    
                g.endElement( "test" );
        g.endElement( "library" )
            
        
    g.endElement( "explicit-failures-markup" )
    g.endDocument()


def make_expected_results( num_of_libs, num_of_toolsets, num_of_tests ):
    pass

