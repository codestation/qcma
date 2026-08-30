# Shared packaging helpers

Distribution entry points call these scripts; they are not normally invoked
directly. Builds use committed files from qcma and vitamtp and fail when either
working tree is dirty. Set `VITAMTP_SOURCE` when vitamtp is not next to qcma.

For ARM builds on an x86_64 host, run `setup-buildx.sh` once. It explicitly
asks Docker to register QEMU handlers using a privileged helper container.
