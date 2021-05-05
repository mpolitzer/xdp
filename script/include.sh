#!/usr/bin/env ash
die() {
    echo -en "$1" >&2
    exit 1
}
cleanup() {
	[ -n "$C" ] && (
		buildah unmount $C
		buildah rm $C
	)
	die "cleanup interrupted buildah"
}
