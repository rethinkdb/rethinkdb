#include "perfmon.hpp"
#include "arch/arch.hpp"
#include "utils.hpp"
#include <stdarg.h>

/* The var list keeps track of all of the perfmon_watcher_t objects on each core. */

__thread intrusive_list_t<perfmon_watcher_t> *var_list = NULL;

perfmon_watcher_t::perfmon_watcher_t(const char *n, perfmon_combiner_t *combiner,
                                     perfmon_transformer_t *transformer)
    : combiner(combiner), transformer(transformer)
{
    if (n) set_name("%s", n);
    
    if (!var_list) var_list = gnew<intrusive_list_t<perfmon_watcher_t> >();
    var_list->push_back(this);
}

void perfmon_watcher_t::set_name(const char *fmt, ...) {
    
    va_list args;
    va_start(args, fmt);
    vsnprintf(name, sizeof(name), fmt, args);
    va_end(args);
}

perfmon_watcher_t::~perfmon_watcher_t() {
    
    var_list->remove(this);
    if (var_list->empty()) {
        gdelete(var_list);
        var_list = NULL;
    }
}

/* The perfmon_fsm_t goes to all the cores, collects stats for each one, and then goes back. */

struct perfmon_fsm_t :
    public home_cpu_mixin_t,
    public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, perfmon_fsm_t>
{
    // The map to which we output data.
    perfmon_stats_t *dest;

    // A temporary, local map, to which we output untransformed data.
    perfmon_stats_t vars;
    
    std::map<std_string_t, perfmon_transformer_t *, std::less<std_string_t>,
        gnew_alloc<std::pair<std_string_t, perfmon_transformer_t *> > > transformers;

    perfmon_callback_t *callback;
    
    perfmon_fsm_t(perfmon_stats_t *dest) : dest(dest) { }
    ~perfmon_fsm_t() { }
    
    bool run(perfmon_callback_t *cb) {
        callback = NULL;
        if (do_on_cpu(0, this, &perfmon_fsm_t::gather_data)) {
            return true;
        } else {
            callback = cb;
            return false;
        }
    }
    
    bool gather_data() {
        if (var_list) {
            for (perfmon_watcher_t *var = var_list->head(); var; var = var_list->next(var)) {
                assert(var->name[0]);
                const std_string_t &value = var->get_value();
                if (vars.find(var->name) == vars.end()) {
                    vars[var->name] = value;
                    transformers[var->name] = var->transformer;
                } else {
                    if (var->combiner) {
                        vars[var->name] = var->combiner(value, vars[var->name]);
                    } else {
                        fail("Two perfmon watchers are both called %s and one lacks a combiner "
                            "function.", var->name);
                    }
                }
            }
        }
        if (get_cpu_id() == get_num_cpus() - 1) {
            /* Ok, we're on the last cpu now, all data has been
             * gathered, we can deliver it. */
            return do_on_cpu(home_cpu, this, &perfmon_fsm_t::deliver_data);
        } else {
            /* We're not on the last cpu yet, gather data from the
             * next cpu. */
            return do_on_cpu(get_cpu_id()+1, this, &perfmon_fsm_t::gather_data);
        }
    }
    
    bool deliver_data() {
        
        for (perfmon_stats_t::iterator it = vars.begin(); it != vars.end(); it++) {
            const std_string_t &name = it->first;
            perfmon_transformer_t *trans = transformers[name];
            (*dest)[name] = trans ? trans(vars[name]) : vars[name];
        }
        
        if (callback) callback->on_perfmon_stats();
        delete this;
        return true;
    }

private:
    DISABLE_COPYING(perfmon_fsm_t);
};

typedef std::basic_stringstream<char, std::char_traits<char>, gnew_alloc<char> > sstream;

bool perfmon_get_stats(perfmon_stats_t *dest, perfmon_callback_t *cb) {
    
    perfmon_fsm_t *fsm = new perfmon_fsm_t(dest);
    return fsm->run(cb);
}

// Computes (unwords $ map show $ zipWith (+) (map read $ words v1) (map read $ words v2))
std_string_t perfmon_combiner_sum(std_string_t v1, std_string_t v2) {
    sstream s1(v1), s2(v2), out;
    
    bool first = true;
    int64_t i1, i2;
    while ((s1 >> i1) && (s2 >> i2)) {
        if (!first) {
            out << ' ';
        }
        first = false;
        out << (i1 + i2);
    }

    return out.str();
}

static void read_average(const char *s, long long int *num, int *count) {
    switch (sscanf(s, "%lld (average of %d)", num, count)) {
        case 0: fail("Bad input to perfmon_average_combiner: %s", s);
        case 1: *count = 1; break;
        case 2: break;
        default: fail("But there were only two format specifiers in the string!");
    }
}

std_string_t perfmon_combiner_average(std_string_t v1, std_string_t v2) {
    
    long long int num1, num2;
    int count1, count2;
    read_average(v1.c_str(), &num1, &count1);
    read_average(v2.c_str(), &num2, &count2);
    
    char buf[20];
    sprintf(buf, "%lld (average of %d)", (num1*count1+num2*count2)/(count1+count2), count1+count2);
    return buf;
}

// Takes "3 4", for example, and returns "0.75 (3/4)".  Or takes "0 0"
// and returns "0 (0/0)".
std_string_t perfmon_weighted_average_transformer(std_string_t numer_denom_pair) {
    int64_t numer, denom;
    sstream(numer_denom_pair) >> numer >> denom;
    sstream out;
    if (denom == 0) {
        out << "0";
    } else {
        out << double(numer)/double(denom);
    }

    out << " (" << numer << "/" << denom << ")";
    return out.str();
}
