module netchaos

go 1.24

require (
	github.com/vishvananda/netlink v1.3.1
	github.com/vishvananda/netns v0.0.5
	golang.org/x/sys v0.10.0
)

replace golang.org/x/sys => github.com/golang/sys v0.10.0
