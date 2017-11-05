<?xml version="1.0" encoding="utf-8"?>
<!-- Visual C++ 2015 project file template for RethinkDB -->
<xsl:stylesheet version="1.0"
                xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns="http://schemas.microsoft.com/developer/msbuild/2003"
                exclude-result-prefixes="xsl">
  <xsl:output method="xml" indent="yes" encoding="utf-8" />

  <xsl:template match="/config">
    <Project DefaultTargets="Build" ToolsVersion="14.0">
      <xsl:if test="@ImproveParallelBuild = 'yes'">
        <xsl:attribute name="InitialTargets">UNDUPOBJ</xsl:attribute>
      </xsl:if>

      <ItemGroup Label="ProjectConfigurations">
        <xsl:for-each select="build">
          <ProjectConfiguration Include="{@configuration}|{@platform}">
            <Configuration><xsl:value-of select="@configuration" /></Configuration>
            <Platform><xsl:value-of select="@platform" /></Platform>
          </ProjectConfiguration>
        </xsl:for-each>
      </ItemGroup>

      <PropertyGroup Label="Globals">
        <ProjectGuid>{CB045D34-1C67-473B-ABAB-83F0D630596D}</ProjectGuid>
        <Keyword>Win32Proj</Keyword>
        <OutDir>build\$(Configuration)_$(Platform)\</OutDir>
        <xsl:choose>
          <xsl:when test="/config/unittest">
            <IntDir>build\$(Configuration)_$(Platform)\unit\</IntDir>
          </xsl:when>
          <xsl:otherwise>
            <IntDir>build\$(Configuration)_$(Platform)\</IntDir>
          </xsl:otherwise>
        </xsl:choose>
        <WindowsTargetPlatformVersion>
          <xsl:value-of select="target/@version" />
        </WindowsTargetPlatformVersion>
      </PropertyGroup>

      <Import Project="$(VCTargetsPath)\Microsoft.Cpp.Default.props" />

      <xsl:for-each select="build">
        <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='{@configuration}|{@platform}'" Label="Configuration">
          <ConfigurationType>Application</ConfigurationType>
          <UseDebugLibraries>
            <xsl:choose>
              <xsl:when test="@configuration = 'Debug'">true</xsl:when>
              <xsl:otherwise>false</xsl:otherwise>
            </xsl:choose>
          </UseDebugLibraries>
          <PlatformToolset>
            <xsl:value-of select="/config/target/@toolset" />
          </PlatformToolset>
        </PropertyGroup>
      </xsl:for-each>

      <Import Project="$(VCTargetsPath)\Microsoft.Cpp.props" />
      <ImportGroup Label="ExtensionSettings" />
      <ImportGroup Label="Shared" />

      <xsl:for-each select="build">
        <ImportGroup Condition="'$(Configuration)|$(Platform)'=='{@configuration}|{@platform}'" Label="PropertySheets">
          <Import Project="$(UserRootDir)\Microsoft.Cpp.$(Platform).user.props" Condition="exists('$(UserRootDir)\Microsoft.Cpp.$(Platform).user.props')" Label="LocalAppDataPlatform" />
        </ImportGroup>
      </xsl:for-each>

      <PropertyGroup Label="UserMacros" />

      <xsl:for-each select="build">
        <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='{@configuration}|{@platform}'">
          <LinkIncremental>true</LinkIncremental>
          <IncludePath>$(ProjectDir)\build\proto;$(ProjectDir)\src;$(IncludePath)</IncludePath>
          <LibraryPath>$(ProjectDir)\build\windows_deps\lib\<xsl:value-of select="@platform" />\<xsl:value-of select="@configuration" />;$(LibraryPath)</LibraryPath>
        </PropertyGroup>

        <ItemDefinitionGroup Condition="'$(Configuration)|$(Platform)'=='{@configuration}|{@platform}'">
          <ClCompile>
            <PreprocessorDefinitions>
              WIN32;
              WINVER=<xsl:value-of select="/config/target/@winver" />;
              _WIN32_WINNT=<xsl:value-of select="/config/target/@winver" />;
              RETHINKDB_VERSION="<xsl:value-of select="/config/@RethinkDBVersion" />";
              _USE_MATH_DEFINES;
              COMPILER_MSVC;
              RAPIDJSON_HAS_STDSTRING;
              RAPIDJSON_PARSE_DEFAULT_FLAGS=kParseFullPrecisionFlag;
              V8_NEEDS_BUFFER_ALLOCATOR;
              BOOST_DATE_TIME_NO_LIB;
              <xsl:if test="@configuration = 'Release'">
                NDEBUG;
              </xsl:if>
            </PreprocessorDefinitions>

            <DebugInformationFormat>ProgramDatabase</DebugInformationFormat>

            <xsl:choose>
              <xsl:when test="@configuration = 'Debug'">
                <RuntimeLibrary>MultiThreadedDebug</RuntimeLibrary>
                <Optimization>Disabled</Optimization>
              </xsl:when>
              <xsl:otherwise>
                <RuntimeLibrary>MultiThreaded</RuntimeLibrary>
              </xsl:otherwise>
            </xsl:choose>

            <xsl:choose>
              <xsl:when test="/config/@ImproveParallelBuild = 'yes'">
                <ObjectFileName>$(IntDir)</ObjectFileName>
              </xsl:when>
              <xsl:otherwise>
                <ObjectFileName>$(IntDir)/%(RelativeDir)/</ObjectFileName>
              </xsl:otherwise>
            </xsl:choose>

            <AdditionalIncludeDirectories>
              $(ProjectDir)\src\arch\windows_stub;
              $(ProjectDir)\build\windows_deps\include;
            </AdditionalIncludeDirectories>
            <xsl:if test="/config/@PreprocessToFile = 'yes'">
              <PreprocessToFile>true</PreprocessToFile>
            </xsl:if>
            <xsl:if test="/config/@ShowIncludes = 'yes'">
              <ShowIncludes>true</ShowIncludes>
            </xsl:if>

            <SuppressStartupBanner>true</SuppressStartupBanner>
            <MultiProcessorCompilation>true</MultiProcessorCompilation>
            <EnableParallelCodeGeneration>true</EnableParallelCodeGeneration>
            <MinimalRebuild>false</MinimalRebuild>

            <AdditionalOptions>
              /bigobj <!-- Allow more symbols in object files -->
              /Zm10   <!-- Reduce memory allocation of compiler -->
            </AdditionalOptions>

            <!-- Enable this at your own risk. System headers are not excluded.
                <WarningLevel>EnableAllWarnings</WarningLevel> -->
            <DisableSpecificWarnings>
              4244; 4267; <!-- conversion from 'type1' to 'type2', possible loss of data -->
              4804; <!-- unsafe use of type 'bool' -->
              4091; <!-- 'keyword' : ignored on left of 'type' when no variable is declared -->
              4996; <!-- deprecation declaration or function call with parameters that may be unsafe -->
            </DisableSpecificWarnings>
          </ClCompile>
          <Link>
            <GenerateDebugInformation>true</GenerateDebugInformation>
            <SubSystem>Console</SubSystem>
            <AdditionalDependencies>
              dbghelp.lib; iphlpapi.lib; WS2_32.lib; winmm.lib;
              curl.lib;
              libprotobuf.lib;
              re2.lib;
              zlib.lib;
              ssleay32.lib; libeay32.lib;
              v8_base_0.lib; v8_base_1.lib; v8_base_2.lib; v8_base_3.lib;
              v8_snapshot.lib; v8_libbase.lib; v8_libplatform.lib;
              icui18n.lib; icuuc.lib;
              <xsl:if test="@configuration = 'Debug'">
                gtest.lib;
              </xsl:if>
              %(AdditionalDependencies);
            </AdditionalDependencies>
            <EntryPointSymbol>mainCRTStartup</EntryPointSymbol>
            <AdditionalOptions>
            </AdditionalOptions>
            <xsl:if test="@platform = 'Win32'">
              <TargetMachine>MachineX86</TargetMachine>
            </xsl:if>
          </Link>
        </ItemDefinitionGroup>

        <ItemGroup Condition="'$(Configuration)|$(Platform)'=='{@configuration}|{@platform}'">
          <ResourceCompile Include="build\bundle_assets\web_assets.rc" />
          <ClCompile Include="build\bundle_assets\web_assets.cc" />
          <ClCompile Include="build\proto\rdb_protocol\ql2.pb.cc" />
          <ClCompile Include="src\**\*.cc">
            <xsl:choose>
              <xsl:when test="/config/unittest">
                <xsl:message>UNIT</xsl:message>
                <xsl:attribute name="Exclude">src\main.cc</xsl:attribute>
              </xsl:when>
              <xsl:otherwise>
                <xsl:message>NOUNIT</xsl:message>
                <xsl:attribute name="Exclude">src\unittest\**\*.cc</xsl:attribute>
              </xsl:otherwise>
            </xsl:choose>
          </ClCompile>
        </ItemGroup>
      </xsl:for-each>

      <ItemGroup>
        <ClInclude Include="src\**\*.hpp" />
        <ClInclude Include="src\**\*.tcc" />
        <ClInclude Include="src\**\*.h" />
        <ClInclude Include="build\proto\rdb_protocol\ql2.pb.h" />
      </ItemGroup>
      <ItemGroup>
        <None Include="src\rdb_protocol\ql2.proto" />
        <None Include="src\rdb_protocol\ql2_extensions.proto" />
      </ItemGroup>
      <Import Project="$(VCTargetsPath)\Microsoft.Cpp.targets" />
      <ImportGroup Label="ExtensionTargets" />

      <xsl:if test="@ImproveParallelBuild = 'yes'">
        <xsl:call-template name="UNDUPOBJ" />
      </xsl:if>
    </Project>
  </xsl:template>

  <xsl:template name="UNDUPOBJ">
    <!-- copied from -->
    <!-- http://stackoverflow.com/questions/3729515/visual-studio-2010-2008-cant-handle-source-files-with-identical-names-in-diff/26935613 -->
    <!-- relevant topics -->
    <!-- http://stackoverflow.com/questions/7033855/msvc10-mp-builds-not-multicore-across-folders-in-a-project -->
    <!-- http://stackoverflow.com/questions/18304911/how-can-one-modify-an-itemdefinitiongroup-from-an-msbuild-target -->
    <!-- other maybe related info -->
    <!-- http://stackoverflow.com/questions/841913/modify-msbuild-itemgroup-metadata -->
    <UsingTask TaskName="UNDUPOBJ_TASK" TaskFactory="CodeTaskFactory" AssemblyFile="$(MSBuildToolsPath)\Microsoft.Build.Tasks.v4.0.dll">
      <ParameterGroup>
        <OutputDir ParameterType="System.String" Required="true" />
        <ItemList ParameterType="Microsoft.Build.Framework.ITaskItem[]" Required="true" />
        <OutputItemList ParameterType="Microsoft.Build.Framework.ITaskItem[]" Output="true" />
      </ParameterGroup>
      <Task>
        <Code><![CDATA[
        //general outline: for each item (in ClCompile) assign it to a subdirectory of $(IntDir) by allocating subdirectories 0,1,2, etc., as needed to prevent duplicate filenames from clobbering each other
        //this minimizes the number of batches that need to be run, since each subdirectory will necessarily be in a distinct batch due to /Fo specifying that output subdirectory

        var assignmentMap = new Dictionary<string,int>();
        foreach( var item in ItemList )
        {
          var filename = item.GetMetadata("Filename");

          //assign reused filenames to increasing numbers
          //assign previously unused filenames to 0
          int assignment = 0;
          if(assignmentMap.TryGetValue(filename, out assignment))
            assignmentMap[filename] = ++assignment;
          else
            assignmentMap[filename] = 0;

          var thisFileOutdir = Path.Combine(OutputDir,assignment.ToString()) + "/"; //take care it ends in / so /Fo knows it's a directory and not a filename
          item.SetMetadata( "ObjectFileName", thisFileOutdir );
        }

        OutputItemList = ItemList;
        ItemList = new Microsoft.Build.Framework.ITaskItem[0];

        ]]></Code>
      </Task>
    </UsingTask>

    <Target Name="UNDUPOBJ">
      <!-- see stackoverflow topics for discussion on why we need to do some loopy copying stuff here -->
      <ItemGroup>
        <ClCompileCopy Include="@(ClCompile)"/>
        <ClCompile Remove="@(ClCompile)"/>
      </ItemGroup>
      <UNDUPOBJ_TASK OutputDir="$(IntDir)" ItemList="@(ClCompileCopy)" OutputItemList="@(ClCompile)">
        <Output ItemName="ClCompile" TaskParameter="OutputItemList"/>
      </UNDUPOBJ_TASK>
    </Target>
  </xsl:template>
</xsl:stylesheet>
