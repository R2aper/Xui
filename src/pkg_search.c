#include "pkg_search.h"
#include "utils.h"

#include <linux/limits.h>
#include <regex.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ============= Context for callback's  ============= */

struct search_context {
  bool use_regex;
  regex_t regexp;
  const char *pattern;
  search_result_t *results;
};

/* ============= Local search (pkgdb) ============= */

static int local_search_callback(struct xbps_handle *xhp,
                                 xbps_object_t pkg_dict, const char *key,
                                 void *arg, bool *loop_done) {
  (void)xhp;
  (void)key;
  (void)loop_done;

  struct search_context *ctx = (struct search_context *)arg;
  const char *pkgver = NULL, *short_desc = NULL;
  bool match = false;

  // Get metadata
  xbps_dictionary_get_cstring_nocopy(pkg_dict, "pkgver", &pkgver);
  xbps_dictionary_get_cstring_nocopy(pkg_dict, "short_desc", &short_desc);

  if (!pkgver || !short_desc)
    return 0;

  // Check for match
  if (ctx->use_regex) {
    if (regexec(&ctx->regexp, pkgver, 0, 0, 0) == 0 ||
        regexec(&ctx->regexp, short_desc, 0, 0, 0) == 0)
      match = true;
  } else {
    // Case-insensitive substring search
    if (strcasestr_portable(pkgver, ctx->pattern) ||
        strcasestr_portable(short_desc, ctx->pattern)) {
      match = true;
    }
  }

  if (!match)
    return 0;

  ctx->results->packages =
      realloc(ctx->results->packages,
              (ctx->results->count + 1) * sizeof(package_info_t));
  if (!ctx->results->packages)
    return 0;

  package_info_t *pkg = &ctx->results->packages[ctx->results->count];
  memset(pkg, 0, sizeof(package_info_t));

  // Copy info
  pkg->pkgver = strdup(pkgver);
  pkg->short_desc = strdup(short_desc);

  const char *long_desc = NULL;
  xbps_dictionary_get_cstring_nocopy(pkg_dict, "long_desc", &long_desc);
  if (long_desc)
    pkg->long_desc = strdup(long_desc);

  const char *maintainer = NULL;
  xbps_dictionary_get_cstring_nocopy(pkg_dict, "maintainer", &maintainer);
  if (maintainer)
    pkg->maintainer = strdup(maintainer);

  const char *homepage = NULL;
  xbps_dictionary_get_cstring_nocopy(pkg_dict, "homepage", &homepage);
  if (homepage)
    pkg->homepage = strdup(homepage);

  const char *license = NULL;
  xbps_dictionary_get_cstring_nocopy(pkg_dict, "license", &license);
  if (license)
    pkg->license = strdup(license);

  // Get pkg state
  xbps_pkg_state_dictionary(pkg_dict, &pkg->state);

  ctx->results->count++;

  return 0;
}

/* ============= Remote search (Repository) ============= */

struct remote_search_context {
  struct search_context base;
  const char *repo_uri;
};

static int remote_search_callback(struct xbps_handle *xhp,
                                  xbps_object_t pkg_dict, const char *key,
                                  void *arg, bool *loop_done) {

  (void)xhp;
  (void)key;
  (void)loop_done;

  struct remote_search_context *ctx = (struct remote_search_context *)arg;
  const char *pkgver = NULL, *short_desc = NULL;
  bool match = false;

  xbps_dictionary_get_cstring_nocopy(pkg_dict, "pkgver", &pkgver);
  xbps_dictionary_get_cstring_nocopy(pkg_dict, "short_desc", &short_desc);

  if (!pkgver || !short_desc)
    return 0;

  // Get metadata
  if (ctx->base.use_regex) {
    if (regexec(&ctx->base.regexp, pkgver, 0, 0, 0) == 0 ||
        regexec(&ctx->base.regexp, short_desc, 0, 0, 0) == 0)
      match = true;
  } else {
    if (strcasestr(pkgver, ctx->base.pattern) != NULL ||
        strcasestr(short_desc, ctx->base.pattern) != NULL)
      match = true;
  }

  if (!match)
    return 0;

  ctx->base.results->packages =
      realloc(ctx->base.results->packages,
              (ctx->base.results->count + 1) * sizeof(package_info_t));
  if (!ctx->base.results->packages)
    return 0;

  package_info_t *pkg = &ctx->base.results->packages[ctx->base.results->count];
  memset(pkg, 0, sizeof(package_info_t));

  pkg->pkgver = strdup(pkgver);
  pkg->short_desc = strdup(short_desc);
  pkg->repository = strdup(ctx->repo_uri);

  // Copy info
  const char *long_desc = NULL;
  xbps_dictionary_get_cstring_nocopy(pkg_dict, "long_desc", &long_desc);
  if (long_desc)
    pkg->long_desc = strdup(long_desc);

  const char *maintainer = NULL;
  xbps_dictionary_get_cstring_nocopy(pkg_dict, "maintainer", &maintainer);
  if (maintainer)
    pkg->maintainer = strdup(maintainer);

  const char *homepage = NULL;
  xbps_dictionary_get_cstring_nocopy(pkg_dict, "homepage", &homepage);
  if (homepage)
    pkg->homepage = strdup(homepage);

  const char *license = NULL;
  xbps_dictionary_get_cstring_nocopy(pkg_dict, "license", &license);
  if (license)
    pkg->license = strdup(license);

  ctx->base.results->count++;

  return 0;
}

static int remote_repo_callback(struct xbps_repo *repo, void *arg, bool *done) {
  (void)done;

  struct remote_search_context *ctx = (struct remote_search_context *)arg;
  xbps_array_t keys;

  // Get all keys from repo
  keys = xbps_dictionary_all_keys(repo->idx);
  if (!keys)
    return 0;

  ctx->repo_uri = repo->uri;

  xbps_array_foreach_cb(repo->xhp, keys, repo->idx, remote_search_callback,
                        ctx);

  xbps_object_release(keys);

  return 0;
}

/* ============= Search package ============= */

search_result_t *search_packages(struct xbps_handle *xhp, const char *pattern,
                                 REPO_TYPE repo_type, bool use_regex) {
  struct remote_search_context ctx;
  search_result_t *results = malloc(sizeof(search_result_t));
  if (!results)
    return NULL;

  memset(results, 0, sizeof(search_result_t));
  results->packages = NULL;
  results->count = 0;

  ctx.base.pattern = pattern;
  ctx.base.use_regex = use_regex;
  ctx.base.results = results;
  ctx.repo_uri = NULL;

  if (use_regex) {
    if (regcomp(&ctx.base.regexp, pattern,
                REG_EXTENDED | REG_NOSUB | REG_ICASE) != 0) {
      fprintf(stderr, "Failed to compile regex: %s\n", pattern);
      return results;
    }
  }

  if (repo_type == REMOTE)
    xbps_rpool_foreach(xhp, remote_repo_callback, &ctx);
  else if (repo_type == LOCAL)
    xbps_pkgdb_foreach_cb(xhp, local_search_callback, &ctx.base);

  if (use_regex)
    regfree(&ctx.base.regexp);

  return results;
}

/* ============= Get package metadata ============= */

package_info_t *get_package_info(struct xbps_handle *xhp, const char *pkgname,
                                 REPO_TYPE repo_type) {
  xbps_dictionary_t pkg_dict;
  package_info_t *pkg;

  if (repo_type == REMOTE) {
    // Try to find pkg
    pkg_dict = xbps_rpool_get_pkg(xhp, pkgname);

    // If nothing is find, try to search for virtual pkg
    if (!pkg_dict)
      pkg_dict = xbps_rpool_get_virtualpkg(xhp, pkgname);

    if (!pkg_dict)
      return NULL;

    pkg = malloc(sizeof(package_info_t));
    if (!pkg)
      return NULL;
    memset(pkg, 0, sizeof(package_info_t));

  } else if (repo_type == LOCAL) {
    pkg_dict = xbps_pkgdb_get_pkg(xhp, pkgname);
    if (!pkg_dict)
      return NULL;

    pkg = malloc(sizeof(package_info_t));
    if (!pkg)
      return NULL;
    memset(pkg, 0, sizeof(package_info_t));
  } else {
    return NULL;
  }

  const char *value = NULL;

  xbps_dictionary_get_cstring_nocopy(pkg_dict, "pkgver", &value);
  if (value)
    pkg->pkgver = strdup(value);

  xbps_dictionary_get_cstring_nocopy(pkg_dict, "short_desc", &value);
  if (value)
    pkg->short_desc = strdup(value);

  xbps_dictionary_get_cstring_nocopy(pkg_dict, "long_desc", &value);
  if (value)
    pkg->long_desc = strdup(value);

  xbps_dictionary_get_cstring_nocopy(pkg_dict, "maintainer", &value);
  if (value)
    pkg->maintainer = strdup(value);

  xbps_dictionary_get_cstring_nocopy(pkg_dict, "homepage", &value);
  if (value)
    pkg->homepage = strdup(value);

  xbps_dictionary_get_cstring_nocopy(pkg_dict, "license", &value);
  if (value)
    pkg->license = strdup(value);

  if (repo_type == LOCAL)
    xbps_pkg_state_dictionary(pkg_dict, &pkg->state);

  return pkg;
}

/* ============= Get package files  ============= */

package_files_t *get_package_files(struct xbps_handle *xhp, const char *pkgname,
                                   REPO_TYPE repo_type) {
  package_files_t *files = (package_files_t *)malloc(sizeof(package_files_t));
  if (!files)
    return NULL;
  memset(files, 0, sizeof(package_files_t));

  xbps_dictionary_t files_dict, file_obj;
  xbps_array_t files_array;

  if (repo_type == REMOTE) {
    xbps_dictionary_t pkg_dict = xbps_rpool_get_pkg(xhp, pkgname);
    if (!pkg_dict)
      return NULL;

    char bfile[PATH_MAX];
    int rv = xbps_pkg_path_or_url(xhp, bfile, sizeof(bfile), pkg_dict);
    if (rv < 0)
      return NULL;

    files_dict = xbps_archive_fetch_plist(bfile, "/files.plist");
  } else {
    files_dict = xbps_pkgdb_get_pkg_files(xhp, pkgname);
  }

  if (!files_dict)
    return NULL;

  // Ordinary files
  files_array = xbps_dictionary_get(files_dict, "files");
  if (files_array) {
    for (uint32_t i = 0; i < xbps_array_count(files_array); i++) {
      file_obj = xbps_array_get(files_array, i);
      const char *filepath = NULL;
      xbps_dictionary_get_cstring_nocopy(file_obj, "file", &filepath);
      if (filepath) {
        files->data = realloc(files->data, (files->count + 1) * sizeof(char *));
        files->data[files->count++] = strdup(filepath);
      }
    }
  }

  // Config files
  files_array = xbps_dictionary_get(files_dict, "conf_files");
  if (files_array) {
    for (unsigned int i = 0; i < xbps_array_count(files_array); i++) {
      file_obj = xbps_array_get(files_array, i);
      const char *filepath = NULL;
      xbps_dictionary_get_cstring_nocopy(file_obj, "file", &filepath);
      if (filepath) {
        files->data = realloc(files->data, (files->count + 1) * sizeof(char *));
        files->data[files->count++] = strdup(filepath);
      }
    }
  }

  // Links
  files_array = xbps_dictionary_get(files_dict, "links");
  if (files_array) {
    for (unsigned int i = 0; i < xbps_array_count(files_array); i++) {
      file_obj = xbps_array_get(files_array, i);
      const char *filepath = NULL;
      xbps_dictionary_get_cstring_nocopy(file_obj, "file", &filepath);
      if (filepath) {
        files->data = realloc(files->data, (files->count + 1) * sizeof(char *));
        files->data[files->count++] = strdup(filepath);
      }
    }
  }

  xbps_object_release(files_dict);

  return files;
}

/* ============= Cleanup functions ============= */

void package_info_cleanup(package_info_t *pkg) {
  if (!pkg)
    return;

  if (pkg->pkgver)
    free(pkg->pkgver);

  if (pkg->short_desc)
    free(pkg->short_desc);

  if (pkg->long_desc)
    free(pkg->long_desc);

  if (pkg->maintainer)
    free(pkg->maintainer);

  if (pkg->homepage)
    free(pkg->homepage);

  if (pkg->license)
    free(pkg->license);

  if (pkg->installed_size)
    free(pkg->installed_size);

  if (pkg->repository)
    free(pkg->repository);

  if (pkg)
    free(pkg);
}

void search_result_cleanup(search_result_t *result) {
  if (!result)
    return;

  for (unsigned int i = 0; i < result->count; i++) {
    package_info_t *pkg = &result->packages[i];
    if (pkg->pkgver)
      free(pkg->pkgver);
    if (pkg->short_desc)
      free(pkg->short_desc);
    if (pkg->long_desc)
      free(pkg->long_desc);
    if (pkg->maintainer)
      free(pkg->maintainer);
    if (pkg->homepage)
      free(pkg->homepage);
    if (pkg->license)
      free(pkg->license);
    if (pkg->installed_size)
      free(pkg->installed_size);
    if (pkg->repository)
      free(pkg->repository);
  }

  if (result->packages)
    free(result->packages);

  if (result)
    free(result);
}

void package_files_cleanup(package_files_t *files, unsigned int count) {
  if (!files->data)
    return;

  for (uint32_t i = 0; i < count; i++) {
    if (files->data[i])
      free(files->data[i]);
  }
  if (files->data)
    free(files->data);

  if (files)
    free(files);
}
