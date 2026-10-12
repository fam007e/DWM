#!/usr/bin/env bash
set -euo pipefail

dwm_packages() {
	local distro=${1:-arch}
	local target=${2:-keyring}

	case "$target" in
	keyring)
		printf '%s\n' gnome-keyring
		;;
	build)
		printf '%s\n' \
			base-devel \
			libx11 \
			libxft \
			libxinerama \
			imlib2 \
			libxcb \
			xcb-util \
			freetype2 \
			fontconfig \
			quickshell
		;;
	source-update)
		printf '%s\n' \
			base-devel \
			git \
			jq \
			curl \
			bc
		;;
	*)
		;;
	esac
}

# If executed directly as a script
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
	dwm_packages "${1:-arch}" "${2:-build}"
fi
