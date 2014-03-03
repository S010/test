#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <iterator>
#include <algorithm>

// Allows to retrieve information about a process.
// Linux-only, relies on procfs.
class process {
    public:
        // Return a list of executing processes.
        static std::vector<process> list() {
            std::vector<process> procs;
            DIR*                 dp;
            struct dirent*       de;

            dp = opendir("/proc");
            if (dp == NULL)
                return procs;
            while ((de = readdir(dp)) != NULL) {
                if (de->d_type != DT_DIR)
                    continue;
                if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, ".."))
                    continue;
                if (std::all_of(de->d_name, de->d_name + strlen(de->d_name), isdigit))
                    procs.push_back((pid_t) std::stoul(de->d_name));
            }
            return procs;
        }

        // Constructor.
        process(pid_t pid) :
            _pid(pid)
        {
            std::ostringstream path;
            path << "/proc/" << (unsigned long) _pid << "/";
            _proc_dir = path.str();
        }

        // Return the process ID.
        pid_t pid() const {
            return _pid;
        }

        // Return the absolute path to the process' executable file.
        std::string exe() const {
            const std::string path(_proc_dir + "exe");
            std::vector<char> buf;

            auto n = 512;
            do {
                buf.resize(n * 2);
                readlink(path.c_str(), buf.data(), buf.size());
            } while (n == buf.size());

            return { buf.data(), buf.size() };
        }

        // Return the process' resident set size (RSS) measured in bytes.
        // RSS is basically how much of physical system memory does the process
        // currently use.
        size_t rss() const {
            // Extract the second field from /proc/<pid>/statm file.
            // This field contains the value which is equal to the number of "shared"
            // (i.e. those which map a file) memory pages resident in memory
            // plus the number of anonymous memory pages resident in memory.
            std::ifstream f((_proc_dir + "statm").c_str());
            if (!f)
                return 0;
            std::string line;
            std::getline(f, line);
            std::string::size_type pos, endpos;
            pos = line.find(' ');
            if (pos++ == std::string::npos)
                return 0;
            endpos = line.find(' ', pos);

            return (size_t) std::stoul(line.substr(pos, endpos - pos)) * _page_size();
        }

        // Return the parent process ID.
        pid_t ppid() const {
            std::ifstream f((_proc_dir + "stat").c_str());
            if (!f)
                return 0;
            std::string line;
            std::getline(f, line);
            std::string::size_type pos, endpos;
            pos = line.find_first_not_of("0123456789");
            pos = line.find_first_of("0123456789", pos);
            if (pos == std::string::npos)
                return -1;
            endpos = line.find(' ', pos);
            return (pid_t) std::stoul(line.substr(pos, endpos - pos));
        }

        // Return a list of child processes.
        std::vector<process> children() const {
            std::vector<process> procs(list());
            procs.erase(
                std::remove_if(
                    std::begin(procs),
                    std::end(procs),
                    [this](const process& p) {
                        return this->pid() != p.ppid();
                    }
                ),
                end(procs)
            );
            return procs;
        }

        // Return the command line of the process.
        std::vector<std::string> cmdline() const {
            int fd;
            fd = open((_proc_dir + "cmdline").c_str(), O_RDONLY);
            if (fd == -1)
                return {};

            std::vector<std::string> cmdl;
            std::vector<char> buf;

            ssize_t n;
            do {
                buf.resize(std::max((size_t) 4096, 2 * buf.size()));
            } while ((n = read(fd, buf.data(), buf.size())) == buf.size());
            close(fd);
            if (n < 0)
                return {};

            auto p = buf.data();
            const auto endp = buf.data() + buf.size();
            while (p != nullptr && p < endp) {
                cmdl.push_back(p);
                p = (decltype(buf.data())) memchr(p, '\0', endp - p) + 1;
            }

            return cmdl;
        }

        // Return the program name from cmdline.
        std::string prog_name() const {
            auto cmdl = cmdline();
            if (cmdl.size() > 0)
                return cmdl[0];
            else
                return {};
        }

    private:
        pid_t               _pid;
        std::string         _proc_dir;
        mutable std::string _exe;

        size_t _page_size() const {
            static size_t psize = 0;

            if (psize == 0) {
                auto val = sysconf(_SC_PAGE_SIZE);
                if (val == -1) {
                    // assume the most common value
                    psize = 4096;
                } else {
                    psize = (size_t) val;
                }
            }

            return psize;
        }
};

int main(int argc, char** argv) {
    for (auto& i : process::list())
        std::cout << i.pid() << ' ' << i.prog_name() << ' ' << i.rss() << std::endl;
    return 0;
}
