#ifndef INC_UTILS_NAMED_VALUES_H_
#define INC_UTILS_NAMED_VALUES_H_

// To help meta reflection
class INamedValues {
public:
  virtual ~INamedValues() {}
  virtual bool debugInMenuInt(const char* title, void* value) const = 0;
  virtual const char* nameOfInt(int value) const = 0;
  virtual int intValueOf(const char* name) const = 0;
};

template< typename T >
class NamedValues : public INamedValues {

public:

  struct TEntry {
    T           value = 0;
    const char* name = "unknown";
  };

private:
  TEntry* data = nullptr;
  int     count = 0;

public:
  NamedValues(const NamedValues& other) = delete;
  NamedValues(const std::initializer_list<TEntry>& new_data) {
    count = (int)new_data.size();
    // Need to make a copy of new_data
    data = (TEntry*)malloc(count * sizeof(TEntry));
    std::copy(new_data.begin(), new_data.end(), data);
  }
  ~NamedValues() {
    if (data) free(data), data = nullptr;
  }

  int size() const { return count; }

  const TEntry* nth(int idx) {
    assert(idx >= 0 && idx < size());
    return data + idx;
  }

  const char* nameOf(T value) const {
    for (int i = 0; i < count; ++i) {
      if (data[i].value == value)
        return data[i].name;
    }
    return "unknown";
  }

  T valueOf(const char* name) const {
    assert(name);
    for (int i = 0; i < count; ++i) {
      if (strcmp(data[i].name, name) == 0)
        return data[i].value;
    }
    //fatal("Can't identify value for name '%s' in the %d entries\n", name, count);
    return T();
  }

  bool debugInMenu(const char* title, T& state) const {

    // Simplified one-liner Combo() using an accessor function
    struct FuncHolder {
      static bool ItemGetter(void* data, int idx, const char** out_str) {
        NamedValues<T>* nv = (NamedValues<T>*)data;
        *out_str = nv->nth(idx)->name;
        return true;
      }
    };

    // Convert T value to index for ImGui
    int curr_idx = 0;
    for (int i = 0; i < count; ++i) {
      if (data[i].value == state) {
        curr_idx = i;
        break;
      }
    }

    //if (ImGui::Combo(title, &curr_idx, &FuncHolder::ItemGetter, (void*)this, this->size())) {
    //  state = data[curr_idx].value;
    //  return true;
    //}
    return false;
  }

  // ----------------------------------------------------------------
  bool debugInMenuInt(const char* title, void* addr) const override {
    if (addr)
      return debugInMenu(title, *(T*)addr);
    return false;
  }

  const char* nameOfInt(int value) const override {
    return nameOf(*(T*)&value);
  }
  int intValueOf(const char* name) const override {
    return (int)valueOf(name);
  }

};

#endif

