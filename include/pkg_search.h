#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <xbps.h>

typedef enum REPO_TYPE {
  LOCAL = 0,
  REMOTE = 1,

} REPO_TYPE;

typedef struct package_info_t {
  REPO_TYPE repo_type;
  char *pkgver; // name-version
  char *short_desc;
  char *long_desc;
  char *maintainer;
  char *homepage;
  char *license;
  char *installed_size;
  char *repository;  // only for online repo
  pkg_state_t state; // only for local repo

} package_info_t;

typedef struct search_result_t {
  package_info_t *packages;
  uint32_t count;

} search_result_t;

typedef struct package_files_t {
  char **data;
  uint32_t count;

} package_files_t;

/// @brief Search for packages in a repositories
/// @note Return value should be freed after usage
/// @param xhp Generic XBPS structure handler for initialization
/// @param pattern A pattern to look for
///
/// @return Allocated search_result_t struct or NULL
search_result_t *search_packages(struct xbps_handle *xhp, const char *pattern, REPO_TYPE repo_type,
                                 bool use_regex);

/// @brief Get full package info
/// @note Return value should be freed after usage
/// @param xhp Generic XBPS structure handler for initialization
/// @param pkgname Package name
///
/// @return Allocated package_info_t struct or NULL
package_info_t *get_package_info(struct xbps_handle *xhp, const char *pkgname, REPO_TYPE repo_type);

/// @brief Get package files
/// @note Return value should be freed after usage
/// @param xhp Generic XBPS structure handler for initialization
/// @param pkgname Package name
///
/// @return Allocated  package_files_t struct or NULL
package_files_t *get_package_files(struct xbps_handle *xhp, const char *pkgname,
                                   REPO_TYPE repo_type);

/// @brief Cleanup functions
void search_result_cleanup(search_result_t *result);
void package_info_cleanup(package_info_t *info);
void package_files_cleanup(package_files_t *files, unsigned int count);
