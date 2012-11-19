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
            std::cerr << "lisp()" << std::endl;
        }

        lisp(const std::vector<lisp> &v) :
            m_type(LISP),
            m_v(v)
        {
            std::cerr << "lisp(const std::vector<lisp> &v)" << std::endl;
        }

        lisp(const std::vector<lisp>::const_iterator first, const std::vector<lisp>::const_iterator last) :
            m_type(LISP),
            m_v(first, last)
        {
            std::cerr << "lisp(const std::vector<lisp>::const_iterator first, const std::vector<lisp>::const_iterator last)" << std::endl;
        }

        template<typename ... Types> lisp(const lisp &l, Types ... args) :
            m_type(LISP)
        {
            std::cerr << "template<typename ... Types> lisp(const lisp &l, Types ... args)" << std::endl;
            vctor(l, args...);
        }

        template<typename ... Types> lisp(const std::string &s, Types ... args) :
            m_type(LISP)
        {
            std::cerr << "template<typename ... Types> lisp(const std::string &s, Types ... args)" << std::endl;
            vctor(s, args...);
        }

        bool operator== (const std::string &rhs) {
            return m_type == ATOM && m_s == rhs;
        }

        lisp eval() {
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
                } else
                    return *this;
            } else
                return {};
        }

        std::string str() {
            std::ostringstream os;

            if (m_type == ATOM)
                os << '"' << m_s << '"';
            else {
                os << '(';
                for (auto &i : m_v) {
                    os << i.str();
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

void test() {
    lisp l { "atom", lisp { "aaa", "bbb", "ccc" } };
    std::cout << "expression:" << std::endl;
    std::cout << l.str() << std::endl;
    std::cout << "evaluation:" << std::endl;
    std::cout << l.eval().str() << std::endl;
    //std::cout << l.str() << std::endl;
}

int main(int argc, char **argv) {
    test();
    return 0;
}
