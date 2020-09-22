// ----------------------------------------------------
// ----------------------------------------------------
//					Warning!!
// This file must be compatible with PC/PSP/PS3
// ----------------------------------------------------
// ----------------------------------------------------
#ifndef INC_STRS_H_
#define INC_STRS_H_

// Helpers
inline bool isAnyOf(char c, const char* candidates) {
  while (*candidates) {
    if (c == *candidates++)
      return true;
  }
  return false;
}

// --------------------------------------------------
template< unsigned N >
struct TStr {
  typedef TStr< N > TI;
  enum { max_length = N - 1 };
private:
  char txt[N];

public:

  void formatVaList(const char* fmt, va_list argp) {
    // Behaviour of _snprintf differs by platform/compiler!!!
#ifdef PLATFORM_WINDOWS    // On Win32 vsnprintf is a troll and does not take into account the null termination character
    int n = vsnprintf(txt, N, fmt, argp);   // Copies N characters // vsnprintf_s could be used, has the same behavior and interrupts the program
    if (n == -1 || n == N) {
      assert(fatal("TStr<%d> can't fit input text [%s]\n", N, fmt));
      txt[N - 1] = 0x00;                      // Manually set termination char
    }
    assert(n >= 0);                          // Just in case
#else           // PSVITA works a lot better
    int n = vsnprintf(txt, N, fmt, argp);   // Copies N-1 characters and then the null character
    (void)n;
    assert(n >= 0 && n < N);                 // Return value n indicates the total characters it attempted to copy.
#endif
    //va_end (argp);
    }

  TStr() { txt[0] = 0x00; }

  explicit TStr(const char* fmt, ...) {
    va_list argp;
    va_start(argp, fmt);
    formatVaList(fmt, argp);
    va_end(argp);
  }
  const char* format(const char* fmt, ...) {
    va_list argp;
    va_start(argp, fmt);
    formatVaList(fmt, argp);
    va_end(argp);
    return txt;
  }
  explicit TStr(const char* fmt, va_list argp) {
    formatVaList(fmt, argp);
    va_end(argp);
  }
  void from(const char* src) {
    unsigned int last = capacity();
    assert(std::strlen(src) <= last || fatal("Text '%s' is too long (%d)\n", src, last));     // Length should be smaller than N-1;
    strncpy(txt, src, last);                // Copies up until last. Does not explicitly put the null character
    txt[last] = '\0';                       // Manually adding the null character.
    //format( "%s", src );                    // This is a slower method
  }
  void from(const char* src, size_t num_characters) {
    assert(std::strlen(src) >= num_characters);     // Characters to be copied should be less or equal than src length
    assert(num_characters <= capacity());            // Characters to be copied should be less than the capacity
    strncpy(txt, src, num_characters);                // Copies up until num_characters. Does not explicitly put the null character
    txt[num_characters] = '\0';                       // Manually adding the null character.
    //format( "%s", src );                    // This is a slower method
  }
  const char* append(const char* xtra_str) {
    return format("%s%s", txt, xtra_str);
  }
  const char* prepend(const char* xtra_str) {
    TI temp;
    temp.from(txt);
    return format("%s%s", xtra_str, temp.c_str());
  }
  bool beginsWith(const char* other_text) const {
    size_t nchars = std::strlen(other_text);
    return strncmp(txt, other_text, nchars) == 0;
  }
  bool endsWith(const char* other_text) const {
    size_t other_nchars = std::strlen(other_text);
    size_t my_nchars = std::strlen(txt);
    if (other_nchars > my_nchars)
      return false;
    return strcmp(txt + my_nchars - other_nchars, other_text) == 0;
  }
  bool trimLeft(const char* pattern) {
    if (beginsWith(pattern)) {
      TI temp;
      temp.from(txt);
      format("%s", temp.c_str() + strlen(pattern));
      return true;
    }
    return false;
  }
  bool trimRight(const char* pattern) {
    if (endsWith(pattern)) {
      txt[length() - std::strlen(pattern)] = '\0';
      return true;
    }
    return false;
  }
  void substr(const char* src, size_t nchars) {
    assert(nchars <= capacity());
    strncpy(txt, src, nchars);
    txt[nchars] = '\0';
  }
  void resize(size_t new_size) {
    assert(new_size < capacity());
    txt[new_size] = 0x00;
  }
  bool subStrRight(const char* src, char separator) {
    int src_length = int(std::strlen(src));
    for (; src_length--; ) { // If src_length == 0, stops ( does not enter for ) AND decreases to -1
      if (src[src_length] == separator)
        break;
    }
    from(src + src_length + 1);
    return src_length >= 0;
  }
  void trimRightUntil(char c) {
    size_t src_length = size();
    while (src_length > 0) {
      src_length--;
      if (txt[src_length] == c) {
        txt[src_length] = 0x00;
        break;
      }
    }
  }

  // Attempts to read a space-delimited token from src, storing it into this.
  // Return value indicates whether a token was actually extracted.
  // Src argument is modified to point to the beginning of the next token to be extracted
  // (or end-of-string if no more tokens)
  bool extractToken(const char*& src, const char* delimiters = " \t\n") {
    // Foolproof
    if (!src)
      return false;

    // Skip delimiters
    while (*src && isAnyOf(*src, delimiters))
      src++;

    // Read token
    unsigned txt_idx = 0;
    while (*src && !isAnyOf(*src, delimiters)) {
      txt[txt_idx++] = *src++;
      assert(txt_idx < N);
    }
    txt[txt_idx] = '\0';

    // Skip whitespaces
    while (*src && isAnyOf(*src, delimiters))
      src++;

    return !empty();
  }

  bool operator == (const char* other_txt) const {
    return strcmp(txt, other_txt) == 0;
  }
  bool operator != (const char* other_txt) const {
    return !(*this == other_txt);
  }

#ifdef PLATFORM_WINDOWS
  bool operator == (const std::string& other_txt) const {
    return strcmp(txt, other_txt.c_str()) == 0;
  }
  bool operator != (const std::string& other_txt) const {
    return strcmp(txt, other_txt.c_str()) != 0;
  }
  TStr& operator = (const std::string& other_txt) {
    from(other_txt.c_str());
    return *this;
  }
#endif

  TStr& operator = (const char* other_txt) {
    from(other_txt);
    return *this;
  }
  unsigned int length() const {
    return (unsigned int)std::strlen(txt);
  }
  void clear(bool full_clear = false) {
    txt[0] = 0x00;
    if (full_clear) {
      memset(txt, 0x0, N);
    }
  }
  void toNetFormat() {
    littleEndianDWordsToHostEndian(&txt[0], N / 4);
  }
  void toHostFormat() {
    toNetFormat();
  }
  void toLittleEndian() {
    convertLittleEndianDWords(&txt[0], N / 4);
  }
  inline bool empty() const { return txt[0] == 0; }
  inline size_t size() const { return std::strlen(txt); }
  inline unsigned int capacity() const { return N - 1; }
#ifdef __SNC__
#pragma diag_error=1304                     // Treats warning 1304 ( non POD object in a varg list ) as error
#endif
  operator const char* () const { return txt; }
  const char* c_str() const { return txt; }  // To be used in places where the char* conversion is not called automatically ( for example formats, printfs ( variable args, void*'s, etc ) )
  char* data() { return txt; }  // To be used in places where the char* conversion is not called automatically ( for example formats, printfs ( variable args, void*'s, etc ) )
  };

template< unsigned M, unsigned N >
bool operator == (const TStr<M>& txt_m, const TStr<N>& txt_n) {
  return std::strcmp(txt_m.c_str(), txt_n.c_str()) == 0;
}

template< unsigned M, unsigned N >
bool operator != (const TStr<M>& txt_m, const TStr<N>& txt_n) {
  return !(txt_m == txt_n);
}

template< unsigned N >
bool operator == (const char* other_txt, const TStr<N>& txt) {
  return txt == other_txt;
}

template< unsigned N >
bool operator != (const char* other_txt, const TStr<N>& txt) {
  return !(txt == other_txt);
}

typedef TStr<16> TStr16;
typedef TStr<32> TStr32;
typedef TStr<64> TStr64;
typedef TStr<128> TStr128;
typedef TStr<256> TStr256;
typedef TStr<256> TTempStr;

#endif
