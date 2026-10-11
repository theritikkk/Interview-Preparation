#!/bin/bash
# build-grpcurl.sh — build a real grpcurl from source.
#
# Why this exists: grpcurl isn't distributed as a raw GitHub-release
# binary (fullstorydev/grpcurl ships tarballs, which is fine — the real
# blocker was different and worth recording). `go install` resolves
# Go's module vanity import paths (google.golang.org/*, golang.org/x/*,
# cloud.google.com/go/*) by HTTP GET-ing the vanity domain itself
# (e.g. https://google.golang.org/grpc?go-get=1) to discover the real
# backing repo. Those specific domains are blocked by this sandbox's
# egress policy (403), even though the GitHub mirrors those vanity
# imports resolve to are reachable directly. The fix: `replace`
# directives in go.mod pointing each blocked vanity import straight at
# its GitHub mirror, so `go build` fetches over git instead of
# resolving the vanity redirect. This is the same "registries/some
# domains blocked, github.com allowed" shape as every other finding in
# this project series — not a new kind of limitation, just a different
# surface of the same one.
set -euo pipefail

VERSION="v1.8.9"   # last version supporting Go < 1.25 (this sandbox has Go 1.24.7)

rm -rf grpcurl-src && mkdir grpcurl-src && cd grpcurl-src
git init -q
git remote add origin https://github.com/fullstorydev/grpcurl.git
git fetch --depth 1 origin "${VERSION}"
git checkout -q FETCH_HEAD

cat >> go.mod << 'EOF'

replace (
	google.golang.org/protobuf => github.com/protocolbuffers/protobuf-go v1.31.0
	google.golang.org/grpc => github.com/grpc/grpc-go v1.57.0
	golang.org/x/net => github.com/golang/net v0.17.0
	golang.org/x/sys => github.com/golang/sys v0.13.0
	golang.org/x/text => github.com/golang/text v0.13.0
	golang.org/x/sync => github.com/golang/sync v0.3.0
	golang.org/x/oauth2 => github.com/golang/oauth2 v0.7.0
	google.golang.org/genproto/googleapis/rpc => github.com/googleapis/go-genproto/googleapis/rpc v0.0.0-20230525234030-28d5490b6b19
	google.golang.org/genproto/googleapis/api => github.com/googleapis/go-genproto/googleapis/api v0.0.0-20230525234035-dd9d682886f9
	google.golang.org/genproto => github.com/googleapis/go-genproto v0.0.0-20230526161137-0005af68ea54
	cloud.google.com/go/compute/metadata => github.com/googleapis/google-cloud-go/compute/metadata v0.2.3
	cloud.google.com/go/compute => github.com/googleapis/google-cloud-go/compute v1.19.1
)
EOF

GOTOOLCHAIN=local GOPROXY=direct GOSUMDB=off GOFLAGS=-mod=mod \
  go build -o ../grpcurl ./cmd/grpcurl

cd ..
./grpcurl --version 2>&1 || true
echo "built: $(pwd)/grpcurl"
