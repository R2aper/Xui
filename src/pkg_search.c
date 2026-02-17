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

// TODO:

search_result_t *search_packages(struct xbps_handle *xhp, const char *pattern,
                                 REPO_TYPE repo_type, bool use_regex) {
  (void)xhp;
  (void)pattern;
  (void)repo_type;
  (void)use_regex;

  return NULL;
}

package_info_t *get_package_info(struct xbps_handle *xhp, const char *pkgname,
                                 REPO_TYPE repo_type) {
  (void)xhp;
  (void)pkgname;
  (void)repo_type;

  return NULL;
}

package_files_t *get_package_files(struct xbps_handle *xhp, const char *pkgname,
                                   REPO_TYPE repo_type) {
  (void)xhp;
  (void)pkgname;
  (void)repo_type;

  return NULL;
}
