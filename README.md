# Features
- No macros
- read/write from/to json (wip for binary)
- Same serialization exposed to ImGui
- Types and Data members can be tagged with user-defined attributes
  - Editing ranges for ImGui
  - Custom visualization (Rad shown in degrees for example)
  - Conditional visualization
- Easy to plug support for enums. (Example included)
- Components
  - Type: Any C++ type
  - Data: Members associated to a type.
  - Ref: A pair of {Type + Address}. Address not owned by the Ref
- Copy values between refs

# Requirements
- C++ 17
- Requires declare members at runtime. Allow to delete types also.
- Memory is owned by the user types. If class `Foo` has a member data `const char* text`, we can export
  it to json, but we can't read back to a new instance of Foo automatically without knowing who owns 
  the pointer.

```cpp
struct House {
  int life = 1;
  float size = 2.0f;
};

::reflect<House>("House")
  .data<&House::life>("Life")
  .data<&House::size>("Size");

House my_house;
Ref r_house(&my_house);
Ref r_life = r_house.get("Life");
r_life.set(10);
assert( my_house.life == 10);

// Serialize to json
json j;
toJson(j, r_house);  // j = { "Life" : 10, "size" : 2.0 }

// Read from json
House your_house;
Ref r_house2;
fromJson(j, r_house2);
assert( your_house.life == my_house.life );
assert( your_house.size == my_house.size );

```

## ToDo

- [ ] I/O ImGui
- [ ] I/O Binary
- [ ] members using fn to set
- [ ] base class, derived class
- [ ] Support for dicts
- [ ] Release allocated memory

## Done

- [x] Improve register
- [x] Serialize to/from json using the nlohmann as json data type
- [x] invoke super basic fns
- [x] Support for std::strings
- [x] Support for std::vector
- [x] Check duped members, duped names
- [x] Confirm json of enums work
