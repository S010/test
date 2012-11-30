#include <iostream>
#include <string>
#include <vector>
#include <sstream>

class lisp {
    public:
        enum types {
            LISP,
            ATOM,
        };

        lisp() :
            m_type(LISP)
        {
            std::clog << "lisp()" << std::endl;
        }

        lisp(const std::string &s) :
            m_type(ATOM),
            m_s(s)
        {
            std::clog << "lisp(const std::string &s)" << std::endl;
        }

        lisp(const std::vector<lisp> &v) :
            m_type(LISP),
            m_v(v)
        {
            std::clog << "lisp(const std::vector<lisp> &v)" << std::endl;
        }

        lisp(const std::vector<lisp>::const_iterator first, const std::vector<lisp>::const_iterator last) :
            m_type(LISP),
            m_v(first, last)
        {
            std::clog << "lisp(const std::vector<lisp>::const_iterator first, const std::vector<lisp>::const_iterator last)" << std::endl;
        }

        template<typename ... Types> lisp(const lisp &l, Types ... args) :
            m_type(LISP)
        {
            std::clog << "template<typename ... Types> lisp(const lisp &l, Types ... args)" << std::endl;
            vctor(l, args...);
        }

        template<typename ... Types> lisp(const std::string &s, Types ... args) :
            m_type(LISP)
        {
            std::clog << "template<typename ... Types> lisp(const std::string &s, Types ... args)" << std::endl;
            vctor(s, args...);
        }

        bool operator==(const std::string &rhs) const {
            return m_type == ATOM && m_s == rhs;
        }

        bool operator==(const lisp &rhs) const {
            if (m_type != rhs.m_type)
                return false;
            if (m_type == ATOM)
                return m_s == rhs.m_s;
            else
                return m_v == rhs.m_v;
        }

        lisp eval() const {
            if (m_type == ATOM)
                return *this;
            else if (m_v.size() > 0) {
                if (m_v[0] == "quote") {
                    return { m_v.begin() + 1, m_v.end() };
                } else if (m_v[0] == "atom") {
                    if (m_v[1].eval().m_type == ATOM) {
                        lisp l;
                        l.m_type = ATOM;
                        l.m_s = "t";
                        return l;
                    } else
                        return {}; // empty list
                } else if (m_v[0] == "car") {
                    lisp v1(m_v[1].eval());
                    if (v1.m_type == LISP) {
                        if (v1.m_v.size() > 0)
                            return v1.m_v[0];
                        else
                            return lisp();
                    } else
                        return lisp();
                } else if (m_v[0] == "cdr") {
                    lisp v1(m_v[1].eval());
                    if (v1.m_type == LISP) {
                        if (v1.m_v.size() > 0) {
                            v1.m_v.erase(v1.m_v.begin());
                            return v1;
                        } else
                            return lisp();
                    } else
                        return lisp();
                } else if (m_v[0] == "eq") {
                    lisp a(m_v[1].eval());
                    lisp b(m_v[2].eval());
                    if (a == b)
                        return lisp("t");
                    else
                        return lisp();
                } else if (m_v[0] == "cons") {
                    if (m_v.size() > 1) {
                        lisp a(m_v[1].eval());
                        lisp b(m_v[2].eval());
                        if (a.m_type == ATOM && b.m_type == LISP) {
                            b.m_v.insert(b.m_v.begin(), a);
                            return b;
                        } else
                            return lisp();
                    } else {
                        return lisp();
                    }
                } else
                    return *this;
            } else
                return {};
        }

        std::string str() const {
            std::ostringstream os;

            if (m_type == ATOM)
                os << m_s;
            else {
                os << '(';
                for (auto i = m_v.begin(); i != m_v.end(); ++i) {
                    if (i != m_v.begin())
                        os << ", ";
                    os << i->str();
                }
                os << ')';
            }

            return os.str();
        }

    private:
        enum types m_type;
        std::vector<lisp> m_v;
        std::string m_s;

        template<typename ... Types> void vctor(const std::string &s, Types ... args) {
            lisp l;
            l.m_type = ATOM;
            l.m_s = s;
            m_v.push_back(l);
            vctor(args...);
        }

        template<typename ... Types> void vctor(const lisp &l, Types ... args) {
            m_v.push_back(l);
            vctor(args...);
        }

        void vctor() {
        }
};

void test(const lisp &l) {
    std::cout << "expression: " << l.str() << std::endl;
    std::cout << "evaluation: " << l.eval().str() << std::endl;
}

int main(int argc, char **argv) {
    test({ "atom", lisp { "aaa", "bbb", "ccc" } });
    test({ "eq", "aaa", "bbb" });
    test({ "eq", "aaa", "aaa" });
    test({ "eq", "aaa", lisp { "quote", lisp { "aaa", "bbb" } } });
    test({ "aaa", "bbb", "ccc" });
    test({ "cdr", lisp { "aaa", "bbb", "ccc" } });
    test({ "cons", "aaa", lisp { "123", "xxx", "ttt" } });

    return 0;
}
