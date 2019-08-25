#define PKG_DIR "/"
#define PKG_LDB "/var/pkg/local/"
#define PKG_RDB "/var/pkg/remote/"
#define PKG_TMP "/var/pkg/cache/"

#define PKG_SRC "url"
#define PKG_SDB "url"
#define PKG_FDB "remote.db"
#define PKG_FMT ".pkg.tzz"
#define PKG_SIG ".sig"

enum {
	MPOOLSIZE   = 65536,
	POOLSIZE    = 2048,
	PKG_NAMEMAX = 256,
	PKG_VERMAX  = 128,
	PKG_LICMAX  = 256,
	PKG_DESCMAX = 512,
	PKG_PATHMAX = 512,
	PKG_VARSIZE = MPOOLSIZE/4,
};
