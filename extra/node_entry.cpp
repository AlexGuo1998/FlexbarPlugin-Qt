#include <node_api.h>

#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>

#else
#ifdef __linux__

#include <dlfcn.h>

#else

// TODO: MacOS dynamic loader implementation
#error "Not implemented"

#endif
#endif

#include "../app.h"

// Wraps every Node-API method: call GetProcAddress on the main executable

template<const char *, typename>
struct wrapper;

template<const char * Name, typename R, typename... Args>
struct wrapper<Name, R(Args...)> {
    R operator()(Args... args) {
        auto func = reinterpret_cast<R(*)(Args...)>(
#ifdef _WIN32
            GetProcAddress(nullptr, Name)
#else
#ifdef __linux__
            dlsym(RTLD_DEFAULT, Name)
#else
#error "Not implemented"
#endif
#endif
        );
        return func(std::forward<Args>(args)...);
    }
};

#define MAKE_WRAPPER(original) \
    const char s_##original[] = #original; \
    wrapper<s_##original, decltype(original)> w_##original

MAKE_WRAPPER(napi_create_function);
MAKE_WRAPPER(napi_get_cb_info);
MAKE_WRAPPER(napi_is_array);
MAKE_WRAPPER(napi_throw_type_error);
MAKE_WRAPPER(napi_get_array_length);
MAKE_WRAPPER(napi_get_element);
MAKE_WRAPPER(napi_get_value_string_utf8);
MAKE_WRAPPER(napi_create_double);

static napi_value node_main(napi_env env, napi_callback_info info) {
    size_t argc;
    napi_status status = w_napi_get_cb_info(env, info, &argc, nullptr, nullptr, nullptr);
    if (status != napi_ok) return nullptr;

    if (argc != 1) {
        w_napi_throw_type_error(env, nullptr, "Wrong number of arguments");
        return nullptr;
    }

    napi_value args[1];
    status = w_napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    if (status != napi_ok) return nullptr;

    bool isArray;
    status = w_napi_is_array(env, args[0], &isArray);
    if (status != napi_ok) return nullptr;
    if (!isArray) {
        w_napi_throw_type_error(env, nullptr, "Wrong arguments");
        return nullptr;
    }

    uint32_t len;
    w_napi_get_array_length(env, args[0], &len);
    if (status != napi_ok) return nullptr;

    std::vector<std::string> args_ss;
    for (uint32_t i = 0; i < len; ++i) {
        napi_value item;
        status = w_napi_get_element(env, args[0], i, &item);
        if (status != napi_ok) return nullptr;
        size_t length;
        status = w_napi_get_value_string_utf8(env, item, nullptr, 0, &length);
        if (status != napi_ok) return nullptr;

        std::string value(length, '\0');
        value.reserve(length + 1);
        status = w_napi_get_value_string_utf8(env, item, &value.front(), value.capacity(), nullptr);
        if (status != napi_ok) return nullptr;
        args_ss.push_back(value);
    }
    const int ret = qt_main(args_ss);

    napi_value result;
    w_napi_create_double(env, ret, &result);
    return result;
}

static napi_value node_init(napi_env env, napi_value exports) {
    napi_value method = nullptr;
    w_napi_create_function(env, nullptr, 0, node_main, nullptr, &method);
    return method;
}

NAPI_MODULE(m, node_init)
