#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<'EOF'
Usage:
  tools/render_preset_gallery.sh <input-audio> <output-dir> [horizon-cli-path]

Example:
  tools/render_preset_gallery.sh loops/drums.wav docs/audio ./cmake-out/linux-clang/horizon_cli
EOF
}

if [[ $# -lt 2 || $# -gt 3 ]]; then
  usage
  exit 1
fi

input_audio="$1"
output_dir="$2"
cli_path="${3:-./cmake-out/linux-clang/horizon_cli}"

if [[ ! -f "$input_audio" ]]; then
  echo "[render_preset_gallery] Input file not found: $input_audio" >&2
  exit 1
fi

if [[ ! -x "$cli_path" ]]; then
  echo "[render_preset_gallery] horizon_cli is not executable: $cli_path" >&2
  echo "[render_preset_gallery] Build first: cmake --preset linux-clang && cmake --build --preset linux-clang" >&2
  exit 1
fi

mkdir -p "$output_dir"

input_base="$(basename "${input_audio%.*}")"
input_ext="${input_audio##*.}"
dry_copy="$output_dir/${input_base}_dry.${input_ext}"
cp "$input_audio" "$dry_copy"

mapfile -t presets < <("$cli_path" --list-presets | sed -n 's/^  - \([^ ]*\).*/\1/p')

if [[ ${#presets[@]} -eq 0 ]]; then
  echo "[render_preset_gallery] No presets found in horizon_cli --list-presets output." >&2
  exit 1
fi

echo "[render_preset_gallery] Dry reference: $dry_copy"
for preset in "${presets[@]}"; do
  before="$output_dir/${preset}_before.${input_ext}"
  after="$output_dir/${preset}_after.wav"
  cp "$input_audio" "$before"
  "$cli_path" "$input_audio" "$after" --preset "$preset"
  echo "[render_preset_gallery] Rendered preset '$preset' -> $after"
done

echo "[render_preset_gallery] Completed ${#presets[@]} preset renders in $output_dir"
