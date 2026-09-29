#!/usr/bin/env bash
# ==============================================================================
# Qualcomm TelAF Simulation - Upstream Patch Manager
#
# Manages patch sets applied to upstream/ source tree.
# Commands:
#   apply   - Apply all patches in sequence
#   revert  - Revert all patches in reverse sequence
#   status  - Report applied/unapplied status of each patch
# ==============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
PATCH_DIR="${REPO_ROOT}/patches"

cd "${REPO_ROOT}"

# Collect all patch files sorted
collect_patches() {
    if [[ -d "${PATCH_DIR}" ]]; then
        find "${PATCH_DIR}" -type f -name "*.patch" | sort
    fi
}

check_patch_status() {
    local patch_file="$1"
    # Check if patch is already applied
    if patch -p1 -R --dry-run < "${patch_file}" >/dev/null 2>&1; then
        echo "APPLIED"
    elif patch -p1 -N --dry-run < "${patch_file}" >/dev/null 2>&1; then
        echo "UNAPPLIED"
    else
        echo "CONFLICT"
    fi
}

do_status() {
    echo "=== Upstream Patch Status ==="
    local count=0
    while IFS= read -r patch_file; do
        [[ -z "${patch_file}" ]] && continue
        local rel_path="${patch_file#"${REPO_ROOT}/"}"
        local status
        status=$(check_patch_status "${patch_file}")
        case "${status}" in
            APPLIED)
                echo "  [APPLIED]    ${rel_path}"
                ;;
            UNAPPLIED)
                echo "  [UNAPPLIED]  ${rel_path}"
                ;;
            CONFLICT)
                echo "  [CONFLICT]   ${rel_path}"
                ;;
        esac
        ((count++)) || true
    done < <(collect_patches)

    if [[ ${count} -eq 0 ]]; then
        echo "  No patches found in ${PATCH_DIR}."
    fi
}

do_apply() {
    echo "=== Applying Upstream Patches ==="
    local count=0
    while IFS= read -r patch_file; do
        [[ -z "${patch_file}" ]] && continue
        local rel_path="${patch_file#"${REPO_ROOT}/"}"
        local status
        status=$(check_patch_status "${patch_file}")
        if [[ "${status}" == "APPLIED" ]]; then
            echo "  [SKIP]       ${rel_path} (already applied)"
        elif [[ "${status}" == "UNAPPLIED" ]]; then
            echo "  [APPLYING]   ${rel_path}..."
            patch -p1 -N < "${patch_file}"
            echo "  [DONE]       ${rel_path}"
        else
            echo "  [ERROR]      ${rel_path} cannot be applied cleanly!" >&2
            exit 1
        fi
        ((count++)) || true
    done < <(collect_patches)

    if [[ ${count} -eq 0 ]]; then
        echo "  No patches to apply."
    else
        echo "All patches applied successfully."
    fi
}

do_revert() {
    echo "=== Reverting Upstream Patches ==="
    # Reverse sort for reverting
    local patches
    mapfile -t patches < <(collect_patches | sort -r)
    local count=0

    for patch_file in "${patches[@]}"; do
        [[ -z "${patch_file}" ]] && continue
        local rel_path="${patch_file#"${REPO_ROOT}/"}"
        local status
        status=$(check_patch_status "${patch_file}")
        if [[ "${status}" == "UNAPPLIED" ]]; then
            echo "  [SKIP]       ${rel_path} (not applied)"
        elif [[ "${status}" == "APPLIED" ]]; then
            echo "  [REVERTING]  ${rel_path}..."
            patch -p1 -R < "${patch_file}"
            echo "  [DONE]       ${rel_path}"
        else
            echo "  [ERROR]      ${rel_path} cannot be reverted cleanly!" >&2
            exit 1
        fi
        ((count++)) || true
    done

    if [[ ${count} -eq 0 ]]; then
        echo "  No patches to revert."
    else
        echo "All patches reverted successfully."
    fi
}

usage() {
    echo "Usage: $0 {apply|revert|status}"
    exit 1
}

case "${1:-}" in
    apply)
        do_apply
        ;;
    revert)
        do_revert
        ;;
    status)
        do_status
        ;;
    *)
        usage
        ;;
esac
