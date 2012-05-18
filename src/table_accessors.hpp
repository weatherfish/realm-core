/*************************************************************************
 *
 * TIGHTDB CONFIDENTIAL
 * __________________
 *
 *  [2011] - [2012] TightDB Inc
 *  All Rights Reserved.
 *
 * NOTICE:  All information contained herein is, and remains
 * the property of TightDB Incorporated and its suppliers,
 * if any.  The intellectual and technical concepts contained
 * herein are proprietary to TightDB Incorporated
 * and its suppliers and may be covered by U.S. and Foreign Patents,
 * patents in process, and are protected by trade secret or copyright law.
 * Dissemination of this information or reproduction of this material
 * is strictly forbidden unless prior written permission is obtained
 * from TightDB Incorporated.
 *
 **************************************************************************/
#ifndef TIGHTDB_TABLE_ACCESSORS_HPP
#define TIGHTDB_TABLE_ACCESSORS_HPP

#include <utility>

#include "mixed.hpp"

namespace tightdb {


/**
 * A convenience base class for Spec classes that are to be used with
 * BasicTable.
 *
 * There are two reasons why you might want to derive your spec class
 * from this one. First, it offers short hand names for each of the
 * available column types. Second, it makes it easier when you do not
 * want to specify colum names or convenience methods, since suitable
 * fallbacks are defined here.
 */
struct SpecBase {
    typedef int64_t         Int;
    typedef bool            Bool;
    typedef const char*     String;
    typedef std::time_t     Date;
//    typedef tightdb::Binary Binary; // FIXME: Use tightdb::BinaryData here?
    typedef tightdb::Mixed  Mixed;
    template<class E> class Enum {
    public:
        Enum(E v) : m_value(v) {};
        operator E() const { return m_value; }
    private:
        E m_value;
    };

    /**
     * By default, there are no static column names defined for a
     * BasicTable. One may define a set of column mames as follows:
     *
     * <pre>
     *
     *   struct MyTableSpec {
     *     typedef tightdb::TypeAppend<void, int>::type Columns1;
     *     typedef tightdb::TypeAppend<Columns1, bool>::type Columns;
     *
     *     template<template<int> class Col, class Init> struct ColNames {
     *       typename Col<0>::type foo;
     *       typename Col<1>::type bar;
     *       ColNames(Init i): foo(i), bar(i) {}
     *     };
     *   };
     *
     * </pre>
     *
     * Note that 'i' in Col<i> links the name that you specify to a
     * particular column index. You may specify the column names in
     * any order. Multiple names may refer to the same column, and you
     * do not have to specify a name for every column.
     */
    template<template<int> class Col, class Init> struct ColNames { ColNames(Init) {} };

    /**
     *
     */
    static const char* const* dyn_col_names() { return 0; }

    /**
     * This is the fallback class that is used when no convenience
     * methods are specified in the users Spec class.
     *
     * If you would like to add a convenient add() method, here is how
     * you could do it:
     */
    struct ConvenienceMethods {
/*
        void add(int foo, bool bar)
        {
            BasicTable<SpecBase>* t = static_cast<BasicTable<SpecBase>*>(this);
            t->add((::tightdb::tuple(), foo, bar));
        }
*/
    };
};


template<class T> class BasicTableView;


namespace _impl {


/**
 * If T is BasicTableView<T2>, then return T2, else simply return T.
 */
template<class T> struct GetTableFromView { typedef T type; };
template<class T> struct GetTableFromView<BasicTableView<T> > { typedef T type; };
template<class T> struct GetTableFromView<const BasicTableView<T> > { typedef T type; };




/**
 * This class gives access to a field of a row of a table.
 *
 * \tparam Tab Either a table or a table view. Constness of access is
 * controlled by what is allowed to be done with/on a 'Tab*'.
 */
template<class Tab, int col_idx, class Type> class FieldAccessor;


/**
 * Commmon base class for all field accessor specializations.
 */
template<class Tab> class FieldAccessorBase {
protected:
    typedef std::pair<Tab*, std::size_t> Init;
    Tab* const m_table;
    const std::size_t m_row_idx;
    FieldAccessorBase(Init i): m_table(i.first), m_row_idx(i.second) {}
};


/**
 * Field accessor specialization for integers.
 */
template<class Tab, int col_idx>
class FieldAccessor<Tab, col_idx, int64_t>: public FieldAccessorBase<Tab> {
private:
    typedef FieldAccessorBase<Tab> Base;

public:
    explicit FieldAccessor(typename Base::Init i): Base(i) {}

    operator int64_t() const
    {
        return Base::m_table->get_impl()->get_int(col_idx, Base::m_row_idx);
    }

    const FieldAccessor& operator=(int64_t value) const
    {
        Base::m_table->get_impl()->set_int(col_idx, Base::m_row_idx, value);
        return *this;
    }

    const FieldAccessor& operator+=(int64_t value) const
    {
        // FIXME: Should be optimized (can be both optimized and
        // generalized by using a form of expression templates).
        value = Base::m_table->get_impl()->get_int(col_idx, Base::m_row_idx) + value;
        Base::m_table->get_impl()->set_int(col_idx, Base::m_row_idx, value);
        return *this;
    }
};


/**
 * Field accessor specialization for booleans.
 */
template<class Tab, int col_idx>
class FieldAccessor<Tab, col_idx, bool>: public FieldAccessorBase<Tab> {
private:
    typedef FieldAccessorBase<Tab> Base;

public:
    explicit FieldAccessor(typename Base::Init i): Base(i) {}

    operator bool() const
    {
        return Base::m_table->get_impl()->get_bool(col_idx, Base::m_row_idx);
    }

    const FieldAccessor& operator=(bool value) const
    {
        Base::m_table->get_impl()->set_bool(col_idx, Base::m_row_idx, value);
        return *this;
    }
};


/**
 * Field accessor specialization for enumerations.
 */
template<class Tab, int col_idx, class E>
class FieldAccessor<Tab, col_idx, SpecBase::Enum<E> >: public FieldAccessorBase<Tab> {
private:
    typedef FieldAccessorBase<Tab> Base;

public:
    explicit FieldAccessor(typename Base::Init i): Base(i) {}

    operator E() const
    {
        return static_cast<E>(Base::m_table->get_impl()->get_int(col_idx, Base::m_row_idx));
    }

    const FieldAccessor& operator=(E value) const
    {
        Base::m_table->get_impl()->set_int(col_idx, Base::m_row_idx, value);
        return *this;
    }
};


/**
 * Field accessor specialization for strings.
 */
template<class Tab, int col_idx>
class FieldAccessor<Tab, col_idx, const char*>: public FieldAccessorBase<Tab> {
private:
    typedef FieldAccessorBase<Tab> Base;

public:
    explicit FieldAccessor(typename Base::Init i): Base(i) {}

    operator const char*() const
    {
        return Base::m_table->get_impl()->get_string(col_idx, Base::m_row_idx);
    }

    const FieldAccessor& operator=(const char* value) const
    {
        Base::m_table->get_impl()->set_string(col_idx, Base::m_row_idx, value);
        return *this;
    }

    // FIXME: Not good to define operator==() here, beacuse it does
    // not have this semantic for char pointers in general. However,
    // if we choose to keep it, we should also have all the other
    // comparison operators, and many other operators need to be
    // disabled such that e.g. 't.foo - 10' is no longer possible (it
    // is now, due to the conversion operator). A much better approach
    // would probably be to define a special tightdb::String type.
    bool operator==(const char* value) const
    {
        const char* const v = Base::m_table->get_impl()->get_string(col_idx, Base::m_row_idx);
        return std::strcmp(v, value) == 0;
    }
};


/**
 * Field accessor specialization for mixed type.
 */
template<class Tab, int col_idx>
class FieldAccessor<Tab, col_idx, Mixed>: public FieldAccessorBase<Tab> {
private:
    typedef FieldAccessorBase<Tab> Base;

public:
    explicit FieldAccessor(typename Base::Init i): Base(i) {}

    operator Mixed() const
    {
        return Base::m_table->get_impl()->get_mixed(col_idx, Base::m_row_idx);
    }

    const FieldAccessor& operator=(const Mixed& value) const
    {
        Base::m_table->get_impl()->set_mixed(col_idx, Base::m_row_idx, value);
        return *this;
    }

    ColumnType get_type() const
    {
        return Base::m_table->get_impl()->get_mixed_type(col_idx, Base::m_row_idx);
    }

    int64_t get_int() const { return Mixed(*this).get_int(); }

    bool get_bool() const { return Mixed(*this).get_bool(); }

    std::time_t get_date() const { return Mixed(*this).get_date(); }

    const char* get_string() const { return Mixed(*this).get_string(); }

    BinaryData get_binary() const { return Mixed(*this).get_binary(); }
};




/**
 * Field accessor specialization for subtables.
 */
template<class Tab, int col_idx, class Subspec>
class FieldAccessor<Tab, col_idx, BasicTable<Subspec> >: public FieldAccessorBase<Tab> {
private:
    typedef FieldAccessorBase<Tab> Base;
    typedef typename GetTableFromView<Tab>::type RealTable;
    typedef typename CopyConstness<RealTable, BasicTable<Subspec> >::type Subtab;
    struct SubtabRowAccessor: Subtab::RowAccessor {
    public:
        SubtabRowAccessor(Tab* subtab, std::size_t row_idx):
            Subtab::RowAccessor(std::make_pair(subtab, row_idx)),
            m_owner(Table::make_ref(subtab)) {}

    private:
        BasicTableRef<Tab> const m_owner;
    };

public:
    explicit FieldAccessor(typename Base::Init i): Base(i) {}

    BasicTableRef<Subtab> operator->() const
    {
        Subtab* subtab = static_cast<Subtab*>(Base::m_table->get_impl()->
                                              get_subtable_ptr(col_idx, Base::m_row_idx));
        return Table::make_ref(subtab);
    }

    SubtabRowAccessor operator[](std::size_t row_idx) const
    {
        Subtab* subtab = static_cast<Subtab*>(Base::m_table->get_impl()->
                                              get_subtable_ptr(col_idx, Base::m_row_idx));
        return RowAccessor(subtab, row_idx);
    }
};




/**
 * This class gives access to a column of a table.
 *
 * \tparam Tab Either a table or a table view. Constness of access is
 * controlled by what is allowed to be done with/on a 'Tab*'.
 */
template<class Tab, int col_idx, class Type> class ColumnAccessor;


/**
 * Commmon base class for all column accessor specializations.
 */
template<class Tab, int col_idx, class Type> class ColumnAccessorBase {
protected:
    typedef typename GetTableFromView<Tab>::type RealTable;

public:
    FieldAccessor<Tab, col_idx, Type> operator[](std::size_t row_idx) const
    {
        return FieldAccessor<Tab, col_idx, Type>(std::make_pair(m_table, row_idx));
    }

    bool has_index() const { return m_table->get_impl()->has_index(col_idx); }
    void set_index() const { m_table->get_impl()->set_index(col_idx); }

    BasicTableView<RealTable> sorted(bool ascending=true) const
    {
        return m_table->get_impl()->sorted(col_idx, ascending);
    }

    void sort(bool ascending = true) const { m_table->get_impl()->sort(col_idx, ascending); }

protected:
    Tab* const m_table;

    explicit ColumnAccessorBase(Tab* t): m_table(t) {}
};


/**
 * Column accessor specialization for integers.
 */
template<class Tab, int col_idx>
class ColumnAccessor<Tab, col_idx, int64_t>: public ColumnAccessorBase<Tab, col_idx, int64_t> {
private:
    typedef ColumnAccessorBase<Tab, col_idx, int64_t> Base;

public:
    explicit ColumnAccessor(Tab* t): Base(t) {}

    std::size_t find_first(int64_t value) const
    {
        return Base::m_table->get_impl()->find_first_int(col_idx, value);
    }

    std::size_t find_pos(int64_t value) const
    {
        return Base::m_table->get_impl()->find_pos_int(col_idx, value);
    }

    BasicTableView<typename Base::RealTable> find_all(int64_t value) const
    {
        return Base::m_table->get_impl()->find_all_int(col_idx, value);
    }

    int64_t sum() const
    {
        return Base::m_table->get_impl()->sum(col_idx);
    }

    int64_t maximum() const
    {
        return Base::m_table->get_impl()->maximum(col_idx);
    }

    int64_t minimum() const
    {
        return Base::m_table->get_impl()->minimum(col_idx);
    }

    const ColumnAccessor& operator+=(int64_t value) const
    {
        Base::m_table->get_impl()->add_int(col_idx, value);
        return *this;
    }

    void _insert(std::size_t row_idx, int64_t value) const // FIXME: Should not be public (maybe send specialized columns accessor to Spec::insert(), then in Spec::insert() do 'op(cols.name1, v1)')
    {
        Base::m_table->get_impl()->insert_int(col_idx, row_idx, value);
    }
};


/**
 * Column accessor specialization for booleans.
 */
template<class Tab, int col_idx>
class ColumnAccessor<Tab, col_idx, bool>: public ColumnAccessorBase<Tab, col_idx, bool> {
private:
    typedef ColumnAccessorBase<Tab, col_idx, bool> Base;

public:
    explicit ColumnAccessor(Tab* t): Base(t) {}

    std::size_t find_first(bool value) const
    {
        return Base::m_table->get_impl()->find_first_bool(col_idx, value);
    }

    BasicTableView<typename Base::RealTable> find_all(bool value) const
    {
        return Base::m_table->get_impl()->find_all_bool(col_idx, value);
    }

    void _insert(std::size_t row_idx, bool value) const // FIXME: Should not be public (maybe send specialized columns accessor to Spec::insert(), then in Spec::insert() do 'op(cols.name1, v1)')
    {
        Base::m_table->get_impl()->insert_bool(col_idx, row_idx, value);
    }
};


/**
 * Column accessor specialization for enumerations.
 */
template<class Tab, int col_idx, class E>
class ColumnAccessor<Tab, col_idx, SpecBase::Enum<E> >:
    public ColumnAccessorBase<Tab, col_idx, SpecBase::Enum<E> > {
private:
    typedef ColumnAccessorBase<Tab, col_idx, SpecBase::Enum<E> > Base;

public:
    explicit ColumnAccessor(Tab* t): Base(t) {}

    std::size_t find_first(E value) const
    {
        return Base::m_table->get_impl()->find_first_int(col_idx, int64_t(value));
    }

    BasicTableView<typename Base::RealTable> find_all(E value) const
    {
        return Base::m_table->get_impl()->find_all_int(col_idx, int64_t(value));
    }

    void _insert(std::size_t row_idx, E value) const // FIXME: Should not be public (maybe send specialized columns accessor to Spec::insert(), then in Spec::insert() do 'op(cols.name1, v1)')
    {
        Base::m_table->get_impl()->insert_enum(col_idx, row_idx, value);
    }
};


/**
 * Column accessor specialization for strings.
 */
template<class Tab, int col_idx>
class ColumnAccessor<Tab, col_idx, const char*>:
    public ColumnAccessorBase<Tab, col_idx, const char*> {
private:
    typedef ColumnAccessorBase<Tab, col_idx, const char*> Base;

public:
    explicit ColumnAccessor(Tab* t): Base(t) {}

    std::size_t find_first(const char* value) const
    {
        return Base::m_table->get_impl()->find_first_string(col_idx, value);
    }

    BasicTableView<typename Base::RealTable> find_all(const char* value) const
    {
        return Base::m_table->get_impl()->find_all_string(col_idx, value);
    }

    void _insert(std::size_t row_idx, const char* value) const // FIXME: Should not be public (maybe send specialized columns accessor to Spec::insert(), then in Spec::insert() do 'op(cols.name1, v1)')
    {
        Base::m_table->get_impl()->insert_string(col_idx, row_idx, value);
    }
};


/**
 * Column accessor specialization for mixed type.
 */
template<class Tab, int col_idx>
class ColumnAccessor<Tab, col_idx, Mixed>: public ColumnAccessorBase<Tab, col_idx, Mixed> {
private:
    typedef ColumnAccessorBase<Tab, col_idx, Mixed> Base;

public:
    explicit ColumnAccessor(Tab* t): Base(t) {}

    void _insert(std::size_t row_idx, const Mixed& value) const // FIXME: Should not be public (maybe send specialized columns accessor to Spec::insert(), then in Spec::insert() do 'op(cols.name1, v1)')
    {
        Base::m_table->get_impl()->insert_mixed(col_idx, row_idx, value);
    }
};


/**
 * Column accessor specialization for subtables.
 */
template<class Tab, int col_idx, class Subspec>
class ColumnAccessor<Tab, col_idx, BasicTable<Subspec> >:
    public ColumnAccessorBase<Tab, col_idx, BasicTable<Subspec> > {
private:
    typedef ColumnAccessorBase<Tab, col_idx, BasicTable<Subspec> > Base;

public:
    explicit ColumnAccessor(Tab* t): Base(t) {}
};




/**
 * This class implements a column of a table as used in a table query.
 */
template<class Tab, int col_idx, class Type> class QueryColumn;


/**
 * Commmon base class for all query column specializations.
 */
template<class Tab, int col_idx, class Type> class QueryColumnBase {
protected:
    typedef typename Tab::Query Query;
    Query* const m_query;
    explicit QueryColumnBase(Query* q): m_query(q) {}

    Query& equal(const Type& value) const
    {
        m_query->m_impl.equal(col_idx, value);
        return *m_query;
    }

    Query& not_equal(const Type& value) const
    {
        m_query->m_impl.not_equal(col_idx, value);
        return *m_query;
    }
};


/**
 * QueryColumn specialization for integers.
 */
template<class Tab, int col_idx>
class QueryColumn<Tab, col_idx, int64_t>: public QueryColumnBase<Tab, col_idx, int64_t> {
private:
    typedef QueryColumnBase<Tab, col_idx, int64_t> Base;
    typedef typename Tab::Query Query;

public:
    explicit QueryColumn(Query* q): Base(q) {}
    using Base::equal;
    using Base::not_equal;

    Query& greater(int64_t value) const
    {
        Base::m_query->m_impl.greater(col_idx, value);
        return *Base::m_query;
    }

    Query& greater_equal(int64_t value) const
    {
        Base::m_query->m_impl.greater_equal(col_idx, value);
        return *Base::m_query;
    }

    Query& less(int64_t value) const
    {
        Base::m_query->m_impl.less(col_idx, value);
        return *Base::m_query;
    }

    Query& less_equal(int64_t value) const
    {
        Base::m_query->m_impl.less_equal(col_idx, value);
        return *Base::m_query;
    }

    Query& between(int64_t from, int64_t to) const
    {
        Base::m_query->m_impl.between(col_idx, from, to);
        return *Base::m_query;
    };
};


/**
 * QueryColumn specialization for booleans.
 */
template<class Tab, int col_idx>
class QueryColumn<Tab, col_idx, bool>: public QueryColumnBase<Tab, col_idx, bool> {
private:
    typedef QueryColumnBase<Tab, col_idx, bool> Base;
    typedef typename Tab::Query Query;

public:
    explicit QueryColumn(Query* q): Base(q) {}
    using Base::equal;
    using Base::not_equal;
};


/**
 * QueryColumn specialization for enumerations.
 */
template<class Tab, int col_idx, class E>
class QueryColumn<Tab, col_idx, SpecBase::Enum<E> >:
    public QueryColumnBase<Tab, col_idx, SpecBase::Enum<E> > {
private:
    typedef QueryColumnBase<Tab, col_idx, SpecBase::Enum<E> > Base;
    typedef typename Tab::Query Query;

public:
    explicit QueryColumn(Query* q): Base(q) {}
    using Base::equal;
    using Base::not_equal;
};


/**
 * QueryColumn specialization for strings.
 */
template<class Tab, int col_idx>
class QueryColumn<Tab, col_idx, const char*>: public QueryColumnBase<Tab, col_idx, const char*> {
private:
    typedef QueryColumnBase<Tab, col_idx, const char*> Base;
    typedef typename Tab::Query Query;

public:
    explicit QueryColumn(Query* q): Base(q) {}

    Query& equal(const char* value, bool case_sensitive=true) const
    {
        Base::m_query->m_impl.equal(col_idx, value, case_sensitive);
        return *Base::m_query;
    }

    Query& not_equal(const char* value, bool case_sensitive=true) const
    {
        Base::m_query->m_impl.not_equal(col_idx, value, case_sensitive);
        return *Base::m_query;
    }

    Query& begins_with(const char* value, bool case_sensitive=true) const
    {
        Base::m_query->m_impl.begins_with(col_idx, value, case_sensitive);
        return *Base::m_query;
    }

    Query& ends_with(const char* value, bool case_sensitive=true) const
    {
        Base::m_query->m_impl.ends_with(col_idx, value, case_sensitive);
        return *Base::m_query;
    }

    Query& contains(const char* value, bool case_sensitive=true) const
    {
        Base::m_query->m_impl.contains(col_idx, value, case_sensitive);
        return *Base::m_query;
    }
};


/**
 * QueryColumn specialization for mixed type.
 */
template<class Tab, int col_idx> class QueryColumn<Tab, col_idx, Mixed> {
private:
    typedef typename Tab::Query Query;

public:
    explicit QueryColumn(Query*) {}
};


/**
 * QueryColumn specialization for subtables.
 */
template<class Tab, int col_idx, class Subspec>
class QueryColumn<Tab, col_idx, BasicTable<Subspec> >:
    public QueryColumnBase<Tab, col_idx, BasicTable<Subspec> > {
private:
    typedef QueryColumnBase<Tab, col_idx, const char*> Base;
    typedef typename Tab::Query Query;

public:
    explicit QueryColumn(Query* q): Base(q) {}

    Query& subtable()
    {
        Base::m_query->m_impl.subtable(col_idx);
        return *Base::m_query;
    }
};


} // namespace _impl
} // namespaced tightdb

#endif // TIGHTDB_TABLE_ACCESSORS_HPP
