#!/usr/bin/env bash
set -euo pipefail

ENV_FILE="${HOME}/.env"
SSH_DIR="${HOME}/.ssh"
KEY_FILE="${SSH_DIR}/id_ed25519"

# /root (and everything cloned under it) is a bind mount from the host, so
# its UID won't match the container's root user — Git's ownership check
# would otherwise refuse to touch any repo in here.
git config --global --add safe.directory '*' || true

setup_git_ssh() {
    if [[ ! -f "${ENV_FILE}" ]]; then
        echo "No ${ENV_FILE} found — skipping git/SSH setup (copy .env.example to .env to enable it)."
        return 0
    fi

    # shellcheck disable=SC1090
    source "${ENV_FILE}"

    [[ -n "${GIT_USER_NAME:-}" ]] && git config --global user.name "${GIT_USER_NAME}"
    [[ -n "${GIT_USER_EMAIL:-}" ]] && git config --global user.email "${GIT_USER_EMAIL}"

    if [[ -n "${SSH_PRIVATE_KEY:-}" ]]; then
        mkdir -p "${SSH_DIR}"
        chmod 700 "${SSH_DIR}"

        if [[ ! -f "${KEY_FILE}" ]]; then
            printf '%s\n' "${SSH_PRIVATE_KEY}" > "${KEY_FILE}"
            chmod 600 "${KEY_FILE}"
            echo "Installed SSH key at ${KEY_FILE}"
        fi

        touch "${SSH_DIR}/known_hosts"
        chmod 600 "${SSH_DIR}/known_hosts"
        if ! grep -q "^github.com " "${SSH_DIR}/known_hosts" 2>/dev/null; then
            ssh-keyscan -H github.com >>"${SSH_DIR}/known_hosts" 2>/dev/null || true
        fi
    fi

    echo "Git identity: ${GIT_USER_NAME:-<unset>} <${GIT_USER_EMAIL:-<unset>}>"
}

setup_git_ssh || echo "Warning: git/SSH setup failed, continuing without it."

exec "$@"
