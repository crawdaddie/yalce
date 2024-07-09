#define T(...)                                                                 \
  (Type *[]) { __VA_ARGS__ }

#define TUPLE(num, ...) tcons("Tuple", T(__VA_ARGS__), num)
#define TLIST(t) tcons("List", T(t), 1)

#define TVAR(name)                                                             \
  (Type) {                                                                     \
    T_VAR, { .T_VAR = name }                                                   \
  }
