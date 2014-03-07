/*=============================================================================
    Copyright (c) 2011, 2013 Daniel James

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#include <cctype>
#include "id_manager_impl.hpp"
#include <boost/make_shared.hpp>
#include <boost/unordered_map.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>
#include <boost/range/algorithm.hpp>

// TODO: This should possibly try to always generate valid XML ids:
// http://www.w3.org/TR/REC-xml/#NT-NameStartChar

namespace quickbook {
    //
    // The maximum size of a generated part of an id.
    //
    // Not a strict maximum, sometimes broken because the user
    // explicitly uses a longer id, or for backwards compatibility.

    static const std::size_t max_size = 32;

    typedef std::vector<id_placeholder const*> placeholder_index;
    placeholder_index index_placeholders(id_state const&, boost::string_ref);

    void generate_id_block(
            placeholder_index::iterator, placeholder_index::iterator,
            std::vector<std::string>& generated_ids);

    std::vector<std::string> generate_ids(id_state const& state, boost::string_ref xml)
    {
        std::vector<std::string> generated_ids(state.placeholders.size());

        // Get a list of the placeholders in the order that we wish to
        // process them.
        placeholder_index placeholders = index_placeholders(state, xml);

        typedef std::vector<id_placeholder const*>::iterator iterator;
        iterator it = placeholders.begin(), end = placeholders.end();

        while (it != end) {
            // We process all the ids that have the same number of dots
            // together. Note that ids with different parents can clash, e.g.
            // because of old fashioned id generation or anchors containing
            // multiple dots.
            //
            // So find the group of placeholders with the same number of dots.
            iterator group_begin = it, group_end = it;
            while (group_end != end && (*group_end)->num_dots == (*it)->num_dots)
                ++group_end;

            generate_id_block(group_begin, group_end, generated_ids);
            it = group_end;
        }

        return generated_ids;
    }

    //
    // index_placeholders
    //
    // Create a sorted index of the placeholders, in order
    // to make numbering duplicates easy. A total order.
    //

    struct placeholder_compare
    {
        std::vector<unsigned>& order;

        placeholder_compare(std::vector<unsigned>& order) : order(order) {}

        bool operator()(id_placeholder const* x, id_placeholder const* y) const
        {
            bool x_explicit = x->category.c >= id_category::explicit_id;
            bool y_explicit = y->category.c >= id_category::explicit_id;

            return
                x->num_dots < y->num_dots ? true :
                x->num_dots > y->num_dots ? false :
                x_explicit > y_explicit ? true :
                x_explicit < y_explicit ? false :
                order[x->index] < order[y->index];
        }
    };

    struct get_placeholder_order_callback : xml_processor::callback
    {
        id_state const& state;
        std::vector<unsigned>& order;
        unsigned count;

        get_placeholder_order_callback(id_state const& state,
                std::vector<unsigned>& order)
          : state(state),
            order(order),
            count(0)
        {}

        void id_value(boost::string_ref value)
        {
            set_placeholder_order(state.get_placeholder(value));
        }

        void set_placeholder_order(id_placeholder const* p)
        {
            if (p && !order[p->index]) {
                set_placeholder_order(p->parent);
                order[p->index] = ++count;
            }
        }
    };

    placeholder_index index_placeholders(
            id_state const& state,
            boost::string_ref xml)
    {
        // The order that the placeholder appear in the xml source.
        std::vector<unsigned> order(state.placeholders.size());

        xml_processor processor;
        get_placeholder_order_callback callback(state, order);
        processor.parse(xml, callback);

        placeholder_index sorted_placeholders;
        sorted_placeholders.reserve(state.placeholders.size());
        BOOST_FOREACH(id_placeholder const& p, state.placeholders)
            if (order[p.index]) sorted_placeholders.push_back(&p);
        boost::sort(sorted_placeholders, placeholder_compare(order));

        return sorted_placeholders;
    }

    // Resolve and generate ids.

    struct generate_id_block_type
    {
        // The ids which won't require duplicate handling.
        typedef boost::unordered_map<std::string, id_placeholder const*>
            chosen_id_map;
        chosen_id_map chosen_ids;
        std::vector<std::string>& generated_ids;

        generate_id_block_type(std::vector<std::string>& generated_ids) :
            generated_ids(generated_ids) {}

        void generate(placeholder_index::iterator begin,
            placeholder_index::iterator end);

        std::string resolve_id(id_placeholder const*);
        std::string generate_id(id_placeholder const*, std::string const&);
    };

    void generate_id_block(placeholder_index::iterator begin,
            placeholder_index::iterator end,
            std::vector<std::string>& generated_ids)
    {
        generate_id_block_type impl(generated_ids);
        impl.generate(begin, end);
    }

    void generate_id_block_type::generate(placeholder_index::iterator begin,
            placeholder_index::iterator end)
    {
        std::vector<std::string> resolved_ids;

        for (placeholder_index::iterator i = begin; i != end; ++i)
            resolved_ids.push_back(resolve_id(*i));

        unsigned index = 0;
        for (placeholder_index::iterator i = begin; i != end; ++i, ++index)
        {
            generated_ids[(**i).index] =
                generate_id(*i, resolved_ids[index]);
        }
    }

    std::string generate_id_block_type::resolve_id(id_placeholder const* p)
    {
        std::string id = p->parent ?
            generated_ids[p->parent->index] + "." + p->id :
            p->id;

        if (p->category.c > id_category::numbered) {
            // Reserve the id if it isn't already reserved.
            chosen_id_map::iterator pos = chosen_ids.emplace(id, p).first;

            // If it was reserved by a placeholder with a lower category,
            // then overwrite it.
            if (p->category.c > pos->second->category.c)
                pos->second = p;
        }

        return id;
    }

    std::string generate_id_block_type::generate_id(id_placeholder const* p,
            std::string const& resolved_id)
    {
        if (p->category.c > id_category::numbered &&
                chosen_ids.at(resolved_id) == p)
        {
            return resolved_id;
        }

        // Split the id into its parent part and child part.
        //
        // Note: can't just use the placeholder's parent, as the
        // placeholder id might contain dots.
        unsigned child_start = resolved_id.rfind('.');
        std::string parent_id, base_id;

        if (child_start == std::string::npos) {
            base_id = normalize_id(resolved_id, max_size - 1);
        }
        else {
            parent_id = resolved_id.substr(0, child_start + 1);
            base_id = normalize_id(resolved_id.substr(child_start + 1),
                    max_size - 1);
        }

        // Since we're adding digits, don't want an id that ends in
        // a digit.

        unsigned int length = base_id.size();

        if (length > 0 && std::isdigit(base_id[length - 1])) {
            if (length < max_size - 1) {
                base_id += '_';
                ++length;
            }
            else {
                while (length > 0 && std::isdigit(base_id[length -1]))
                    --length;
                base_id.erase(length);
            }
        }

        unsigned count = 0;

        while (true)
        {
            std::string postfix =
                boost::lexical_cast<std::string>(count++);

            if ((base_id.size() + postfix.size()) > max_size) {
                // The id is now too long, so reduce the length and
                // start again.

                // Would need a lot of ids to get this far....
                if (length == 0) throw std::runtime_error("Too many ids");

                // Trim a character.
                --length;

                // Trim any trailing digits.
                while (length > 0 && std::isdigit(base_id[length -1]))
                    --length;

                base_id.erase(length);
                count = 0;
            }
            else {
                // Try to reserve this id.
                std::string generated_id = parent_id + base_id + postfix;

                if (chosen_ids.emplace(generated_id, p).second) {
                    return generated_id;
                }
            }
        }
    }

    //
    // replace_ids
    //
    // Return a copy of the xml with all the placeholders replaced by
    // generated_ids.
    //

    struct replace_ids_callback : xml_processor::callback
    {
        id_state const& state;
        std::vector<std::string> const* ids;
        boost::string_ref::const_iterator source_pos;
        std::string result;

        replace_ids_callback(id_state const& state,
                std::vector<std::string> const* ids)
          : state(state),
            ids(ids),
            source_pos(),
            result()
        {}

        void start(boost::string_ref xml)
        {
            source_pos = xml.begin();
        }

        void id_value(boost::string_ref value)
        {
            if (id_placeholder const* p = state.get_placeholder(value))
            {
                boost::string_ref id = ids ?
                    (*ids)[p->index] : p->unresolved_id;

                result.append(source_pos, value.begin());
                result.append(id.begin(), id.end());
                source_pos = value.end();
            }
        }

        void finish(boost::string_ref xml)
        {
            result.append(source_pos, xml.end());
            source_pos = xml.end();
        }
    };

    std::string replace_ids(id_state const& state, boost::string_ref xml,
            std::vector<std::string> const* ids)
    {
        xml_processor processor;
        replace_ids_callback callback(state, ids);
        processor.parse(xml, callback);
        return callback.result;
    }

    //
    // normalize_id
    //
    // Normalizes generated ids.
    //

    std::string normalize_id(boost::string_ref src_id)
    {
        return normalize_id(src_id, max_size);
    }

    std::string normalize_id(boost::string_ref src_id, std::size_t size)
    {
        std::string id(src_id.begin(), src_id.end());

        std::size_t src = 0;
        std::size_t dst = 0;

        while (src < id.length() && id[src] == '_') {
            ++src;
        }

        if (src == id.length()) {
            id = "_";
        }
        else {
            while (src < id.length() && dst < size) {
                if (id[src] == '_') {
                    do {
                        ++src;
                    } while(src < id.length() && id[src] == '_');

                    if (src < id.length()) id[dst++] = '_';
                }
                else {
                    id[dst++] = id[src++];
                }
            }

            id.erase(dst);
        }

        return id;
    }
}
