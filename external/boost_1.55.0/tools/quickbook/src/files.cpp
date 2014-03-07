/*=============================================================================
    Copyright (c) 2002 2004 2006 Joel de Guzman
    Copyright (c) 2004 Eric Niebler
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#include "files.hpp"
#include <boost/filesystem/fstream.hpp>
#include <boost/unordered_map.hpp>
#include <boost/range/algorithm/upper_bound.hpp>
#include <boost/range/algorithm/transform.hpp>
#include <boost/foreach.hpp>
#include <fstream>
#include <iterator>

namespace quickbook
{
    namespace
    {
        boost::unordered_map<fs::path, file_ptr> files;
    }

    // Read the first few bytes in a file to see it starts with a byte order
    // mark. If it doesn't, then write the characters we've already read in.
    // Although, given how UTF-8 works, if we've read anything in, the files
    // probably broken.

    template <typename InputIterator, typename OutputIterator>
    bool check_bom(InputIterator& begin, InputIterator end,
            OutputIterator out, char const* chars, int length)
    {
        char const* ptr = chars;

        while(begin != end && *begin == *ptr) {
            ++begin;
            ++ptr;
            --length;
            if(length == 0) return true;
        }

        // Failed to match, so write the skipped characters to storage:
        while(chars != ptr) *out++ = *chars++;

        return false;
    }

    template <typename InputIterator, typename OutputIterator>
    std::string read_bom(InputIterator& begin, InputIterator end,
            OutputIterator out)
    {
        if(begin == end) return "";

        const char* utf8 = "\xef\xbb\xbf" ;
        const char* utf32be = "\0\0\xfe\xff";
        const char* utf32le = "\xff\xfe\0\0";

        unsigned char c = *begin;
        switch(c)
        {
        case 0xEF: { // UTF-8
            return check_bom(begin, end, out, utf8, 3) ? "UTF-8" : "";
        }
        case 0xFF: // UTF-16/UTF-32 little endian
            return !check_bom(begin, end, out, utf32le, 2) ? "" :
                check_bom(begin, end, out, utf32le + 2, 2) ? "UTF-32" : "UTF-16";
        case 0: // UTF-32 big endian
            return check_bom(begin, end, out, utf32be, 4) ? "UTF-32" : "";
        case 0xFE: // UTF-16 big endian
            return check_bom(begin, end, out, utf32be + 2, 2) ? "UTF-16" : "";
        default:
            return "";
        }
    }

    // Copy a string, converting mac and windows style newlines to unix
    // newlines.

    template <typename InputIterator, typename OutputIterator>
    void normalize(InputIterator begin, InputIterator end,
            OutputIterator out)
    {
        std::string encoding = read_bom(begin, end, out);

        if(encoding != "UTF-8" && encoding != "")
        throw load_error(encoding +
            " is not supported. Please use UTF-8.");

        while(begin != end) {
            if(*begin == '\r') {
                *out++ = '\n';
                ++begin;
                if(begin != end && *begin == '\n') ++begin;
            }
            else {
                *out++ = *begin++;
            }
        }
    }

    file_ptr load(fs::path const& filename, unsigned qbk_version)
    {
        boost::unordered_map<fs::path, file_ptr>::iterator pos
            = files.find(filename);

        if (pos == files.end())
        {
            fs::ifstream in(filename, std::ios_base::in);

            if (!in)
                throw load_error("Could not open input file.");

            // Turn off white space skipping on the stream
            in.unsetf(std::ios::skipws);

            std::string source;
            normalize(
                std::istream_iterator<char>(in),
                std::istream_iterator<char>(),
                std::back_inserter(source));

            if (in.bad())
                throw load_error("Error reading input file.");

            bool inserted;

            boost::tie(pos, inserted) = files.emplace(
                filename, new file(filename, source, qbk_version));

            assert(inserted);
        }

        return pos->second;
    }

    std::ostream& operator<<(std::ostream& out, file_position const& x)
    {
        return out << "line: " << x.line << ", column: " << x.column;
    }

    file_position relative_position(
        boost::string_ref::const_iterator begin,
        boost::string_ref::const_iterator iterator)
    {
        file_position pos;
        boost::string_ref::const_iterator line_begin = begin;

        while (begin != iterator)
        {
            if (*begin == '\r')
            {
                ++begin;
                ++pos.line;
                line_begin = begin;
            }
            else if (*begin == '\n')
            {
                ++begin;
                ++pos.line;
                line_begin = begin;
                if (begin == iterator) break;
                if (*begin == '\r')
                {
                    ++begin;
                    line_begin = begin;
                }
            }
            else
            {
                ++begin;
            }
        }

        pos.column = iterator - line_begin + 1;
        return pos;
    }

    file_position file::position_of(boost::string_ref::const_iterator iterator) const
    {
        return relative_position(source().begin(), iterator);
    }

    // Mapped files.

    struct mapped_file_section
    {
        enum section_types {
            normal,
            empty,
            indented
        };
    
        std::string::size_type original_pos;
        std::string::size_type our_pos;
        section_types section_type;

        mapped_file_section(
                std::string::size_type original_pos,
                std::string::size_type our_pos,
                section_types section_type = normal) :
            original_pos(original_pos), our_pos(our_pos),
            section_type(section_type) {}
    };

    struct mapped_section_original_cmp
    {
        bool operator()(mapped_file_section const& x,
                mapped_file_section const& y)
        {
            return x.original_pos < y.original_pos;
        }

        bool operator()(mapped_file_section const& x,
                std::string::size_type const& y)
        {
            return x.original_pos < y;
        }

        bool operator()(std::string::size_type const& x,
                mapped_file_section const& y)
        {
            return x < y.original_pos;
        }
    };

    struct mapped_section_pos_cmp
    {
        bool operator()(mapped_file_section const& x,
                mapped_file_section const& y)
        {
            return x.our_pos < y.our_pos;
        }

        bool operator()(mapped_file_section const& x,
                std::string::size_type const& y)
        {
            return x.our_pos < y;
        }

        bool operator()(std::string::size_type const& x,
                mapped_file_section const& y)
        {
            return x < y.our_pos;
        }
    };
    
    struct mapped_file : file
    {
        mapped_file(file_ptr original) :
            file(*original, std::string()),
            original(original), mapped_sections()
        {}

        file_ptr original;
        std::vector<mapped_file_section> mapped_sections;
        
        void add_empty_mapped_file_section(boost::string_ref::const_iterator pos) {
            std::string::size_type original_pos =
                pos - original->source().begin();
        
            if (mapped_sections.empty() ||
                    mapped_sections.back().section_type !=
                        mapped_file_section::empty ||
                    mapped_sections.back().original_pos != original_pos)
            {
                mapped_sections.push_back(mapped_file_section(
                        original_pos, source().size(),
                        mapped_file_section::empty));
            }
        }

        void add_mapped_file_section(boost::string_ref::const_iterator pos) {
            mapped_sections.push_back(mapped_file_section(
                pos - original->source().begin(), source().size()));
        }

        void add_indented_mapped_file_section(boost::string_ref::const_iterator pos)
        {
            mapped_sections.push_back(mapped_file_section(
                pos - original->source().begin(), source().size(),
                mapped_file_section::indented));
        }

        std::string::size_type to_original_pos(
            std::vector<mapped_file_section>::const_iterator section,
            std::string::size_type pos) const
        {
            switch (section->section_type) {
                case mapped_file_section::normal:
                    return pos - section->our_pos + section->original_pos;

                case mapped_file_section::empty:
                    return section->original_pos;

                case mapped_file_section::indented: {
                    // Will contain the start of the current line.
                    boost::string_ref::size_type our_line = section->our_pos;

                    // Will contain the number of lines in the block before
                    // the current line.
                    unsigned newline_count = 0;

                    for(boost::string_ref::size_type i = section->our_pos;
                        i != pos; ++i)
                    {
                        if (source()[i] == '\n') {
                            our_line = i + 1;
                            ++newline_count;
                        }
                    }

                    // The start of the line in the original source.
                    boost::string_ref::size_type original_line =
                        section->original_pos;
                    
                    while(newline_count > 0) {
                        if (original->source()[original_line] == '\n')
                            --newline_count;
                        ++original_line;
                    }

                    // The start of line content (i.e. after indentation).
                    our_line = skip_indentation(source(), our_line);

                    // The position is in the middle of indentation, so
                    // just return the start of the whitespace, which should
                    // be good enough.
                    if (our_line > pos) return original_line;

                    original_line =
                        skip_indentation(original->source(), original_line);

                    // Confirm that we are actually in the same position.
                    assert(original->source()[original_line] ==
                        source()[our_line]);

                    // Calculate the position
                    return original_line + (pos - our_line);
                }
                default:
                    assert(false);
                    return section->original_pos;
            }
        }
        
        std::vector<mapped_file_section>::const_iterator find_section(
            boost::string_ref::const_iterator pos) const
        {
            std::vector<mapped_file_section>::const_iterator section =
                boost::upper_bound(mapped_sections,
                    std::string::size_type(pos - source().begin()),
                    mapped_section_pos_cmp());
            assert(section != mapped_sections.begin());
            --section;

            return section;
        }

        virtual file_position position_of(boost::string_ref::const_iterator) const;

    private:

        static std::string::size_type skip_indentation(
                boost::string_ref src, std::string::size_type i)
        {
            while (i != src.size() && (src[i] == ' ' || src[i] == '\t')) ++i;
            return i;
        }

    };

    namespace {
        std::list<mapped_file> mapped_files;
    }

    struct mapped_file_builder_data
    {
        mapped_file_builder_data() { reset(); }
        void reset() { new_file.reset(); }
    
        boost::intrusive_ptr<mapped_file> new_file;
    };

    mapped_file_builder::mapped_file_builder() : data(0) {}
    mapped_file_builder::~mapped_file_builder() { delete data; }

    void mapped_file_builder::start(file_ptr f)
    {
        if (!data) {
            data = new mapped_file_builder_data;
        }

        assert(!data->new_file);
        data->new_file = new mapped_file(f);
    }

    file_ptr mapped_file_builder::release()
    {
        file_ptr r = data->new_file;
        data->reset();
        return r;
    }

    void mapped_file_builder::clear()
    {
        data->reset();
    }
    
    bool mapped_file_builder::empty() const
    {
        return data->new_file->source().empty();
    }

    mapped_file_builder::pos mapped_file_builder::get_pos() const
    {
        return data->new_file->source().size();
    }
    
    void mapped_file_builder::add_at_pos(boost::string_ref x, iterator pos)
    {
        data->new_file->add_empty_mapped_file_section(pos);
        data->new_file->source_.append(x.begin(), x.end());
    }

    void mapped_file_builder::add(boost::string_ref x)
    {
        data->new_file->add_mapped_file_section(x.begin());
        data->new_file->source_.append(x.begin(), x.end());
    }

    void mapped_file_builder::add(mapped_file_builder const& x)
    {
        add(x, 0, x.data->new_file->source_.size());
    }

    void mapped_file_builder::add(mapped_file_builder const& x,
            pos begin, pos end)
    {
        assert(data->new_file->original == x.data->new_file->original);
        assert(begin <= x.data->new_file->source_.size());
        assert(end <= x.data->new_file->source_.size());

        if (begin != end)
        {
            std::vector<mapped_file_section>::const_iterator start =
                x.data->new_file->find_section(
                    x.data->new_file->source().begin() + begin);
    
            std::string::size_type size = data->new_file->source_.size();
    
            data->new_file->mapped_sections.push_back(mapped_file_section(
                    x.data->new_file->to_original_pos(start, begin),
                    size, start->section_type));
    
            for (++start; start != x.data->new_file->mapped_sections.end() &&
                    start->our_pos < end; ++start)
            {
                data->new_file->mapped_sections.push_back(mapped_file_section(
                    start->original_pos, start->our_pos - begin + size,
                    start->section_type));
            }
    
            data->new_file->source_.append(
                x.data->new_file->source_.begin() + begin,
            x.data->new_file->source_.begin() + end);
        }
    }

    boost::string_ref::size_type indentation_count(boost::string_ref x)
    {
        unsigned count = 0;

        for(boost::string_ref::const_iterator begin = x.begin(), end = x.end();
            begin != end; ++begin)
        {
            switch(*begin)
            {
            case ' ':
                ++count;
                break;
            case '\t':
                // hardcoded tab to 4 for now
                count = count - (count % 4) + 4;
                break;
            default:
                assert(false);
            }
        }

        return count;
    }

    void mapped_file_builder::unindent_and_add(boost::string_ref x)
    {
        // I wanted to do everything using a string_ref, but unfortunately
        // they don't have all the overloads used in here. So...
        std::string const program(x.begin(), x.end());

        // Erase leading blank lines and newlines:
        std::string::size_type start = program.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) return;

        start = program.find_last_of("\r\n", start);
        start = start == std::string::npos ? 0 : start + 1;

        assert(start < program.size());

        // Get the first line indentation
        std::string::size_type indent = program.find_first_not_of(" \t", start) - start;
        boost::string_ref::size_type full_indent = indentation_count(
            boost::string_ref(&program[start], indent));

        std::string::size_type pos = start;

        // Calculate the minimum indent from the rest of the lines
        // Detecting a mix of spaces and tabs.
        while (std::string::npos != (pos = program.find_first_of("\r\n", pos)))
        {
            pos = program.find_first_not_of("\r\n", pos);
            if (std::string::npos == pos) break;

            std::string::size_type n = program.find_first_not_of(" \t", pos);
            if (n == std::string::npos) break;

            char ch = program[n];
            if (ch == '\r' || ch == '\n') continue; // ignore empty lines

            indent = (std::min)(indent, n-pos);
            full_indent = (std::min)(full_indent, indentation_count(
                boost::string_ref(&program[pos], n-pos)));
        }

        // Detect if indentation is mixed.
        bool mixed_indentation = false;
        boost::string_ref first_indent(&program[start], indent);
        pos = start;

        while (std::string::npos != (pos = program.find_first_of("\r\n", pos)))
        {
            pos = program.find_first_not_of("\r\n", pos);
            if (std::string::npos == pos) break;

            std::string::size_type n = program.find_first_not_of(" \t", pos);
            if (n == std::string::npos || n-pos < indent) continue;

            if (boost::string_ref(&program[pos], indent) != first_indent) {
                mixed_indentation = true;
                break;
            }
        }

        // Trim white spaces from column 0..indent
        std::string unindented_program;
        std::string::size_type copy_start = start;
        pos = start;

        do {
            if (std::string::npos == (pos = program.find_first_not_of("\r\n", pos)))
                break;

            unindented_program.append(program.begin() + copy_start, program.begin() + pos);
            copy_start = pos;

            // Find the end of the indentation.
            std::string::size_type next = program.find_first_not_of(" \t", pos);
            if (next == std::string::npos) next = program.size();

            if (mixed_indentation)
            {
                unsigned length = indentation_count(boost::string_ref(
                    &program[pos], next - pos));

                if (length > full_indent) {
                    std::string new_indentation(length - full_indent, ' ');
                    unindented_program.append(new_indentation);
                }

                copy_start = next;
            }
            else
            {
                copy_start = (std::min)(pos + indent, next);
            }

            pos = next;
        } while (std::string::npos !=
            (pos = program.find_first_of("\r\n", pos)));

        unindented_program.append(program.begin() + copy_start, program.end());

        data->new_file->add_indented_mapped_file_section(x.begin());
        data->new_file->source_.append(unindented_program);
    }

    file_position mapped_file::position_of(boost::string_ref::const_iterator pos) const
    {
        return original->position_of(original->source().begin() +
            to_original_pos(find_section(pos), pos - source().begin()));
    }
}
