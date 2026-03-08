#include "internal_binding/dispatch.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <unordered_map>

#include <openssl/ec.h>
#include <openssl/rsa.h>
#include <openssl/ssl.h>

#include "internal_binding/helpers.h"

namespace internal_binding {

namespace {

std::unordered_map<napi_env, napi_ref> g_constants_refs;

bool IsObjectLike(napi_env env, napi_value value) {
  if (value == nullptr) return false;
  napi_valuetype type = napi_undefined;
  if (napi_typeof(env, value, &type) != napi_ok) return false;
  return type == napi_object || type == napi_function;
}

napi_value GetNamedProperty(napi_env env, napi_value target, const char* key) {
  if (target == nullptr) return nullptr;
  napi_value out = nullptr;
  if (napi_get_named_property(env, target, key, &out) != napi_ok || out == nullptr) return nullptr;
  return out;
}

napi_value CreateNullPrototypeObject(napi_env env) {
  const napi_value undefined = Undefined(env);
  const napi_value global = GetGlobal(env);
  if (global == nullptr) return undefined;

  napi_value object_ctor = nullptr;
  if (napi_get_named_property(env, global, "Object", &object_ctor) != napi_ok ||
      object_ctor == nullptr || !IsObjectLike(env, object_ctor)) {
    return undefined;
  }

  napi_value create_fn = nullptr;
  if (napi_get_named_property(env, object_ctor, "create", &create_fn) != napi_ok ||
      create_fn == nullptr) {
    return undefined;
  }
  napi_valuetype fn_type = napi_undefined;
  if (napi_typeof(env, create_fn, &fn_type) != napi_ok || fn_type != napi_function) {
    return undefined;
  }

  napi_value null_value = nullptr;
  if (napi_get_null(env, &null_value) != napi_ok || null_value == nullptr) return undefined;

  napi_value out = nullptr;
  napi_value argv[1] = {null_value};
  if (napi_call_function(env, object_ctor, create_fn, 1, argv, &out) != napi_ok || out == nullptr) {
    bool pending = false;
    if (napi_is_exception_pending(env, &pending) == napi_ok && pending) {
      napi_value ignored = nullptr;
      napi_get_and_clear_last_exception(env, &ignored);
    }
    return undefined;
  }
  return out;
}

napi_value CreatePlainObject(napi_env env) {
  napi_value out = nullptr;
  if (napi_create_object(env, &out) != napi_ok || out == nullptr) return Undefined(env);
  return out;
}

napi_value CreateBestEffortNullProtoObject(napi_env env) {
  napi_value out = CreateNullPrototypeObject(env);
  if (out == nullptr || IsUndefined(env, out) || !IsObjectLike(env, out)) {
    out = CreatePlainObject(env);
  }
  return out;
}

napi_value EnsureObjectProperty(napi_env env, napi_value target, const char* key) {
  napi_value current = GetNamedProperty(env, target, key);
  if (!IsObjectLike(env, current)) {
    current = CreatePlainObject(env);
    if (IsObjectLike(env, current)) napi_set_named_property(env, target, key, current);
  }
  return current;
}

void EnsureInt32Default(napi_env env, napi_value target, const char* key, int32_t value) {
  if (!IsObjectLike(env, target)) return;
  bool has_key = false;
  if (napi_has_named_property(env, target, key, &has_key) != napi_ok || has_key) return;
  SetInt32(env, target, key, value);
}

void EnsureInt64Default(napi_env env, napi_value target, const char* key, int64_t value) {
  if (!IsObjectLike(env, target)) return;
  bool has_key = false;
  if (napi_has_named_property(env, target, key, &has_key) != napi_ok || has_key) return;
  napi_value out = nullptr;
  if (napi_create_int64(env, value, &out) == napi_ok && out != nullptr) {
    napi_set_named_property(env, target, key, out);
  }
}

void CopyOwnProperties(napi_env env, napi_value src, napi_value dst) {
  if (!IsObjectLike(env, src) || !IsObjectLike(env, dst)) return;
  napi_value keys = nullptr;
  if (napi_get_property_names(env, src, &keys) != napi_ok || keys == nullptr) return;
  uint32_t key_count = 0;
  if (napi_get_array_length(env, keys, &key_count) != napi_ok) return;
  for (uint32_t i = 0; i < key_count; i++) {
    napi_value key = nullptr;
    if (napi_get_element(env, keys, i, &key) != napi_ok || key == nullptr) continue;
    napi_value value = nullptr;
    if (napi_get_property(env, src, key, &value) != napi_ok || value == nullptr) continue;
    napi_set_property(env, dst, key, value);
  }
}

bool CopyNumericOwnProperties(napi_env env, napi_value src, napi_value dst) {
  napi_value keys = nullptr;
  if (napi_get_property_names(env, src, &keys) != napi_ok || keys == nullptr) return false;
  uint32_t key_count = 0;
  if (napi_get_array_length(env, keys, &key_count) != napi_ok) return false;
  for (uint32_t i = 0; i < key_count; i++) {
    napi_value key = nullptr;
    if (napi_get_element(env, keys, i, &key) != napi_ok || key == nullptr) continue;

    napi_value value = nullptr;
    if (napi_get_property(env, src, key, &value) != napi_ok || value == nullptr) continue;

    napi_valuetype t = napi_undefined;
    if (napi_typeof(env, value, &t) != napi_ok || t != napi_number) continue;

    napi_set_property(env, dst, key, value);
  }
  return true;
}

napi_value CreateInternalConstants(napi_env env) {
  napi_value internal = nullptr;
  if (napi_create_object(env, &internal) != napi_ok || internal == nullptr) return Undefined(env);
  SetInt32(env, internal, "EXTENSIONLESS_FORMAT_JAVASCRIPT", 0);
  SetInt32(env, internal, "EXTENSIONLESS_FORMAT_WASM", 1);
  return internal;
}

napi_value CreateTraceConstants(napi_env env) {
  napi_value trace = nullptr;
  if (napi_create_object(env, &trace) != napi_ok || trace == nullptr) return Undefined(env);
  SetInt32(env, trace, "TRACE_EVENT_PHASE_NESTABLE_ASYNC_BEGIN", 'b');
  SetInt32(env, trace, "TRACE_EVENT_PHASE_NESTABLE_ASYNC_END", 'e');
  return trace;
}

napi_value CreateDefaultFsConstants(napi_env env) {
  napi_value fs_obj = CreateBestEffortNullProtoObject(env);
  if (!IsObjectLike(env, fs_obj)) return Undefined(env);
#ifdef UV_FS_SYMLINK_DIR
  SetInt32(env, fs_obj, "UV_FS_SYMLINK_DIR", UV_FS_SYMLINK_DIR);
#endif
#ifdef UV_FS_SYMLINK_JUNCTION
  SetInt32(env, fs_obj, "UV_FS_SYMLINK_JUNCTION", UV_FS_SYMLINK_JUNCTION);
#endif
#ifdef O_RDONLY
  SetInt32(env, fs_obj, "O_RDONLY", O_RDONLY);
#endif
#ifdef O_WRONLY
  SetInt32(env, fs_obj, "O_WRONLY", O_WRONLY);
#endif
#ifdef O_RDWR
  SetInt32(env, fs_obj, "O_RDWR", O_RDWR);
#endif
#ifdef UV_DIRENT_UNKNOWN
  SetInt32(env, fs_obj, "UV_DIRENT_UNKNOWN", UV_DIRENT_UNKNOWN);
#else
  SetInt32(env, fs_obj, "UV_DIRENT_UNKNOWN", 0);
#endif
#ifdef UV_DIRENT_FILE
  SetInt32(env, fs_obj, "UV_DIRENT_FILE", UV_DIRENT_FILE);
#else
  SetInt32(env, fs_obj, "UV_DIRENT_FILE", 1);
#endif
#ifdef UV_DIRENT_DIR
  SetInt32(env, fs_obj, "UV_DIRENT_DIR", UV_DIRENT_DIR);
#else
  SetInt32(env, fs_obj, "UV_DIRENT_DIR", 2);
#endif
#ifdef UV_DIRENT_LINK
  SetInt32(env, fs_obj, "UV_DIRENT_LINK", UV_DIRENT_LINK);
#else
  SetInt32(env, fs_obj, "UV_DIRENT_LINK", 3);
#endif
#ifdef UV_DIRENT_FIFO
  SetInt32(env, fs_obj, "UV_DIRENT_FIFO", UV_DIRENT_FIFO);
#else
  SetInt32(env, fs_obj, "UV_DIRENT_FIFO", 4);
#endif
#ifdef UV_DIRENT_SOCKET
  SetInt32(env, fs_obj, "UV_DIRENT_SOCKET", UV_DIRENT_SOCKET);
#else
  SetInt32(env, fs_obj, "UV_DIRENT_SOCKET", 5);
#endif
#ifdef UV_DIRENT_CHAR
  SetInt32(env, fs_obj, "UV_DIRENT_CHAR", UV_DIRENT_CHAR);
#else
  SetInt32(env, fs_obj, "UV_DIRENT_CHAR", 6);
#endif
#ifdef UV_DIRENT_BLOCK
  SetInt32(env, fs_obj, "UV_DIRENT_BLOCK", UV_DIRENT_BLOCK);
#else
  SetInt32(env, fs_obj, "UV_DIRENT_BLOCK", 7);
#endif
#ifdef S_IFMT
  SetInt32(env, fs_obj, "S_IFMT", S_IFMT);
#endif
#ifdef S_IFREG
  SetInt32(env, fs_obj, "S_IFREG", S_IFREG);
#endif
#ifdef S_IFDIR
  SetInt32(env, fs_obj, "S_IFDIR", S_IFDIR);
#endif
#ifdef S_IFCHR
  SetInt32(env, fs_obj, "S_IFCHR", S_IFCHR);
#endif
#ifdef S_IFBLK
  SetInt32(env, fs_obj, "S_IFBLK", S_IFBLK);
#endif
#ifdef S_IFIFO
  SetInt32(env, fs_obj, "S_IFIFO", S_IFIFO);
#endif
#ifdef S_IFLNK
  SetInt32(env, fs_obj, "S_IFLNK", S_IFLNK);
#endif
#ifdef S_IFSOCK
  SetInt32(env, fs_obj, "S_IFSOCK", S_IFSOCK);
#endif
#ifdef O_CREAT
  SetInt32(env, fs_obj, "O_CREAT", O_CREAT);
#endif
#ifdef O_EXCL
  SetInt32(env, fs_obj, "O_EXCL", O_EXCL);
#endif
#ifdef UV_FS_O_FILEMAP
  SetInt32(env, fs_obj, "UV_FS_O_FILEMAP", UV_FS_O_FILEMAP);
#endif
#ifdef O_NOCTTY
  SetInt32(env, fs_obj, "O_NOCTTY", O_NOCTTY);
#endif
#ifdef O_TRUNC
  SetInt32(env, fs_obj, "O_TRUNC", O_TRUNC);
#endif
#ifdef O_APPEND
  SetInt32(env, fs_obj, "O_APPEND", O_APPEND);
#endif
#ifdef O_DIRECTORY
  SetInt32(env, fs_obj, "O_DIRECTORY", O_DIRECTORY);
#endif
#ifdef O_NOATIME
  SetInt32(env, fs_obj, "O_NOATIME", O_NOATIME);
#endif
#ifdef O_NOFOLLOW
  SetInt32(env, fs_obj, "O_NOFOLLOW", O_NOFOLLOW);
#endif
#ifdef O_SYNC
  SetInt32(env, fs_obj, "O_SYNC", O_SYNC);
#endif
#ifdef O_DSYNC
  SetInt32(env, fs_obj, "O_DSYNC", O_DSYNC);
#endif
#ifdef O_SYMLINK
  SetInt32(env, fs_obj, "O_SYMLINK", O_SYMLINK);
#endif
#ifdef O_DIRECT
  SetInt32(env, fs_obj, "O_DIRECT", O_DIRECT);
#endif
#ifdef O_NONBLOCK
  SetInt32(env, fs_obj, "O_NONBLOCK", O_NONBLOCK);
#endif
#ifdef S_IRWXU
  SetInt32(env, fs_obj, "S_IRWXU", S_IRWXU);
#endif
#ifdef S_IRUSR
  SetInt32(env, fs_obj, "S_IRUSR", S_IRUSR);
#endif
#ifdef S_IWUSR
  SetInt32(env, fs_obj, "S_IWUSR", S_IWUSR);
#endif
#ifdef S_IXUSR
  SetInt32(env, fs_obj, "S_IXUSR", S_IXUSR);
#endif
#ifdef S_IRWXG
  SetInt32(env, fs_obj, "S_IRWXG", S_IRWXG);
#endif
#ifdef S_IRGRP
  SetInt32(env, fs_obj, "S_IRGRP", S_IRGRP);
#endif
#ifdef S_IWGRP
  SetInt32(env, fs_obj, "S_IWGRP", S_IWGRP);
#endif
#ifdef S_IXGRP
  SetInt32(env, fs_obj, "S_IXGRP", S_IXGRP);
#endif
#ifdef S_IRWXO
  SetInt32(env, fs_obj, "S_IRWXO", S_IRWXO);
#endif
#ifdef S_IROTH
  SetInt32(env, fs_obj, "S_IROTH", S_IROTH);
#endif
#ifdef S_IWOTH
  SetInt32(env, fs_obj, "S_IWOTH", S_IWOTH);
#endif
#ifdef S_IXOTH
  SetInt32(env, fs_obj, "S_IXOTH", S_IXOTH);
#endif
#ifdef F_OK
  SetInt32(env, fs_obj, "F_OK", F_OK);
#endif
#ifdef R_OK
  SetInt32(env, fs_obj, "R_OK", R_OK);
#endif
#ifdef W_OK
  SetInt32(env, fs_obj, "W_OK", W_OK);
#endif
#ifdef X_OK
  SetInt32(env, fs_obj, "X_OK", X_OK);
#endif
#ifdef UV_FS_COPYFILE_EXCL
  SetInt32(env, fs_obj, "UV_FS_COPYFILE_EXCL", UV_FS_COPYFILE_EXCL);
  SetInt32(env, fs_obj, "COPYFILE_EXCL", UV_FS_COPYFILE_EXCL);
#else
  SetInt32(env, fs_obj, "UV_FS_COPYFILE_EXCL", 1);
  SetInt32(env, fs_obj, "COPYFILE_EXCL", 1);
#endif
#ifdef UV_FS_COPYFILE_FICLONE
  SetInt32(env, fs_obj, "UV_FS_COPYFILE_FICLONE", UV_FS_COPYFILE_FICLONE);
  SetInt32(env, fs_obj, "COPYFILE_FICLONE", UV_FS_COPYFILE_FICLONE);
#else
  SetInt32(env, fs_obj, "UV_FS_COPYFILE_FICLONE", 2);
  SetInt32(env, fs_obj, "COPYFILE_FICLONE", 2);
#endif
#ifdef UV_FS_COPYFILE_FICLONE_FORCE
  SetInt32(env, fs_obj, "UV_FS_COPYFILE_FICLONE_FORCE", UV_FS_COPYFILE_FICLONE_FORCE);
  SetInt32(env, fs_obj, "COPYFILE_FICLONE_FORCE", UV_FS_COPYFILE_FICLONE_FORCE);
#else
  SetInt32(env, fs_obj, "UV_FS_COPYFILE_FICLONE_FORCE", 4);
  SetInt32(env, fs_obj, "COPYFILE_FICLONE_FORCE", 4);
#endif
  return fs_obj;
}

napi_value CreateEmptyObject(napi_env env) {
  return CreatePlainObject(env);
}

napi_value CreateDefaultOsConstants(napi_env env) {
  napi_value os_obj = CreatePlainObject(env);
  if (!IsObjectLike(env, os_obj)) return Undefined(env);
  napi_value signals = CreateBestEffortNullProtoObject(env);
  if (IsObjectLike(env, signals)) {
    napi_set_named_property(env, os_obj, "signals", signals);
  }
  return os_obj;
}

void SetNamedObjectIfValid(napi_env env, napi_value target, const char* key, napi_value value) {
  if (value != nullptr && !IsUndefined(env, value) && IsObjectLike(env, value)) {
    napi_set_named_property(env, target, key, value);
  }
}

void NormalizeConstantsShape(napi_env env, napi_value constants) {
  if (!IsObjectLike(env, constants)) return;

  // Ensure fs access constants are always available.
  napi_value fs_obj = EnsureObjectProperty(env, constants, "fs");
  EnsureInt32Default(env, fs_obj, "F_OK", 0);
  EnsureInt32Default(env, fs_obj, "R_OK", 4);
  EnsureInt32Default(env, fs_obj, "W_OK", 2);
  EnsureInt32Default(env, fs_obj, "X_OK", 1);

  // Keep os.signals as a clean map-like object.
  napi_value os_obj = EnsureObjectProperty(env, constants, "os");
  napi_value src_signals = GetNamedProperty(env, os_obj, "signals");
  if (!IsObjectLike(env, src_signals)) src_signals = CreatePlainObject(env);

  napi_value normalized_signals = CreateBestEffortNullProtoObject(env);
  if (!IsObjectLike(env, normalized_signals)) return;
  CopyOwnProperties(env, src_signals, normalized_signals);
  napi_object_freeze(env, normalized_signals);
  napi_set_named_property(env, os_obj, "signals", normalized_signals);

  // Keep zlib constants minimally non-empty so zlib.js can compute
  // parameter-array bounds during module initialization.
  napi_value zlib_obj = EnsureObjectProperty(env, constants, "zlib");
  EnsureInt32Default(env, zlib_obj, "Z_NO_FLUSH", 0);
  EnsureInt32Default(env, zlib_obj, "Z_PARTIAL_FLUSH", 1);
  EnsureInt32Default(env, zlib_obj, "Z_SYNC_FLUSH", 2);
  EnsureInt32Default(env, zlib_obj, "Z_FULL_FLUSH", 3);
  EnsureInt32Default(env, zlib_obj, "Z_FINISH", 4);
  EnsureInt32Default(env, zlib_obj, "Z_BLOCK", 5);
  EnsureInt32Default(env, zlib_obj, "Z_OK", 0);
  EnsureInt32Default(env, zlib_obj, "Z_STREAM_END", 1);
  EnsureInt32Default(env, zlib_obj, "Z_NEED_DICT", 2);
  EnsureInt32Default(env, zlib_obj, "Z_ERRNO", -1);
  EnsureInt32Default(env, zlib_obj, "Z_STREAM_ERROR", -2);
  EnsureInt32Default(env, zlib_obj, "Z_DATA_ERROR", -3);
  EnsureInt32Default(env, zlib_obj, "Z_MEM_ERROR", -4);
  EnsureInt32Default(env, zlib_obj, "Z_BUF_ERROR", -5);
  EnsureInt32Default(env, zlib_obj, "Z_VERSION_ERROR", -6);
  EnsureInt32Default(env, zlib_obj, "Z_NO_COMPRESSION", 0);
  EnsureInt32Default(env, zlib_obj, "Z_BEST_SPEED", 1);
  EnsureInt32Default(env, zlib_obj, "Z_BEST_COMPRESSION", 9);
  EnsureInt32Default(env, zlib_obj, "Z_DEFAULT_COMPRESSION", -1);
  EnsureInt32Default(env, zlib_obj, "Z_DEFAULT_LEVEL", -1);
  EnsureInt32Default(env, zlib_obj, "Z_FILTERED", 1);
  EnsureInt32Default(env, zlib_obj, "Z_HUFFMAN_ONLY", 2);
  EnsureInt32Default(env, zlib_obj, "Z_RLE", 3);
  EnsureInt32Default(env, zlib_obj, "Z_FIXED", 4);
  EnsureInt32Default(env, zlib_obj, "Z_DEFAULT_STRATEGY", 0);
  EnsureInt32Default(env, zlib_obj, "DEFLATE", 1);
  EnsureInt32Default(env, zlib_obj, "INFLATE", 2);
  EnsureInt32Default(env, zlib_obj, "GZIP", 3);
  EnsureInt32Default(env, zlib_obj, "GUNZIP", 4);
  EnsureInt32Default(env, zlib_obj, "DEFLATERAW", 5);
  EnsureInt32Default(env, zlib_obj, "INFLATERAW", 6);
  EnsureInt32Default(env, zlib_obj, "UNZIP", 7);
  EnsureInt32Default(env, zlib_obj, "BROTLI_DECODE", 8);
  EnsureInt32Default(env, zlib_obj, "BROTLI_ENCODE", 9);
  EnsureInt32Default(env, zlib_obj, "ZSTD_COMPRESS", 10);
  EnsureInt32Default(env, zlib_obj, "ZSTD_DECOMPRESS", 11);
  EnsureInt32Default(env, zlib_obj, "Z_MIN_WINDOWBITS", 8);
  EnsureInt32Default(env, zlib_obj, "Z_MAX_WINDOWBITS", 15);
  EnsureInt32Default(env, zlib_obj, "Z_DEFAULT_WINDOWBITS", 15);
  EnsureInt32Default(env, zlib_obj, "Z_MIN_CHUNK", 64);
  EnsureInt32Default(env, zlib_obj, "Z_MAX_CHUNK", 2147483647);
  EnsureInt32Default(env, zlib_obj, "Z_DEFAULT_CHUNK", 16384);
  EnsureInt32Default(env, zlib_obj, "Z_MIN_MEMLEVEL", 1);
  EnsureInt32Default(env, zlib_obj, "Z_MAX_MEMLEVEL", 9);
  EnsureInt32Default(env, zlib_obj, "Z_DEFAULT_MEMLEVEL", 8);
  EnsureInt32Default(env, zlib_obj, "Z_MIN_LEVEL", -1);
  EnsureInt32Default(env, zlib_obj, "Z_MAX_LEVEL", 9);
  EnsureInt32Default(env, zlib_obj, "BROTLI_OPERATION_PROCESS", 0);
  EnsureInt32Default(env, zlib_obj, "BROTLI_OPERATION_FLUSH", 1);
  EnsureInt32Default(env, zlib_obj, "BROTLI_OPERATION_FINISH", 2);
  EnsureInt32Default(env, zlib_obj, "BROTLI_OPERATION_EMIT_METADATA", 3);
  EnsureInt32Default(env, zlib_obj, "BROTLI_PARAM_QUALITY", 1);
  EnsureInt32Default(env, zlib_obj, "BROTLI_PARAM_MODE", 0);
  EnsureInt32Default(env, zlib_obj, "BROTLI_PARAM_LGWIN", 2);
  EnsureInt32Default(env, zlib_obj, "BROTLI_PARAM_LGBLOCK", 3);
  EnsureInt32Default(env, zlib_obj, "BROTLI_PARAM_DISABLE_LITERAL_CONTEXT_MODELING", 4);
  EnsureInt32Default(env, zlib_obj, "BROTLI_PARAM_SIZE_HINT", 5);
  EnsureInt32Default(env, zlib_obj, "BROTLI_PARAM_LARGE_WINDOW", 6);
  EnsureInt32Default(env, zlib_obj, "BROTLI_PARAM_NPOSTFIX", 7);
  EnsureInt32Default(env, zlib_obj, "BROTLI_PARAM_NDIRECT", 8);
  EnsureInt32Default(env, zlib_obj, "ZSTD_e_continue", 0);
  EnsureInt32Default(env, zlib_obj, "ZSTD_e_flush", 1);
  EnsureInt32Default(env, zlib_obj, "ZSTD_e_end", 2);
  EnsureInt32Default(env, zlib_obj, "ZSTD_c_compressionLevel", 1);
  EnsureInt32Default(env, zlib_obj, "ZSTD_d_windowLogMax", 1);

  // Keep critical crypto constants aligned with Node's constants module shape.
  napi_value crypto_obj = EnsureObjectProperty(env, constants, "crypto");
#ifdef RSA_PKCS1_PADDING
  EnsureInt32Default(env, crypto_obj, "RSA_PKCS1_PADDING", RSA_PKCS1_PADDING);
#endif
#ifdef RSA_NO_PADDING
  EnsureInt32Default(env, crypto_obj, "RSA_NO_PADDING", RSA_NO_PADDING);
#endif
#ifdef RSA_PKCS1_OAEP_PADDING
  EnsureInt32Default(env, crypto_obj, "RSA_PKCS1_OAEP_PADDING", RSA_PKCS1_OAEP_PADDING);
#endif
#ifdef RSA_X931_PADDING
  EnsureInt32Default(env, crypto_obj, "RSA_X931_PADDING", RSA_X931_PADDING);
#endif
#ifdef RSA_PKCS1_PSS_PADDING
  EnsureInt32Default(env, crypto_obj, "RSA_PKCS1_PSS_PADDING", RSA_PKCS1_PSS_PADDING);
#endif
#ifdef RSA_PSS_SALTLEN_DIGEST
  EnsureInt32Default(env, crypto_obj, "RSA_PSS_SALTLEN_DIGEST", RSA_PSS_SALTLEN_DIGEST);
#endif
#ifdef RSA_PSS_SALTLEN_MAX_SIGN
  EnsureInt32Default(env, crypto_obj, "RSA_PSS_SALTLEN_MAX_SIGN", RSA_PSS_SALTLEN_MAX_SIGN);
#endif
#ifdef RSA_PSS_SALTLEN_AUTO
  EnsureInt32Default(env, crypto_obj, "RSA_PSS_SALTLEN_AUTO", RSA_PSS_SALTLEN_AUTO);
#endif
#ifdef TLS1_VERSION
  EnsureInt32Default(env, crypto_obj, "TLS1_VERSION", TLS1_VERSION);
#endif
#ifdef TLS1_1_VERSION
  EnsureInt32Default(env, crypto_obj, "TLS1_1_VERSION", TLS1_1_VERSION);
#endif
#ifdef TLS1_2_VERSION
  EnsureInt32Default(env, crypto_obj, "TLS1_2_VERSION", TLS1_2_VERSION);
#endif
#ifdef TLS1_3_VERSION
  EnsureInt32Default(env, crypto_obj, "TLS1_3_VERSION", TLS1_3_VERSION);
#endif
#ifdef SSL_OP_CIPHER_SERVER_PREFERENCE
  EnsureInt64Default(env,
                     crypto_obj,
                     "SSL_OP_CIPHER_SERVER_PREFERENCE",
                     static_cast<int64_t>(SSL_OP_CIPHER_SERVER_PREFERENCE));
#endif
  EnsureInt32Default(env, crypto_obj, "POINT_CONVERSION_COMPRESSED", POINT_CONVERSION_COMPRESSED);
  EnsureInt32Default(env, crypto_obj, "POINT_CONVERSION_UNCOMPRESSED", POINT_CONVERSION_UNCOMPRESSED);
  EnsureInt32Default(env, crypto_obj, "POINT_CONVERSION_HYBRID", POINT_CONVERSION_HYBRID);
}

}  // namespace

napi_value ResolveConstants(napi_env env, const ResolveOptions& options) {
  const napi_value undefined = Undefined(env);
  auto cached_it = g_constants_refs.find(env);
  if (cached_it != g_constants_refs.end() && cached_it->second != nullptr) {
    napi_value cached = nullptr;
    if (napi_get_reference_value(env, cached_it->second, &cached) == napi_ok && cached != nullptr) {
      return cached;
    }
  }

  napi_value out = nullptr;
  if (napi_create_object(env, &out) != napi_ok || out == nullptr) return undefined;

  SetNamedObjectIfValid(env, out, "os", CreateDefaultOsConstants(env));
  SetNamedObjectIfValid(env, out, "fs", CreateDefaultFsConstants(env));
  SetNamedObjectIfValid(env, out, "crypto", CreateEmptyObject(env));
  SetNamedObjectIfValid(env, out, "zlib", CreateEmptyObject(env));

  // Prefer native ubi constants when present.
  napi_value os_constants = nullptr;
  if (options.callbacks.resolve_binding != nullptr) {
    os_constants = options.callbacks.resolve_binding(env, options.state, "os_constants");
  }
  SetNamedObjectIfValid(env, out, "os", os_constants);

  // Derive crypto constants from the native crypto binding surface to avoid
  // requiring JS modules while constants are initializing.
  napi_value crypto_binding = nullptr;
  if (options.callbacks.resolve_binding != nullptr) {
    crypto_binding = options.callbacks.resolve_binding(env, options.state, "crypto");
  }
  if (!IsUndefined(env, crypto_binding) && IsObjectLike(env, crypto_binding)) {
    napi_value crypto_constants_obj = nullptr;
    if (napi_create_object(env, &crypto_constants_obj) == napi_ok && crypto_constants_obj != nullptr) {
      CopyNumericOwnProperties(env, crypto_binding, crypto_constants_obj);
      SetNamedObjectIfValid(env, out, "crypto", crypto_constants_obj);
    }
  }

  SetNamedObjectIfValid(env, out, "internal", CreateInternalConstants(env));
  SetNamedObjectIfValid(env, out, "trace", CreateTraceConstants(env));
  NormalizeConstantsShape(env, out);

  auto& ref = g_constants_refs[env];
  if (ref != nullptr) {
    napi_delete_reference(env, ref);
    ref = nullptr;
  }
  napi_create_reference(env, out, 1, &ref);

  return out;
}

}  // namespace internal_binding
