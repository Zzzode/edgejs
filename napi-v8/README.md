 # napi-v8

 `napi-v8` is a standalone compatibility layer that implements Node-API (N-API)
 on top of V8 while keeping V8 details internal.

 ## Scope

 - Public surface: N-API headers and ABI (`js_native_api.h`, `node_api.h`).
 - Internal implementation: V8-backed code only in `src/`.
 - Test strategy: port portable tests from `node/test/js-native-api` first.

 ## Layout

 - `include/`: public C headers (V8-agnostic surface)
 - `src/`: V8-backed implementation and environment glue
 - `tests/`: ported tests and tier-1 runner

 ## Current Phase

 This directory implements the initial `napi-v8` scaffold, a core runtime slice,
and a first portable test subset (`2_function_arguments`, `3_callbacks`) using
GoogleTest.

See `tests/README.md` for build/run instructions and
`tests/PORTABILITY_MATRIX.md` for the current portability classification.
