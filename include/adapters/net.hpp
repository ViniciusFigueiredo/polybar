#pragma once

#include <chrono>
#include <cstdlib>

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <iwlib.h>

#ifdef inline
#undef inline
#endif

#include "common.hpp"
#include "settings.hpp"
#include "errors.hpp"
#include "components/logger.hpp"
#include "utils/math.hpp"

POLYBAR_NS

class file_descriptor;

namespace net {
  DEFINE_ERROR(network_error);

  bool is_wireless_interface(const string& ifname);

  // types {{{

  struct quality_range {
    int val{0};
    int max{0};

    int percentage() const {
      if (val < 0) {
        return std::max(std::min(std::abs(math_util::percentage(val, max, -20)), 100), 0);
      }
      return std::max(std::min(math_util::percentage(val, 0, max), 100), 0);
    }
  };

  using bytes_t = unsigned int;

  struct link_activity {
    bytes_t transmitted{0};
    bytes_t received{0};
    std::chrono::system_clock::time_point time;
  };

  struct link_status {
    string ip;
    string ip6;
    link_activity previous{};
    link_activity current{};
  };

  // }}}
  // class : network {{{

  class network {
   public:
    explicit network(string interface);
    virtual ~network() {}

    virtual bool query(bool accumulate = false);
    virtual bool connected() const = 0;
    virtual bool ping() const;

    string ip() const;
    string ip6() const;
    string downspeed(int minwidth = 3) const;
    string upspeed(int minwidth = 3) const;
    void set_unknown_up(bool unknown = true);

   protected:
    void check_tuntap();
    bool test_interface() const;
    string format_speedrate(float bytes_diff, int minwidth) const;
    void query_ip6();

    const logger& m_log;
    unique_ptr<file_descriptor> m_socketfd;
    link_status m_status{};
    string m_interface;
    bool m_tuntap{false};
    bool m_unknown_up{false};
  };

  // }}}
  // class : wired_network {{{

  class wired_network : public network {
   public:
    explicit wired_network(string interface) : network(interface) {}

    bool query(bool accumulate = false) override;
    bool connected() const override;
    string linkspeed() const;

   private:
    int m_linkspeed{0};
  };

  // }}}
  // class : wireless_network {{{

  class wireless_network : public network {
   public:
    wireless_network(string interface) : network(interface) {}

    bool query(bool accumulate = false) override;
    bool connected() const override;

    string essid() const;
    int signal() const;
    int quality() const;

   protected:
    void query_essid(const int& socket_fd);
    void query_quality(const int& socket_fd);

   private:
    shared_ptr<wireless_info> m_info{};
    string m_essid{};
    quality_range m_signalstrength{};
    quality_range m_linkquality{};
  };

  // }}}

  using wireless_t = unique_ptr<wireless_network>;
  using wired_t = unique_ptr<wired_network>;
}

POLYBAR_NS_END
