// Copyright (c) 2016 Sviatoslav Chagaev <sviatoslav.chagaev@gmail.com>
//
// Permission to use, copy, modify, and distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
// WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
// ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
// WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
// ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
// OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <utility>
#include <cstdint>
#include <climits>
#include <cstring>
#include <cstdio>
#include <cstddef>

namespace dns {

typedef std::vector<uint8_t> ByteVector;

enum class RRType : uint16_t {
  A = 1,
  NS = 2,
  MD = 3,
  MF = 4,
  CNAME = 5,
  SOA = 6,
  MB = 7,
  MG = 8,
  MR = 9,
  RR_NULL = 10,
  WKS = 11,
  PTR = 12,
  HINFO = 13,
  MINFO = 14,
  MX = 15,
  TXT = 16
};

enum class QType : uint16_t {
  A = 1,
  NS = 2,
  MD = 3,
  MF = 4,
  CNAME = 5,
  SOA = 6,
  MB = 7,
  MG = 8,
  MR = 9,
  RR_NULL = 10,
  WKS = 11,
  PTR = 12,
  HINFO = 13,
  MINFO = 14,
  MX = 15,
  TXT = 16,

  AXFR = 252,
  MAILB = 253,
  MAILA = 254,
  ANY = 255
};

enum class RRClass : uint16_t {
  IN = 1,
  CS = 2,
  CH = 3,
  HS = 4
};

enum class QClass : uint16_t {
  IN = 1,
  CS = 2,
  CH = 3,
  HS = 4,
  ANY = 255
};

struct RR {
  std::string name;
  RRType type;
  RRClass klass;
  int32_t ttl;
  ByteVector rdata;
};
typedef std::vector<RR> RRVector;

struct Question {
  std::string name;
  QType type;
  QClass klass;
};
typedef std::vector<Question> QuestionVector;

// FIXME
struct Hdr {
  uint16_t id;
  uint16_t flags;
};

struct Msg {
  Hdr hdr;
  QuestionVector question;
  RRVector answer;
  RRVector authority;
  RRVector additional;

  Msg& operator<<(Question&& q) {
    question.push_back(q);
    return *this;
  }

  Msg& operator<<(const Question& q) {
    question.push_back(q);
    return *this;
  }
};

// Encodes and decodes DNS messages to and from wire format.
// Example:
//   dns::Codec codec;
//   dns::Msg msg;
//   msg << dns::Question{"google.com", dns::QType::A, dns::QClass::IN};
//   codec << msg;
//   write(udp_sock, codec.Data(), codec.Size());
class Codec {
 public:
  // Rewinds the data stream to the beginning: next Msg write will overwrite
  // what's already in Codec and next Msg read will start from the beginning.
  Codec& Rewind() {
    pos_ = 0;
    return *this;
  }

  // Rewinds the data stream to the beginning and resizes the underlying 
  Codec& Resize(ByteVector::size_type size) {
    buf_.resize(size);
    pos_ = 0;
    return *this;
  }

  // Returns the pointer to raw data currently held by Codec.
  const uint8_t* Data() const {
    return buf_.data();
  }

  // Returns the size of raw data currently held by Codec, i.e. the size of
  // DNS messages encoded into wire format currently held by Codec.
  ByteVector::size_type Size() const {
    return pos_;
  }

  ByteVector::size_type Capacity() const {
    return buf_.size();
  }

  // Encodes a DNS message into wire format.
  Codec& operator<<(const Msg& msg) {
    *this << msg.hdr.id
          << msg.hdr.flags
          << static_cast<uint16_t>(msg.question.size())
          << static_cast<uint16_t>(msg.answer.size())
          << static_cast<uint16_t>(msg.authority.size())
          << static_cast<uint16_t>(msg.additional.size())
          << msg.question
          << msg.answer
          << msg.authority
          << msg.additional;
    return *this;
  }

 private:
  ByteVector buf_;
  ByteVector::size_type pos_;

  void GrowIfNeeded(ByteVector::size_type size) {
    if (buf_.size() < (pos_ + size))
      buf_.resize(pos_ + size);
  }

  Codec& operator<<(int32_t i) {
    GrowIfNeeded(sizeof(i));
    buf_[pos_++] = static_cast<uint8_t>((i >> (CHAR_BIT * 3)) & 0xff);
    buf_[pos_++] = static_cast<uint8_t>((i >> (CHAR_BIT * 2)) & 0xff);
    buf_[pos_++] = static_cast<uint8_t>((i >> CHAR_BIT) & 0xff);
    buf_[pos_++] = static_cast<uint8_t>(i & 0xff);
    return *this;
  }

  Codec& operator<<(uint16_t i) {
    GrowIfNeeded(sizeof(i));
    buf_[pos_++] = static_cast<uint8_t>(i >> CHAR_BIT);
    buf_[pos_++] = static_cast<uint8_t>(i & 0xff);
    return *this;
  }

  Codec& operator<<(RRType v) {
    *this << static_cast<uint16_t>(v);
    return *this;
  }

  Codec& operator<<(QType v) {
    *this << static_cast<uint16_t>(v);
    return *this;
  }

  Codec& operator<<(RRClass v) {
    *this << static_cast<uint16_t>(v);
    return *this;
  }

  Codec& operator<<(QClass v) {
    *this << static_cast<uint16_t>(v);
    return *this;
  }

  Codec& operator<<(const QuestionVector& v) {
    for (const auto& i : v)
      *this << i;
    return *this;
  }

  Codec& operator<<(const RRVector& v) {
    for (const auto& i : v)
      *this << i;
    return *this;
  }

  Codec& operator<<(const ByteVector& bytes) {
    GrowIfNeeded(bytes.size());
    memcpy(&buf_[pos_], bytes.data(), bytes.size());
    pos_ += bytes.size();
    return *this;
  }

  Codec& operator<<(const std::string& name) {
    const ByteVector::size_type npos = ~static_cast<ByteVector::size_type>(0);
    ByteVector::size_type len_pos = npos;
    for (size_t i = 0; i < name.size(); ++i) {
      uint8_t ch = static_cast<uint8_t>(name[i]);
      if (ch == '.' || len_pos == npos) {
        if (pos_ > 0 && len_pos == pos_ - 1)
          continue; // Skip repeating dots.
        GrowIfNeeded(1);
        len_pos = pos_;
        buf_[pos_++] = 0;
        if (ch == '.')
          continue;
      }
      ++buf_[len_pos];
      GrowIfNeeded(1);
      buf_[pos_++] = ch;
    }
    if (name.empty() || name[name.size() - 1] != '.') {
      GrowIfNeeded(1);
      buf_[pos_++] = 0;
    }
    return *this;
  }

  Codec& operator<<(const Question& q) {
    *this << q.name
          << q.type
          << q.klass;
    return *this;
  }

  Codec& operator<<(const RR& rr) {
    *this << rr.name
          << rr.type
          << rr.klass
          << rr.ttl
          << static_cast<uint16_t>(rr.rdata.size())
          << rr.rdata;
    return *this;
  }
};

/*
struct Tree {
  std::string name;
  int32_t ttl;
  RRVector rrs;
  std::vector<Tree> child;

  Tree& operator<<(Tree&& t) {
    auto node = Find(t.name);
    if (node.name == t.name)
      node = t;
    else
      node.child.push_back(t);
    return *this;
  }

  Tree& operator<<(RR&& rr) {
    rrs.push_back(rr);
    return *this;
  }

  // FIXME see also <<
  Tree& Find(const std::string&) {
    return *this;
  }
};
*/

std::string& Fqdn(std::string& name) {
  if (!name.empty() && name[name.size() - 1] != '.')
    name += '.';
  return name;
}

/* FIXME
class Conn {
 public:
  Conn(const std::string& addr) {
  }

  Msg&& Exchange(const Codec& codec) {
    return std::move(Msg{});
  }

  Msg&& Exchange(const Msg& msg) {
    return Exchange(Codec{} << msg);
  }

 private:
  int sock_;
};
*/

} // namespace dns

std::string Dump(const uint8_t* ptr, size_t size) {
  const char hex[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
                       'a', 'b', 'c', 'd', 'e', 'f' };
  std::ostringstream ostr;
  ostr << std::hex;
  for (size_t i = 0; i < size; ++i) {
    if (i != 0 && (i % 8) == 0)
      ostr << std::endl;
    else if (i != 0 && (i % 4) == 0)
      ostr << ' ';
    uint8_t ch = *(ptr++);
    ostr << hex[ch >> 4] << hex[ch & 0xf];
  }
  return ostr.str();
}

int main(int argc, char** argv) {
  dns::Codec codec;

  while (++argv, --argc) {
    dns::Msg msg;
    msg << dns::Question{*argv, dns::QType::A, dns::QClass::IN};
    codec.Rewind() << msg;
    std::cout << "ptr: " << static_cast<const void*>(codec.Data()) << ", size: " << codec.Size() << std::endl;
    std::cout << Dump(codec.Data(), codec.Size());
    std::cout << std::endl;
    write(STDERR_FILENO, codec.Data(), codec.Size());
  }

  return 0;
}
