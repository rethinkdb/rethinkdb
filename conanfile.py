from conan import ConanFile
from conan.tools.cmake import cmake_layout


class RethinkDBConan(ConanFile):
    name = "rethinkdb"
    version = "2.4.4"
    license = "Apache-2.0"
    author = "RethinkDB"
    url = "https://github.com/rethinkdb/rethinkdb"
    description = "The open-source database for the realtime web"
    topics = ("database", "nosql", "realtime", "json")
    settings = "os", "compiler", "build_type", "arch"
    
    options = {
        "with_jemalloc": [True, False],
        "with_tests": [True, False],
    }
    
    default_options = {
        "with_jemalloc": True,
        "with_tests": False,
        "protobuf/*:shared": False,
        "openssl/*:shared": False,
        "libcurl/*:shared": False,
    }
    
    generators = "CMakeDeps", "CMakeToolchain"
    
    def requirements(self):
        """Define dependencies"""
        self.requires("protobuf/3.21.12")
        self.requires("openssl/3.2.0")
        self.requires("zlib/1.3.1")
        self.requires("libcurl/8.5.0")
        self.requires("re2/20231101")
        self.requires("boost/1.84.0")
        
        if self.options.with_jemalloc:
            self.requires("jemalloc/5.3.0")
    
    def build_requirements(self):
        """Build-time dependencies"""
        if self.options.with_tests:
            self.requires("gtest/1.14.0")
    
    def layout(self):
        cmake_layout(self)
    
    def configure(self):
        """Configure options based on platform"""
        if self.settings.os == "Windows":
            # Windows doesn't support jemalloc well
            self.options.with_jemalloc = False
