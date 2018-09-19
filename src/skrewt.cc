#include <cstdio>
#include <chrono>
#include <srt.h>

#include "skrewt_util.h"
#include "node_api.h"

napi_value testMethod(napi_env env, napi_callback_info info) {
  napi_status status;

  int start_state = srt_startup();

  SRTSOCKET sock = srt_create_socket();
  // int close_state = srt_close(sock);

  int end_state = srt_cleanup();

  napi_value result;
  status = napi_get_undefined(env, &result);
  CHECK_STATUS;

  printf("Guess who got called! %i %i\n", start_state, end_state);

  return result;
}

napi_value Init(napi_env env, napi_value exports) {
  napi_status status;
  napi_property_descriptor desc[] = {
    DECLARE_NAPI_METHOD("test", testMethod)
   };
  status = napi_define_properties(env, exports, 1, desc);
  CHECK_STATUS;

  return exports;
}

NAPI_MODULE(nodencl, Init)
