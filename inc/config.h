#define PKG_DIR "/"
#define PKG_LDB "/var/pkg/local/"
#define PKG_RDB "/var/pkg/remote/"
#define PKG_CHK "/etc/chksum"
#define PKG_TMP "/var/pkg/cache/"

#define PKG_SRC "url"
#define PKG_SDB "url"
#define PKG_FSG "chksum"
#define PKG_FDB "remote.db"
#define PKG_FMT ".pkg.tzz"

enum {
	MPOOLSIZE = 32768,
	POOLSIZE  = 16384,
	NETBUF    = 2048,
};
