#include "arg.h"
#include "compat.h"
#include "pkg.h"

/* PKG_DIR: ROOT  PATH
 * PKG_LDB: LOCAL DATABASE
 * PKG_RDB: REPO  DATABASE
 * PKG_SRC: REPO  FILE
 * PKG_TMP: TMP   DIR
 * PKG_FMT: PKG   FORMAT
 * PKG_SIG: PKG   SIGNATURE
 */
#define PKG_DIR "/"
#define PKG_LDB "/var/pkg/local"
#define PKG_RDB "/var/pkg/remote"
#define PKG_SRC "/var/pkg/source.list"
#define PKG_TMP "/tmp"
#define PKG_FMT ".tar.gz"
#define PKG_SIG ".sig"
