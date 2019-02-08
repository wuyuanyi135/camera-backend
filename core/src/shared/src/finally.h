//
// Created by wuyuanyi on 07/01/19.
//

#ifndef CAMERA_BACKEND_FINALLY_H
#define CAMERA_BACKEND_FINALLY_H
template <class T>
struct finop_t {
  T& t;
  ~finop_t() { t(); }
};
#define FINALLY__(l, cl) \
    auto FIN ## l ## clo = cl; \
    finop_t<decltype(FIN ## l ## clo)> FIN ## l ## fin { FIN ## l ## clo}
#define FINALLY_(l, cl) FINALLY__(l, cl)
#define FINALLY(...) FINALLY_(__LINE__, ([=](){__VA_ARGS__}))
#endif //CAMERA_BACKEND_FINALLY_H
