#ifndef __CLUSTERING_ADMINISTRATION_ISSUES_GLOBAL_HPP__
#define __CLUSTERING_ADMINISTRATION_ISSUES_GLOBAL_HPP__

class global_issue_t {
public:
    virtual std::string get_description() const = 0;

    virtual global_issue_t *clone() const = 0;
};

#endif /* __CLUSTERING_ADMINISTRATION_ISSUES_GLOBAL_HPP__ */
