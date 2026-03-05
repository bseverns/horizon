#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<'EOF'
Usage:
  tools/install_vst3.sh <path-to-Horizon.vst3> [--system]

Examples:
  tools/install_vst3.sh plugins/build/HorizonJuce_artefacts/VST3/Horizon.vst3
  tools/install_vst3.sh ./Horizon.vst3 --system
EOF
}

if [[ $# -lt 1 || $# -gt 2 ]]; then
  usage
  exit 1
fi

source_bundle="$1"
install_scope="${2:-}"

if [[ ! -d "$source_bundle" ]]; then
  echo "[install_vst3] VST3 bundle not found: $source_bundle" >&2
  exit 1
fi

case "$(uname -s)" in
  Darwin)
    user_target="$HOME/Library/Audio/Plug-Ins/VST3"
    system_target="/Library/Audio/Plug-Ins/VST3"
    ;;
  Linux)
    user_target="$HOME/.vst3"
    system_target="/usr/lib/vst3"
    ;;
  *)
    echo "[install_vst3] Unsupported OS for this helper. Install manually using PLUGIN_INSTALL.md." >&2
    exit 1
    ;;
esac

target_dir="$user_target"
if [[ "$install_scope" == "--system" ]]; then
  target_dir="$system_target"
elif [[ -n "$install_scope" ]]; then
  usage
  exit 1
fi

mkdir -p "$target_dir"
cp -R "$source_bundle" "$target_dir/"

echo "[install_vst3] Installed $(basename "$source_bundle") to $target_dir"
echo "[install_vst3] If your host does not see it, run a full plugin rescan."
